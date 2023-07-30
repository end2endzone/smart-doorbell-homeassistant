#ifndef HA_MQTT_DISCOVERY_MQTT_ADAPTOR_PUBSUBCLIENT
#define HA_MQTT_DISCOVERY_MQTT_ADAPTOR_PUBSUBCLIENT

#include "HaMqttDiscovery.hpp"
#include "MqttAdaptor.hpp"
#include <PubSubClient.h>   // https://www.arduino.cc/reference/en/libraries/pubsubclient/

namespace HaMqttDiscovery {

class MqttAdaptorPubSubClient : public virtual MqttState {
  private:
    PubSubClient * client;
  public:
    MqttAdaptorPubSubClient() {
      client = NULL;
    }
    MqttAdaptorPubSubClient(PubSubClient * client) {
      client = NULL;
      setPubSubClient(client);
    }
    virtual ~MqttAdaptorPubSubClient() {}

    void setPubSubClient(PubSubClient * client) {
      this->client = client;
    }

    virtual bool connected() {
      if (client == NULL) return false;
      return client->connected();
    }

    virtual bool publish(const char* topic, const char* payload) {
      if (client == NULL) return false;
      return client->publish(topic, payload);
    }

    virtual bool publish(const char* topic, const char* payload, bool retained) {
      if (client == NULL) return false;
      return client->publish(topic, payload, retained);
    }

    virtual bool publish(const char* topic, const uint8_t* payload, size_t length) {
      if (client == NULL) return false;
      return client->publish(topic, payload, length);
    }

    virtual bool publish(const char* topic, const uint8_t* payload, size_t length, bool retained) {
      if (client == NULL) return false;
      return client->publish(topic, payload, length, retained);
    }

    virtual bool subscribe(const char* topic) {
      if (client == NULL) return false;
      return client->subscribe(topic);
    }

    virtual bool unsubscribe(const char* topic) {
      if (client == NULL) return false;
      return client->unsubscribe(topic);
    }

};

}; // namespace HaMqttDiscovery

#endif // HA_MQTT_DISCOVERY_MQTT_ADAPTOR_PUBSUBCLIENT
