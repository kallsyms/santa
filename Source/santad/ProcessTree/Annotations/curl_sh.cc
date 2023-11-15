/// Copyright 2023 Google LLC
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     https://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
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

std::optional<pb::Annotations> CurlShAnnotator::Proto() const {
  if (state_ == CurlShState::kSeenBoth) {
    auto annotation = pb::Annotations();
    annotation.set_curl_sh(true);
    return annotation;
  }
  return std::nullopt;
}
}  // namespace process_tree
