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
#include <fstream>
#include "disk_interface.h"

using namespace std;

ManifestParser::ManifestParser(State* state, FileReader* file_reader,
                               ManifestParserOptions options)
    : Parser(state, file_reader),
      options_(options),
      quiet_(false) {
  env_ = state->bindings_;
}

string Evaluate(Env * env, const man_vector_iterable<man_eval_pair> parsed) {
  string result;
  for (const auto& piece : parsed) {
    auto str = piece.value.c_str(parsed.buffer);
    if (piece.type == man_eval_t::RAW)
      result.append(str);
    else
      result.append(env->LookupVariable(str));
  }
  return result;
}

EvalString ToEvalString(const man_vector_iterable<man_eval_pair>& parse_eval_string) {
  EvalString result;
  for(const auto& element : parse_eval_string) {
    auto str = element.value.c_str(parse_eval_string.buffer);
    if (element.type == man_eval_t::RAW) {
      result.AddText(str);
    } else {
      result.AddSpecial(str);
    }
  }
  return result;
}

void ToEvalStrings(
    const man_vector_iterable<man_eval_string> & parse_eval_strings,
    vector<EvalString> * result) {
  for(const auto& parse_eval_string : parse_eval_strings) {
    result->push_back(ToEvalString(parse_eval_string.elements(parse_eval_strings.buffer)));
  }
}

bool ManifestParser::Parse(const string& filename, const string& input,
                           string* err) {

  lexer_.Start(filename, input);

  if (std::ifstream(filename).good()) {
    auto bin = filename + ".bin";
    auto create = false;

    if (!std::ifstream(bin).good()) {
      cout << "Creating " << bin << " because it didn't exist" << endl;
      create = true;
    } else {
      auto disk = (DiskInterface*)file_reader_;
      auto filename_time = disk->Stat(filename, err);
      auto bin_time = disk->Stat(bin, err);
      if (filename_time > bin_time) {
        cout << "Recreating " << bin << " because it was older than " << filename << endl;
        create = true;
      }
    }

    if (!create) {
      in_ = manifest_istream::create(bin);
      if (!in_->IsCurrentVersion()) {
        cout << "Recreating " << bin << " because it had a different schema version" << endl;
        create = true;
      }
    }

    if (create) {
      std::ofstream outfile(bin, std::ios::binary);

      ManifestToBinParser m2b(state_, file_reader_);
      if (!m2b.Parse(filename, input, outfile, err)) {
        outfile.close();
        return false;
      }
      outfile.close();
    } else {
      cout << "Reading existing " << bin << endl;
    }
    in_ = manifest_istream::create(bin);
    if (in_ == nullptr) {
      lexer_.Error("could not create " + bin, err);
      return false;
    }

  } else {
    std::stringstream memory(std::ios::binary | std::ios::in | std::ios::out);
    ManifestToBinParser m2b(state_, file_reader_);
    if (!m2b.Parse(filename, input, memory, err)) {
      return false;
    }
    auto str = memory.str();
    char* buffer = new char[str.size()];
    std::memcpy(buffer, str.c_str(), str.size());
    in_ = make_shared<manifest_istream>(buffer, true);
  }


  in_->EatStartParse();


  man_node_t type;
  while ((type = in_->NextRecordType()) != man_node_t::END_PARSE) {
    switch(type) {
    case man_node_t::POOL:
      if (!ParsePool(err))
        return false;
      break;
    case man_node_t::BUILD:
      if (!ParseEdge(err))
        return false;
      break;
    case man_node_t::RULE:
      if (!ParseRule(err))
        return false;
      break;
    case man_node_t::DEFAULT:
      if (!ParseDefault(err))
        return false;
      break;
    case man_node_t::BINDING: {
      auto binding = in_->ReadBinding();
      string value = Evaluate(env_.get(), binding->value.elements(in_->buffer));
      string key = std::string(binding->name.c_str(in_->buffer));
      // Check ninja_required_version immediately so we can exit
      // before encountering any syntactic surprises.
      if (key == "ninja_required_version") {
        CheckNinjaVersion(value);
      }
      env_->AddBinding(key, value);
      break;
    }
    case man_node_t::INCLUDE:
      if (!ParseFileInclude(err))
        return false;
      break;
    default:
      assert(0); // Unexpected
    }
  }
  in_->EatEndParse();
  return true;
}


bool ManifestParser::ParsePool(string* err) {
  auto node = in_->ReadPool();
  auto name = node->name.c_str(in_->buffer);
  auto depth_str = Evaluate(env_.get(), node->depth.elements(in_->buffer));
  auto depth = (int)strtol(depth_str.c_str(), nullptr, 10);

  if (state_->LookupPool(name) != nullptr)
    return lexer_.Error("duplicate pool '" + string(name) + "'", err, node->pool_position);

  if (depth < 0)
    return lexer_.Error("invalid pool depth", err, node->depth_position);

  state_->AddPool(new Pool(name, depth));
  return true;
}

