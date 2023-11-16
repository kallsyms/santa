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
#import <Foundation/Foundation.h>
#import <XCTest/XCTest.h>

#include <bsm/libbsm.h>

#include <memory>
#include <string>

#include "Source/santad/ProcessTree/Annotations/base.h"
#include "Source/santad/ProcessTree/process.h"
#include "Source/santad/ProcessTree/tree_test_helpers.h"
#include "absl/synchronization/mutex.h"

namespace process_tree {

static constexpr std::string_view kAnnotatedExecutable = "/usr/bin/login";

class TestAnnotator : public Annotator {
 public:
  TestAnnotator() {}
  void AnnotateFork(ProcessTree &tree, const Process &parent,
                    const Process &child) override;
  void AnnotateExec(ProcessTree &tree, const Process &orig_process,
                    const Process &new_process) override;
  std::optional<pb::Annotations> Proto() const override;
};

void TestAnnotator::AnnotateFork(ProcessTree &tree, const Process &parent,
                                       const Process &child) {
  // "Base case". Propagate existing annotations down to descendants.
  if (auto annotation = tree.GetAnnotation<TestAnnotator>(parent)) {
    tree.AnnotateProcess(child, std::move(*annotation));
  }
}

void TestAnnotator::AnnotateExec(ProcessTree &tree,
                                       const Process &orig_process,
                                       const Process &new_process) {
  if (auto annotation = tree.GetAnnotation<TestAnnotator>(orig_process)) {
    tree.AnnotateProcess(new_process, std::move(*annotation));
    return;
  }

  if (new_process.program_->executable == kAnnotatedExecutable) {
    tree.AnnotateProcess(
        new_process,
        std::make_shared<TestAnnotator>());
  }
}

std::optional<pb::Annotations> TestAnnotator::Proto() const {
  return std::nullopt;
}
}  // namespace process_tree

using namespace process_tree;

@interface ProcessTreeTest : XCTestCase
@property std::shared_ptr<ProcessTreeTestPeer> tree;
@property int client_id;
@property std::shared_ptr<const Process> init_proc;
@end

@implementation ProcessTreeTest

- (void)setUp {
  self.tree = std::make_shared<ProcessTreeTestPeer>();
  self.client_id = self.tree->RegisterClient();
  self.init_proc = self.tree->InsertInit();
}

- (void)testSimpleOps {
  uint64_t event_id = 1;
  // PID 1.1: fork() -> PID 2.2
  const struct pid child_pid = {.pid = 2, .pidversion = 2};
  self.tree->Step(self.client_id, event_id);
  self.tree->HandleFork(event_id++, *self.init_proc, child_pid);

  auto child_opt = self.tree->Get(child_pid);
  XCTAssertTrue(child_opt.has_value());
  std::shared_ptr<const Process> child = *child_opt;
  XCTAssertEqual(child->pid_, child_pid);
  XCTAssertEqual(child->program_, self.init_proc->program_);
  XCTAssertEqual(child->effective_cred_, self.init_proc->effective_cred_);
  XCTAssertEqual(self.tree->GetParent(*child), self.init_proc);

  // PID 2.2: exec("/bin/bash") -> PID 2.3
  const struct pid child_exec_pid = {.pid = 2, .pidversion = 3};
  const struct program child_exec_prog = {.executable = "/bin/bash", .arguments = {"/bin/bash", "-i"}};
  self.tree->Step(self.client_id, event_id);
  self.tree->HandleExec(event_id++, *child, child_exec_pid, child_exec_prog, child->effective_cred_);

  child_opt = self.tree->Get(child_exec_pid);
  XCTAssertTrue(child_opt.has_value());
  child = *child_opt;
  XCTAssertEqual(child->pid_, child_exec_pid);
  XCTAssertEqual(*child->program_, child_exec_prog);
  // Assert we specifically re-use the same cred struct pointer.
  XCTAssertEqual(child->effective_cred_, self.init_proc->effective_cred_);
}

// We can't test the full backfill process, as retrieving information on
// processes (with task_name_for_pid) requires privileges.
// Test what we can by LoadPID'ing ourselves.
- (void)testLoadPID {
  auto proc = LoadPID(getpid()).value();

  audit_token_t self_tok;
  mach_msg_type_number_t count = TASK_AUDIT_TOKEN_COUNT;
  XCTAssertEqual(task_info(mach_task_self(), TASK_AUDIT_TOKEN, (task_info_t)&self_tok, &count), KERN_SUCCESS);

  XCTAssertEqual(proc.pid_.pid, audit_token_to_pid(self_tok));
  XCTAssertEqual(proc.pid_.pidversion, audit_token_to_pidversion(self_tok));

  XCTAssertEqual(proc.effective_cred_.uid, geteuid());
  XCTAssertEqual(proc.effective_cred_.gid, getegid());

  [[[NSProcessInfo processInfo] arguments]
    enumerateObjectsUsingBlock:^(NSString *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop){
    XCTAssertEqualObjects(@(proc.program_->arguments[idx].c_str()), obj);
    if (idx == 0) {
      XCTAssertEqualObjects(@(proc.program_->executable.c_str()), obj);
    }
  }];
}

