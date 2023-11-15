#include "Source/santad/ProcessTree/Annotations/originator.h"

#include <optional>

#include "Source/santad/ProcessTree/process_tree.pb.h"
#include "Source/santad/ProcessTree/tree.h"

namespace process_tree {
void OriginatorAnnotator::AnnotateFork(ProcessTree &tree, const Process &parent,
                                       const Process &child) {
  // "Base case". Propagate existing annotations down to descendants.
  if (auto annotation = tree.GetAnnotation<OriginatorAnnotator>(parent)) {
    tree.AnnotateProcess(child, std::move(*annotation));
    return;
  }
}

void OriginatorAnnotator::AnnotateExec(ProcessTree &tree,
                                       const Process &orig_process,
                                       const Process &new_process) {
  if (auto annotation = tree.GetAnnotation<OriginatorAnnotator>(orig_process)) {
    tree.AnnotateProcess(new_process, std::move(*annotation));
    return;
  }

  if (new_process.program_->executable == "/usr/bin/login") {
    tree.AnnotateProcess(
        new_process,
        std::make_shared<OriginatorAnnotator>(
            pb::Annotations::Originator::Annotations_Originator_LOGIN));
  } else if (new_process.program_->executable == "/usr/sbin/cron") {
    tree.AnnotateProcess(
        new_process,
        std::make_shared<OriginatorAnnotator>(
            pb::Annotations::Originator::Annotations_Originator_CRON));
  }
}

std::optional<pb::Annotations> OriginatorAnnotator::Proto() const {
  auto annotation = pb::Annotations();
  annotation.set_ancestor(originator_);
  return annotation;
}
}  // namespace process_tree
