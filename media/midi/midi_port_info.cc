// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/midi/midi_port_info.h"

#include <iostream>

using std::cout;

namespace media {

MIDIPortInfo::MIDIPortInfo() {}

MIDIPortInfo::MIDIPortInfo(const std::string& in_id,
                           const std::string& in_manufacturer,
                           const std::string& in_name,
                           const std::string& in_version)
    : id(in_id),
      manufacturer(in_manufacturer),
      name(in_name),
      version(in_version) {}

MIDIPortInfo::~MIDIPortInfo() {}

MIDIPortInfo::MIDIPortInfo(const MIDIPortInfo& info)
    : id(info.id),
      manufacturer(info.manufacturer),
      name(info.name),
      version(info.version) {}

}  // namespace media