bool ManifestParser::ParseRule(string* err) {
  auto node = in_->ReadRule();
  auto name = node->name.c_str(in_->buffer);
  if (env_->LookupRuleCurrentScope(name) != nullptr)
    return lexer_.Error("duplicate rule '" + string(name) + "'", err, node->rule_position);

  Rule* rule = new Rule(name);  // XXX scoped_ptr
  for (const auto& binding : node->bindings.elements(in_->buffer)) {
    rule->AddBinding(
        binding.name.c_str(in_->buffer),
        ToEvalString(binding.value.elements(in_->buffer)));
  }

  env_->AddRule(rule);
  return true;
}

bool ManifestParser::ParseDefault(string* err) {
  auto node = in_->ReadDefault();
  auto defaults = node->defaults.elements(in_->buffer);
  for(size_t i = 0; i < defaults.size(); ++i) {
    auto path = Evaluate(env_.get(), defaults[i].elements(in_->buffer));
    if (path.empty())
      return lexer_.Error("empty path", err, node->default_positions.elements(in_->buffer)[i]);
    uint64_t slash_bits;  // Unused because this only does lookup.
    CanonicalizePath(&path, &slash_bits);
    std::string default_err;
    if (!state_->AddDefault(path, &default_err)) {
      auto position = node->default_positions.elements(in_->buffer)[i];
      return lexer_.Error(default_err, err, position);
    }
  }
  return true;
}

bool ManifestParser::ParseEdge(string* err) {
  auto node = in_->ReadBuild();
  vector<EvalString> ins, outs, validations;
  ToEvalStrings(node->in.elements(in_->buffer), &ins);
  ToEvalStrings(node->out.elements(in_->buffer), &outs);
  ToEvalStrings(node->validations.elements(in_->buffer), &validations);
  auto bindings = node->bindings.elements(in_->buffer);
  auto implicit = node->implicit_in_count;
  auto implicit_outs = node->implicit_out_count;
  auto order_only = node->order_only_in_count;

  auto rule_name = node->rule_name.c_str(in_->buffer);
  const Rule* rule = env_->LookupRule(rule_name);
  if (!rule)
    return lexer_.Error("unknown build rule '" + string(rule_name) + "'", err, node->rule_position);

  // Bindings on edges are rare, so allocate per-edge envs only when needed.
  auto env = bindings.size() != 0 ? std::make_shared<BindingEnv>(env_) : env_;
  for(const auto& binding: bindings) {
    env->AddBinding(
        binding.name.c_str(in_->buffer),
        Evaluate(env.get(), binding.value.elements(in_->buffer)));
  }

  Edge* edge = state_->AddEdge(rule);
  edge->env_ = env;

  string pool_name = edge->GetBinding("pool");
  if (!pool_name.empty()) {
    Pool* pool = state_->LookupPool(pool_name);
    if (pool == nullptr)
      return lexer_.Error("unknown pool name '" + pool_name + "'", err, node->final_position);
    edge->pool_ = pool;
  }

  edge->outputs_.reserve(outs.size());
  for (size_t i = 0, e = outs.size(); i != e; ++i) {
    string path = outs[i].Evaluate(env.get());
    if (path.empty())
      return lexer_.Error("empty path", err, node->final_position);
    uint64_t slash_bits;
    CanonicalizePath(&path, &slash_bits);
    if (!state_->AddOut(edge, path, slash_bits)) {
      if (options_.dupe_edge_action_ == kDupeEdgeActionError) {
        lexer_.Error("multiple rules generate " + path, err, node->final_position);
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
    string path = in.Evaluate(env.get());
    if (path.empty())
      return lexer_.Error("empty path", err, node->final_position);
    uint64_t slash_bits;
    CanonicalizePath(&path, &slash_bits);
    state_->AddIn(edge, path, slash_bits);
  }
  edge->implicit_deps_ = implicit;
  edge->order_only_deps_ = order_only;

  edge->validations_.reserve(validations.size());
  for (auto & validation : validations) {
    string path = validation.Evaluate(env.get());
    if (path.empty())
      return lexer_.Error("empty path", err, node->final_position);
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
      return lexer_.Error("dyndep '" + dyndep + "' is not an input", err, node->final_position);
    }
  }
  return true;
}

bool ManifestParser::ParseFileInclude(string* err) {
  auto node = in_->ReadInclude();
  string path = Evaluate(env_.get(), node->path.elements(in_->buffer));

  ManifestParser subparser(state_, file_reader_, options_);
  if (node->new_scope) {
    subparser.env_ = std::make_shared<BindingEnv>(env_);
  } else {
    subparser.env_ = env_;
  }
  return subparser.Load(path, err, &lexer_, node->final_position);
}
