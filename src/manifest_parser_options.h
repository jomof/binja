
#ifndef NINJA_MANIFEST_PARSER_OPTIONS_H
#define NINJA_MANIFEST_PARSER_OPTIONS_H
enum DupeEdgeAction {
  kDupeEdgeActionWarn,
  kDupeEdgeActionError,
};

enum PhonyCycleAction {
  kPhonyCycleActionWarn,
  kPhonyCycleActionError,
};

struct ManifestParserOptions {
  ManifestParserOptions()
      : dupe_edge_action_(kDupeEdgeActionWarn),
        phony_cycle_action_(kPhonyCycleActionWarn) {}
  DupeEdgeAction dupe_edge_action_;
  PhonyCycleAction phony_cycle_action_;
};

#endif  // NINJA_MANIFEST_PARSER_OPTIONS_H
