load("@rules_cc//cc:defs.bzl", "cc_proto_library")

package(
    default_visibility = ["//:santa_package_group"],
)

cc_library(
  name = "process",
  srcs = ["process.cc"],
  hdrs = ["process.h"],
  deps = [
      "@com_google_absl//absl/container:flat_hash_map",
      "@com_google_absl//absl/synchronization",
      "@com_google_absl//absl/status:statusor",
      "//Source/santad/ProcessTree/Annotations:base",
  ],
)

cc_library(
  name = "process_tree",
  srcs = ["tree.cc"],
  hdrs = ["tree.h"],
  deps = [
      "@com_google_absl//absl/container:flat_hash_map",
      "@com_google_absl//absl/synchronization",
      "@com_google_absl//absl/status",
      ":process",
      "//Source/santad/ProcessTree/Annotations:base",
  ],
)

proto_library(
    name = "process_tree_proto",
    srcs = ["process_tree.proto"],
)

cc_proto_library(
    name = "process_tree_cc_proto",
    deps = [":process_tree_proto"],
)

cc_library(
  name = "EndpointSecurityAdapter",
  srcs = ["EndpointSecurityAdapter.cc"],
  hdrs = ["EndpointSecurityAdapter.h"],
  deps = [
      ":process_tree",
  ],
)
