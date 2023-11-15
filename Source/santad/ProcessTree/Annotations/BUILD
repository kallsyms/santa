load("//:helper.bzl", "santa_unit_test")

package(
    default_visibility = ["//:santa_package_group"],
)

cc_library(
  name = "base",
  hdrs = ["base.h"],
  deps = [
    "//Source/santad/ProcessTree:process_tree_cc_proto",
  ],
)

cc_library(
  name = "curl_sh",
  hdrs = ["curl_sh.h"],
  srcs = ["curl_sh.cc"],
  deps = [
    ":base",
    "//Source/santad/ProcessTree:process_tree",
  ],
)

cc_library(
  name = "originator",
  hdrs = ["originator.h"],
  srcs = ["originator.cc"],
  deps = [
    ":base",
    "//Source/santad/ProcessTree:process_tree",
  ],
)

santa_unit_test(
  name = "originator_test",
  srcs = ["originator_test.mm"],
  deps = [
    ":originator",
    "//Source/santad/ProcessTree:process_tree",
    "//Source/santad/ProcessTree:process_tree_cc_proto",
    "//Source/santad/ProcessTree:tree_test_helpers",
  ],
)

cc_library(
  name = "all_annotations",
  deps = [
    ":curl_sh",
    ":originator",
  ],
)
