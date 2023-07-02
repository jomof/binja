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

#include "manifest_parser.h"
#include "manifest_to_bin_parser.h"

#include <cstdlib>
#include <vector>

#include "graph.h"
#include "state.h"
#include "util.h"
#include "version.h"
#include "binja_generated.h"
#include <fstream>

using namespace std;

ManifestParser::ManifestParser(State* state, FileReader* file_reader,
                               ManifestParserOptions options)
    : Parser(state, file_reader),
      options_(options),
      m2b_(new ManifestToBinParser(state, file_reader)),
      quiet_(false) {
  env_ = &state->bindings_;
}

ManifestParser::~ManifestParser() {
  delete m2b_;
}

const binja::ParseRule * GetRule(const binja::CompiledBuildNinja & compiled, unsigned int index) {
  auto node = compiled.parse_node()->Get(index);
  assert(node->type() == binja::ParseNode_::Type_RULE);
  auto table_offset = node->table_offset();
  return compiled.rule()->Get(table_offset);
}

const binja::ParseBuild * GetBuild(const binja::CompiledBuildNinja & compiled, unsigned int index) {
  auto node = compiled.parse_node()->Get(index);
  assert(node->type() == binja::ParseNode_::Type_BUILD);
  return compiled.build()->Get(node->table_offset());
}

const binja::ParsePool * GetPool(const binja::CompiledBuildNinja & compiled, unsigned int index) {
  auto node = compiled.parse_node()->Get(index);
  assert(node->type() == binja::ParseNode_::Type_POOL);
  return compiled.pool()->Get(node->table_offset());
}

const binja::ParseDefault * GetDefault(const binja::CompiledBuildNinja & compiled, unsigned int index) {
  auto node = compiled.parse_node()->Get(index);
  assert(node->type() == binja::ParseNode_::Type_DEFAULT);
  return compiled.default_()->Get(node->table_offset());
}

const binja::ParseBinding * GetBinding(const binja::CompiledBuildNinja & compiled, unsigned int index) {
  auto node = compiled.parse_node()->Get(index);
  assert(node->type() == binja::ParseNode_::Type_BINDING);
  return compiled.binding()->Get(node->table_offset());
}

const binja::ParseInclude * GetInclude(const binja::CompiledBuildNinja & compiled, unsigned int index) {
  auto node = compiled.parse_node()->Get(index);
  assert(node->type() == binja::ParseNode_::Type_INCLUDE);
  return compiled.include()->Get(node->table_offset());
}

string Evaluate(Env* env, const binja::ParseEvalString & parsed) {
  string result;
  for (const auto& piece : *parsed.piece()) {
    if (piece->type() == binja::ParseStringPiece_::Type_RAW)
      result.append(piece->value()->c_str());
    else
      result.append(env->LookupVariable(piece->value()->c_str()));
  }
  return result;
}

EvalString ToEvalString(const binja::ParseEvalString& parse_eval_string) {
  EvalString result;
  for(const auto& element : *parse_eval_string.piece()) {
    if (element->type() == binja::ParseStringPiece_::Type_RAW) {
      result.AddText(element->value()->c_str());
    } else {
      result.AddSpecial(element->value()->c_str());
    }
  }
  return result;
}

void ToEvalStrings(
    const flatbuffers::Vector<flatbuffers::Offset<binja::ParseEvalString>> & parse_eval_strings,
    vector<EvalString> * result) {
  for(const auto& parse_eval_string : parse_eval_strings) {
    result->push_back(ToEvalString(*parse_eval_string));
  }
}

