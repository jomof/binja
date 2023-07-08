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

#include "manifest_to_bin_parser.h"

#include <cstdlib>
#include <vector>

#include "graph.h"
#include "state.h"
#include "util.h"
#include "manifest_stream.h"

using namespace std;

ManifestToBinParser::ManifestToBinParser(State* state, FileReader* file_reader)
    : Parser(state, file_reader), out_(nullptr) {
}

bool ManifestToBinParser::Parse(const string& filename, const string& input,
                           string* err) {
  assert(out_);
  out_->StartParse();

  lexer_.Start(filename, input);

  for (;;) {
    Lexer::Token token = lexer_.ReadToken();
    ++next_node_;
    switch (token) {
    case Lexer::POOL:
      if (!ParsePool(err))
        return false;
      break;
    case Lexer::BUILD:
      if (!ParseEdge(err))
        return false;
      break;
    case Lexer::RULE:
      if (!ParseRule(err))
        return false;
      break;
    case Lexer::DEFAULT:
      if (!ParseDefault(err))
        return false;
      break;
    case Lexer::IDENT: {
      lexer_.UnreadToken();
      string name;
      EvalString let_value;
      if (!ParseLet(&name, &let_value, err))
        return false;
      ++next_node_;
      out_->WriteBinding(name, let_value);
      break;
    }
    case Lexer::INCLUDE:
      if (!ParseFileInclude(false, err))
        return false;
      break;
    case Lexer::SUBNINJA:
      if (!ParseFileInclude(true, err))
        return false;
      break;
    case Lexer::ERROR: return lexer_.Error(lexer_.DescribeLastError(), err);
    case Lexer::TEOF:
      out_->EndParse();
      return true;
    case Lexer::NEWLINE:
      break;
    default:
      return lexer_.Error(string("unexpected ") + Lexer::TokenName(token), err);
    }
  }
  // not reached
}

bool ManifestToBinParser::ParsePool(string* err) {
  string name;
  if (!lexer_.ReadIdent(&name))
    return lexer_.Error("expected pool name", err);

  if (!ExpectToken(Lexer::NEWLINE, err))
    return false;

  auto pool_position = lexer_.GetPosition();

  EvalString depth_value;
  size_t depth_position = SIZE_MAX;

  while (lexer_.PeekToken(Lexer::INDENT)) {
    string key;
    if (!ParseLet(&key, &depth_value, err))
      return false;

    if (key != "depth") {
      return lexer_.Error("unexpected variable '" + key + "'", err);
    }
    depth_position = lexer_.GetPosition();
  }

  if (depth_position == SIZE_MAX)
    return lexer_.Error("expected 'depth =' line", err);

  out_->WritePool(
      name,
      out_->EvalString(depth_value),
      pool_position,
      depth_position,
      lexer_.GetPosition()
      );
  return true;
}

bool ManifestToBinParser::ParseRule(string* err) {
  string name;
  if (!lexer_.ReadIdent(&name))
    return lexer_.Error("expected rule name", err);

  if (!ExpectToken(Lexer::NEWLINE, err))
    return false;

  auto pool_position = lexer_.GetPosition();
  vector<man_binding> eval_bindings;
  bool saw_command = false;
  bool saw_rspfile = false;
  bool saw_rspfile_content = false;

  while (lexer_.PeekToken(Lexer::INDENT)) {
    string key;
    EvalString value;
    if (!ParseLet(&key, &value, err))
      return false;

    if (key == "command") saw_command = true;
    if (key == "rspfile") saw_rspfile = true;
    if (key == "rspfile_content") saw_rspfile_content = true;

    if (Rule::IsReservedBinding(key)) {
      eval_bindings.emplace_back(
          out_->String(key),
          out_->EvalString(value)
          );
    } else {
      // Die on other key values for now; revisit if we want to add a
      // scope here.
      return lexer_.Error("unexpected variable '" + key + "'", err);
    }
  }

  if (saw_rspfile != saw_rspfile_content) {
    return lexer_.Error("rspfile and rspfile_content need to be "
        "both specified", err);
  }

  if (!saw_command)
    return lexer_.Error("expected 'command =' line", err);

  out_->WriteRule(
      name,
      eval_bindings,
      pool_position
      );
  return true;
}

bool ManifestToBinParser::ParseLet(string* key, EvalString* value, string* err) {
  if (!lexer_.ReadIdent(key))
    return lexer_.Error("expected variable name", err);
  if (!ExpectToken(Lexer::EQUALS, err))
    return false;
  if (!lexer_.ReadVarValue(value, err))
    return false;
  return true;
}

