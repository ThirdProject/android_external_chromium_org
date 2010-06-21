// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_INPUT_METHOD_INPUT_METHOD_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_INPUT_METHOD_INPUT_METHOD_UTIL_H_

#include <string>
#include <vector>

#include "base/string16.h"
#include "chrome/browser/chromeos/cros/language_library.h"

namespace chromeos {
namespace input_method {

// The list of language that do not have associated input methods. For
// these languages, we associate input methods here.
const struct ExtraLanguage {
  const char* language_code;
  const char* input_method_id;
} kExtraLanguages[] = {
  { "id", "xkb:us::eng" }, // For Indonesian, use US keyboard layout.
  // The code "fil" comes from app/l10_util.cc.
  { "fil", "xkb:us::eng" },  // For Filipino, use US keyboard layout.
  // The code "es-419" comes from app/l10_util.cc.
  // For Spanish in Latin America, use Spanish keyboard layout.
  { "es-419", "xkb:es::spa" },
};
// TODO(yusukes): Move |kExtraLanguages| to input_method_util.cc.

// Converts a string sent from IBus IME engines, which is written in English,
// into Chrome's string ID, then pulls internationalized resource string from
// the resource bundle and returns it. These functions are not thread-safe.
// Non-UI threads are not allowed to call them.
std::wstring GetString(const std::string& english_string);
std::string GetStringUTF8(const std::string& english_string);
string16 GetStringUTF16(const std::string& english_string);

// This method is ONLY for unit testing. Returns true if the given string is
// supported (i.e. the string is associated with a resource ID).
bool StringIsSupported(const std::string& english_string);

// Normalizes the language code and returns the normalized version.  The
// function normalizes the given language code to be compatible with the
// one used in Chrome's application locales. Otherwise, returns the
// given language code as-is.
//
// Examples:
//
// - "zh_CN" => "zh-CN" (Use - instead of _)
// - "jpn"   => "ja"    (Use two-letter code)
// - "t"     => "t"     (Return as-is if unknown)
std::string NormalizeLanguageCode(const std::string& language_code);

// Returns true if the given input method id is for a keyboard layout.
bool IsKeyboardLayout(const std::string& input_method_id);

// Gets the language code from the given input method descriptor.  This
// encapsulates differences between the language codes used in
// InputMethodDescriptor and Chrome's application locale codes.
std::string GetLanguageCodeFromDescriptor(
    const InputMethodDescriptor& descriptor);

// Gets the keyboard layout name from the given input method ID.
// If the ID is invalid, the default layout name will be returned.
//
// Examples:
//
// "xkb:us::eng"       => "us"
// "xkb:us:dvorak:eng" => "us(dvorak)"
std::string GetKeyboardLayoutName(const std::string& input_method_id);

// Rewrites the language name and returns the modified version if
// necessary. Otherwise, returns the given language name as is.
// In particular, this rewrites the special language name used for input
// methods that don't fall under any other languages.
std::wstring MaybeRewriteLanguageName(const std::wstring& language_name);

// Converts an input method ID to a language code of the IME. Returns "Eng"
// when |input_method_id| is unknown.
// Example: "hangul" => "ko"
std::string GetLanguageCodeFromInputMethodId(
    const std::string& input_method_id);

// Converts an input method ID to a display name of the IME. Returns
// "USA" (US keyboard) when |input_method_id| is unknown.
// Examples: "pinyin" => "Pinyin"
//           "m17n:ar:kbd" => "kbd (m17n)"
std::string GetInputMethodDisplayNameFromId(const std::string& input_method_id);

// Converts a language code to a language display name, using the
// current application locale. MaybeRewriteLanguageName() is called
// internally.
// Examples: "fr"    => "French"
//           "en-US" => "English (United States)"
std::wstring GetLanguageDisplayNameFromCode(const std::string& language_code);

// Sorts the given language codes by their corresponding language names,
// using the unicode string comparator. Uses unstable sorting.
void SortLanguageCodesByNames(std::vector<std::string>* language_codes);

// Sorts the given input method ids by their corresponding language names,
// using the unicode string comparator. Uses stable sorting.
void SortInputMethodIdsByNames(std::vector<std::string>* input_method_ids);

// This function is only for unit tests. Do not use this.
void SortInputMethodIdsByNamesInternal(
    const std::map<std::string, std::string>& id_to_language_code_map,
    std::vector<std::string>* input_method_ids);

// Reorders the given input method ids for the language code. For
// example, if |language_codes| is "fr" and |input_method_ids| contains
// ["xkb:be::fra", and "xkb:fr::fra"], the list is reordered to
// ["xkb:fr::fra", and "xkb:be::fra"], so that French keyboard layout
// comes before Belgian keyboard layout.
void ReorderInputMethodIdsForLanguageCode(
    const std::string& language_code,
    std::vector<std::string>* input_method_ids);

// Gets input method ids that belong to |language_code|.
// If |keyboard_layout_only| is true, the function does not return input methods
// that are not for keybord layout switching. Returns true on success. Note that
// the function might return false if ibus-daemon is not running, or
// |language_code| is unknown.
bool GetInputMethodIdsFromLanguageCode(
    const std::string& language_code,
    bool keyboard_layout_only,
    std::vector<std::string>* out_input_method_ids);

}  // namespace input_method
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_INPUT_METHOD_INPUT_METHOD_UTIL_H_
