#ifndef HA_MQTT_DISCOVERY_MQTT_ADAPTOR
#define HA_MQTT_DISCOVERY_MQTT_ADAPTOR

#include "HaMqttDiscovery.hpp"

namespace HaMqttDiscovery {

class MqttAdaptor {
  public:
    MqttAdaptor() {}
    virtual ~MqttAdaptor() {}

    virtual bool connected();

    virtual bool publish(const char* topic, const char* payload);
    virtual bool publish(const char* topic, const char* payload, bool retained);
    virtual bool publish(const char* topic, const uint8_t* payload, size_t length);
    virtual bool publish(const char* topic, const uint8_t* payload, size_t length, bool retained);

    virtual bool subscribe(const char* topic);
    virtual bool unsubscribe(const char* topic);

};

}; // namespace HaMqttDiscovery

#endif // HA_MQTT_DISCOVERY_MQTT_ADAPTOR
