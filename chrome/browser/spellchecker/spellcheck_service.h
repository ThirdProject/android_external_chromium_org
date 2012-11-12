// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SPELLCHECKER_SPELLCHECK_SERVICE_H_
#define CHROME_BROWSER_SPELLCHECKER_SPELLCHECK_SERVICE_H_

#include "base/gtest_prod_util.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/prefs/public/pref_change_registrar.h"
#include "base/prefs/public/pref_observer.h"
#include "chrome/browser/profiles/profile_keyed_service.h"
#include "chrome/browser/spellchecker/spellcheck_custom_dictionary.h"
#include "chrome/browser/spellchecker/spellcheck_hunspell_dictionary.h"
#include "chrome/common/spellcheck_common.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class Profile;
class SpellCheckHostMetrics;

namespace base {
class WaitableEvent;
}

namespace content {
class RenderProcessHost;
}

// Encapsulates the browser side spellcheck service. There is one of these per
// profile and each is created by the SpellCheckServiceFactory.  The
// SpellCheckService maintains any per-profile information about spellcheck.
class SpellcheckService : public ProfileKeyedService,
                          public PrefObserver,
                          public content::NotificationObserver {
 public:
  // Event types used for reporting the status of this class and its derived
  // classes to browser tests.
  enum EventType {
    BDICT_NOTINITIALIZED,
    BDICT_CORRUPTED,
  };

  explicit SpellcheckService(Profile* profile);
  virtual ~SpellcheckService();

  // This function computes a vector of strings which are to be displayed in
  // the context menu over a text area for changing spell check languages. It
  // returns the index of the current spell check language in the vector.
  // TODO(port): this should take a vector of string16, but the implementation
  // has some dependencies in l10n util that need porting first.
  static int GetSpellCheckLanguages(Profile* profile,
                                    std::vector<std::string>* languages);

  // Computes a vector of strings which are to be displayed in the context
  // menu from |accept_languages| and |dictionary_language|.
  static void GetSpellCheckLanguagesFromAcceptLanguages(
      const std::vector<std::string>& accept_languages,
      const std::string& dictionary_language,
      std::vector<std::string>* languages);

  // Signals the event attached by AttachTestEvent() to report the specified
  // event to browser tests. This function is called by this class and its
  // derived classes to report their status. This function does not do anything
  // when we do not set an event to |status_event_|.
  static bool SignalStatusEvent(EventType type);

  void Initialize();

  // Instantiates SpellCheckHostMetrics object and makes it ready for recording
  // metrics. This should be called only if the metrics recording is active.
  void StartRecordingMetrics(bool spellcheck_enabled);

  // Pass the renderer some basic intialization information. Note that the
  // renderer will not load Hunspell until it needs to.
  void InitForRenderer(content::RenderProcessHost* process);

  // Returns in-memory cache of custom word list.
  const chrome::spellcheck_common::WordList& GetCustomWords();

  // Invoked on the Ui thread when new custom word is registered.
  void CustomWordAddedLocally(const std::string& word);

  // Loads the custom dictionary associated with this profile into the
  // passed in WordList.
 void LoadDictionaryIntoCustomWordList(
      chrome::spellcheck_common::WordList* custom_words);

  // Writes a word to the custom dictionary associated with this profile.
  void WriteWordToCustomDictionary(const std::string& word);

  // Adds the given word to the custom words list and inform renderer of the
  // update.
  void AddWord(const std::string& word);

  // Returns true if the dictionary is ready to use.
  bool IsReady() const;

  // Hunspell dictionary functions.
  virtual const base::PlatformFile& GetDictionaryFile() const;
  virtual const std::string& GetLanguage() const;
  virtual bool IsUsingPlatformChecker() const;

  // The reply point for PostTaskAndReply. Called when AddWord is finished
  // adding a word in the background.
  void AddWordComplete(const std::string& word);

  // Returns a metrics counter associated with this object,
  // or null when metrics recording is disabled.
  SpellCheckHostMetrics* GetMetrics() const;

  // Returns the instance of the custom dictionary. Custom dictionary
  // will be lazily initialized.
  SpellcheckCustomDictionary* GetCustomDictionary();

  // Inform |profile_| that initialization has finished.
  // |custom_words| holds the custom word list which was
  // loaded at the file thread.
  void InformProfileOfInitializationWithCustomWords(
      chrome::spellcheck_common::WordList* custom_words);

  // NotificationProfile implementation.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // PrefObserver implementation.
  virtual void OnPreferenceChanged(PrefServiceBase* service,
                                   const std::string& pref_name) OVERRIDE;
 private:
  FRIEND_TEST_ALL_PREFIXES(SpellcheckServiceBrowserTest, DeleteCorruptedBDICT);

  // Attaches an event so browser tests can listen the status events.
  static void AttachStatusEvent(base::WaitableEvent* status_event);

  // Waits until a spellchecker updates its status. This function returns
  // immediately when we do not set an event to |status_event_|.
  static EventType WaitStatusEvent();

  PrefChangeRegistrar pref_change_registrar_;
  content::NotificationRegistrar registrar_;

  // A pointer to the profile which this service refers to.
  Profile* profile_;

  scoped_ptr<SpellCheckHostMetrics> metrics_;

  scoped_ptr<SpellcheckCustomDictionary> custom_dictionary_;

  scoped_ptr<SpellcheckHunspellDictionary> hunspell_dictionary_;

  base::WeakPtrFactory<SpellcheckService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SpellcheckService);
};

#endif  // CHROME_BROWSER_SPELLCHECKER_SPELLCHECK_SERVICE_H_
