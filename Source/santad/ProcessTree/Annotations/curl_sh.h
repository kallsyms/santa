#ifndef SANTA__SANTAD_PROCESSTREE_ANNOTATIONS_CURL_SH_H
#define SANTA__SANTAD_PROCESSTREE_ANNOTATIONS_CURL_SH_H

#include <optional>

#include "Source/santad/ProcessTree/Annotations/base.h"

namespace process_tree {

enum class CurlShState {
  kNone = 0,
  kSeenCurl = 1,
  kSeenBoth = 2,
}

class CurlShAnnotator : public Annotator {
 public:
  CurlShAnnotator() : state_(CurlShState::kNone){};
  explicit CurlShAnnotator(CurlShState state) : state_(state){};

  void AnnotateFork(ProcessTree &tree, const Process &parent,
                    const Process &child);
  void AnnotateExec(ProcessTree &tree, const Process &orig_process,
                    const Process &new_process);

  std::optional<pb::Annotation> Proto();

 private:
  CurlShState state_;
}

}  // namespace process_tree

#endif
