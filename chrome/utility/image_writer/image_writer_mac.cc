// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOBlockStorageDevice.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOStorageProtocolCharacteristics.h>
#include <sys/socket.h>

#include "base/command_line.h"
#include "base/files/scoped_file.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/mac/scoped_ioobject.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/common/extensions/image_writer/image_writer_util_mac.h"
#include "chrome/utility/image_writer/disk_unmounter_mac.h"
#include "chrome/utility/image_writer/error_messages.h"
#include "chrome/utility/image_writer/image_writer.h"

namespace image_writer {

static const char kAuthOpenPath[] = "/usr/libexec/authopen";

bool ImageWriter::IsValidDevice() {
  base::ScopedCFTypeRef<CFStringRef> cf_bsd_name(
      base::SysUTF8ToCFStringRef(device_path_.value()));
  CFMutableDictionaryRef matching = IOServiceMatching(kIOMediaClass);
  CFDictionaryAddValue(matching, CFSTR(kIOMediaWholeKey), kCFBooleanTrue);
  CFDictionaryAddValue(matching, CFSTR(kIOMediaWritableKey), kCFBooleanTrue);
  CFDictionaryAddValue(matching, CFSTR(kIOBSDNameKey), cf_bsd_name);

  io_service_t disk_obj =
      IOServiceGetMatchingService(kIOMasterPortDefault, matching);
  base::mac::ScopedIOObject<io_service_t> iterator_ref(disk_obj);

  if (disk_obj) {
    CFMutableDictionaryRef dict;
    if (IORegistryEntryCreateCFProperties(
            disk_obj, &dict, kCFAllocatorDefault, 0) != KERN_SUCCESS) {
      LOG(ERROR) << "Unable to get properties of disk object.";
      return false;
    }
    base::ScopedCFTypeRef<CFMutableDictionaryRef> dict_ref(dict);

    CFBooleanRef cf_removable = base::mac::GetValueFromDictionary<CFBooleanRef>(
        dict, CFSTR(kIOMediaRemovableKey));
    bool removable = CFBooleanGetValue(cf_removable);

    bool is_usb = extensions::IsUsbDevice(disk_obj);

    return removable || is_usb;
  }

  return false;
}

void ImageWriter::UnmountVolumes(const base::Closure& continuation) {
  if (unmounter_ == NULL) {
    unmounter_.reset(new DiskUnmounterMac());
  }

  unmounter_->Unmount(
      device_path_.value(),
      continuation,
      base::Bind(
          &ImageWriter::Error, base::Unretained(this), error::kUnmountVolumes));
}

bool ImageWriter::OpenDevice() {
  base::LaunchOptions options = base::LaunchOptions();
  options.wait = false;

  // Create a socket pair for communication.
  int sockets[2];
  int result = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
  if (result == -1) {
    PLOG(ERROR) << "Unable to allocate socket pair.";
    return false;
  }
  base::ScopedFD parent_socket(sockets[0]);
  base::ScopedFD child_socket(sockets[1]);

  // Map the client socket to the client's STDOUT.
  base::FileHandleMappingVector fd_map;
  fd_map.push_back(std::pair<int, int>(child_socket.get(), STDOUT_FILENO));
  options.fds_to_remap = &fd_map;

  // Find the file path to open.
  base::FilePath real_device_path;
  if (device_path_.IsAbsolute()) {
    real_device_path = device_path_;
  } else {
    real_device_path = base::FilePath("/dev").Append(device_path_);
  }

  // Build the command line.
  std::string rdwr = base::StringPrintf("%d", O_RDWR);

  base::CommandLine cmd_line = base::CommandLine(base::FilePath(kAuthOpenPath));
  cmd_line.AppendSwitch("-stdoutpipe");
  // Using AppendSwitchNative will use an equal-symbol which we don't want.
  cmd_line.AppendArg("-o");
  cmd_line.AppendArg(rdwr);
  cmd_line.AppendArgPath(real_device_path);

  // Launch the process.
  base::ProcessHandle process_handle;
  if (!base::LaunchProcess(cmd_line, options, &process_handle)) {
    LOG(ERROR) << "Failed to launch authopen process.";
    return false;
  }

  // Receive a file descriptor from authopen which sends a single FD via
  // sendmsg and the SCM_RIGHTS extension.
  int fd = -1;
  const size_t kDataBufferSize = sizeof(struct cmsghdr) + sizeof(int);
  char data_buffer[kDataBufferSize];

  struct iovec io_vec[1];
  io_vec[0].iov_base = data_buffer;
  io_vec[0].iov_len = kDataBufferSize;

  const socklen_t kCmsgSocketSize =
      static_cast<socklen_t>(CMSG_SPACE(sizeof(int)));
  char cmsg_socket[kCmsgSocketSize];

  struct msghdr message = {0};
  message.msg_iov = io_vec;
  message.msg_iovlen = 1;
  message.msg_control = cmsg_socket;
  message.msg_controllen = kCmsgSocketSize;

  ssize_t size = HANDLE_EINTR(recvmsg(parent_socket.get(), &message, 0));
  if (size > 0) {
    struct cmsghdr* cmsg_socket_header = CMSG_FIRSTHDR(&message);

    if (cmsg_socket_header && cmsg_socket_header->cmsg_level == SOL_SOCKET &&
        cmsg_socket_header->cmsg_type == SCM_RIGHTS) {
      fd = *reinterpret_cast<int*>(CMSG_DATA(cmsg_socket_header));
    }
  }

  device_file_ = base::File(fd);

  // Wait for the child.
  int child_exit_status;
  if (!base::WaitForExitCode(process_handle, &child_exit_status)) {
    LOG(ERROR) << "Unable to wait for child.";
    return false;
  }

  if (child_exit_status) {
    LOG(ERROR) << "Child process returned failure.";
    return false;
  }

  return device_file_.IsValid();
}

}  // namespace image_writer
