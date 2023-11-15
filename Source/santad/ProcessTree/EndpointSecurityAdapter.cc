#include <CoreFoundation/CoreFoundation.h>
#include <EndpointSecurity/EndpointSecurity.h>
#include <bsm/libbsm.h>

#include "Source/santad/ProcessTree/tree.h"

extern "C" {
extern void NSLog(CFStringRef format, ...);
}

namespace process_tree {

struct pid PidFromAuditToken(const audit_token_t &tok) {
  return (struct pid){.pid = audit_token_to_pid(tok),
                      .pidversion = audit_token_to_pidversion(tok)};
}

void InformFromESEvent(int client, ProcessTree &tree, const es_message_t *msg) {
  NSLog(CFSTR("step %d @ %lu"), client, msg->mach_time);
  if (!tree.Step(client, msg->mach_time)) {
    return;
  };

  struct pid event_pid = PidFromAuditToken(msg->process->audit_token);
  auto proc = tree.Get(event_pid);
  NSLog(CFSTR("event type %d @ %lu, pid %d:%d, proc %d %p"), msg->event_type,
        msg->mach_time, event_pid.pid, event_pid.pidversion, proc.has_value(),
        proc->get());

  if (!proc) {
    NSLog(CFSTR("no proc %d:%d in tree, skipping event..."), event_pid.pid,
          event_pid.pidversion);
    return;
  }

  switch (msg->event_type) {
    case ES_EVENT_TYPE_AUTH_EXEC:
    case ES_EVENT_TYPE_NOTIFY_EXEC: {
      NSLog(CFSTR("exec to %d:%d"),
            PidFromAuditToken(msg->event.exec.target->audit_token).pid,
            PidFromAuditToken(msg->event.exec.target->audit_token).pidversion);
      std::vector<std::string> args;
      args.reserve(es_exec_arg_count(&msg->event.exec));
      for (int i = 0; i < es_exec_arg_count(&msg->event.exec); i++) {
        es_string_token_t arg = es_exec_arg(&msg->event.exec, i);
        args.push_back(std::string(arg.data, arg.length));
      }

      es_string_token_t executable = msg->event.exec.target->executable->path;
      tree.HandleExec(
          msg->mach_time, **proc,
          PidFromAuditToken(msg->event.exec.target->audit_token),
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
      NSLog(CFSTR("fork to %d:%d"),
            PidFromAuditToken(msg->event.fork.child->audit_token).pid,
            PidFromAuditToken(msg->event.fork.child->audit_token).pidversion);
      tree.HandleFork(msg->mach_time, **proc,
                      PidFromAuditToken(msg->event.fork.child->audit_token));
      break;
    }
    case ES_EVENT_TYPE_NOTIFY_EXIT:
      tree.HandleExit(msg->mach_time, **proc);
      break;
    default:
      NSLog(CFSTR("Unexpected event type %d"), msg->event_type);
      return;
  }
}
}  // namespace process_tree
