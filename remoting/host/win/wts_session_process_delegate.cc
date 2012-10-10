// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file implements the Windows service controlling Me2Me host processes
// running within user sessions.

#include "remoting/host/win/wts_session_process_delegate.h"

#include <sddl.h>
#include <limits>

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/single_thread_task_runner.h"
#include "base/time.h"
#include "base/timer.h"
#include "base/utf_string_conversions.h"
#include "base/win/scoped_handle.h"
#include "base/win/windows_version.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_channel_proxy.h"
#include "ipc/ipc_message.h"
#include "remoting/host/host_exit_codes.h"
#include "remoting/host/win/launch_process_with_token.h"
#include "remoting/host/win/worker_process_launcher.h"
#include "remoting/host/win/wts_console_monitor.h"
#include "remoting/host/worker_process_ipc_delegate.h"

using base::TimeDelta;
using base::win::ScopedHandle;

const FilePath::CharType kDaemonBinaryName[] =
    FILE_PATH_LITERAL("remoting_daemon.exe");

// The command line switch specifying the name of the daemon IPC endpoint.
const char kDaemonIpcSwitchName[] = "daemon-pipe";

const char kElevateSwitchName[] = "elevate";

// The command line parameters that should be copied from the service's command
// line to the host process.
const char* kCopiedSwitchNames[] = {
    "host-config", switches::kV, switches::kVModule };

namespace remoting {

// A private class actually implementing the functionality provided by
// |WtsSessionProcessDelegate|. This class is ref-counted and implements
// asynchronous fire-and-forget shutdown.
class WtsSessionProcessDelegate::Core
    : public base::RefCountedThreadSafe<WtsSessionProcessDelegate::Core>,
      public base::MessagePumpForIO::IOHandler,
      public WorkerProcessLauncher::Delegate {
 public:
  // The caller must ensure that |delegate| remains valid at least until
  // Stop() method has been called.
  Core(scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
       scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
       const FilePath& binary_path,
       bool launch_elevated);

  // base::MessagePumpForIO::IOHandler implementation.
  virtual void OnIOCompleted(base::MessagePumpForIO::IOContext* context,
                             DWORD bytes_transferred,
                             DWORD error) OVERRIDE;

  // WorkerProcessLauncher::Delegate implementation.
  virtual DWORD GetExitCode() OVERRIDE;
  virtual void KillProcess(DWORD exit_code) OVERRIDE;
  virtual bool LaunchProcess(
      const std::string& channel_name,
      base::win::ScopedHandle* process_exit_event_out) OVERRIDE;

  // Initializes the object returning true on success.
  bool Initialize(uint32 session_id);

  // Stops the object asynchronously.
  void Stop();

 private:
  friend class base::RefCountedThreadSafe<Core>;
  virtual ~Core();

  // Drains the completion port queue to make sure that all job object
  // notifications have been received.
  void DrainJobNotifications();

  // Notified that the completion port queue has been drained.
  void DrainJobNotificationsCompleted();

  // Creates and initializes the job object that will sandbox the launched child
  // processes.
  void InitializeJob();

  // Notified that the job object initialization is complete.
  void InitializeJobCompleted(scoped_ptr<base::win::ScopedHandle> job);

  // Called to process incoming job object notifications.
  void OnJobNotification(DWORD message, DWORD pid);

  // The task runner all public methods of this class should be called on.
  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  // The task runner serving job object notifications.
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // Path to the worker process binary.
  FilePath binary_path_;

  // The job object used to control the lifetime of child processes.
  base::win::ScopedHandle job_;

  // True if the worker process should be launched elevated.
  bool launch_elevated_;

  // A handle that becomes signalled once all processes associated with the job
  // have been terminated.
  base::win::ScopedHandle process_exit_event_;

  // The token to be used to launch a process in a different session.
  base::win::ScopedHandle session_token_;

  // True if Stop() has been called.
  bool stopping_;

  // The handle of the worker process, if launched.
  base::win::ScopedHandle worker_process_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

WtsSessionProcessDelegate::Core::Core(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    const FilePath& binary_path,
    bool launch_elevated)
    : main_task_runner_(main_task_runner),
      io_task_runner_(io_task_runner),
      binary_path_(binary_path),
      launch_elevated_(launch_elevated),
      stopping_(false) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
}

void WtsSessionProcessDelegate::Core::OnIOCompleted(
    base::MessagePumpForIO::IOContext* context,
    DWORD bytes_transferred,
    DWORD error) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  // |bytes_transferred| is used in job object notifications to supply
  // the message ID; |context| carries process ID.
  main_task_runner_->PostTask(FROM_HERE, base::Bind(
      &Core::OnJobNotification, this, bytes_transferred,
      reinterpret_cast<DWORD>(context)));
}

