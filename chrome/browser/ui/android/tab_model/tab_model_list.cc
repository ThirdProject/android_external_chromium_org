// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/tab_model/tab_model_list.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"

namespace {

// Maintains and gives access to a static list of TabModel instances.
static TabModelList::TabModelVector& tab_models() {
  CR_DEFINE_STATIC_LOCAL(TabModelList::TabModelVector,
                         tab_model_vector, ());
  return tab_model_vector;
}

}  // namespace

void TabModelList::AddTabModel(TabModel* tab_model) {
  DCHECK(tab_model);
  tab_models().push_back(tab_model);
}

void TabModelList::RemoveTabModel(TabModel* tab_model) {
  DCHECK(tab_model);
  TabModelList::iterator remove_tab_model =
      std::find(tab_models().begin(), tab_models().end(), tab_model);

  if (remove_tab_model != tab_models().end())
    tab_models().erase(remove_tab_model);
}

TabModel* TabModelList::GetTabModelWithProfile(
    Profile* profile) {
  for (TabModelList::const_iterator i = TabModelList::begin();
      i != TabModelList::end(); ++i) {
    if ((*i)->GetProfile()->IsSameProfile(profile))
      return *i;
  }

  return NULL;
}

TabModel* TabModelList::FindTabModelWithId(
    SessionID::id_type desired_id) {
  for (TabModelList::const_iterator i = TabModelList::begin();
      i != TabModelList::end(); i++) {
    if ((*i)->GetSessionId() == desired_id)
      return *i;
  }

  return NULL;
}

bool TabModelList::IsOffTheRecordSessionActive() {
  for (TabModelList::const_iterator i = TabModelList::begin();
      i != TabModelList::end(); i++) {
    if ((*i)->GetProfile()->IsOffTheRecord() && (*i)->GetTabCount() > 0)
      return true;
  }

  return false;
}

TabModelList::const_iterator TabModelList::begin() {
  return tab_models().begin();
}

TabModelList::const_iterator TabModelList::end() {
  return tab_models().end();
}

bool TabModelList::empty() {
  return tab_models().empty();
}

size_t TabModelList::size() {
  return tab_models().size();
}
