#include "Source/santad/ProcessTree/tree.h"

#include <libproc.h>

#include <functional>
#include <memory>
#include <typeinfo>
#include <vector>

#include "Source/santad/ProcessTree/Annotations/base.h"
#include "Source/santad/ProcessTree/process.h"
#include "absl/status/status.h"
#include "absl/synchronization/mutex.h"

namespace process_tree {

absl::Status ProcessTree::Backfill() {
  absl::MutexLock lock(&mtx_);
  int n_procs = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0);
  if (n_procs < 0) {
    return absl::InternalError("proc_listpids failed");
  }

  std::vector<pid_t> pids;
  pids.reserve(n_procs + 16);  // add space for a few more processes
                               // in case some spawn in-between.

  n_procs = proc_listpids(PROC_ALL_PIDS, 0, pids.data(),
                          pids.capacity() * sizeof(pid_t));
  if (n_procs < 0) {
    return absl::InternalError("proc_listpids failed");
  }

  for (pid_t pid : pids) {
    auto proc = Process::LoadPID(pid);
    if (proc.ok()) {
      // Determine ppid
      // Alternatively, there's a sysctl interface:
      //  https://chromium.googlesource.com/chromium/chromium/+/master/base/process_util_openbsd.cc#32
      struct proc_bsdinfo bsdinfo;
      if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &bsdinfo, sizeof(bsdinfo)) !=
          PROC_PIDTBSDINFO_SIZE) {
        continue;
      };

      // We could also pull e.g. start time, pgid, associated tty, etc. from
      // bsdinfo here.

      proc.parent_ = map_[bsdinfo.pbi_ppid];
      proc.flags_ = proc.parent_.flags_ & proc.parent_.fork_flag_mask_ &
                    proc.parent_.exec_flag_mask_;
      for (size_t i = 0; i < annotators_.size(); i++) {
        auto propagation_rule = annotators_[i](proc);
        proc.fork_flag_mask_ = (proc.fork_flag_mask_ & ~(1 << i)) |
                               ((uint64_t)propagation_rule.on_fork << i);
        proc.exec_flag_mask_ = (proc.exec_flag_mask_ & ~(1 << i)) |
                               ((uint64_t)propagation_rule.on_exec << i);
      }
      map_.insert(pid, std::make_shared<Process>(proc));
    }
  }

  return absl::OkStatus();
}

void ProcessTree::HandleFork(const Process &parent, const struct pid new_pid) {
  absl::MutexLock lock(&mtx_);
  Process child = parent;
  child.pid_ = new_pid;
  child.parent_ = map_[parent.pid_.pid];
  for (auto annotator : annotators_) {
    annotator.ProcessFork(this, parent, child);
  }
  map_.insert(new_pid.pid, std::make_shared<Process>(child));
}

void ProcessTree::HandleExec(const Process &p, const struct program prog,
                             const struct cred c) {
  absl::MutexLock lock(&mtx_);
  Process new_proc = p;
  new_proc.cred_ = c;
  new_proc.program_ = prog;
  for (auto annotator : annotators_) {
    annotator.ProcessExec(this, p, new_proc);
  }
  map_[p.pid_.pid] = new_proc;
}

void ProcessTree::HandleExit(const Process &p) {
  absl::MutexLock lock(&mtx_);
  map_.erase(p.pid_.pid);
}

/*
---
Annotation get/set
---
*/

void ProcessTree::AnnotateProcess(const Process &p, Annotator a) {
  p.annotations_[typeid(a)] = a;
}

std::optional<const Annotator> ProcessTree::GetAnnotation(
    const Process &p, std::type_info annotator_type) {
  auto it = p.annotations_.find(annotator_type);
  if (it == p.annotations_.end()) {
    return std::nullopt;
  }
  return *it;
}

/*
---
Tree inspection methods
---
*/

std::vector<std::shared_ptr<const Process>> ProcessTree::RootSlice(
    std::shared_ptr<const Process> p) {
  std::vector<std::shared_ptr<const Process>> slice;
  while (p) {
    slice.push_back(p);
    p = p->parent_;
  }
  return slice;
}

void ProcessTree::Iterate(
    std::function<void(std::shared_ptr<const Process>)> f) {
  std::vector<std::shared_ptr<const Process>> procs;
  procs.reserve(map_.size());
  {
    absl::MutexLock lock(&mtx_);
    for (auto &[_, proc] : map_) {
      procs.push_back(proc);
    }
  }

  for (auto &p : procs) {
    f(p);
  }
}

std::optional<std::shared_ptr<const Process>> ProcessTree::Get(
    const struct pid target) {
  auto it = map_.find(target.pid);
  if (it == map_.end()) {
    return std::nullopt;
  }
  return *it;
}

std::shared_ptr<const Process> ProcessTree::GetParent(const Process &p) {
  return p.parent_;
}

}  // namespace process_tree
