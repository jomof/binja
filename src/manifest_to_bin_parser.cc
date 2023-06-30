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

ManifestToBinParser::ManifestToBinParser(State* state, FileReader* file_reader,
                               ManifestParserOptions options)
    : Parser(state, file_reader),
      options_(options), quiet_(false) {
}

bool ManifestToBinParser::Parse(const string& filename, const string& input,
                           string* err) {

//  flatbuffers::FlatBufferBuilder builder;
//  auto name = builder.CreateSharedString("bob");
//  auto person = binja::CreatePerson(builder, name);
//  builder.Finish(person);

  lexer_.Start(filename, input);

  for (;;) {
    Lexer::Token token = lexer_.ReadToken();
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
      nodes_.push_back(binja::CreateParseNode(
          fb_,
          binja::ParseNode_::Type_RULE,
          bindings_.size()
          ));
      bindings_.push_back(binja::CreateParseBinding(
          fb_,
          fb_.CreateSharedString(name),
          CreateParseEvalString(let_value)
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
      fb_.Finish(binja::CreateCompiledBuildNinja(
          fb_,
          fb_.CreateVector(nodes_),
          fb_.CreateVector(rules_),
          fb_.CreateVector(edges_),
          fb_.CreateVector(defaults_),
          fb_.CreateVector(pools_),
          fb_.CreateVector(bindings_),
          fb_.CreateVector(includes_)));
      fb_.Finished();
      auto ptr = fb_.GetBufferPointer();
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


bool ManifestToBinParser::ParsePool(string* err) {
  string name;
  if (!lexer_.ReadIdent(&name))
    return lexer_.Error("expected pool name", err);

  if (!ExpectToken(Lexer::NEWLINE, err))
    return false;

  if (state_->LookupPool(name) != NULL)
    return lexer_.Error("duplicate pool '" + name + "'", err);

  EvalString depth_string;

  while (lexer_.PeekToken(Lexer::INDENT)) {
    string key;
    EvalString value;
    if (!ParseLet(&key, &depth_string, err))
      return false;
  }

  nodes_.push_back(binja::CreateParseNode(
      fb_,
      binja::ParseNode_::Type_POOL,
      pools_.size()
      ));
  pools_.push_back(binja::CreateParsePool(
      fb_,
      fb_.CreateSharedString(name),
      CreateParseEvalString(depth_string)
      ));

  return true;
}

bool ManifestToBinParser::ParseRule(string* err) {
  string name;
  if (!lexer_.ReadIdent(&name))
    return lexer_.Error("expected rule name", err);

  if (!ExpectToken(Lexer::NEWLINE, err))
    return false;

  vector<flatbuffers::Offset<binja::ParseBinding>> bindings;
  while (lexer_.PeekToken(Lexer::INDENT)) {
    string key;
    EvalString value;
    if (!ParseLet(&key, &value, err))
      return false;

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

  nodes_.push_back(binja::CreateParseNode(
      fb_,
      binja::ParseNode_::Type_RULE,
      rules_.size()
      ));
  binja::CreateParseRule(
      fb_,
      fb_.CreateSharedString(name),
      fb_.CreateVector(bindings)
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

  vector<EvalString> defaults;
  do {
    defaults.push_back(eval);
    eval.Clear();
    if (!lexer_.ReadPath(&eval, err))
      return false;
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
        CreateParseEvalStringVector(defaults)
        ));
  }
  return result;
}

bool ManifestToBinParser::ParseEdge(string* err) {
  vector<EvalString> ins, outs, validations, bindings;

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
    bindings.push_back(val);
    has_indent_token = lexer_.PeekToken(Lexer::INDENT);
  }

  nodes_.push_back(binja::CreateParseNode(
      fb_,
      binja::ParseNode_::Type_RULE,
      edges_.size()
      ));
  edges_.push_back(binja::CreateParseEdge(
      fb_,
      fb_.CreateSharedString(rule_name),
      CreateParseEvalStringVector(outs),
      implicit_outs,
      CreateParseEvalStringVector(ins),
      implicit,
      order_only,
      CreateParseEvalStringVector(validations),
      CreateParseEvalStringVector(bindings)
      ));

  return true;
}

bool ManifestToBinParser::ParseFileInclude(bool new_scope, string* err) {
  EvalString eval;
  if (!lexer_.ReadPath(&eval, err))
    return false;

  nodes_.push_back(binja::CreateParseNode(
      fb_,
      binja::ParseNode_::Type_INCLUDE,
      includes_.size()
      ));
  includes_.push_back(binja::CreateParseInclude(
      fb_,
      new_scope,
      CreateParseEvalString(eval)
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
    strings.push_back(CreateParseEvalString(eval_string));
  }
  return fb_.CreateVector(strings);
}
