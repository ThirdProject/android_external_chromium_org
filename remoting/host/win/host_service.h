// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_WIN_HOST_SERVICE_H_
#define REMOTING_HOST_WIN_HOST_SERVICE_H_

#include <windows.h>

#include <list>

#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/waitable_event.h"
#include "base/win/message_window.h"
#include "remoting/host/win/wts_terminal_monitor.h"

class CommandLine;

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace remoting {

class AutoThreadTaskRunner;
class DaemonProcess;
class WtsTerminalObserver;

class HostService : public base::win::MessageWindow::Delegate,
                    public WtsTerminalMonitor {
 public:
  static HostService* GetInstance();

  // This function parses the command line and selects the action routine.
  bool InitWithCommandLine(const CommandLine* command_line);

  // Invoke the choosen action routine.
  int Run();

  // WtsTerminalMonitor implementation
  virtual bool AddWtsTerminalObserver(const std::string& terminal_id,
                                      WtsTerminalObserver* observer) OVERRIDE;
  virtual void RemoveWtsTerminalObserver(
      WtsTerminalObserver* observer) OVERRIDE;

 private:
  HostService();
  ~HostService();

  // Notifies the service of changes in session state.
  void OnSessionChange(uint32 event, uint32 session_id);

  // Creates the process launcher.
  void CreateLauncher(scoped_refptr<AutoThreadTaskRunner> task_runner);

  // This function handshakes with the service control manager and starts
  // the service.
  int RunAsService();

  // Runs the service on the service thread. A separate routine is used to make
  // sure all local objects are destoyed by the time |stopped_event_| is
  // signalled.
  void RunAsServiceImpl();

  // This function starts the service in interactive mode (i.e. as a plain
  // console application).
  int RunInConsole();

  // Stops and deletes |daemon_process_|.
  void StopDaemonProcess();

  // base::win::MessageWindow::Delegate interface.
  virtual bool HandleMessage(HWND hwnd,
                             UINT message,
                             WPARAM wparam,
                             LPARAM lparam,
                             LRESULT* result) OVERRIDE;

  static BOOL WINAPI ConsoleControlHandler(DWORD event);

  // The control handler of the service.
  static DWORD WINAPI ServiceControlHandler(DWORD control,
                                            DWORD event_type,
                                            LPVOID event_data,
                                            LPVOID context);

  // The main service entry point.
  static VOID WINAPI ServiceMain(DWORD argc, WCHAR* argv[]);

  struct RegisteredObserver {
    // Unique identifier of the terminal to observe.
    std::string terminal_id;

    // Specifies ID of the attached session or |kInvalidSession| if no session
    // is attached to the WTS terminal.
    uint32 session_id;

    // Points to the observer receiving notifications about the WTS terminal
    // identified by |terminal_id|.
    WtsTerminalObserver* observer;
  };

  // The list of observers receiving session notifications.
  std::list<RegisteredObserver> observers_;

  scoped_ptr<DaemonProcess> daemon_process_;

  // Service message loop. |main_task_runner_| must be valid as long as the
  // Control+C or service notification handler is registered.
  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  // The action routine to be executed.
  int (HostService::*run_routine_)();

  // The service status handle.
  SERVICE_STATUS_HANDLE service_status_handle_;

  // A waitable event that is used to wait until the service is stopped.
  base::WaitableEvent stopped_event_;

  // Used to post session change notifications and control events.
  base::WeakPtrFactory<HostService> weak_factory_;
  base::WeakPtr<HostService> weak_ptr_;

  // Singleton.
  friend struct DefaultSingletonTraits<HostService>;

  DISALLOW_COPY_AND_ASSIGN(HostService);
};

}  // namespace remoting

#endif  // REMOTING_HOST_WIN_HOST_SERVICE_H_