DWORD WtsSessionProcessDelegate::Core::GetExitCode() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  DWORD exit_code = CONTROL_C_EXIT;
  if (worker_process_.IsValid()) {
    if (!::GetExitCodeProcess(worker_process_, &exit_code)) {
      LOG_GETLASTERROR(INFO)
          << "Failed to query the exit code of the worker process";
      exit_code = CONTROL_C_EXIT;
    }
  }

  return exit_code;
}

void WtsSessionProcessDelegate::Core::KillProcess(DWORD exit_code) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (launch_elevated_) {
    if (job_.IsValid()) {
      TerminateJobObject(job_, exit_code);
    }
  } else {
    if (worker_process_.IsValid()) {
      TerminateProcess(worker_process_, exit_code);
    }
  }
}

bool WtsSessionProcessDelegate::Core::LaunchProcess(
    const std::string& channel_name,
    ScopedHandle* process_exit_event_out) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  CommandLine command_line(CommandLine::NO_PROGRAM);
  if (launch_elevated_) {
    // The job object is not ready. Retry starting the host process later.
    if (!job_.IsValid()) {
      return false;
    }

    // Construct the helper binary name.
    FilePath dir_path;
    if (!PathService::Get(base::DIR_EXE, &dir_path)) {
      LOG(ERROR) << "Failed to get the executable file name.";
      return false;
    }
    FilePath daemon_binary = dir_path.Append(kDaemonBinaryName);

    // Create the command line passing the name of the IPC channel to use and
    // copying known switches from the caller's command line.
    command_line.SetProgram(daemon_binary);
    command_line.AppendSwitchPath(kElevateSwitchName, binary_path_);

    CHECK(ResetEvent(process_exit_event_));
  } else {
    command_line.SetProgram(binary_path_);
  }

  // Create the command line passing the name of the IPC channel to use and
  // copying known switches from the caller's command line.
  command_line.AppendSwitchNative(kDaemonIpcSwitchName,
                                  UTF8ToWide(channel_name));
  command_line.CopySwitchesFrom(*CommandLine::ForCurrentProcess(),
                                kCopiedSwitchNames,
                                arraysize(kCopiedSwitchNames));

  // Try to launch the process.
  ScopedHandle worker_process;
  ScopedHandle worker_thread;
  if (!LaunchProcessWithToken(command_line.GetProgram(),
                              command_line.GetCommandLineString(),
                              session_token_,
                              CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB,
                              &worker_process,
                              &worker_thread)) {
    return false;
  }

  HANDLE local_process_exit_event;
  if (launch_elevated_) {
    if (!AssignProcessToJobObject(job_, worker_process)) {
      LOG_GETLASTERROR(ERROR)
          << "Failed to assign the worker to the job object";
      TerminateProcess(worker_process, CONTROL_C_EXIT);
      return false;
    }

    local_process_exit_event = process_exit_event_;
  } else {
    worker_process_ = worker_process.Pass();
    local_process_exit_event = worker_process_;
  }

  if (!ResumeThread(worker_thread)) {
    LOG_GETLASTERROR(ERROR) << "Failed to resume the worker thread";
    KillProcess(CONTROL_C_EXIT);
    return false;
  }

  // Return a handle that the caller can wait on to get notified when
  // the process terminates.
  ScopedHandle process_exit_event;
  if (!DuplicateHandle(GetCurrentProcess(),
                       local_process_exit_event,
                       GetCurrentProcess(),
                       process_exit_event.Receive(),
                       SYNCHRONIZE,
                       FALSE,
                       0)) {
    LOG_GETLASTERROR(ERROR) << "Failed to duplicate a handle";
    KillProcess(CONTROL_C_EXIT);
    return false;
  }

  *process_exit_event_out = process_exit_event.Pass();
  return true;
}

bool WtsSessionProcessDelegate::Core::Initialize(uint32 session_id) {
  if (base::win::GetVersion() == base::win::VERSION_XP)
    launch_elevated_ = false;

  if (launch_elevated_) {
    process_exit_event_.Set(CreateEvent(NULL, TRUE, FALSE, NULL));
    if (!process_exit_event_.IsValid()) {
      LOG(ERROR) << "Failed to create a nameless event";
      return false;
    }

    // To receive job object notifications the job object is registered with
    // the completion port represented by |io_task_runner|. The registration has
    // to be done on the I/O thread because
    // MessageLoopForIO::RegisterJobObject() can only be called via
    // MessageLoopForIO::current().
    io_task_runner_->PostTask(FROM_HERE,
                              base::Bind(&Core::InitializeJob, this));
  }

  // Create a session token for the launched process.
  return CreateSessionToken(session_id, &session_token_);
}

