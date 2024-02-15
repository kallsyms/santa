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
#include "Source/santad/ProcessTree/SNTEndpointSecurityAdapter.h"

#include <EndpointSecurity/EndpointSecurity.h>
#include <Foundation/Foundation.h>
#include <bsm/libbsm.h>

#include "Source/santad/EventProviders/EndpointSecurity/EndpointSecurityAPI.h"
#include "Source/santad/EventProviders/EndpointSecurity/Message.h"
#include "Source/santad/ProcessTree/process_tree.h"
#include "Source/santad/ProcessTree/process_tree_macos.h"
#include "absl/status/statusor.h"

using santa::santad::event_providers::endpoint_security::EndpointSecurityAPI;
using santa::santad::event_providers::endpoint_security::Message;
using santa::santad::event_providers::endpoint_security::ForkMessage;
using santa::santad::event_providers::endpoint_security::ExecMessage;
using santa::santad::event_providers::endpoint_security::ExitMessage;

namespace santa::santad::process_tree {

// std::visit overload pattern
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void InformFromESEvent(ProcessTree &tree, const Message &msg) {
  struct Pid event_pid = PidFromAuditToken(msg->process->audit_token);
  auto proc = tree.Get(event_pid);

  if (!proc) {
    return;
  }

  msg.variant(overloaded{
    [&tree, proc](ForkMessage fork) {
      tree.HandleFork(Message(fork)->mach_time, **proc,
                      PidFromAuditToken(fork->child->audit_token));
    },
    [&tree, proc](ExecMessage exec) {
      std::vector<std::string> args;
      args.reserve(exec.ArgCount());
      for (int i = 0; i < exec.ArgCount(); i++) {
        es_string_token_t arg = exec.Arg(i);
        args.push_back(std::string(arg.data, arg.length));
      }

      es_string_token_t executable = exec->target->executable->path;
      tree.HandleExec(
        Message(exec)->mach_time, **proc, PidFromAuditToken(exec->target->audit_token),
        (struct Program){.executable = std::string(executable.data, executable.length),
                         .arguments = args},
        (struct Cred){
          .uid = audit_token_to_euid(exec->target->audit_token),
          .gid = audit_token_to_egid(exec->target->audit_token),
        });
    },
    [&tree, proc](ExitMessage exit) {
      tree.HandleExit(Message(exit)->mach_time, **proc);
    },
    [](Message m) {
      // TODO: programming error
    }
  });
}

}  // namespace santa::santad::process_tree
