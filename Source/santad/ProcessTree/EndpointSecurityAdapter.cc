#include <EndpointSecurity/EndpointSecurity.h>
#include <bsm/libbsm.h>

#include "Source/santad/ProcessTree/tree.h"

namespace process_tree {

namespace {
struct pid PidFromAuditToken(const audit_token_t &tok) {
  return (struct pid){.pid = audit_token_to_pid(tok),
                      .pidversion = audit_token_to_pidversion(tok)};
}
}  // namespace

void InformFromESEvent(ProcessTree &tree, const es_message_t *msg) {
  static uint64_t ts = 0;
  if (msg->mach_time < ts) {
    return;
  }
  ts = msg->mach_time;

  struct pid event_pid = PidFromAuditToken(msg->process->audit_token);
  auto proc = tree.Get(event_pid);

  switch (msg->event_type) {
    case ES_EVENT_TYPE_AUTH_EXEC: {
      std::vector<std::string> args;
      args.reserve(es_exec_arg_count(&msg->event.exec));
      for (int i = 0; i < es_exec_arg_count(&msg->event.exec); i++) {
        es_string_token_t arg = es_exec_arg(&msg->event.exec, i);
        args.push_back(std::string(arg.data, arg.length));
      }

      es_string_token_t executable = msg->event.exec.target->executable->path;
      tree.HandleExec(
          **proc, PidFromAuditToken(msg->event.exec.target->audit_token),
          (struct program){
              .executable = std::string(executable.data, executable.length),
              .arguments = args},
          (struct cred){
              .uid = audit_token_to_euid(msg->event.exec.target->audit_token),
              .gid = audit_token_to_egid(msg->event.exec.target->audit_token),
          });

      break;
    }
    case ES_EVENT_TYPE_NOTIFY_FORK: {
      tree.HandleFork(**proc,
                      PidFromAuditToken(msg->event.fork.child->audit_token));
      break;
    }
    case ES_EVENT_TYPE_NOTIFY_EXIT:
      tree.HandleExit(**proc);
      break;
    default:
      return;
  }
}
}  // namespace process_tree
