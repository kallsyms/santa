/// Copyright 2022 Google Inc. All rights reserved.
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///    http://www.apache.org/licenses/LICENSE-2.0
///
///    Unless required by applicable law or agreed to in writing, software
///    distributed under the License is distributed on an "AS IS" BASIS,
///    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
///    See the License for the specific language governing permissions and
///    limitations under the License.

#import "Source/santad/EventProviders/SNTEndpointSecurityTreeAwareClient.h"

#include <EndpointSecurity/EndpointSecurity.h>

#include "Source/santad/EventProviders/EndpointSecurity/EndpointSecurityAPI.h"
#include "Source/santad/EventProviders/EndpointSecurity/Message.h"
#include "Source/santad/ProcessTree/tree.h"
#include "Source/santad/ProcessTree/EndpointSecurityAdapter.h"
#include "Source/santad/Metrics.h"

using santa::santad::EventDisposition;
using santa::santad::Metrics;
using santa::santad::Processor;
using santa::santad::event_providers::endpoint_security::EndpointSecurityAPI;
using santa::santad::event_providers::endpoint_security::Message;

@implementation SNTEndpointSecurityTreeAwareClient {
  std::vector<bool> _addedEvents;
}

- (instancetype)initWithESAPI:(std::shared_ptr<EndpointSecurityAPI>)esApi
                      metrics:(std::shared_ptr<Metrics>)metrics
                    processor:(Processor)processor
  processTree:(std::shared_ptr<process_tree::ProcessTree>)processTree {
  self = [super initWithESAPI:std::move(esApi) metrics:std::move(metrics) processor:processor];
  if (self) {
    _processTree = std::move(processTree);
    _addedEvents.resize(ES_EVENT_TYPE_LAST, false);
  }
  return self;
}

- (bool)subscribe:(const std::set<es_event_type_t> &)events {
  std::set<es_event_type_t> eventsWithLifecycle = events;
  if (events.find(ES_EVENT_TYPE_NOTIFY_FORK) == events.end()) {
    eventsWithLifecycle.insert(ES_EVENT_TYPE_NOTIFY_FORK);
    _addedEvents[ES_EVENT_TYPE_NOTIFY_FORK] = true;
  }
  if (events.find(ES_EVENT_TYPE_NOTIFY_EXEC) == events.end()) {
    eventsWithLifecycle.insert(ES_EVENT_TYPE_NOTIFY_EXEC);
    _addedEvents[ES_EVENT_TYPE_NOTIFY_EXEC] = true;
  }
  if (events.find(ES_EVENT_TYPE_NOTIFY_EXIT) == events.end()) {
    eventsWithLifecycle.insert(ES_EVENT_TYPE_NOTIFY_EXIT);
    _addedEvents[ES_EVENT_TYPE_NOTIFY_EXIT] = true;
  }

  return [super subscribe:eventsWithLifecycle];
}

- (bool)handleContextMessage:(Message &)esMsg {
  switch (esMsg->event_type) {
    case ES_EVENT_TYPE_NOTIFY_FORK:
    case ES_EVENT_TYPE_NOTIFY_EXEC:
    case ES_EVENT_TYPE_NOTIFY_EXIT:
      process_tree::InformFromESEvent(*_processTree, &*esMsg);
      return _addedEvents[esMsg->event_type];

    default:
      return false;
  }
}

@end
