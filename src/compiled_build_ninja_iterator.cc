
#include "compiled_build_ninja_iterator.h"
#include "binja_generated.h"

CompiledBuildNinjaIterator:: CompiledBuildNinjaIterator(
    binja::CompiledBuildNinja * compiled) : compiled_(compiled) {
  compiled_ = compiled;
}
