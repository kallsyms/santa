#ifndef SANTA__SANTAD_PROCESSTREE_ANNOTATIONS_ORIGINATOR_H
#define SANTA__SANTAD_PROCESSTREE_ANNOTATIONS_ORIGINATOR_H

#include <optional>

#include "Source/santad/ProcessTree/Annotations/base.h"

namespace process_tree {

class OriginatorAnnotator : public Annotator {
 public:
  OriginatorAnnotator()
      : originator_(pb::Annotation::Originator::UNSPECIFIED){};
  explicit OriginatorAnnotator(pb::Annotation::Originator originator)
      : originator_(originator){};

  void AnnotateFork(ProcessTree &tree, const Process &parent,
                    const Process &child);
  void AnnotateExec(ProcessTree &tree, const Process &orig_process,
                    const Process &new_process);

  std::optional<pb::Annotation> Proto();

 private:
  pb::Annotation::Originator originator_;
}

}  // namespace process_tree

#endif
