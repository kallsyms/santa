#include "Source/santad/ProcessTree/Annotations/originator.h"

#include <optional>

#include "Source/santad/ProcessTree/process_tree.pb.h"

namespace process_tree {
void OriginatorAnnotator::AnnotateFork(ProcessTree &tree, const Process &parent,
                                       const Process &child) {
  // "Base case". Propagate existing annotations down to descendants.
  if (auto annotation =
          tree.GetAnnotation(parent, typeid(OriginatorAnnotator))) {
    tree.SetAnnotation(child, annotation);
    return;
  }

  if (parent.program_->executable == "/sbin/launchd") {
    tree.AnnotateProcess(
        child,
        OriginatorAnnotator(
            pb::Annotations::Originator::Annotations_Originator_LAUNCHD));
  } else if (parent.program_->executable == "/usr/sbin/cron") {
    tree.AnnotateProcess(
        child, OriginatorAnnotator(
                   pb::Annotations::Originator::Annotations_Originator_CRON));
  }
}

void OriginatorAnnotator::AnnotateExec(ProcessTree &tree,
                                       const Process &orig_process,
                                       const Process &new_process) {
  if (auto annotation =
          tree.GetAnnotation(orig_process, typeid(OriginatorAnnotator));
      annotation) {
    tree.AnnotateProcess(new_process, *annotation);
  }
}

std::optional<pb::Annotations> OriginatorAnnotator::Proto() {
  auto annotation = pb::Annotations();
  annotation->set_originator(originator_);
  return annotation;
}
}  // namespace process_tree
