#include "Source/santad/ProcessTree/Annotations/curl_sh.h"

#include <optional>

#include "Source/santad/ProcessTree/process_tree.pb.h"
#include "Source/santad/ProcessTree/tree.h"

namespace process_tree {
void CurlShAnnotator::AnnotateFork(ProcessTree &tree, const Process &parent,
                                   const Process &child) {}
void CurlShAnnotator::AnnotateExec(ProcessTree &tree,
                                   const Process &orig_process,
                                   const Process &new_process) {
  if (new_process.program_->executable == "/usr/bin/curl") {
    tree.AnnotateProcess(
        *tree.GetParent(new_process),
        std::make_shared<CurlShAnnotator>(CurlShState::kSeenCurl));
    return;
  }

  if (new_process.program_->executable == "/bin/sh") {
    auto parent = tree.GetParent(new_process);
    auto parent_state = tree.GetAnnotation<CurlShAnnotator>(*parent);
    if (parent_state && (*parent_state)->state_ == CurlShState::kSeenCurl) {
      tree.AnnotateProcess(
          *parent, std::make_shared<CurlShAnnotator>(CurlShState::kSeenBoth));
    }
  }
}

std::optional<pb::Annotations> CurlShAnnotator::Proto() {
  if (state_ == CurlShState::kSeenBoth) {
    auto annotation = pb::Annotations();
    annotation.set_curl_sh(true);
    return annotation;
  }
  return std::nullopt;
}
}  // namespace process_tree
