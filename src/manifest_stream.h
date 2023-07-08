
#ifndef NINJA_MANIFEST_STREAM_H
#define NINJA_MANIFEST_STREAM_H

#include <iostream>
#include <streambuf>
#include <istream>
#include "eval_env.h"
#include <fstream>

typedef uint32_t man_offset_t;
typedef uint16_t man_node_byte_count_t;
typedef uint16_t man_vector_count_t;
enum class man_node_t : char {
  UNKNOWN = 0,
  STRING = 's',
  START_PARSE = '+',
  END_PARSE = '-',
  RULE = 'r',
  BUILD = 'b',
  INCLUDE = 'i',
  BINDING = '=',
  DEFAULT = 'd',
  POOL = 'p',
  VECTOR = 'v'
};

enum class man_eval_t : char {
  UNKNOWN = 0,
  RAW = 'R',
  SPECIAL = 'S'
};

struct __attribute__((packed)) man_vector_base {
  man_offset_t offset;
  inline man_vector_base() : offset(0) { }
  inline explicit man_vector_base(man_offset_t offset) : offset(offset) { }
  man_vector_count_t size(const char * p) const { return *((man_vector_count_t *)(p + offset)); }
  const void * raw(const char * p) const { return ((const void *)(p + offset + sizeof(man_vector_count_t))); }
};

template<typename t_elem> struct man_vector_iterable {
  const char * buffer;
  const t_elem * begin_element;
  const t_elem * end_element;
  const t_elem * begin() const { return begin_element; }
  const t_elem * end() const { return end_element; }
  const t_elem& operator[](std::size_t index) const {
    auto result = begin_element + index;
    assert(result < end_element);
    return *result;
  }
  size_t size() const { return end_element - begin_element; }
  man_vector_iterable(
      const char * buffer,
      const t_elem * begin,
      const t_elem * end) :
        buffer(buffer),
        begin_element(begin),
        end_element(end) { }
};

template<typename t_elem> struct __attribute__((packed)) man_vector : man_vector_base {
  inline man_vector() : man_vector_base(0) { }
  inline explicit man_vector(man_offset_t storage) : man_vector_base(storage) { }

  const t_elem * ptr(const char * p) const { return (const t_elem *)man_vector_base::raw(p); }
  const man_vector_iterable<t_elem> elements(const char * p) const {
    return man_vector_iterable<t_elem>(
        p,
        ptr(p),
        ptr(p) + size(p)
        );
  }
};

struct __attribute__((packed)) man_string : man_vector< char> {
  inline man_string() : man_vector() { }
  inline man_string(man_offset_t storage) : man_vector(storage) { }

  const char * c_str(const char * p) const {
    return ptr(p);
  }
};

struct __attribute__((packed)) man_eval_pair {
  man_string value;
  man_eval_t type;
  man_eval_pair(man_string value, man_eval_t type) : value(value), type(type) { }
};

struct __attribute__((packed)) man_eval_string : man_vector<man_eval_pair> {
  inline man_eval_string() : man_vector() { }
  inline man_eval_string(man_offset_t storage) : man_vector(storage) { }
};

struct __attribute__((packed)) man_binding {
  man_string name;
  man_eval_string value;
  man_binding(
      man_string name,
      man_eval_string value) : name(name), value(value){ }
};

struct __attribute__((packed)) man_node {
  man_node_t type;
  man_node_byte_count_t size;
};

struct __attribute__((packed)) RuleNode : man_node {
  man_string name;
  man_vector<man_binding> bindings;
  uint64_t rule_position;
};

struct __attribute__((packed)) BuildNode : man_node {
  man_string rule_name;
  man_vector<man_eval_string> out;
  uint16_t implicit_out_count;
  man_vector<man_eval_string> in;
  uint16_t implicit_in_count;
  uint16_t order_only_in_count;
  man_vector<man_eval_string> validations;
  man_vector<man_binding> bindings;
  uint64_t rule_position;
  uint64_t final_position;
};

struct __attribute__((packed)) IncludeNode : man_node {
  bool new_scope;
  man_eval_string path;
  uint64_t final_position;
};

struct __attribute__((packed)) BindingNode : man_node {
  man_string name;
  man_eval_string value;
};

struct __attribute__((packed)) DefaultNode : man_node {
  man_vector<man_eval_string> defaults;
  man_vector<uint64_t> default_positions;
};

struct __attribute__((packed)) PoolNode : man_node {
  man_string name;
  man_eval_string depth;
  uint64_t pool_position;
  uint64_t depth_position;
};

const uint16_t MANIFEST_SCHEMA_VERSION = 1;
const uint16_t MANIFEST_SCHEMA_CHECKSUM = sizeof(PoolNode)
    + sizeof(DefaultNode)
    + sizeof(BindingNode)
    + sizeof(IncludeNode)
    + sizeof(BuildNode)
    + sizeof(RuleNode)
    ;
struct __attribute__((packed)) ParseStartNode : man_node {
  uint16_t version = MANIFEST_SCHEMA_VERSION;
  uint16_t checksum = MANIFEST_SCHEMA_CHECKSUM;
};

