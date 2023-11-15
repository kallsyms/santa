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
#ifndef SANTA__SANTAD_PROCESSTREE_ANNOTATIONS_CURL_SH_H
#define SANTA__SANTAD_PROCESSTREE_ANNOTATIONS_CURL_SH_H

#include <optional>

#include "Source/santad/ProcessTree/Annotations/base.h"
#include "Source/santad/ProcessTree/process_tree.pb.h"

namespace process_tree {

enum class CurlShState {
  kNone = 0,
  kSeenCurl = 1,
  kSeenBoth = 2,
};

class CurlShAnnotator : public Annotator {
 public:
  CurlShAnnotator() : state_(CurlShState::kNone){};
  explicit CurlShAnnotator(CurlShState state) : state_(state){};

  void AnnotateFork(ProcessTree &tree, const Process &parent,
                    const Process &child) override;
  void AnnotateExec(ProcessTree &tree, const Process &orig_process,
                    const Process &new_process) override;

  std::optional<pb::Annotations> Proto() const override;

 private:
  CurlShState state_;
};

}  // namespace process_tree

#endif
