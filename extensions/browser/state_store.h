// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_STATE_STORE_H_
#define CHROME_BROWSER_EXTENSIONS_STATE_STORE_H_

#include <set>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/browser/value_store/value_store_frontend.h"

namespace content {
class BrowserContext;
}

namespace extensions {

class ExtensionRegistry;

// A storage area for per-extension state that needs to be persisted to disk.
class StateStore : public base::SupportsWeakPtr<StateStore>,
                   public ExtensionRegistryObserver,
                   public content::NotificationObserver {
 public:
  typedef ValueStoreFrontend::ReadCallback ReadCallback;

  // If |deferred_load| is true, we won't load the database until the first
  // page has been loaded.
  StateStore(content::BrowserContext* context,
             const base::FilePath& db_path,
             bool deferred_load);
  // This variant is useful for testing (using a mock ValueStore).
  StateStore(content::BrowserContext* context, scoped_ptr<ValueStore> store);
  virtual ~StateStore();

  // Register a key for removal upon extension install/uninstall. We remove
  // for install to reset state when an extension upgrades.
  void RegisterKey(const std::string& key);

  // Get the value associated with the given extension and key, and pass
  // it to |callback| asynchronously.
  void GetExtensionValue(const std::string& extension_id,
                         const std::string& key,
                         ReadCallback callback);

  // Sets a value for a given extension and key.
  void SetExtensionValue(const std::string& extension_id,
                         const std::string& key,
                         scoped_ptr<base::Value> value);

  // Removes a value for a given extension and key.
  void RemoveExtensionValue(const std::string& extension_id,
                            const std::string& key);

  // Return whether or not the StateStore has initialized itself.
  bool IsInitialized() const;

 private:
  class DelayedTaskQueue;

  // content::NotificationObserver
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  void Init();

  // Removes all keys registered for the given extension.
  void RemoveKeysForExtension(const std::string& extension_id);

  // ExtensionRegistryObserver implementation.
  virtual void OnExtensionUninstalled(
      content::BrowserContext* browser_context,
      const Extension* extension,
      extensions::UninstallReason reason) OVERRIDE;
  virtual void OnExtensionWillBeInstalled(
      content::BrowserContext* browser_context,
      const Extension* extension,
      bool is_update,
      bool from_ephemeral,
      const std::string& old_name) OVERRIDE;

  // Path to our database, on disk. Empty during testing.
  base::FilePath db_path_;

  // The store that holds our key/values.
  ValueStoreFrontend store_;

  // List of all known keys. They will be cleared for each extension when it is
  // (un)installed.
  std::set<std::string> registered_keys_;

  // Keeps track of tasks we have delayed while starting up.
  scoped_ptr<DelayedTaskQueue> task_queue_;

  content::NotificationRegistrar registrar_;

  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
      extension_registry_observer_;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_STATE_STORE_H_
