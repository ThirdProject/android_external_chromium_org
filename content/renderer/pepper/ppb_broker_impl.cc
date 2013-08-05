// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/ppb_broker_impl.h"

#include "base/logging.h"
#include "content/renderer/pepper/common.h"
#include "content/renderer/pepper/host_globals.h"
#include "content/renderer/pepper/pepper_broker.h"
#include "content/renderer/pepper/pepper_helper_impl.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/plugin_module.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/shared_impl/platform_file.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"

using ppapi::PlatformFileToInt;
using ppapi::thunk::PPB_Broker_API;
using ppapi::TrackedCallback;

namespace content {

// PPB_Broker_Impl ------------------------------------------------------

PPB_Broker_Impl::PPB_Broker_Impl(PP_Instance instance)
    : Resource(::ppapi::OBJECT_IS_IMPL, instance),
      broker_(NULL),
      connect_callback_(),
      pipe_handle_(PlatformFileToInt(base::kInvalidPlatformFileValue)) {
}

PPB_Broker_Impl::~PPB_Broker_Impl() {
  if (broker_) {
    broker_->Disconnect(this);
    broker_ = NULL;
  }

  // The plugin owns the handle.
  pipe_handle_ = PlatformFileToInt(base::kInvalidPlatformFileValue);
}

PPB_Broker_API* PPB_Broker_Impl::AsPPB_Broker_API() {
  return this;
}

int32_t PPB_Broker_Impl::Connect(
    scoped_refptr<TrackedCallback> connect_callback) {
  // TODO(ddorwin): Return PP_ERROR_FAILED if plugin is in-process.

  if (broker_) {
    // May only be called once.
    return PP_ERROR_FAILED;
  }

  PepperPluginInstanceImpl* plugin_instance =
      HostGlobals::Get()->GetInstance(pp_instance());
  if (!plugin_instance)
    return PP_ERROR_FAILED;

  // The callback must be populated now in case we are connected to the broker
  // and BrokerConnected is called before ConnectToBroker returns.
  // Because it must be created now, it must be aborted and cleared if
  // ConnectToBroker fails.
  connect_callback_ = connect_callback;

  broker_ = plugin_instance->helper()->ConnectToBroker(this);
  if (!broker_) {
    connect_callback_->Abort();
    return PP_ERROR_FAILED;
  }

  return PP_OK_COMPLETIONPENDING;
}

int32_t PPB_Broker_Impl::GetHandle(int32_t* handle) {
  if (pipe_handle_ == PlatformFileToInt(base::kInvalidPlatformFileValue))
    return PP_ERROR_FAILED;  // Handle not set yet.
  *handle = pipe_handle_;
  return PP_OK;
}

GURL PPB_Broker_Impl::GetDocumentUrl() {
  PepperPluginInstanceImpl* plugin_instance =
      HostGlobals::Get()->GetInstance(pp_instance());
  return plugin_instance->container()->element().document().url();
}

// Transfers ownership of the handle to the plugin.
void PPB_Broker_Impl::BrokerConnected(int32_t handle, int32_t result) {
  DCHECK(pipe_handle_ ==
         PlatformFileToInt(base::kInvalidPlatformFileValue));
  DCHECK(result == PP_OK ||
         handle == PlatformFileToInt(base::kInvalidPlatformFileValue));

  pipe_handle_ = handle;

  // Synchronous calls are not supported.
  DCHECK(TrackedCallback::IsPending(connect_callback_));

  connect_callback_->Run(result);
}

}  // namespace content
