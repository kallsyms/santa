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

#ifndef SANTA__SANTAD__EVENTPROVIDERS_ENDPOINTSECURITY_MESSAGE_H
#define SANTA__SANTAD__EVENTPROVIDERS_ENDPOINTSECURITY_MESSAGE_H

#include <EndpointSecurity/EndpointSecurity.h>

#include <memory>
#include <string>

namespace santa::santad::event_providers::endpoint_security {

class EndpointSecurityAPI;

class Message {
 public:
  Message(std::shared_ptr<EndpointSecurityAPI> esapi,
          const es_message_t* es_msg);
  ~Message();

  Message(Message&& other);
  // Note: Safe to implement this, just not currently needed so left deleted.
  Message& operator=(Message&& rhs) = delete;

  Message(const Message& other);
  Message& operator=(const Message& other) = delete;

  // Operators to access underlying es_message_t
  const es_message_t* operator->() const { return es_msg_; }
  const es_message_t& operator*() const { return *es_msg_; }

  template<typename Visitor>
  decltype(auto) variant(Visitor&& visitor) const;

  std::string ParentProcessName() const;

 protected:
  std::shared_ptr<EndpointSecurityAPI> esapi_;
  const es_message_t* es_msg_;

  std::string GetProcessName(pid_t pid) const;
};

class ForkMessage : public Message {
 public:
  ForkMessage(const Message& other);
  const es_event_fork_t* operator->() const { return &es_msg_->event.fork; }
};

class ExecMessage : public Message {
 public:
  ExecMessage(const Message& other);
  const es_event_exec_t* operator->() const { return &es_msg_->event.exec; }
  uint32_t ArgCount() const;
  es_string_token_t Arg(uint32_t index) const;
};

class ExitMessage : public Message {
 public:
  ExitMessage(const Message& other);
  const es_event_exit_t* operator->() const { return &es_msg_->event.exit; }
};

using MessageVariant = std::variant<ForkMessage, ExecMessage, ExitMessage, Message>;

template<typename Visitor>
decltype(auto) Message::variant(Visitor&& visitor) const {
  MessageVariant v = Message(*this);
  switch (es_msg_->event_type) {
    case ES_EVENT_TYPE_NOTIFY_FORK:
      v.emplace<ForkMessage>(*this);
      break;
    case ES_EVENT_TYPE_AUTH_EXEC:
    case ES_EVENT_TYPE_NOTIFY_EXEC:
      v.emplace<ExecMessage>(*this);
      break;
    case ES_EVENT_TYPE_NOTIFY_EXIT:
      v.emplace<ExitMessage>(*this);
    default:
      break;
  }
  return std::visit(visitor, v);
}

}  // namespace santa::santad::event_providers::endpoint_security

#endif
