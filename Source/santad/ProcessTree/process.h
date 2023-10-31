#ifndef SANTA__SANTAD_PROCESSTREE_PROCESS_H
#define SANTA__SANTAD_PROCESSTREE_PROCESS_H

#include <unistd.h>

#include <memory>
#include <string>
#include <vector>

#include "Source/santad/ProcessTree/tree.h"
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

 private:
  friend class ProcessTree;

  const struct pid pid_;
  std::shared_ptr<const cred> effective_cred_;
  std::shared_ptr<const program> program_;

  std::shared_ptr<const Process> parent_;
  uint64_t flags_;
  uint64_t exec_flag_mask_;
  uint64_t fork_flag_mask_;
};

}  // namespace process_tree

#endif
