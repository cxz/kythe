load("//tools:build_rules/shims.bzl", "go_test", "go_library")

package(default_visibility = ["//kythe:default_visibility"])

go_library(
    name = "keyvalue",
    srcs = ["keyvalue.go"],
    deps = [
        "//kythe/go/services/graphstore",
        "//kythe/go/util/datasize",
        "//kythe/proto:storage_go_proto",
    ],
)

go_test(
    name = "keyvalue_test",
    size = "small",
    srcs = ["keyvalue_test.go"],
    library = "keyvalue",
    visibility = ["//visibility:private"],
    deps = [
        "//kythe/proto:storage_go_proto",
        "@com_github_golang_protobuf//proto:go_default_library",
    ],
)
