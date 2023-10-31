load("//:helper.bzl", "run_command", "santa_unit_test")

package(default_visibility = ["//:santa_package_group"])

licenses(["notice"])

santa_unit_test(
    name = "SNTExecTest",
    srcs = [
        "SNTExecTest.m",
    ],
    data = [
        "//Source/santad/testdata:binaryrules_testdata",
    ],
    minimum_os_version = "11.0",
    deps = [],
)

test_suite(
    name = "integration_tests",
    tests = [
        ":SNTExecTest",
    ],
)

run_command(
    name = "install_profile",
    cmd = "osascript $${BUILD_WORKSPACE_DIRECTORY}/Testing/integration/install_profile.scpt $${BUILD_WORKSPACE_DIRECTORY}/$$1",
)

run_command(
    name = "allow_sysex",
    cmd = "osascript $${BUILD_WORKSPACE_DIRECTORY}/Testing/integration/allow_sysex.scpt",
)

run_command(
    name = "dismiss_santa_popup",
    cmd = "osascript $${BUILD_WORKSPACE_DIRECTORY}/Testing/integration/dismiss_santa_popup.scpt",
)

run_command(
    name = "dismiss_usb_popup",
    cmd = "osascript $${BUILD_WORKSPACE_DIRECTORY}/Testing/integration/dismiss_usb_popup.scpt",
)

run_command(
    name = "reset_vm",
    cmd = """
sudo killall moroz || true

sudo $${BUILD_WORKSPACE_DIRECTORY}/Conf/uninstall.sh

sudo systemextensionsctl reset &  # Blocks on a password dialog
sleep 3
osascript -e "tell application \\"System Events\\"
    tell process \\"SecurityAgent\\"
        set value of text field 2 of window 1 to system attribute \\"VM_PASSWORD\\"
        click button \\"OK\\" of window 1
    end tell
end tell
"
""",
)
