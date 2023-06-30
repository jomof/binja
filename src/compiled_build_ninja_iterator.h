
#ifndef NINJA_COMPILED_BUILD_NINJA_ITERATOR_H
#define NINJA_COMPILED_BUILD_NINJA_ITERATOR_H

namespace binja {
  struct CompiledBuildNinja;
}

class CompiledBuildNinjaIterator{
 public:
  CompiledBuildNinjaIterator(binja::CompiledBuildNinja * compiled);
  binja::CompiledBuildNinja * compiled_;
};

#endif  // NINJA_COMPILED_BUILD_NINJA_ITERATOR_H
