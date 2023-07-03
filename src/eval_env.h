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

#ifndef NINJA_EVAL_ENV_H_
#define NINJA_EVAL_ENV_H_

#include <map>
#include <string>
#include <utility>
#include <vector>
#include <memory>
#include "string_piece.h"

struct Rule;

/// An interface for a scope for variable (e.g. "$foo") lookups.
struct Env {
  virtual ~Env() = default;
  virtual std::string LookupVariable(const std::string& var) = 0;
};

/// A tokenized string that contains variable references.
/// Can be evaluated relative to an Env.
struct EvalString {
  /// @return The evaluated string with variable expanded using value found in
  ///         environment @a env.
  std::string Evaluate(Env* env) const;

  /// @return The string with variables not expanded.
  [[nodiscard]] std::string Unparse() const;

  void Clear() { parsed_.clear(); }
  [[nodiscard]] bool empty() const { return parsed_.empty(); }

  void AddText(StringPiece text);
  void AddSpecial(StringPiece text);

  /// Construct a human-readable representation of the parsed state,
  /// for use in tests.
  [[nodiscard]] std::string Serialize() const;

private:
  friend struct ManifestToBinParser;
  enum TokenType { RAW, SPECIAL };
  typedef std::vector<std::pair<std::string, TokenType> > TokenList;
  TokenList parsed_;
};

/// An invocable build command and associated metadata (description, etc.).
struct Rule {
  explicit Rule(std::string name) : name_(std::move(name)) { }

  [[nodiscard]] const std::string& name() const { return name_; }

  void AddBinding(const std::string& key, const EvalString& val);

  static bool IsReservedBinding(const std::string& var);

  [[nodiscard]] const EvalString* GetBinding(const std::string& key) const;

 private:
  // Allow the parsers to reach into this object and fill out its fields.
  friend struct ManifestParser;
  friend struct ManifestToBinParser;

  std::string name_;
  typedef std::map<std::string, EvalString> Bindings;
  Bindings bindings_;
};

/// An Env which contains a mapping of variables to values
/// as well as a pointer to a parent scope.
struct BindingEnv : public Env {
  BindingEnv() : parent_(nullptr) {}
  explicit BindingEnv(std::shared_ptr<BindingEnv> parent) : parent_(std::move(parent)) {}

  ~BindingEnv() override;
  std::string LookupVariable(const std::string& var) override;

  void AddRule(const Rule* rule);
  const Rule* LookupRule(const std::string& rule_name);
  const Rule* LookupRuleCurrentScope(const std::string& rule_name);
  [[nodiscard]] const std::map<std::string, const Rule*>& GetRules() const;

  void AddBinding(const std::string& key, const std::string& val);

  /// This is tricky.  Edges want lookup scope to go in this order:
  /// 1) value set on edge itself (edge_->env_)
  /// 2) value set on rule, with expansion in the edge's scope
  /// 3) value set on enclosing scope of edge (edge_->env_->parent_)
  /// This function takes as parameters the necessary info to do (2).
  std::string LookupWithFallback(const std::string& var, const EvalString* eval,
                                 Env* env);

private:
  std::map<std::string, std::string> bindings_;
  std::map<std::string, const Rule*> rules_;
  std::shared_ptr<BindingEnv> parent_;
};

#endif  // NINJA_EVAL_ENV_H_
