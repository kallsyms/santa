#ifndef SANTA__SANTAD_PROCESSTREE_ENDPOINTSECURITYADAPTER_H
#define SANTA__SANTAD_PROCESSTREE_ENDPOINTSECURITYADAPTER_H

#include <EndpointSecurity/ESTypes.h>

#include "Source/santad/ProcessTree/tree.h"

namespace process_tree {
// Create a struct pid from the given audit token.
struct pid PidFromAuditToken(const audit_token_t &tok);

// Inform the tree of the ES event in msg.
// This is idempotent on the tree, so can be called from multiple places with
// the same msg.
void InformFromESEvent(int client, ProcessTree &tree, const es_message_t *msg);
}  // namespace process_tree

#endif
