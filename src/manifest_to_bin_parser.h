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
#include <sstream>
#include "manifest_stream.h"

struct BindingEnv;
struct EvalString;

/// Parses .ninja files.
struct ManifestToBinParser : public Parser {
  ManifestToBinParser(State* state, FileReader* file_reader);
  virtual ~ManifestToBinParser() { }

  /// Parse a text string of input.  Used by tests.
  bool ParseTest(const std::string& input, std::string* err) {
    std::stringstream dummy;
    return Parse("input", input, dummy, err);
  }

  /// Parse a file, given its contents as a string.
  bool Parse(const std::string& filename, const std::string& input,
             std::string* err);

  bool Parse(
      const std::string& filename,
      const std::string& input,
      std::ostream& output,
      std::string* err) {
    manifest_ostream manifest_out(output);
    out_ = &manifest_out;
    return Parse(filename, input, err);
  }
public:
  /// Parse various statement types.
  bool ParsePool(std::string* err);
  bool ParseRule(std::string* err);
  bool ParseLet(std::string* key, EvalString* val, std::string* err);
  bool ParseEdge(std::string* err);
  bool ParseDefault(std::string* err);

  /// Parse either a 'subninja' or 'include' line.
  bool ParseFileInclude(bool new_scope, std::string* err);

  manifest_ostream * out_ = nullptr;
  unsigned int next_node_ = 0;
};

#endif  // NINJA_MANIFEST_TO_BIN_PARSER_H_