class manifest_ostream
{
  std::ostream& out_;
  std::unordered_map<std::string, man_offset_t> strings_;
  std::unordered_map<std::string, man_vector_base> vectors_;

 public:
  manifest_ostream(std::ostream& out) : out_(out) {}

  template<typename t> void Write(const t& value) {
    out_.write(reinterpret_cast<const char *>(&value), sizeof(value));
  }

  void StartParse() {
    auto data = ParseStartNode();
    data.type = man_node_t::START_PARSE;
    data.size = sizeof(data);
    out_.write(reinterpret_cast<const char*>(&data), data.size);
  }

  void EndParse() { Write(man_node_t::END_PARSE); }

  man_string String(const std::string & string) {
    auto found = strings_.find(string);
    if (found == strings_.end()) {
      Write(man_node_t::STRING);
      auto offset = (man_offset_t) out_.tellp();
      auto offset_string = man_string(offset);
      strings_[string] = offset;
      man_vector_count_t size = string.size() + 1;
      Write(size);
      out_.write(reinterpret_cast<const std::istream::char_type*>(string.data()), size);
      return offset_string;
    }

    return strings_.find(string)->second;
  }

  man_eval_string EvalString(const struct EvalString & eval_string) {
    std::vector<man_eval_pair> pairs;

    for(const auto& elem : eval_string.parsed_) {
      pairs.emplace_back(
          String(elem.first),
          elem.second == EvalString::RAW ? man_eval_t::RAW : man_eval_t::SPECIAL
          );
    }
    return man_eval_string(Vector<man_eval_pair>(pairs).offset);
  }

  /**
   * Layout of a vector:
   * type VECTOR
   * man_node_byte_count_t -- total size of vector in bytes
   * man_vector_count_t -- count of elements
   * element0
   * element1
   * ...
   * elementN
   */
  template<typename t_elem> man_vector<t_elem> Vector(const std::vector<t_elem> & vec) {
    auto bytes = sizeof(man_node_byte_count_t) + vec.size() * sizeof(t_elem) + 1;
    auto buffer = new char[bytes];
    auto p = buffer;
    *((man_vector_count_t*)p) = vec.size();
    p += sizeof(man_vector_count_t);
    
    for(const auto& elem : vec) {
      *((t_elem*)p) = elem;
      p += sizeof(elem);
    }
    *((char*)p) = 0;
    
    std::string key = std::string(buffer, bytes);

    auto found = strings_.find(key);
    if (found == strings_.end()) {
      Write(man_node_t::VECTOR);
      Write<man_node_byte_count_t>(bytes);
      auto offset = man_vector<t_elem>((man_offset_t) out_.tellp());
      vectors_[key] = offset;
      out_.write(buffer, bytes);
      return offset;
    }

    return man_vector<t_elem>(strings_.find(key)->second);
  }

  void WriteRule(
      const std::string & name,
      const std::vector<man_binding>& bindings,
      uint64_t rule_position) {
    auto data = RuleNode();
    data.type = man_node_t::RULE;
    data.size = sizeof(data);
    data.name = String(name);
    data.bindings = Vector<man_binding>(bindings);
    data.rule_position = rule_position;
    out_.write(reinterpret_cast<const char*>(&data), data.size);
  }

  void WriteBuild(
      const std::string & rule_name,
      man_vector<man_eval_string> out,
      uint16_t implicit_out_count,
      man_vector<man_eval_string> in,
      uint16_t implicit_in_count,
      uint16_t order_only_in_count,
      man_vector<man_eval_string> validations,
      man_vector<man_binding> bindings,
      uint64_t rule_position,
      uint64_t final_position
      ) {
    auto data = BuildNode();
    data.type = man_node_t::BUILD;
    data.size = sizeof(data);
    data.rule_name = String(rule_name);
    data.out = out;
    data.implicit_out_count = implicit_out_count;
    data.in = in;
    data.implicit_in_count = implicit_in_count;
    data.order_only_in_count = order_only_in_count;
    data.validations = validations;
    data.bindings = bindings;
    data.rule_position = rule_position;
    data.final_position = final_position;
    out_.write(reinterpret_cast<const char*>(&data), data.size);
  }

  void WriteInclude(bool new_scope, man_eval_string path,uint64_t final_position) {
    auto data = IncludeNode();
    data.type = man_node_t::INCLUDE;
    data.size = sizeof(data);
    data.new_scope = new_scope;
    data.path = path;
    data.final_position = final_position;
    out_.write(reinterpret_cast<const char*>(&data), data.size);
  }

  void WriteBinding(const std::string & name, const struct EvalString & value) {
    auto data = BindingNode();
    data.type = man_node_t::BINDING;
    data.size = sizeof(data);
    data.name = String(name);
    data.value = EvalString(value);
    out_.write(reinterpret_cast<const char*>(&data), data.size);
  }

