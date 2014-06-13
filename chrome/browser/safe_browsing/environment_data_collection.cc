// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/environment_data_collection.h"

#include <string>

#include "base/cpu.h"
#include "base/sys_info.h"
#include "chrome/common/chrome_version_info.h"
#include "chrome/common/safe_browsing/csd.pb.h"

namespace safe_browsing {

// Populates |process| with platform-specific data related to the chrome browser
// process.
void CollectPlatformProcessData(
    ClientIncidentReport_EnvironmentData_Process* process);

namespace {

ClientIncidentReport_EnvironmentData_Process_Channel MapChannelToProtobuf(
    chrome::VersionInfo::Channel channel) {
  typedef chrome::VersionInfo VersionInfo;
  typedef ClientIncidentReport_EnvironmentData_Process Process;
  switch (channel) {
    case VersionInfo::CHANNEL_CANARY:
      return Process::CHANNEL_CANARY;
    case VersionInfo::CHANNEL_DEV:
      return Process::CHANNEL_DEV;
    case VersionInfo::CHANNEL_BETA:
      return Process::CHANNEL_BETA;
    case VersionInfo::CHANNEL_STABLE:
      return Process::CHANNEL_STABLE;
    default:
      return Process::CHANNEL_UNKNOWN;
  }
}

// Populates |process| with data related to the chrome browser process.
void CollectProcessData(ClientIncidentReport_EnvironmentData_Process* process) {
  chrome::VersionInfo version_info;
  if (version_info.is_valid()) {
    // TODO(grt): Move this logic into VersionInfo (it also appears in
    // ChromeMetricsServiceClient).
    std::string version(version_info.Version());
#if defined(ARCH_CPU_64_BITS)
    version += "-64";
#endif  // defined(ARCH_CPU_64_BITS)
    if (!version_info.IsOfficialBuild())
      version += "-devel";
    process->set_version(version);
  }

  process->set_chrome_update_channel(
      MapChannelToProtobuf(chrome::VersionInfo::GetChannel()));

  CollectPlatformProcessData(process);
}

}  // namespace

void CollectEnvironmentData(ClientIncidentReport_EnvironmentData* data) {
  // OS
  {
    ClientIncidentReport_EnvironmentData_OS* os = data->mutable_os();
    os->set_os_name(base::SysInfo::OperatingSystemName());
    os->set_os_version(base::SysInfo::OperatingSystemVersion());
  }

  // Machine
  {
    base::CPU cpu_info;
    ClientIncidentReport_EnvironmentData_Machine* machine =
        data->mutable_machine();
    machine->set_cpu_architecture(base::SysInfo::OperatingSystemArchitecture());
    machine->set_cpu_vendor(cpu_info.vendor_name());
    machine->set_cpuid(cpu_info.signature());
  }

  // Process
  CollectProcessData(data->mutable_process());
}

#if !defined(OS_WIN)
void CollectPlatformProcessData(
    ClientIncidentReport_EnvironmentData_Process* process) {
  // Empty implementation for platforms that do not (yet) have their own
  // implementations.
}
#endif

}  // namespace safe_browsing