void WtsSessionProcessDelegate::Core::Stop() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (!stopping_) {
    stopping_ = true;

    // Drain the completion queue to make sure all job object notifications have
    // been received.
    DrainJobNotificationsCompleted();
  }
}

WtsSessionProcessDelegate::Core::~Core() {
}

void WtsSessionProcessDelegate::Core::DrainJobNotifications() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  // DrainJobNotifications() is posted after the job object is destroyed, so
  // by this time all notifications from the job object have been processed
  // already. Let the main thread know that the queue has been drained.
  main_task_runner_->PostTask(FROM_HERE, base::Bind(
      &Core::DrainJobNotificationsCompleted, this));
}

void WtsSessionProcessDelegate::Core::DrainJobNotificationsCompleted() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (job_.IsValid()) {
    job_.Close();

    // Drain the completion queue to make sure all job object notification have
    // been received.
    io_task_runner_->PostTask(FROM_HERE, base::Bind(
        &Core::DrainJobNotifications, this));
  }
}

void WtsSessionProcessDelegate::Core::InitializeJob() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  ScopedHandle job;
  job.Set(CreateJobObject(NULL, NULL));
  if (!job.IsValid()) {
    LOG_GETLASTERROR(ERROR) << "Failed to create a job object";
    return;
  }

  // Limit the number of active processes in the job to two (the process
  // performing elevation and the host) and make sure that all processes will be
  // killed once the job object is destroyed.
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
  memset(&info, 0, sizeof(info));
  info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_ACTIVE_PROCESS |
      JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  info.BasicLimitInformation.ActiveProcessLimit = 2;
  if (!SetInformationJobObject(job,
                               JobObjectExtendedLimitInformation,
                               &info,
                               sizeof(info))) {
    LOG_GETLASTERROR(ERROR) << "Failed to set limits on the job object";
    return;
  }

  // Register to receive job notifications via the I/O thread's completion port.
  if (!MessageLoopForIO::current()->RegisterJobObject(job, this)) {
    LOG_GETLASTERROR(ERROR)
        << "Failed to associate the job object with a completion port";
    return;
  }

  // ScopedHandle is not compatible with base::Passed, so we wrap it to a scoped
  // pointer.
  scoped_ptr<ScopedHandle> job_wrapper(new ScopedHandle());
  *job_wrapper = job.Pass();

  // Let the main thread know that initialization is complete.
  main_task_runner_->PostTask(FROM_HERE, base::Bind(
      &Core::InitializeJobCompleted, this, base::Passed(&job_wrapper)));
}

void WtsSessionProcessDelegate::Core::InitializeJobCompleted(
    scoped_ptr<ScopedHandle> job) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DCHECK(!job_.IsValid());

  job_ = job->Pass();
}

void WtsSessionProcessDelegate::Core::OnJobNotification(DWORD message,
                                                      DWORD pid) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  switch (message) {
    case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
      CHECK(SetEvent(process_exit_event_));
      break;

    case JOB_OBJECT_MSG_NEW_PROCESS:
      // We report the exit code of the worker process to be |CONTROL_C_EXIT|
      // if we cannot get the actual exit code. So here we can safely ignore
      // the error returned by OpenProcess().
      worker_process_.Set(OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid));
      break;
  }
}

WtsSessionProcessDelegate::WtsSessionProcessDelegate(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    const FilePath& binary_path,
    uint32 session_id,
    bool launch_elevated) {
  core_ = new Core(main_task_runner, io_task_runner, binary_path,
                   launch_elevated);
  if (!core_->Initialize(session_id)) {
    core_->Stop();
    core_ = NULL;
  }
}

WtsSessionProcessDelegate::~WtsSessionProcessDelegate() {
  core_->Stop();
}

DWORD WtsSessionProcessDelegate::GetExitCode() {
  if (!core_)
    return CONTROL_C_EXIT;

  return core_->GetExitCode();
}

void WtsSessionProcessDelegate::KillProcess(DWORD exit_code) {
  if (core_) {
    core_->KillProcess(exit_code);
  }
}

bool WtsSessionProcessDelegate::LaunchProcess(
    const std::string& channel_name,
    base::win::ScopedHandle* process_exit_event_out) {
  if (!core_)
    return false;

  return core_->LaunchProcess(channel_name, process_exit_event_out);
}

} // namespace remoting
