#ifndef SANTA__SANTAD_PROCESSTREE_ANNOTATIONS_ORIGINATOR_H
#define SANTA__SANTAD_PROCESSTREE_ANNOTATIONS_ORIGINATOR_H

#include <optional>

#include "Source/santad/ProcessTree/Annotations/base.h"
#include "Source/santad/ProcessTree/process_tree.pb.h"

namespace process_tree {

class OriginatorAnnotator : public Annotator {
 public:
  OriginatorAnnotator()
      : originator_(
            pb::Annotations::Originator::Annotations_Originator_UNSPECIFIED){};
  explicit OriginatorAnnotator(pb::Annotations::Originator originator)
      : originator_(originator){};

  void AnnotateFork(ProcessTree &tree, const Process &parent,
                    const Process &child) override;
  void AnnotateExec(ProcessTree &tree, const Process &orig_process,
                    const Process &new_process) override;

  std::optional<pb::Annotations> Proto() override;

 private:
  pb::Annotations::Originator originator_;
};

}  // namespace process_tree

#endif
