#include "Source/santad/ProcessTree/tree.h"

#include <libproc.h>

#include <functional>
#include <memory>
#include <typeindex>
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

  // TODO: topo-sort
  for (pid_t pid : pids) {
    auto proc_status = Process::LoadPID(pid);
    if (proc_status.ok()) {
      auto unlinked_proc = proc_status.value();

      // Determine ppid
      // Alternatively, there's a sysctl interface:
      //  https://chromium.googlesource.com/chromium/chromium/+/master/base/process_util_openbsd.cc#32
      struct proc_bsdinfo bsdinfo;
      if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &bsdinfo, sizeof(bsdinfo)) !=
          PROC_PIDTBSDINFO_SIZE) {
        continue;
      };

      std::shared_ptr<const Process> parent = map_[bsdinfo.pbi_ppid];

      // We could also pull e.g. start time, pgid, associated tty, etc. from
      // bsdinfo here.
      auto proc = std::make_shared<Process>(
          Process(unlinked_proc.pid_,
                  // Re-use shared pointers from parent if value equivalent
                  *(unlinked_proc.effective_cred_) == *(parent->effective_cred_)
                      ? parent->effective_cred_
                      : unlinked_proc.effective_cred_,
                  *(unlinked_proc.program_) == *(parent->program_)
                      ? parent->program_
                      : unlinked_proc.program_,
                  parent));
      map_.emplace(pid, proc);

      for (auto &annotator : annotators_) {
        annotator.AnnotateFork(*this, *(proc->parent_), *proc);
        if (proc->program_ != proc->parent_->program_) {
          annotator.AnnotateExec(*this, *(proc->parent_), *proc);
        }
      }
    }
  }

  return absl::OkStatus();
}

void ProcessTree::HandleFork(const Process &parent, const pid new_pid) {
  absl::MutexLock lock(&mtx_);
  auto child = std::make_shared<Process>(
      new_pid, parent.effective_cred_, parent.program_, map_[parent.pid_.pid]);
  map_.emplace(new_pid.pid, child);
  for (auto annotator : annotators_) {
    annotator.AnnotateFork(*this, parent, *child);
  }
}

void ProcessTree::HandleExec(const Process &p, const pid new_pid,
                             const program prog, const cred c) {
  absl::MutexLock lock(&mtx_);

  // TODO(nickmg): should struct pid be reworked and only pid_version be passed?
  assert(new_pid.pid == p.pid_.pid);

  auto new_proc = std::make_shared<Process>(
      new_pid, std::make_shared<const cred>(c),
      std::make_shared<const program>(prog), p.parent_);
  map_.emplace(new_proc->pid_.pid, new_proc);
  for (auto annotator : annotators_) {
    annotator.AnnotateExec(*this, p, *new_proc);
  }
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

void ProcessTree::AnnotateProcess(const Process &p, Annotator &&a) {
  absl::MutexLock lock(&mtx_);
  map_[p.pid_.pid]->annotations_.emplace(
      std::type_index(typeid(a)), std::make_unique<Annotator>(std::move(a)));
}

std::optional<const Annotator> ProcessTree::GetAnnotation(
    const Process &p, const std::type_info annotator_type) {
  auto it = p.annotations_.find(std::type_index(annotator_type));
  if (it == p.annotations_.end()) {
    return std::nullopt;
  }
  return *(it->second);
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
    std::function<void(std::shared_ptr<const Process> p)> f) {
  std::vector<std::shared_ptr<const Process>> procs;
  {
    absl::MutexLock lock(&mtx_);
    procs.reserve(map_.size());
    for (auto &[_, proc] : map_) {
      procs.push_back(proc);
    }
  }

  for (auto &p : procs) {
    f(p);
  }
}

std::optional<std::shared_ptr<const Process>> ProcessTree::Get(
    const pid target) {
  absl::MutexLock lock(&mtx_);
  auto it = map_.find(target.pid);
  if (it == map_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::shared_ptr<const Process> ProcessTree::GetParent(const Process &p) {
  return p.parent_;
}

void ProcessTree::DebugDump(std::ostream &stream) {
  absl::MutexLock lock(&mtx_);
  DebugDumpLocked(stream, 0, 0);
}

void ProcessTree::DebugDumpLocked(std::ostream &stream, int depth, pid_t ppid)
    ABSL_SHARED_LOCKS_REQUIRED(mtx_) {
  for (auto &[_, process] : map_) {
    if ((ppid == 0 && !process->parent_) ||
        process->parent_->pid_.pid == ppid) {
      stream << std::string(2 * depth, ' ') << process->pid_.pid
             << process->program_->executable << std::endl;
      DebugDumpLocked(stream, depth + 1, process->pid_.pid);
    }
  }
}

}  // namespace process_tree
