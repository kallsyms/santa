#ifndef SANTA__SANTAD_PROCESSTREE_TREE_H
#define SANTA__SANTAD_PROCESSTREE_TREE_H

#include <functional>
#include <memory>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"
#include "process.h"

namespace process_tree {
using Annotator = std::function<AnnotationRule(std::shared_ptr<const Process>)>;

struct AnnotationRule {
  T value;
  bool on_exec;
  bool on_fork;
};

class ProcessTree {
 public:
  explicit ProcessTree() {}

  void RegisterAnnotator(Annotator a) { annotators_.push_back(a); };
  absl::Status Backfill();
  void HandleFork(const Process &parent, const struct pid child);
  void HandleExec(const Process &p, const struct program prog,
                  const struct cred c);
  void HandleExit(const Process &p);
  std::vector<std::shared_ptr<Process>> RootSlice(std::shared_ptr<Process> p);
  void Iterate(std::function<void(std::shared_ptr<const Process>)> f);
  std::shared_ptr<const Process> Get(const struct pid target);

 private:
  std::vector<Annotator> annotators_;
  absl::Mutex mtx_;
  absl::flat_hash_map<pid_t, std::shared_ptr<const Process>> map_
      ABSL_GUARDED_BY(mtx_);
};
}  // namespace process_tree

#endif
