// Copyright 2011 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cassert>

#include "eval_env.h"

using namespace std;

BindingEnv::~BindingEnv() {
  for(const auto& rule : rules_) {
    if (rule.second->name() != "phony")
      delete rule.second;
  }
  rules_.clear();
}

string BindingEnv::LookupVariable(const string& var) {
  auto i = bindings_.find(var);
  if (i != bindings_.end())
    return i->second;
  if (parent_)
    return parent_->LookupVariable(var);
  return "";
}

void BindingEnv::AddBinding(const string& key, const string& val) {
  bindings_[key] = val;
}

void BindingEnv::AddRule(const Rule* rule) {
  assert(LookupRuleCurrentScope(rule->name()) == nullptr);
  rules_[rule->name()] = rule;
}

const Rule* BindingEnv::LookupRuleCurrentScope(const string& rule_name) {
  auto i = rules_.find(rule_name);
  if (i == rules_.end())
    return nullptr;
  return i->second;
}

const Rule* BindingEnv::LookupRule(const string& rule_name) {
  auto i = rules_.find(rule_name);
  if (i != rules_.end())
    return i->second;
  if (parent_)
    return parent_->LookupRule(rule_name);
  return nullptr;
}

void Rule::AddBinding(const string& key, const EvalString& val) {
  bindings_[key] = val;
}

const EvalString* Rule::GetBinding(const string& key) const {
  auto i = bindings_.find(key);
  if (i == bindings_.end())
    return nullptr;
  return &i->second;
}

// static
bool Rule::IsReservedBinding(const string& var) {
  return var == "command" ||
      var == "depfile" ||
      var == "dyndep" ||
      var == "description" ||
      var == "deps" ||
      var == "generator" ||
      var == "pool" ||
      var == "restat" ||
      var == "rspfile" ||
      var == "rspfile_content" ||
      var == "msvc_deps_prefix" ||
      var == "symlink_outputs"; // From android platform
}

const map<string, const Rule*>& BindingEnv::GetRules() const {
  return rules_;
}

string BindingEnv::LookupWithFallback(const string& var,
                                      const EvalString* eval,
                                      Env* env) {
  auto i = bindings_.find(var);
  if (i != bindings_.end())
    return i->second;

  if (eval)
    return eval->Evaluate(env);

  if (parent_)
    return parent_->LookupVariable(var);

  return "";
}

string EvalString::Evaluate(Env* env) const {
  string result;
  for (const auto & i : parsed_) {
    if (i.second == RAW)
      result.append(i.first);
    else
      result.append(env->LookupVariable(i.first));
  }
  return result;
}

void EvalString::AddText(StringPiece text) {
  // Add it to the end of an existing RAW token if possible.
  if (!parsed_.empty() && parsed_.back().second == RAW) {
    parsed_.back().first.append(text.str_, text.len_);
  } else {
    parsed_.emplace_back(text.AsString(), RAW);
  }
}
void EvalString::AddSpecial(StringPiece text) {
  parsed_.emplace_back(text.AsString(), SPECIAL);
}

string EvalString::Serialize() const {
  string result;
  for (const auto & i : parsed_) {
    result.append("[");
    if (i.second == SPECIAL)
      result.append("$");
    result.append(i.first);
    result.append("]");
  }
  return result;
}

string EvalString::Unparse() const {
  string result;
  for (const auto & i : parsed_) {
    bool special = (i.second == SPECIAL);
    if (special)
      result.append("${");
    result.append(i.first);
    if (special)
      result.append("}");
  }
  return result;
}
