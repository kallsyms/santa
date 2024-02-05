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
#ifndef SANTA__SANTAD_PROCESSTREE_ENDPOINTSECURITYADAPTER_H
#define SANTA__SANTAD_PROCESSTREE_ENDPOINTSECURITYADAPTER_H

#include <EndpointSecurity/ESTypes.h>

#include "Source/santad/ProcessTree/tree.h"

namespace santa::santad::process_tree {

// Inform the tree of the ES event in msg.
// This is idempotent on the tree, so can be called from multiple places with
// the same msg.
void InformFromESEvent(int client, ProcessTree &tree, const es_message_t *msg);

}  // namespace santa::santad::process_tree

#endif