- (void)testAnnotation {
  uint64_t event_id = 1;
  const struct cred cred = {.uid = 0, .gid = 0};
  self.tree->RegisterAnnotator(std::make_unique<TestAnnotator>());

  // PID 1.1: fork() -> PID 2.2
  const struct pid login_pid = {.pid = 2, .pidversion = 2};
  self.tree->Step(self.client_id, event_id);
  self.tree->HandleFork(event_id++, *self.init_proc, login_pid);

  // PID 2.2: exec("/usr/bin/login") -> PID 2.3
  const struct pid login_exec_pid = {.pid = 2, .pidversion = 3};
  const struct program login_prog = {.executable = std::string(kAnnotatedExecutable), .arguments = {}};
  auto login = *self.tree->Get(login_pid);
  self.tree->Step(self.client_id, event_id);
  self.tree->HandleExec(event_id++, *login, login_exec_pid, login_prog, cred);

  // Ensure we have an annotation on login itself...
  login = *self.tree->Get(login_exec_pid);
  auto annotation = self.tree->GetAnnotation<TestAnnotator>(*login);
  XCTAssertTrue(annotation.has_value());

  // PID 2.3: fork() -> PID 3.3
  const struct pid shell_pid = {.pid = 3, .pidversion = 3};
  self.tree->Step(self.client_id, event_id);
  self.tree->HandleFork(event_id++, *login, shell_pid);
  // PID 3.3: exec("/bin/zsh") -> PID 3.4
  const struct pid shell_exec_pid = {.pid = 3, .pidversion = 4};
  const struct program shell_prog = {.executable = "/bin/zsh", .arguments = {}};
  auto shell = *self.tree->Get(shell_pid);
  self.tree->Step(self.client_id, event_id);
  self.tree->HandleExec(event_id++, *shell, shell_exec_pid, shell_prog, cred);

  // ... and also ensure we have an annotation on the descendant zsh.
  shell = *self.tree->Get(shell_exec_pid);
  annotation = self.tree->GetAnnotation<TestAnnotator>(*shell);
  XCTAssertTrue(annotation.has_value());
}

- (void)testCleanup {
  uint64_t event_id = 1;
  const struct pid child_pid = {.pid = 2, .pidversion = 2};
  {
    self.tree->Step(self.client_id, event_id);
    self.tree->HandleFork(event_id++, *self.init_proc, child_pid);
    auto child = *self.tree->Get(child_pid);
    self.tree->Step(self.client_id, event_id);
    self.tree->HandleExit(event_id++, *child);
  }

  // We should still be able to get a handle to child...
  {
    auto child = self.tree->Get(child_pid);
    XCTAssertTrue(child.has_value());
  }

  // ... until we step far enough into the future (32 event).
  for (int i = 0; i < 32; i++) {
    self.tree->Step(self.client_id, event_id++);
  }

  // Now when we try processing the next event, it should have fallen out of the tree.
  self.tree->Step(self.client_id, event_id++);
  {
    auto child = self.tree->Get(child_pid);
    XCTAssertFalse(child.has_value());
  }
}

- (void)testRefcountCleanup {
  uint64_t event_id = 1;
  const struct pid child_pid = {.pid = 2, .pidversion = 2};
  {
    self.tree->Step(self.client_id, event_id);
    self.tree->HandleFork(event_id++, *self.init_proc, child_pid);
    auto child = *self.tree->Get(child_pid);
    self.tree->Step(self.client_id, event_id);
    self.tree->HandleExit(event_id++, *child);
  }

  {
    auto child = self.tree->Get(child_pid);
    XCTAssertTrue(child.has_value());
    self.tree->RetainProcess((*child)->pid_);
  }

  // Even if we step far into the future, we should still be able to lookup
  // the child.
  for (int i = 0; i < 1000; i++) {
    self.tree->Step(self.client_id, event_id++);
    auto child = self.tree->Get(child_pid);
    XCTAssertTrue(child.has_value());
  }

  // But when released...
  {
    auto child = self.tree->Get(child_pid);
    XCTAssertTrue(child.has_value());
    self.tree->ReleaseProcess((*child)->pid_);
  }

  // ... it should immediately be removed.
  {
    auto child = self.tree->Get(child_pid);
    XCTAssertFalse(child.has_value());
  }
}

@end

