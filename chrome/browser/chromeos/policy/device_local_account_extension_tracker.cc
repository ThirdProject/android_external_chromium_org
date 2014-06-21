// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/device_local_account_extension_tracker.h"

#include "base/logging.h"
#include "base/prefs/pref_value_map.h"
#include "base/values.h"
#include "chrome/browser/chromeos/policy/device_local_account.h"
#include "chrome/browser/extensions/policy_handlers.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/schema.h"
#include "components/policy/core/common/schema_registry.h"
#include "extensions/browser/pref_names.h"

namespace policy {

DeviceLocalAccountExtensionTracker::DeviceLocalAccountExtensionTracker(
    const DeviceLocalAccount& account,
    CloudPolicyStore* store,
    SchemaRegistry* schema_registry)
    : store_(store),
      schema_registry_(schema_registry) {
  if (account.type == DeviceLocalAccount::TYPE_KIOSK_APP) {
    // This is easy: Just add a component for the app id.
    PolicyNamespace ns(POLICY_DOMAIN_EXTENSIONS, account.kiosk_app_id);
    schema_registry_->RegisterComponent(ns, Schema());
  } else if (account.type == DeviceLocalAccount::TYPE_PUBLIC_SESSION) {
    // For public sessions, track the value of the ExtensionInstallForcelist
    // policy.
    store_->AddObserver(this);
    UpdateFromStore();
  } else {
    NOTREACHED();
  }

  schema_registry_->SetReady(POLICY_DOMAIN_EXTENSIONS);
}

DeviceLocalAccountExtensionTracker::~DeviceLocalAccountExtensionTracker() {
  store_->RemoveObserver(this);
}

void DeviceLocalAccountExtensionTracker::OnStoreLoaded(
    CloudPolicyStore* store) {
  UpdateFromStore();
}

void DeviceLocalAccountExtensionTracker::OnStoreError(CloudPolicyStore* store) {
  UpdateFromStore();
}

void DeviceLocalAccountExtensionTracker::UpdateFromStore() {
  const policy::PolicyMap& policy_map = store_->policy_map();

  extensions::ExtensionInstallForcelistPolicyHandler policy_handler;
  if (!policy_handler.CheckPolicySettings(policy_map, NULL))
    return;

  PrefValueMap pref_value_map;
  policy_handler.ApplyPolicySettings(policy_map, &pref_value_map);

  const base::Value* value = NULL;
  const base::DictionaryValue* dict = NULL;
  if (!pref_value_map.GetValue(extensions::pref_names::kInstallForceList,
                               &value) ||
      !value->GetAsDictionary(&dict)) {
    return;
  }

  for (base::DictionaryValue::Iterator it(*dict); !it.IsAtEnd(); it.Advance()) {
    PolicyNamespace ns(POLICY_DOMAIN_EXTENSIONS, it.key());
    schema_registry_->RegisterComponent(ns, Schema());
  }

  // Removing an extension from a public session at runtime can happen but is
  // a rare event. In that case we leave the extension ID in the SchemaRegistry,
  // and it will be purged on the next restart.
}

}  // namespace policy
