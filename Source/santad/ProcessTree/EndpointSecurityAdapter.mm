/// Copyright 2023 Google LLC
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     https://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
#include <Foundation/Foundation.h>
#include <EndpointSecurity/EndpointSecurity.h>
#include <bsm/libbsm.h>

#include "Source/santad/ProcessTree/tree.h"
#include "Source/santad/ProcessTree/tree_darwin.h"
#include "absl/status/statusor.h"

namespace process_tree {

void InformFromESEvent(int client, ProcessTree &tree, const es_message_t *msg) {
  NSLog(@"step %d @ %llu", client, msg->mach_time);
  if (!tree.Step(client, msg->mach_time)) {
    return;
  };

  struct pid event_pid = PidFromAuditToken(msg->process->audit_token);
  auto proc = tree.Get(event_pid);
  NSLog(@"event type %d @ %llu, pid %d:%d, proc %d %p", msg->event_type,
        msg->mach_time, event_pid.pid, event_pid.pidversion, proc.has_value(),
        proc->get());

  if (!proc) {
    NSLog(@"no proc %d:%d in tree, skipping event...", event_pid.pid,
          event_pid.pidversion);
    return;
  }

  switch (msg->event_type) {
    case ES_EVENT_TYPE_AUTH_EXEC:
    case ES_EVENT_TYPE_NOTIFY_EXEC: {
      NSLog(@"exec to %d:%d",
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
      NSLog(@"fork to %d:%d",
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
      NSLog(@"Unexpected event type %d", msg->event_type);
      return;
  }
}
}  // namespace process_tree
