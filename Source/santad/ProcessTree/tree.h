#ifndef SANTA__SANTAD_PROCESSTREE_TREE_H
#define SANTA__SANTAD_PROCESSTREE_TREE_H

#include <memory>
#include <typeinfo>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"
#include "process.h"

namespace process_tree {

class ProcessTree {
 public:
  explicit ProcessTree() {}

  void RegisterAnnotator(Annotator a) { annotators_.push_back(a); };
  absl::Status Backfill();
  void HandleFork(const Process &parent, const struct pid child);
  void HandleExec(const Process &p, const struct program prog,
                  const struct cred c);
  void HandleExit(const Process &p);

  void AnnotateProcess(const Process &p, Annotator a);
  Annotator GetAnnotation(const Process &p, std::type_info annotator_type);

  std::vector<std::shared_ptr<Process>> RootSlice(
      std::shared_ptr<const Process> p);
  void Iterate(std::function<void(std::shared_ptr<const Process>)> f);
  std::optional<std::shared_ptr<const Process>> Get(const struct pid target);
  std::shared_ptr<const Process> GetParent(const Process &p);

 private:
  std::vector<Annotator> annotators_;
  absl::Mutex mtx_;
  // N.B. Map from pid_t not struct pid since only 1 process with the given pid
  // can be active. We don't need to key off of the "pid + version".
  absl::flat_hash_map<pid_t, std::shared_ptr<const Process>> map_
      ABSL_GUARDED_BY(mtx_);
};
}  // namespace process_tree

#endif
