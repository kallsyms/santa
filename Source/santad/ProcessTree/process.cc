#include "Source/santad/ProcessTree/process.h"

#include <bsm/libbsm.h>
#include <libproc.h>
#include <mach/message.h>
#include <string.h>
#include <sys/sysctl.h>

#include <string>
#include <vector>

#include "absl/status/status.h"

namespace process_tree {

namespace {
// Modified from
// https://chromium.googlesource.com/crashpad/crashpad/+/360e441c53ab4191a6fd2472cc57c3343a2f6944/util/posix/process_util_mac.cc
// TODO: https://github.com/apple-oss-distributions/adv_cmds/blob/main/ps/ps.c
absl::StatusOr<std::vector<std::string>> ProcessArgumentsForPID(pid_t pid) {
  // The format of KERN_PROCARGS2 is explained in 10.9.2 adv_cmds-153/ps/print.c
  // getproclline(). It is an int (argc) followed by the executable’s string
  // area. The string area consists of NUL-terminated strings, beginning with
  // the executable path, and then starting on an aligned boundary, all of the
  // elements of argv, envp, and applev.
  // It is possible for a process to exec() in between the two sysctl() calls
  // below. If that happens, and the string area of the new program is larger
  // than that of the old one, args_size_estimate will be too small. To detect
  // this situation, the second sysctl() attempts to fetch args_size_estimate +
  // 1 bytes, expecting to only receive args_size_estimate. If it gets the extra
  // byte, it indicates that the string area has grown, and the sysctl() pair
  // will be retried a limited number of times.
  size_t args_size_estimate;
  size_t args_size;
  std::string args;
  int tries = 3;
  do {
    int mib[] = {CTL_KERN, KERN_PROCARGS2, pid};
    int rv = sysctl(mib, 3, nullptr, &args_size_estimate, nullptr, 0);
    if (rv != 0) {
      return absl::InternalError("KERN_PROCARGS2");
    }
    args_size = args_size_estimate + 1;
    args.resize(args_size);
    rv = sysctl(mib, 3, &args[0], &args_size, nullptr, 0);
    if (rv != 0) {
      return absl::InternalError("KERN_PROCARGS2");
    }
  } while (args_size == args_size_estimate + 1 && tries--);
  if (args_size == args_size_estimate + 1) {
    return absl::InternalError("Couldn't determine size");
  }
  // KERN_PROCARGS2 needs to at least contain argc.
  if (args_size < sizeof(int)) {
    return absl::InternalError("Bad args_size");
  }
  args.resize(args_size);
  // Get argc.
  int argc;
  memcpy(&argc, &args[0], sizeof(argc));
  // Find the end of the executable path.
  size_t start_pos = sizeof(argc);
  size_t nul_pos = args.find('\0', start_pos);
  if (nul_pos == std::string::npos) {
    return absl::InternalError("Can't find end of executable path");
  }
  // Find the beginning of the string area.
  start_pos = args.find_first_not_of('\0', nul_pos);
  if (start_pos == std::string::npos) {
    return absl::InternalError("Can't find args after executable path");
  }
  std::vector<std::string> local_argv;
  while (argc-- && nul_pos != std::string::npos) {
    nul_pos = args.find('\0', start_pos);
    local_argv.push_back(args.substr(start_pos, nul_pos - start_pos));
    start_pos = nul_pos + 1;
  }
  return local_argv;
}
}  // namespace

static absl::StatusOr<Process> Process::LoadPID(pid_t pid) {
  kern_return_t kernReturn;
  task_name_t task;
  mach_msg_type_number_t size = TASK_AUDIT_TOKEN_COUNT;
  audit_token_t token;

  kernReturn = task_name_for_pid(mach_task_self(), pid, &task);
  if (kernReturn != KERN_SUCCESS) {
    return absl::InternalError("task_name_for_pid");
  }

  kernReturn = task_info(task, TASK_AUDIT_TOKEN, (task_info_t)&token, &size);
  mach_port_deallocate(mach_task_self(), task);

  if (kernReturn != KERN_SUCCESS) {
    return absl::InternalError("task_info(TASK_AUDIT_TOKEN)");
  }

  char path[PROC_PIDPATHINFO_MAXSIZE];
  if (proc_pidpath_audittoken(&token, path, sizeof(path)) <= 0) {
    return absl::InternalError("proc_pidpath_audittoken");
  }

  // Don't fail Process creation if args can't be recovered.
  std::vector<std::string> args =
      ProcessArgumentsForPID(audit_token_to_pid(token)).value_or({});

  return Process(
      (struct pid){.pid = audit_token_to_pid(token),
                   .pidversion = audit_token_to_pidversion(token)},
      (struct cred){
          .uid = audit_token_to_euid(token), .gid = audit_token_to_egid(token),
          // TODO username, group
      },
      (struct program){
          .executable = path,
          .arguments = args,
      },
      nullptr, 0);
}

}  // namespace process_tree
