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

#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "graph.h"
#include "state.h"
#include "util.h"
#include "version.h"
#include "flatbuffers/flatbuffer_builder.h"
#include "binja_generated.h"

using namespace std;

ManifestToBinParser::ManifestToBinParser(State* state, FileReader* file_reader)
    : Parser(state, file_reader) {
  env_ = &state->bindings_;
}

bool ManifestToBinParser::Parse(const string& filename, const string& input,
                           string* err) {
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

      nodes_.push_back(binja::CreateParseNode(
          fb_,
          binja::ParseNode_::Type_BINDING,
          bindings_.size()
          ));
      bindings_.push_back(binja::CreateParseBinding(
          fb_,
          fb_.CreateSharedString(name),
          CreateParseEvalString(let_value),
          lexer_.GetPosition()
          ));
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
    case Lexer::ERROR: {
      return lexer_.Error(lexer_.DescribeLastError(), err);
    }
    case Lexer::TEOF: {
//      nodes_.push_back(binja::CreateParseNode(
//          fb_,
//          binja::ParseNode_::Type_END_OF_FILE,
//          0
//          ));
//      fb_.Finish(binja::CreateCompiledBuildNinja(
//          fb_,
//          fb_.CreateVector(nodes_),
//          fb_.CreateVector(rules_),
//          fb_.CreateVector(builds_),
//          fb_.CreateVector(defaults_),
//          fb_.CreateVector(pools_),
//          fb_.CreateVector(bindings_),
//          fb_.CreateVector(includes_)));
//      compiled_ = flatbuffers::GetRoot<binja::CompiledBuildNinja>(fb_.GetBufferPointer());
    //  auto x = (compiled_->parse_node())->Get(0);

      //throw "FINISHED";
      //auto ptr = fb_.GetBufferPointer();
      return true;
    }
    case Lexer::NEWLINE:
      break;
    default:
      return lexer_.Error(string("unexpected ") + Lexer::TokenName(token),
                          err);
    }
  }
  return false;  // not reached
}

const binja::CompiledBuildNinja * ManifestToBinParser::GetCompiled() {
  auto offset = binja::CreateCompiledBuildNinja(
      fb_,
      fb_.CreateVector(nodes_),
      fb_.CreateVector(rules_),
      fb_.CreateVector(builds_),
      fb_.CreateVector(defaults_),
      fb_.CreateVector(pools_),
      fb_.CreateVector(bindings_),
      fb_.CreateVector(includes_));
  return flatbuffers::GetTemporaryPointer<binja::CompiledBuildNinja>(fb_, offset);
}


bool ManifestToBinParser::ParsePool(string* err) {
  string name;
  if (!lexer_.ReadIdent(&name))
    return lexer_.Error("expected pool name", err);

  if (!ExpectToken(Lexer::NEWLINE, err))
    return false;

  long pool_position = lexer_.GetPosition();

  EvalString depth_value;
  long depth_line = -1;

  while (lexer_.PeekToken(Lexer::INDENT)) {
    string key;
    if (!ParseLet(&key, &depth_value, err))
      return false;

    if (key != "depth") {
      return lexer_.Error("unexpected variable '" + key + "'", err);
    }
    depth_line = lexer_.GetPosition();
  }

  if (depth_line < 0)
    return lexer_.Error("expected 'depth =' line", err);

  nodes_.push_back(binja::CreateParseNode(
      fb_,
      binja::ParseNode_::Type_POOL,
      pools_.size()
      ));
  pools_.push_back(binja::CreateParsePool(
      fb_,
      fb_.CreateSharedString(name),
      CreateParseEvalString(depth_value),
      pool_position,
      depth_line,
      lexer_.GetPosition()
      ));
  return true;
}

