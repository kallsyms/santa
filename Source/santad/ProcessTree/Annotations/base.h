#ifndef SANTA__SANTAD_PROCESSTREE_ANNOTATIONS_BASE_H
#define SANTA__SANTAD_PROCESSTREE_ANNOTATIONS_BASE_H

#include <optional>

#include "Source/santad/ProcessTree/process.h"

namespace process_tree {

class Annotator {
 public:
  virtual void AnnotateFork(ProcessTree &tree, const Process &parent,
                            const Process &child);
  virtual void AnnotateExec(ProcessTree &tree, const Process &orig_process,
                            const Process &new_process);
  virtual std::optional<pb::Annotation> Proto();
}

}  // namespace process_tree

#endif
