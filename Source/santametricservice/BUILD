load("@build_bazel_rules_apple//apple:macos.bzl", "macos_command_line_application")
load("@santametricservice_crate_index//:defs.bzl", "aliases", "all_crate_deps")
load("@rules_rust//rust:defs.bzl", "rust_static_library")

package(
    default_visibility = ["//:santa_package_group"],
)

# TODO(nickmg): can this be a normal rust_library with suggestions from
# https://github.com/bazelbuild/rules_rust/issues/637
# (see also https://github.com/bazelbuild/rules_rust/pull/1490)
# (currently dies because "___rust_alloc" & friends isn't linked)
rust_static_library(
    name = "santametricservice_lib",
    srcs = glob(["src/*.rs"]),
    crate_root = "src/main.rs",
    edition = "2021",
    aliases = aliases(),
    deps = all_crate_deps(
        normal = True,
    ) + [
        "//Source/common:SNTConfigurator",
    ],
    proc_macro_deps = all_crate_deps(
        proc_macro = True,
    ),
)

# TODO: bad hack
# these aren't included properly by the rust static lib, so we have to do it ourselves
objc_library(
    name = "frameworks",
    hdrs = [],
    sdk_frameworks = [
        "Security",
        "IOKit",
    ],
)

macos_command_line_application(
    name = "santametricservice",
    bundle_id = "com.google.santa.metricservice",
    codesignopts = [
        "--timestamp",
        "--force",
        "--options library,kill,runtime",
    ],
    infoplists = ["Info.plist"],
    minimum_os_version = "11.0",
    provisioning_profile = select({
        "//:adhoc_build": None,
        "//conditions:default": "//profiles:santa_dev",
    }),
    version = "//:version",
    visibility = ["//:santa_package_group"],
    deps = [
        ":santametricservice_lib",
        ":frameworks",
    ],
)
