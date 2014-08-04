// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/autofill_data_validation.h"

#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/form_field_data.h"
#include "components/autofill/core/common/password_form_fill_data.h"
#include "url/gurl.h"

namespace autofill {

const size_t kMaxDataLength = 1024;

// Allow enough space for all countries (roughly 300 distinct values) and all
// timezones (roughly 400 distinct values), plus some extra wiggle room.
const size_t kMaxListSize = 512;

bool IsValidString(const std::string& str) {
  return str.size() <= kMaxDataLength;
}

bool IsValidString16(const base::string16& str) {
  return str.size() <= kMaxDataLength;
}

bool IsValidGURL(const GURL& url) {
  return url.is_empty() || url.is_valid();
}

bool IsValidFormFieldData(const FormFieldData& field) {
  return
      IsValidString16(field.label) &&
      IsValidString16(field.name) &&
      IsValidString16(field.value) &&
      IsValidString(field.form_control_type) &&
      IsValidString(field.autocomplete_attribute) &&
      IsValidString16Vector(field.option_values) &&
      IsValidString16Vector(field.option_contents);
}

bool IsValidFormData(const FormData& form) {
  if (!IsValidString16(form.name) ||
      !IsValidGURL(form.origin) ||
      !IsValidGURL(form.action))
    return false;

  if (form.fields.size() > kMaxListSize)
    return false;

  for (std::vector<FormFieldData>::const_iterator it = form.fields.begin();
       it != form.fields.end(); ++it) {
    if (!IsValidFormFieldData(*it))
      return false;
  }

  return true;
}

bool IsValidPasswordFormFillData(const PasswordFormFillData& form) {
  if (!IsValidFormData(form.basic_data) ||
      !IsValidString(form.preferred_realm))
    return false;

  for (PasswordFormFillData::LoginCollection::const_iterator it =
           form.additional_logins.begin();
       it != form.additional_logins.end(); ++it) {
    if (!IsValidString16(it->first) ||
        !IsValidString16(it->second.password) ||
        !IsValidString(it->second.realm))
      return false;
  }

  for (PasswordFormFillData::UsernamesCollection::const_iterator it =
           form.other_possible_usernames.begin();
       it != form.other_possible_usernames.end(); ++it) {
    if (!IsValidString16(it->first.username) ||
        !IsValidString16(it->first.password) ||
        !IsValidString(it->first.realm) ||
        !IsValidString16Vector(it->second))
      return false;
  }

  return true;
}

bool IsValidString16Vector(const std::vector<base::string16>& v) {
  if (v.size() > kMaxListSize)
    return false;

  for (std::vector<base::string16>::const_iterator it = v.begin();
       it != v.end(); ++it) {
    if (!IsValidString16(*it))
      return false;
  }

  return true;
}

bool IsValidFormDataVector(const std::vector<FormData>& v) {
  if (v.size() > kMaxListSize)
    return false;

  for (std::vector<FormData>::const_iterator it = v.begin(); it != v.end();
       ++it) {
    if (!IsValidFormData(*it))
      return false;
  }

  return true;
}

}  // namespace autofill