bool ManifestParser::Parse(const string& filename, const string& input,
                           string* err) {
  if (!m2b_->Parse(filename, input, err)) {
    return false;
  }

  lexer_.Start(filename, input);
//  compiled_ = m2b_->GetCompiled();
//  if (ifstream(filename).good()) {
//    throw filename;
//  }

  for (;next_node_ < compiled_->parse_node()->size(); ++next_node_) {
    auto node = compiled_->parse_node()->Get(next_node_);
    switch(node->type()) {
    case binja::ParseNode_::Type_POOL:
      if (!ParsePool(err))
        return false;
      break;
    case binja::ParseNode_::Type_BUILD:
      if (!ParseEdge(err))
        return false;
      break;
    case binja::ParseNode_::Type_RULE:
      if (!ParseRule(err))
        return false;
      break;
    case binja::ParseNode_::Type_DEFAULT:
      if (!ParseDefault(err))
        return false;
      break;
    case binja::ParseNode_::Type_BINDING: {
      auto binding = GetBinding(*compiled_, next_node_);
      string value = Evaluate(env_, *binding->value());
      // Check ninja_required_version immediately so we can exit
      // before encountering any syntactic surprises.
      if (binding->key()->str() == "ninja_required_version") {
        CheckNinjaVersion(value);
      }
      env_->AddBinding(binding->key()->c_str(), value);
      break;
    }
    case binja::ParseNode_::Type_INCLUDE:
      if (!ParseFileInclude(err))
        return false;
      break;
    default:
      assert(0); // Unexpected
    }
  }
  return true;
}


bool ManifestParser::ParsePool(string* err) {
  auto node = GetPool(*compiled_, next_node_);
  auto name = node->name()->str();
  auto depth_str = Evaluate(env_, *node->depth());
  auto depth = (int)strtol(depth_str.c_str(), nullptr, 10);

  if (state_->LookupPool(name) != nullptr)
    return lexer_.Error("duplicate pool '" + name + "'", err, node->pool_position());

  if (depth < 0)
    return lexer_.Error("invalid pool depth", err, node->depth_position());

  state_->AddPool(new Pool(name, depth));
  return true;
}

bool ManifestParser::ParseRule(string* err) {
  auto node = GetRule(*compiled_, next_node_);

  if (env_->LookupRuleCurrentScope(node->name()->c_str()) != nullptr)
    return lexer_.Error("duplicate rule '" + node->name()->str() + "'", err, node->rule_position());

  Rule* rule = new Rule(node->name()->c_str());  // XXX scoped_ptr
  for (const auto& binding : *node->binding()) {
    rule->AddBinding(
        binding->key()->c_str(),
        ToEvalString(*binding->value()));
  }

  env_->AddRule(rule);
  return true;
}

bool ManifestParser::ParseDefault(string* err) {
  auto node = GetDefault(*compiled_, next_node_);
  auto defaults = node->default_();
  for(size_t i = 0; i < defaults->size(); ++i) {
    auto def = defaults->Get(i);
    auto path = Evaluate(env_, *def);
    if (path.empty())
      return lexer_.Error("empty path", err, node->default_positions()->Get(i));
    uint64_t slash_bits;  // Unused because this only does lookup.
    CanonicalizePath(&path, &slash_bits);
    std::string default_err;
    if (!state_->AddDefault(path, &default_err))
      return lexer_.Error(default_err, err, node->default_positions()->Get(i));
  }
  return true;
}

