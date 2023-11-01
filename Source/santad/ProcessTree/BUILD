load("@rules_cc//cc:defs.bzl", "cc_proto_library")

cc_library(
  name = "process_tree",
  srcs = ["process.cc", "tree.cc"],
  hdrs = ["process.h", "tree.h"],
  deps = [
      "@com_google_absl//absl/container:flat_hash_map",
      "@com_google_absl//absl/synchronization:mutex",
      "//Source/santad/ProcessTree/Annotations:all_annotations"
      "//Source/santad/ProcessTree/Annotations:base"
  ]
)

proto_library(
    name = "process_tree_proto",
    srcs = ["process_tree.proto"],
)

cc_proto_library(
    name = "process_tree_cc_proto",
    deps = [":process_tree_proto"],
)
