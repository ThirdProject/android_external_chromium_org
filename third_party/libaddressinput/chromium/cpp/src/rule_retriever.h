// Copyright (C) 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// An object to retrieve validation rules.

#ifndef I18N_ADDRESSINPUT_RULE_RETRIEVER_H_
#define I18N_ADDRESSINPUT_RULE_RETRIEVER_H_

#include <libaddressinput/callback.h>
#include <libaddressinput/util/basictypes.h>
#include <libaddressinput/util/scoped_ptr.h>

#include <string>

namespace i18n {
namespace addressinput {

class Retriever;
class Rule;

// Retrieves validation rules. Sample usage:
//    const Retriever* retriever = ...
//    RuleRetriever rules(retriever);
//    scoped_ptr<RuleRetriever::Callback> rule_ready(BuildCallback(
//        this, &MyClass::OnRuleReady));
//    rules.RetrieveRule("data/CA/AB--fr", *rule_ready);
class RuleRetriever {
 public:
  typedef i18n::addressinput::Callback<std::string, Rule> Callback;

  // Takes ownership of |retriever|.
  explicit RuleRetriever(const Retriever* retriever);
  ~RuleRetriever();

  // Retrieves the rule for |key| and invokes the |rule_ready| callback.
  void RetrieveRule(const std::string& key, const Callback& rule_ready) const;

 private:
  scoped_ptr<const Retriever> data_retriever_;

  DISALLOW_COPY_AND_ASSIGN(RuleRetriever);
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_RULE_RETRIEVER_H_