bool ManifestParser::ParseEdge(string* err) {
  auto node = GetBuild(*compiled_, next_node_);
  vector<EvalString> ins, outs, validations;
  ToEvalStrings(*node->in(), &ins);
  ToEvalStrings(*node->out(), &outs);
  ToEvalStrings(*node->validations(), &validations);
  auto bindings = node->bindings();
  auto implicit = node->implicit_in_count();
  auto implicit_outs = node->implicit_out_count();
  auto order_only = node->order_only_in_count();

  const Rule* rule = env_->LookupRule(node->name()->c_str());
  if (!rule)
    return lexer_.Error("unknown build rule '" + node->name()->str() + "'", err, node->rule_position());

  // Bindings on edges are rare, so allocate per-edge envs only when needed.
  BindingEnv* env = bindings->size() != 0 ? new BindingEnv(env_) : env_;
  for(const auto& binding: *bindings) {
    env->AddBinding(binding->key()->c_str(), Evaluate(env, *binding->value()));
  }

  Edge* edge = state_->AddEdge(rule);
  edge->env_ = env;

  string pool_name = edge->GetBinding("pool");
  if (!pool_name.empty()) {
    Pool* pool = state_->LookupPool(pool_name);
    if (pool == nullptr)
      return lexer_.Error("unknown pool name '" + pool_name + "'", err, node->final_position());
    edge->pool_ = pool;
  }

  edge->outputs_.reserve(outs.size());
  for (size_t i = 0, e = outs.size(); i != e; ++i) {
    string path = outs[i].Evaluate(env);
    if (path.empty())
      return lexer_.Error("empty path", err, node->final_position());
    uint64_t slash_bits;
    CanonicalizePath(&path, &slash_bits);
    if (!state_->AddOut(edge, path, slash_bits)) {
      if (options_.dupe_edge_action_ == kDupeEdgeActionError) {
        lexer_.Error("multiple rules generate " + path, err, node->final_position());
        return false;
      } else {
        if (!quiet_) {
          Warning(
              "multiple rules generate %s. builds involving this target will "
              "not be correct; continuing anyway",
              path.c_str());
        }
        if (e - i <= static_cast<size_t>(implicit_outs))
          --implicit_outs;
      }
    }
  }

  if (edge->outputs_.empty()) {
    // All outputs of the edge are already created by other edges. Don't add
    // this edge.  Do this check before input nodes are connected to the edge.
    state_->edges_.pop_back();
    delete edge;
    return true;
  }
  edge->implicit_outs_ = implicit_outs;

  edge->inputs_.reserve(ins.size());
  for (auto & in : ins) {
    string path = in.Evaluate(env);
    if (path.empty())
      return lexer_.Error("empty path", err, node->final_position());
    uint64_t slash_bits;
    CanonicalizePath(&path, &slash_bits);
    state_->AddIn(edge, path, slash_bits);
  }
  edge->implicit_deps_ = implicit;
  edge->order_only_deps_ = order_only;

  edge->validations_.reserve(validations.size());
  for (auto & validation : validations) {
    string path = validation.Evaluate(env);
    if (path.empty())
      return lexer_.Error("empty path", err, node->final_position());
    uint64_t slash_bits;
    CanonicalizePath(&path, &slash_bits);
    state_->AddValidation(edge, path, slash_bits);
  }

  if (options_.phony_cycle_action_ == kPhonyCycleActionWarn &&
      edge->maybe_phonycycle_diagnostic()) {
    // CMake 2.8.12.x and 3.0.x incorrectly write phony build statements
    // that reference themselves.  Ninja used to tolerate these in the
    // build graph but that has since been fixed.  Filter them out to
    // support users of those old CMake versions.
    Node* out = edge->outputs_[0];
    auto new_end =
        remove(edge->inputs_.begin(), edge->inputs_.end(), out);
    if (new_end != edge->inputs_.end()) {
      edge->inputs_.erase(new_end, edge->inputs_.end());
      if (!quiet_) {
        Warning("phony target '%s' names itself as an input; "
                "ignoring [-w phonycycle=warn]",
                out->path().c_str());
      }
    }
  }

  // Lookup, validate, and save any dyndep binding.  It will be used later
  // to load generated dependency information dynamically, but it must
  // be one of our manifest-specified inputs.
  string dyndep = edge->GetUnescapedDyndep();
  if (!dyndep.empty()) {
    uint64_t slash_bits;
    CanonicalizePath(&dyndep, &slash_bits);
    edge->dyndep_ = state_->GetNode(dyndep, slash_bits);
    edge->dyndep_->set_dyndep_pending(true);
    auto dgi =
      std::find(edge->inputs_.begin(), edge->inputs_.end(), edge->dyndep_);
    if (dgi == edge->inputs_.end()) {
      return lexer_.Error("dyndep '" + dyndep + "' is not an input", err, node->final_position());
    }
  }
  return true;
}

bool ManifestParser::ParseFileInclude(string* err) {
  auto node = GetInclude(*compiled_, next_node_);
  string path = Evaluate(env_, *node->path());

  ManifestParser subparser(state_, file_reader_, options_);
  if (node->new_scope()) {
    subparser.env_ = new BindingEnv(env_);
  } else {
    subparser.env_ = env_;
  }
  return subparser.Load(path, err, &lexer_, node->final_position());
}
