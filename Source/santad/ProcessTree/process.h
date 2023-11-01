#ifndef SANTA__SANTAD_PROCESSTREE_PROCESS_H
#define SANTA__SANTAD_PROCESSTREE_PROCESS_H

#include <unistd.h>

#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "Source/santad/ProcessTree/Annotations/base.h"
#include "Source/santad/ProcessTree/tree.h"
#include "absl/container/flat_hash_map.h"
#include "absl/time/time.h"

namespace process_tree {

// TODO: maybe change to a u64 and helpers to pack/unpack for the various OSs
struct pid {
  pid_t pid;
  int pidversion;
};

struct cred {
  uid_t uid;
  gid_t gid;
  std::optional<std::string> user;
  std::optional<std::string> group;
};

struct program {
  std::string executable;
  std::vector<std::string> arguments;
};

class ProcessTree;  // fwd decl

class Process {
 public:
  explicit Process() {}
  explicit Process(pid pid, std::shared_ptr<cred> cred,
                   std::shared_ptr<program> program,
                   std::shared_ptr<Process> parent, uint64_t flags)
      : pid_(pid),
        cred_(cred),
        program_(program),
        parent_(parent),
        flags_(flags) {}
  static absl::StatusOr<Process> LoadPID(pid_t pid);

  // Const "attributes" are public
  const struct pid pid_;
  const std::shared_ptr<const cred> effective_cred_;
  const std::shared_ptr<const program> program_;

 private:
  friend class ProcessTree;

  // This is not API.
  // The tree helper methods are the API, and we just happen to implement
  // annotation storage and the parent relation in memory on the process right
  // now.
  absl::flat_hash_map<std::type_info, Annotator> annotations_;
  std::shared_ptr<const Process> parent_;
};

}  // namespace process_tree

#endif
