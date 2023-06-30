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

#ifndef NINJA_MANIFEST_TO_BIN_PARSER_H_
#define NINJA_MANIFEST_TO_BIN_PARSER_H_

#include "parser.h"
#include "manifest_parser_options.h"
#include "binja_generated.h"

struct BindingEnv;
struct EvalString;

/// Parses .ninja files.
struct ManifestToBinParser : public Parser {
  ManifestToBinParser(State* state, FileReader* file_reader,
                 ManifestParserOptions options = ManifestParserOptions());
  virtual ~ManifestToBinParser() { }

  /// Parse a text string of input.  Used by tests.
  bool ParseTest(const std::string& input, std::string* err) {
    quiet_ = true;
    return Parse("input", input, err);
  }

  binja::CompiledBuildNinja * compiled_ = 0;

  /// Parse a file, given its contents as a string.
  bool Parse(const std::string& filename, const std::string& input,
             std::string* err);
private:
  /// Parse various statement types.
  bool ParsePool(std::string* err);
  bool ParseRule(std::string* err);
  bool ParseLet(std::string* key, EvalString* val, std::string* err);
  bool ParseEdge(std::string* err);
  bool ParseDefault(std::string* err);

  /// Parse either a 'subninja' or 'include' line.
  bool ParseFileInclude(bool new_scope, std::string* err);

  /// Binja flatbuffer support
  flatbuffers::Offset<binja::ParseEvalString> CreateParseEvalString(const EvalString & eval_string);
  flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<binja::ParseEvalString>>> CreateParseEvalStringVector(const std::vector<EvalString> & eval_strings);

  flatbuffers::FlatBufferBuilder fb_;
  std::vector<flatbuffers::Offset<binja::ParseNode>> nodes_;
  std::vector<flatbuffers::Offset<binja::ParseRule>> rules_;
  std::vector<flatbuffers::Offset<binja::ParseEdge>> edges_;
  std::vector<flatbuffers::Offset<binja::ParseDefault>> defaults_;
  std::vector<flatbuffers::Offset<binja::ParsePool>> pools_;
  std::vector<flatbuffers::Offset<binja::ParseBinding>> bindings_;
  std::vector<flatbuffers::Offset<binja::ParseInclude>> includes_;
  ManifestParserOptions options_;
  bool quiet_;
};

#endif  // NINJA_MANIFEST_TO_BIN_PARSER_H_
