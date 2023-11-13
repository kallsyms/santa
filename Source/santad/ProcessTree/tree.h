#ifndef SANTA__SANTAD_PROCESSTREE_TREE_H
#define SANTA__SANTAD_PROCESSTREE_TREE_H

#include <memory>
#include <typeinfo>
#include <vector>

#include "Source/santad/ProcessTree/process.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/synchronization/mutex.h"
#include "process.h"

namespace process_tree {

class ProcessTree {
 public:
  explicit ProcessTree() {}

  // Register an Annotator class to be automatically processed on process
  // lifecycle events.
  void RegisterAnnotator(std::unique_ptr<Annotator> a);

  // Initialize the tree with the processes currently running on the system.
  absl::Status Backfill();

  // Inform the tree of a fork event, in which the parent process spawns a child
  // with the only difference between the two being the pid.
  void HandleFork(const Process &parent, const struct pid child);

  // Inform the tree of an exec event, in which the program and potentially cred
  // of a Process change.
  // N.B. new_pid is required as the "pid version" will have changed.
  // It is a programming error to pass a new_pid such that
  // p.pid_.pid != new_pid.pid.
  void HandleExec(const Process &p, const struct pid new_pid,
                  const struct program prog, const struct cred c);

  // Inform the tree of a process exit.
  void HandleExit(const Process &p);

  // Annotate the given process with an Annotator (state).
  void AnnotateProcess(const Process &p, std::shared_ptr<const Annotator> &&a);

  // Get the given annotation on the given process if it exists, or nullopt if
  // the annotation is not set.
  template <typename T>
  std::optional<std::shared_ptr<const T>> GetAnnotation(const Process &p) const;

  // Atomically get the slice of Processes going from the given process "up" to
  // the root. The root process has no parent.
  // N.B. There may be more than one root process. E.g. on Linux, both init (PID
  // 1) and kthread (PID 2) are considered roots, as they are reported to have
  // PPID=0.
  std::vector<std::shared_ptr<const Process>> RootSlice(
      std::shared_ptr<const Process> p) const;

  // Call f for all processes in the tree. The list of processes is captured
  // before invoking f, so it is safe to mutate the tree in f.
  void Iterate(std::function<void(std::shared_ptr<const Process>)> f) const;

  // Get the Process for the given pid in the tree if it exists.
  std::optional<std::shared_ptr<const Process>> Get(
      const struct pid target) const;

  // Traverse the tree from the given Process to its parent.
  std::shared_ptr<const Process> GetParent(const Process &p) const;

  // Dump the tree in a human readable form to the given ostream.
  void DebugDump(std::ostream &stream) const;

 private:
  void BackfillInsertChildren(
      absl::flat_hash_map<pid_t, std::vector<const Process>> &parent_map,
      std::shared_ptr<Process> parent, const Process &unlinked_proc)
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(mtx_);

  void DebugDumpLocked(std::ostream &stream, int depth, pid_t ppid) const;

  std::vector<std::unique_ptr<Annotator>> annotators_;

  mutable absl::Mutex mtx_;
  // N.B. Map from pid_t not struct pid since only 1 process with the given pid
  // can be active. We don't need to key off of the "pid + version".
  absl::flat_hash_map<pid_t, std::shared_ptr<Process>> map_
      ABSL_GUARDED_BY(mtx_);
};

template <typename T>
std::optional<std::shared_ptr<const T>> ProcessTree::GetAnnotation(
    const Process &p) const {
  auto it = p.annotations_.find(std::type_index(typeid(T)));
  if (it == p.annotations_.end()) {
    return std::nullopt;
  }
  return std::dynamic_pointer_cast<const T>(it->second);
}

}  // namespace process_tree

#endif
