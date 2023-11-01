#include "Source/santad/ProcessTree/Annotations/curl_sh.h"

#include <optional>

#include "Source/santad/ProcessTree/tree.h"

namespace process_tree {
void CurlShAnnotator::AnnotateExec(ProcessTree &tree,
                                   const Process &orig_process,
                                   const Process &new_process) {
  if (new_process.program_.executable == "/usr/bin/curl") {
    tree.AnnotateProcess(tree.GetParent(new_process),
                         CurlShAnnotator(CurlShState::kSeenCurl));
    return;
  }

  if (new_process.program_.executable == "/bin/sh") {
    auto parent = tree.GetParent(new_process);
    auto parent_state = tree.GetAnnotation(parent, typeid(CurlShAnnotator));
    if (parent_state.state_ == CurlShState::kSeenCurl) {
      tree.AnnotateProcess(parent, CurlShAnnotator(CurlShState::kSeenBoth));
    }
  }
}

std::optional<pb::Annotation> CurlShAnnotator::Proto() {
  if (state_ == CurlShState::kSeenBoth) {
    auto annotation = pb::Annotation();
    annotation->set_curl_sh(true);
    return annotation;
  }
  return std::nullopt;
}
}  // namespace process_tree
