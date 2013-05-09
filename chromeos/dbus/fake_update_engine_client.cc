// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_update_engine_client.h"

namespace chromeos {

FakeUpdateEngineClient::FakeUpdateEngineClient()
  : update_check_result_(UpdateEngineClient::UPDATE_RESULT_SUCCESS),
    reboot_after_update_call_count_(0) {
}

FakeUpdateEngineClient::~FakeUpdateEngineClient() {
}

void FakeUpdateEngineClient::AddObserver(Observer* observer) {
}

void FakeUpdateEngineClient::RemoveObserver(Observer* observer) {
}

bool FakeUpdateEngineClient::HasObserver(Observer* observer) {
  return false;
}

void FakeUpdateEngineClient::RequestUpdateCheck(
    const UpdateCheckCallback& callback) {
  callback.Run(update_check_result_);
}

void FakeUpdateEngineClient::RebootAfterUpdate() {
  reboot_after_update_call_count_++;
}

void FakeUpdateEngineClient::SetReleaseTrack(const std::string& track) {
}

void FakeUpdateEngineClient::GetReleaseTrack(
    const GetReleaseTrackCallback& callback) {
}

FakeUpdateEngineClient::Status FakeUpdateEngineClient::GetLastStatus() {
  return update_engine_client_status_;
}

void FakeUpdateEngineClient::set_update_engine_client_status(
    const UpdateEngineClient::Status& status) {
  update_engine_client_status_ = status;
}

void FakeUpdateEngineClient::set_update_check_result(
    const UpdateEngineClient::UpdateCheckResult& result) {
  update_check_result_ = result;
}

}  // namespace chromeos
