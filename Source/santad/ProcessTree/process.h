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
#ifndef SANTA__SANTAD_PROCESSTREE_PROCESS_H
#define SANTA__SANTAD_PROCESSTREE_PROCESS_H

#include <unistd.h>

#include <memory>
#include <string>
#include <typeindex>
#include <vector>

#include "Source/santad/ProcessTree/Annotations/base.h"
#include "absl/container/flat_hash_map.h"

namespace process_tree {

// TODO: maybe change to a u64 and helpers to pack/unpack for the various OSs
struct pid {
  pid_t pid;
  int pidversion;

  friend bool operator==(const struct pid &lhs, const struct pid &rhs) {
    return lhs.pid == rhs.pid && lhs.pidversion == rhs.pidversion;
  }
  friend bool operator!=(const struct pid &lhs, const struct pid &rhs) {
    return !(lhs == rhs);
  }
};

template <typename H>
H AbslHashValue(H h, const struct pid &p) {
  return H::combine(std::move(h), p.pid, p.pidversion);
}

struct cred {
  uid_t uid;
  gid_t gid;

  friend bool operator==(const struct cred &lhs, const struct cred &rhs) {
    return lhs.uid == rhs.uid && lhs.gid == rhs.gid;
  }
  friend bool operator!=(const struct cred &lhs, const struct cred &rhs) {
    return !(lhs == rhs);
  }
};

struct program {
  std::string executable;
  std::vector<std::string> arguments;

  friend bool operator==(const struct program &lhs, const struct program &rhs) {
    return lhs.executable == rhs.executable && lhs.arguments == rhs.arguments;
  }
  friend bool operator!=(const struct program &lhs, const struct program &rhs) {
    return !(lhs == rhs);
  }
};

// Fwd decls
class ProcessTree;

class Process {
 public:
  explicit Process(pid pid, std::shared_ptr<const cred> cred,
                   std::shared_ptr<const program> program,
                   std::shared_ptr<const Process> parent)
      : pid_(pid),
        effective_cred_(cred),
        program_(program),
        annotations_(),
        parent_(parent) {}

  // Const "attributes" are public
  const struct pid pid_;
  const std::shared_ptr<const cred> effective_cred_;
  const std::shared_ptr<const program> program_;

 private:
  // This is not API.
  // The tree helper methods are the API, and we just happen to implement
  // annotation storage and the parent relation in memory on the process right
  // now.
  friend class ProcessTree;
  absl::flat_hash_map<std::type_index, std::shared_ptr<const Annotator>>
      annotations_;
  std::shared_ptr<const Process> parent_;
  // TODO(nickmg): atomic here breaks the build.
  int refcnt_;
  // If the process is tombstoned, the event removing it from the tree has been
  // processed, but refcnt>0 keeps it alive.
  bool tombstoned_;
};

}  // namespace process_tree

#endif
