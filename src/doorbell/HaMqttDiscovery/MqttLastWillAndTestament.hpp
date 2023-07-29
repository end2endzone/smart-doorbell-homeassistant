#pragma once

#include "HaMqttDiscovery.hpp"

namespace HaMqttDiscovery {

class MqttLastWillAndTestament {
  public:
    String topic;
    String payload;
    uint8_t qos;
    bool retain;

  public:
    MqttLastWillAndTestament() {
      clear();
    }

    void clear() {
      topic.clear();
      payload.clear();
      qos = 0;
      retain = false;
    }

    bool isValid() const {
      if (!topic.isEmpty() && !payload.isEmpty()) {
        return true;
      }
      return false;
    }

};

}; // namespace HaMqttDiscovery
