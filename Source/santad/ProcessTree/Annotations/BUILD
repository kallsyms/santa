cc_library(
  name = "base",
  hdrs = ["base.h"],
  deps = [
    "//Source/santad/ProcessTree:process_tree_cc_proto",
  ]
)

cc_library(
  name = "curl_sh"
  hdrs = ["curl_sh.h"],
  srcs = ["curl_sh.cc"],
  deps = [
    ":base",
  ],
)

cc_library(
  name = "originator"
  hdrs = ["originator.h"],
  srcs = ["originator.cc"],
  deps = [
    ":base",
  ],
)

cc_library(
  name = "all_annotations",
  deps = [
    ":curl_sh",
    ":originator",
  ],
)