bool ManifestToBinParser::ParseRule(string* err) {
  string name;
  if (!lexer_.ReadIdent(&name))
    return lexer_.Error("expected rule name", err);

  if (!ExpectToken(Lexer::NEWLINE, err))
    return false;

  auto pool_position = lexer_.GetPosition();
  vector<flatbuffers::Offset<binja::ParseBinding>> bindings;
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
      bindings.push_back(binja::CreateParseBinding(
          fb_,
          fb_.CreateSharedString(key),
          CreateParseEvalString(value)));
    } else {
      // Die on other keyvals for now; revisit if we want to add a
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

  nodes_.push_back(binja::CreateParseNode(
      fb_,
      binja::ParseNode_::Type_RULE,
      rules_.size()
      ));
  rules_.push_back(binja::CreateParseRule(
      fb_,
      fb_.CreateSharedString(name),
      fb_.CreateVector(bindings),
      pool_position,
      lexer_.GetPosition()
      ));
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

  vector<EvalString> defaults;
  vector<uint64_t> defaults_positions;
  do {
    defaults.push_back(eval);
    eval.Clear();
    if (!lexer_.ReadPath(&eval, err))
      return false;
    defaults_positions.push_back(lexer_.GetPosition());
  } while (!eval.empty());

  auto result = ExpectToken(Lexer::NEWLINE, err);
  if (result) {
    nodes_.push_back(binja::CreateParseNode(
        fb_,
        binja::ParseNode_::Type_DEFAULT,
        defaults_.size()
        ));
    defaults_.push_back(binja::CreateParseDefault(
        fb_,
        CreateParseEvalStringVector(defaults),
        fb_.CreateVector(defaults_positions),
        lexer_.GetPosition()
        ));
  }
  return result;
}

bool ManifestToBinParser::ParseEdge(string* err) {
  vector<EvalString> ins, outs, validations;
  vector<flatbuffers::Offset<binja::ParseBinding>> bindings;


  {
    EvalString out;
    if (!lexer_.ReadPath(&out, err))
      return false;
    while (!out.empty()) {
      outs.push_back(out);

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
      outs.push_back(out);
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
    ins.push_back(in);
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
      ins.push_back(in);
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
      ins.push_back(in);
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
      validations.push_back(validation);
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
    bindings.push_back(binja::CreateParseBinding(
        fb_,
        fb_.CreateSharedString(key),
        CreateParseEvalString(val),
        lexer_.GetPosition()));
    has_indent_token = lexer_.PeekToken(Lexer::INDENT);
  }

  nodes_.push_back(binja::CreateParseNode(
      fb_,
      binja::ParseNode_::Type_BUILD,
      builds_.size()
      ));
  builds_.push_back(binja::CreateParseBuild(
      fb_,
      fb_.CreateSharedString(rule_name),
      CreateParseEvalStringVector(outs),
      implicit_outs,
      CreateParseEvalStringVector(ins),
      implicit,
      order_only,
      CreateParseEvalStringVector(validations),
      fb_.CreateVector(bindings),
      rule_position,
      lexer_.GetPosition()
      ));

  return true;
}

bool ManifestToBinParser::ParseFileInclude(bool new_scope, string* err) {
  EvalString eval;
  if (!lexer_.ReadPath(&eval, err))
    return false;

  if (!ExpectToken(Lexer::NEWLINE, err))
    return false;

  nodes_.push_back(binja::CreateParseNode(
      fb_,
      binja::ParseNode_::Type_INCLUDE,
      includes_.size()
      ));
  includes_.push_back(binja::CreateParseInclude(
      fb_,
      new_scope,
      CreateParseEvalString(eval),
      lexer_.GetPosition()
      ));
  return true;
}

flatbuffers::Offset<binja::ParseEvalString> ManifestToBinParser::CreateParseEvalString(const EvalString & eval_string) {
  vector<flatbuffers::Offset<binja::ParseStringPiece>> pieces;
  for (const auto& token : eval_string.parsed_) {
    auto text = fb_.CreateSharedString(token.first);
    auto type = token.second == EvalString::TokenType::RAW
                    ? binja::ParseStringPiece_::Type_RAW
                    : binja::ParseStringPiece_::Type_SPECIAL;
    pieces.push_back(binja::CreateParseStringPiece(fb_, type, text));
  }
  return binja::CreateParseEvalString(fb_, fb_.CreateVector(pieces));
}


flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<binja::ParseEvalString>>> ManifestToBinParser::CreateParseEvalStringVector(const vector<EvalString> & eval_strings) {
  vector<flatbuffers::Offset<binja::ParseEvalString>> strings;
  for (const auto& eval_string : eval_strings) {
    auto parse_eval_string = CreateParseEvalString(eval_string);
    strings.push_back(parse_eval_string);
  }
  return fb_.CreateVector(strings);
}
