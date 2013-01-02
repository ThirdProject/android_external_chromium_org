// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_PEPPER_AUDIO_INPUT_HOST_H_
#define CONTENT_RENDERER_PEPPER_PEPPER_AUDIO_INPUT_HOST_H_

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/shared_memory.h"
#include "base/sync_socket.h"
#include "content/renderer/pepper/pepper_device_enumeration_host_helper.h"
#include "ipc/ipc_platform_file.h"
#include "ppapi/c/ppb_audio_config.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/resource_host.h"
#include "webkit/plugins/ppapi/plugin_delegate.h"

namespace content {

class RendererPpapiHost;

class PepperAudioInputHost
    : public ppapi::host::ResourceHost,
      public webkit::ppapi::PluginDelegate::PlatformAudioInputClient,
      public PepperDeviceEnumerationHostHelper::Delegate {
 public:
  PepperAudioInputHost(RendererPpapiHost* host,
                       PP_Instance instance,
                       PP_Resource resource);
  virtual ~PepperAudioInputHost();

  virtual int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) OVERRIDE;

  // PluginDelegate::PlatformAudioInputClient implementation.
  virtual void StreamCreated(base::SharedMemoryHandle shared_memory_handle,
                             size_t shared_memory_size,
                             base::SyncSocket::Handle socket) OVERRIDE;
  virtual void StreamCreationFailed() OVERRIDE;

  // PepperDeviceEnumerationHostHelper::Delegate implementation.
  virtual webkit::ppapi::PluginDelegate* GetPluginDelegate() OVERRIDE;

 private:
  int32_t OnOpen(ppapi::host::HostMessageContext* context,
                 const std::string& device_id,
                 PP_AudioSampleRate sample_rate,
                 uint32_t sample_frame_count);
  int32_t OnStartOrStop(ppapi::host::HostMessageContext* context,
                        bool capture);
  int32_t OnClose(ppapi::host::HostMessageContext* context);

  void OnOpenComplete(int32_t result,
                      base::SharedMemoryHandle shared_memory_handle,
                      size_t shared_memory_size,
                      base::SyncSocket::Handle socket_handle);

  int32_t GetRemoteHandles(
      const base::SyncSocket& socket,
      const base::SharedMemory& shared_memory,
      IPC::PlatformFileForTransit* remote_socket_handle,
      base::SharedMemoryHandle* remote_shared_memory_handle);

  void Close();

  // Non-owning pointer.
  RendererPpapiHost* renderer_ppapi_host_;

  scoped_ptr<ppapi::host::ReplyMessageContext> open_context_;

  // PluginDelegate audio input object that we delegate audio IPC through.
  // We don't own this pointer but are responsible for calling Shutdown on it.
  webkit::ppapi::PluginDelegate::PlatformAudioInput* audio_input_;

  PepperDeviceEnumerationHostHelper enumeration_helper_;

  DISALLOW_COPY_AND_ASSIGN(PepperAudioInputHost);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_PEPPER_AUDIO_INPUT_HOST_H_