bool ManifestToBinParser::ParseDefault(string* err) {
  EvalString eval;
  if (!lexer_.ReadPath(&eval, err))
    return false;
  if (eval.empty())
    return lexer_.Error("expected target name", err);

  vector<man_eval_string> defaults;
  vector<uint64_t> defaults_positions;
  do {
    defaults.push_back(out_->EvalString(eval));
    eval.Clear();
    if (!lexer_.ReadPath(&eval, err))
      return false;
    defaults_positions.push_back(lexer_.GetPosition());
  } while (!eval.empty());

  auto result = ExpectToken(Lexer::NEWLINE, err);
  if (result) {
    out_->WriteDefault(
        out_->Vector(defaults),
        out_->Vector(defaults_positions),
        lexer_.GetPosition()
        );
  }
  return result;
}

bool ManifestToBinParser::ParseEdge(string* err) {
  //vector<EvalString> ins, outs, validations;
  vector<man_eval_string> ins, outs, validations;
  vector<man_binding> bindings;

  {
    EvalString out;
    if (!lexer_.ReadPath(&out, err))
      return false;
    while (!out.empty()) {
      outs.push_back(out_->EvalString(out));

      out.Clear();
      if (!lexer_.ReadPath(&out, err))
        return false;
    }
  }

  // Add all implicit outs, counting how many as we go.
  int implicit_outs = 0;
  if (lexer_.PeekToken(Lexer::PIPE)) {
    for (;;) {
      EvalString out;
      if (!lexer_.ReadPath(&out, err))
        return false;
      if (out.empty())
        break;
      outs.push_back(out_->EvalString(out));
      ++implicit_outs;
    }
  }

  if (outs.empty())
    return lexer_.Error("expected path", err);

  if (!ExpectToken(Lexer::COLON, err))
    return false;

  string rule_name;
  if (!lexer_.ReadIdent(&rule_name))
    return lexer_.Error("expected build command name", err);

  auto rule_position = lexer_.GetPosition();

  for (;;) {
    // XXX should we require one path here?
    EvalString in;
    if (!lexer_.ReadPath(&in, err))
      return false;
    if (in.empty())
      break;
    ins.push_back(out_->EvalString(in));
  }

  // Add all implicit deps, counting how many as we go.
  int implicit = 0;
  if (lexer_.PeekToken(Lexer::PIPE)) {
    for (;;) {
      EvalString in;
      if (!lexer_.ReadPath(&in, err))
        return false;
      if (in.empty())
        break;
      ins.push_back(out_->EvalString(in));
      ++implicit;
    }
  }

  // Add all order-only deps, counting how many as we go.
  int order_only = 0;
  if (lexer_.PeekToken(Lexer::PIPE2)) {
    for (;;) {
      EvalString in;
      if (!lexer_.ReadPath(&in, err))
        return false;
      if (in.empty())
        break;
      ins.push_back(out_->EvalString(in));
      ++order_only;
    }
  }

  // Add all validations, counting how many as we go.
  if (lexer_.PeekToken(Lexer::PIPEAT)) {
    for (;;) {
      EvalString validation;
      if (!lexer_.ReadPath(&validation, err))
        return false;
      if (validation.empty())
        break;
      validations.push_back(out_->EvalString(validation));
    }
  }

  if (!ExpectToken(Lexer::NEWLINE, err))
    return false;

  // Bindings on edges are rare, so allocate per-edge envs only when needed.
  bool has_indent_token = lexer_.PeekToken(Lexer::INDENT);
  while (has_indent_token) {
    string key;
    EvalString val;
    if (!ParseLet(&key, &val, err))
      return false;
    bindings.emplace_back(
        out_->String(key),
        out_->EvalString(val));
    has_indent_token = lexer_.PeekToken(Lexer::INDENT);
  }
  out_->WriteBuild(
      rule_name,
      out_->Vector(outs),
      implicit_outs,
      out_->Vector(ins),
      implicit,
      order_only,
      out_->Vector(validations),
      out_->Vector(bindings),
      rule_position,
      lexer_.GetPosition()
      );
  return true;
}

bool ManifestToBinParser::ParseFileInclude(bool new_scope, string* err) {
  EvalString eval;
  if (!lexer_.ReadPath(&eval, err))
    return false;

  if (!ExpectToken(Lexer::NEWLINE, err))
    return false;

  out_->WriteInclude(new_scope, out_->EvalString(eval), lexer_.GetPosition());
  return true;
}
