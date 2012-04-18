// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_INPUT_METHOD_IBUS_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_INPUT_METHOD_IBUS_CONTROLLER_H_
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace chromeos {
namespace input_method {

struct InputMethodConfigValue;
struct InputMethodProperty;
typedef std::vector<InputMethodProperty> InputMethodPropertyList;

// IBusController is used to interact with the system input method framework
// (which is currently IBus).
class IBusController {
 public:
  class Observer {
   public:
    virtual ~Observer() {}
    virtual void PropertyChanged() = 0;
    // TODO(yusukes): Add functions for IPC error handling.
  };

  // Creates an instance of the class.
  static IBusController* Create();

  virtual ~IBusController();

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Starts the system input method framework. No-op if it's already started.
  virtual bool Start() = 0;

  // Stops the system input method framework.
  virtual bool Stop() = 0;

  // Sets a configuration of an input method engine. Returns true if the
  // configuration is successfully set. For example, when you set
  // "engine/Mozc/history_learning_level", |section| should be "engine/Mozc",
  // and |config_name| should be "history_learning_level".
  virtual bool SetInputMethodConfig(const std::string& section,
                                    const std::string& config_name,
                                    const InputMethodConfigValue& value) = 0;

  // Changes the current input method engine to |id|. Returns true on success.
  // Example IDs: "mozc", "m17n:ar:kbd".
  virtual bool ChangeInputMethod(const std::string& id) = 0;

  // Activates the input method property specified by the |key|. Returns true on
  // success.
  virtual bool ActivateInputMethodProperty(const std::string& key) = 0;

  // Gets the latest input method property send from the system input method
  // framework.
  virtual const InputMethodPropertyList& GetCurrentProperties() const = 0;

#if defined(USE_VIRTUAL_KEYBOARD)
  typedef std::vector<std::pair<double, double> > HandwritingStroke;

  // Sends a handwriting stroke to the system input method. The std::pair
  // contains x and y coordinates. (0.0, 0.0) represents the top-left corner of
  // a handwriting area, and (1.0, 1.0) does the bottom-right. For example, the
  // second stroke for U+30ED (Katakana character Ro) would be something like
  // [(0,0), (1,0), (1,1)].  stroke.size() should always be >= 2 (i.e. a single
  // dot is not allowed).
  virtual void SendHandwritingStroke(const HandwritingStroke& stroke) = 0;

  // Clears the last N handwriting strokes. Pass zero for clearing all strokes.
  // TODO(yusukes): Currently ibus-daemon only accepts 0 for |n_strokes|.
  virtual void CancelHandwriting(int n_strokes) = 0;
#endif
};

}  // namespace input_method
}  // namespace chromeos

// TODO(yusukes,nona): This interface does not depend on IBus actually.
// Rename the file if needed.

#endif  // CHROME_BROWSER_CHROMEOS_INPUT_METHOD_IBUS_CONTROLLER_H_
