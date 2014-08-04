// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_LOG_PRIVATE_LOG_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_LOG_PRIVATE_LOG_PRIVATE_API_H_

#include <set>
#include <string>

#include "base/scoped_observer.h"
#include "chrome/browser/extensions/api/log_private/filter_handler.h"
#include "chrome/browser/extensions/api/log_private/log_parser.h"
#include "chrome/browser/extensions/chrome_extension_function.h"
#include "chrome/browser/feedback/system_logs/about_system_logs_fetcher.h"
#include "chrome/common/extensions/api/log_private.h"
#include "extensions/browser/api/api_resource.h"
#include "extensions/browser/api/api_resource_manager.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/extension_registry_observer.h"
#include "net/base/net_log.h"
#include "net/base/net_log_logger.h"

class IOThread;

namespace content {
class BrowserContext;
}

namespace extensions {
class ExtensionRegistry;

// Tracked log files.
class FileResource : public ApiResource {
 public:
  FileResource(const std::string& owner_extension_id,
               const base::FilePath& path);
  virtual ~FileResource();

  // ApiResource overrides.
  virtual bool IsPersistent() const OVERRIDE;

  static const char kSequenceToken[];
  static const base::SequencedWorkerPool::WorkerShutdown kShutdownBehavior =
      base::SequencedWorkerPool::BLOCK_SHUTDOWN;

 private:
  base::FilePath path_;

  DISALLOW_COPY_AND_ASSIGN(FileResource);
};

class LogPrivateAPI : public BrowserContextKeyedAPI,
                      public ExtensionRegistryObserver,
                      public net::NetLog::ThreadSafeObserver {
 public:
  // Convenience method to get the LogPrivateAPI for a profile.
  static LogPrivateAPI* Get(content::BrowserContext* context);

  explicit LogPrivateAPI(content::BrowserContext* context);
  virtual ~LogPrivateAPI();

  void StartNetInternalsWatch(const std::string& extension_id,
                              api::log_private::EventSink event_sink,
                              const base::Closure& closure);
  void StopNetInternalsWatch(const std::string& extension_id,
                             const base::Closure& closure);
  void StopAllWatches(const std::string& extension_id,
                      const base::Closure& closure);
  void RegisterTempFile(const std::string& owner_extension_id,
                        const base::FilePath& file_path);

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<LogPrivateAPI>* GetFactoryInstance();

 private:
  friend class BrowserContextKeyedAPIFactory<LogPrivateAPI>;

  void Initialize();
  // ExtensionRegistryObserver implementation.
  virtual void OnExtensionUnloaded(
      content::BrowserContext* browser_context,
      const Extension* extension,
      UnloadedExtensionInfo::Reason reason) OVERRIDE;

  // ChromeNetLog::ThreadSafeObserver implementation:
  virtual void OnAddEntry(const net::NetLog::Entry& entry) OVERRIDE;

  void PostPendingEntries();
  void AddEntriesOnUI(scoped_ptr<base::ListValue> value);

  // Initializes a new instance of net::NetLogLogger and passes it back via
  // |net_logger| param.
  void InitializeNetLogger(const std::string& owner_extension_id,
                           net::NetLogLogger** net_logger);

  // Starts observing network events with a new |net_logger| instance.
  void StartObservingNetEvents(IOThread* io_thread,
                               net::NetLogLogger** net_logger);
  void MaybeStartNetInternalLogging(const std::string& caller_extension_id,
                                    IOThread* io_thread,
                                    api::log_private::EventSink event_sink);
  void MaybeStopNetInternalLogging(const base::Closure& closure);
  void StopNetInternalLogging();

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() {
    return "LogPrivateAPI";
  }
  static const bool kServiceIsNULLWhileTesting = true;
  static const bool kServiceRedirectedInIncognito = true;

  content::BrowserContext* const browser_context_;
  bool logging_net_internals_;
  api::log_private::EventSink event_sink_;
  std::set<std::string> net_internal_watches_;
  scoped_ptr<base::ListValue> pending_entries_;
  scoped_ptr<net::NetLogLogger> net_log_logger_;
  // Listen to extension unloaded notifications.
  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
      extension_registry_observer_;
  ApiResourceManager<FileResource, WorkerPoolThreadTraits<FileResource> >
      log_file_resources_;
  bool initialized_;

  DISALLOW_COPY_AND_ASSIGN(LogPrivateAPI);
};

class LogPrivateGetHistoricalFunction : public AsyncExtensionFunction {
 public:
  LogPrivateGetHistoricalFunction();
  DECLARE_EXTENSION_FUNCTION("logPrivate.getHistorical",
                             LOGPRIVATE_GETHISTORICAL);

 protected:
  virtual ~LogPrivateGetHistoricalFunction();
  virtual bool RunAsync() OVERRIDE;

 private:
  void OnSystemLogsLoaded(scoped_ptr<system_logs::SystemLogsResponse> sys_info);

  scoped_ptr<FilterHandler> filter_handler_;

  DISALLOW_COPY_AND_ASSIGN(LogPrivateGetHistoricalFunction);
};

class LogPrivateStartEventRecorderFunction : public AsyncExtensionFunction {
 public:
  LogPrivateStartEventRecorderFunction();
  DECLARE_EXTENSION_FUNCTION("logPrivate.startEventRecorder",
                             LOGPRIVATE_STARTEVENTRECODER);

 protected:
  virtual ~LogPrivateStartEventRecorderFunction();
  virtual bool RunAsync() OVERRIDE;

 private:
  void OnEventRecorderStarted();

  DISALLOW_COPY_AND_ASSIGN(LogPrivateStartEventRecorderFunction);
};

class LogPrivateStopEventRecorderFunction : public AsyncExtensionFunction {
 public:
  LogPrivateStopEventRecorderFunction();
  DECLARE_EXTENSION_FUNCTION("logPrivate.stopEventRecorder",
                             LOGPRIVATE_STOPEVENTRECODER);

 protected:
  virtual ~LogPrivateStopEventRecorderFunction();

  // AsyncExtensionFunction overrides.
  virtual bool RunAsync() OVERRIDE;

 private:
  void OnEventRecorderStopped();

  DISALLOW_COPY_AND_ASSIGN(LogPrivateStopEventRecorderFunction);
};

class LogPrivateDumpLogsFunction : public AsyncExtensionFunction {
 public:
  LogPrivateDumpLogsFunction();
  DECLARE_EXTENSION_FUNCTION("logPrivate.dumpLogs", LOGPRIVATE_DUMPLOGS);

 protected:
  virtual ~LogPrivateDumpLogsFunction();

  // AsyncExtensionFunction overrides.
  virtual bool RunAsync() OVERRIDE;

 private:
  // Callback for DebugLogWriter::StoreLogs() call.
  void OnStoreLogsCompleted(const base::FilePath& log_path, bool succeeded);
  // Callback for LogPrivateAPI::StopAllWatches() call.
  void OnStopAllWatches();
  DISALLOW_COPY_AND_ASSIGN(LogPrivateDumpLogsFunction);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_LOG_PRIVATE_LOG_PRIVATE_API_H_