  void WriteDefault(
      const man_vector<man_eval_string>& defaults,
      const man_vector<uint64_t>& default_positions,
      uint64_t final_position) {
    auto data = DefaultNode();
    data.type = man_node_t::DEFAULT;
    data.size = sizeof(data);
    data.defaults = defaults;
    data.default_positions = default_positions;
    out_.write(reinterpret_cast<const char*>(&data), data.size);
  }

  void WritePool(
      const std::string & name,
      man_eval_string depth,
      uint64_t pool_position,
      uint64_t depth_position,
      uint64_t final_position) {
    auto data = PoolNode();
    data.type = man_node_t::POOL;
    data.size = sizeof(data);
    data.name = String(name);
    data.depth = depth;
    data.pool_position = pool_position;
    data.depth_position = depth_position;
    out_.write(reinterpret_cast<const char*>(&data), data.size);
  }
};

class manifest_istream
{
  bool owns_buffer;
  const char * p;
 public:
  const char * buffer;
  explicit manifest_istream(
      const char * buffer,
      bool owns_buffer) : owns_buffer(owns_buffer), p(buffer), buffer(buffer) { }

  ~manifest_istream() {
    if (owns_buffer) {
      delete [] buffer;
    }
  }

  static std::shared_ptr<manifest_istream> create(std::istream& stream) {
    std::streamsize size = stream.tellg();
    stream.seekg(0, std::ios::beg);

    char * buffer = new char[size];
    if (!stream.read(buffer, size)) {
      assert(0);
      return nullptr;
    }
    return std::make_shared<manifest_istream>(buffer, true);
  }

  static std::shared_ptr<manifest_istream> create(std::string path) {
    auto stream = std::ifstream(path, std::ios::binary | std::ios::ate);
    return manifest_istream::create(stream);
  }

  man_node_t ReadNodeType() {
    auto result = *((man_node_t*) p);
    p += sizeof(man_node_t);
    return result;
  }

  man_node_t PeakNodeType() {
    return *((man_node_t*) p);
  }

  man_vector_count_t ReadStringSize() {
    auto result = *((man_vector_count_t*) p);
    p += sizeof(man_vector_count_t);
    return result;
  }

  man_node_byte_count_t ReadNodeSize() {
    auto result = *((man_node_byte_count_t *) p);
    p += sizeof(man_node_byte_count_t);
    return result;
  }

  void EatStartParse() {
    auto node = reinterpret_cast<const ParseStartNode*>(p);
    assert(node->size == sizeof(ParseStartNode));
    assert(node->type == man_node_t::START_PARSE);
    p += node->size;
  }

  bool IsCurrentVersion() {
    auto node = reinterpret_cast<const ParseStartNode*>(p);
    assert(node->size == sizeof(ParseStartNode));
    assert(node->type == man_node_t::START_PARSE);
    return node->version == MANIFEST_SCHEMA_VERSION
        && (node->checksum == 0 || node->checksum == MANIFEST_SCHEMA_CHECKSUM);
  }

  void EatEndParse() {
#ifdef NDEBUG
    ReadNodeType();
#else
    auto type = ReadNodeType();
    assert(type == man_node_t::END_PARSE);
#endif
  }

  man_node_t NextRecordType() {
    // First, consume any new strings declared.
    auto type = PeakNodeType();
    while(type == man_node_t::STRING
           || type == man_node_t::VECTOR) {
      p += sizeof(man_node_t);
      if (type == man_node_t::STRING) {
        p += ReadStringSize();
      } else if (type == man_node_t::VECTOR) {
        auto size = ReadNodeSize();
        p += size;
      } else {
        assert(0);
      }
      type = PeakNodeType();
    }
    return type;
  }

  const RuleNode * ReadRule() {
    auto node = reinterpret_cast<const RuleNode*>(p);
    assert(node->size == sizeof(RuleNode));
    assert(node->type == man_node_t::RULE);
    p += node->size;
    return node;
  }

  const BuildNode * ReadBuild() {
    auto node = reinterpret_cast<const BuildNode*>(p);
    assert(node->size == sizeof(BuildNode));
    assert(node->type == man_node_t::BUILD);
    p += node->size;
    return node;
  }

  const IncludeNode * ReadInclude() {
    auto node = reinterpret_cast<const IncludeNode*>(p);
    assert(node->size == sizeof(IncludeNode));
    assert(node->type == man_node_t::INCLUDE);
    p += node->size;
    return node;
  }

  const BindingNode * ReadBinding() {
    auto node = reinterpret_cast<const BindingNode*>(p);
    assert(node->size == sizeof(BindingNode));
    assert(node->type == man_node_t::BINDING);
    p += node->size;
    return node;
  }

  const DefaultNode * ReadDefault() {
    auto node = reinterpret_cast<const DefaultNode*>(p);
    assert(node->size == sizeof(DefaultNode));
    assert(node->type == man_node_t::DEFAULT);
    p += node->size;
    return node;
  }

  const PoolNode * ReadPool() {
    auto node = reinterpret_cast<const PoolNode*>(p);
    assert(node->size == sizeof(PoolNode));
    assert(node->type == man_node_t::POOL);
    p += node->size;
    return node;
  }
};

#endif  // NINJA_MANIFEST_STREAM_H
