#ifndef HA_MQTT_DISCOVERY_ENTITY
#define HA_MQTT_DISCOVERY_ENTITY

#include "HaMqttDiscovery.hpp"
#include "HaMqttDevice.hpp"
#include "MqttState.hpp"

#include <ArduinoJson.h>

namespace HaMqttDiscovery {

class HaMqttDevice;
class HaMqttEntity;

class HaMqttEntity {
  public:

    struct STATIC_CSTR_ARRAY {
      const char ** values;
      size_t count;      
    };

    HaMqttEntity() {
        this->device = NULL;
        this->type = HA_MQTT_INTEGRATION_TYPE::HA_MQTT_BINARY_SENSOR;
    }

    HaMqttEntity(const HA_MQTT_INTEGRATION_TYPE & type) {
        this->device = NULL;
        this->type = type;
    }

    HaMqttEntity(const HA_MQTT_INTEGRATION_TYPE & type, const char * name, const char * unique_id, const char * object_id) {
        this->device = NULL;
        this->type = type;
        this->name = name;
        this->unique_id = unique_id;
        this->object_id = object_id;
    }

    void setMqttAdaptor(MqttAdaptor * mqtt_adaptor) {
      this->mqtt_adaptor = mqtt_adaptor;
    }

    void setIntegrationType(const HA_MQTT_INTEGRATION_TYPE & type) {
        this->type = type;
    }

    const HA_MQTT_INTEGRATION_TYPE & getIntegrationType() const {
      return type;
    }

    void setName(const String & value) {
        name = value;
    }

    void setName(const char * value) {
        name = value;
    }

    const String & getName() const {
        return name;
    }

    void setUniqueId(const String & value) {
        unique_id = value;
    }

    void setUniqueId(const char * value) {
        unique_id = value;
    }

    const String & getUniqueId() const {
        return unique_id;
    }

    void setObjectId(const String & value) {
        object_id = value;
    }

    void setObjectId(const char * value) {
        object_id = value;
    }

    const String & getObjectId() const {
        return object_id;
    }

    void setCommandTopic(const String & value) {
        command_topic = value;
    }

    void setCommandTopic(const char * value) {
        command_topic = value;
    }

    const String & getCommandTopic() const {
        return command_topic;
    }

    void setStateTopic(const String & value) {
        state_topic = value;
    }

    void setStateTopic(const char * value) {
        state_topic = value;
    }

    const String & getStateTopic() const {
        return state_topic;
    }

    void setState(const char * value) {
        state.set(value);
    }

    void setState(const String & value) {
        state.set(value);
    }

    void setState(const uint8_t * value, size_t length) {
        state.set(value, length);
    }

    MqttState & getState() {
        return state;
    }

    const MqttState & getState() const {
        return state;
    }

    void setUniqueIdFromDeviceId() {
      if (device) {
        const String * first_device_identifier = device->getIdentifier(0);
        if (first_device_identifier) {

          // Is this entity already added to the device ?
          size_t entity_index = device->getEntityIndex(this);
          if (entity_index == (size_t)-1)
            entity_index = device->addEntity(this);
          
          // Build a new unique_id
          String new_unique_id = (*first_device_identifier) + "_" + toString(type) + String(entity_index);
          unique_id = new_unique_id;
        }
      }
    }

    void setDevice(HaMqttDevice * device) {
      this->device = device;

      // Can we generate an automatic unique_id for this entity ?
      if (unique_id.isEmpty()) {
        setUniqueIdFromDeviceId();
      }
    }

    HaMqttDevice * getDevice() const {
      return device;
    }

    void addKeyValue(const String & key, const String & value) {
      KEY_VALUE_PAIR pair;
      pair.key = key;
      pair.value = value;
      more_string_values.push_back(pair);
    }
    inline void addKeyValue(const char * key, const char * value) { addKeyValue(String(key), String(value)); }

    const char * getKeyValue(const String & key) const {
        for(size_t i=0; i<more_string_values.size(); i++) {
          const KEY_VALUE_PAIR & pair = more_string_values[i];
          if (pair.key == key)
            return pair.value.c_str();
        }
        return NULL;
    }
    const char * getKeyValue(const char * key) const { return getKeyValue(String(key)); }

    inline bool hasKeyValue(const String & key) const { return getKeyValue(key) != NULL; }
    inline bool hasKeyValue(const char * key) const { return getKeyValue(String(key)) != NULL; }

    void addStaticCStrArray(const String & key, const char ** values, size_t count) {
      STATIC_CSTR_ARRAY_ENTRY entry;
      entry.key = key;
      entry.the_array.values = values;
      entry.the_array.count = count;
      more_static_cstr_entries.push_back(entry);
    }

    const STATIC_CSTR_ARRAY * getStaticCStrArray(const String & key) const {
        for(size_t i=0; i<more_static_cstr_entries.size(); i++) {
          const STATIC_CSTR_ARRAY_ENTRY & entry = more_static_cstr_entries[i];
          if (entry.key == key)
            return &entry.the_array;
        }
        return NULL;
    }

    inline bool hasStaticCStr(const String & key) const { return getStaticCStrArray(key) != NULL; }
    inline bool hasStaticCStr(const char * key) const { return getStaticCStrArray(String(key)) != NULL; }



    void getDiscoveryTopic(String & topic) const {
      if (!unique_id.isEmpty()) {
        topic = ha_discovery_prefix + "/" + toString(type) + "/" + unique_id + "/config";
      } else {
        const String * device_identifier = NULL;
        if (device) {
          device_identifier = device->getIdentifier(0);
        }
        if (device_identifier)
          topic = ha_discovery_prefix + "/" + toString(type) + "/" + (*device_identifier) + "_" + unique_id + "/config";
        else
          topic = ha_discovery_prefix + "/" + toString(type) + "/" + unique_id + "/config";
      }
    }

    void getDiscoveryPayload(String & payload) const {
      DynamicJsonDocument doc(1024);

      // serialize key-value pairs
      for(size_t i=0; i<more_string_values.size(); i++) {
        const KEY_VALUE_PAIR & pair = more_string_values[i];
        doc[pair.key] = pair.value;
      };

      // serialize static cstr arrays
      for(size_t i=0; i<more_static_cstr_entries.size(); i++) {
        const STATIC_CSTR_ARRAY_ENTRY & entry = more_static_cstr_entries[i];
        JsonArray json_array = doc.createNestedArray(entry.key.c_str());
        for(size_t j=0; j<entry.the_array.count; j++) {
          json_array.add(entry.the_array.values[j]);
        }
      }

      // serialize base attributes
      if (!name.isEmpty())
        doc["name"] = name;
      if (!unique_id.isEmpty())
        doc["unique_id"] = unique_id;
      if (!object_id.isEmpty())
        doc["object_id"] = object_id;
      if (!command_topic.isEmpty())
        doc["command_topic"] = command_topic;
      if (!state_topic.isEmpty())
        doc["state_topic"] = state_topic;

      // serialize device, if any
      if (device) {
        doc["availability_topic"] = device->getAvailabilityTopic();
        doc["payload_available"] = ha_availability_online;
        doc["payload_not_available"] = ha_availability_offline;

        JsonObject json_device = doc.createNestedObject("device");
        device->serializeTo(json_device);
      }

      serializeJson(doc, payload);
    }

    bool publishMqttDiscovery() {
      if (mqtt_adaptor == NULL) return false;
      if (!mqtt_adaptor->connected()) return false;

      String topic;
      String payload;
      getDiscoveryTopic(topic);
      getDiscoveryPayload(payload);

      if (topic.isEmpty() || payload.isEmpty())
        return false;

      static const bool retained = true;
      bool result = mqtt_adaptor->publish(topic.c_str(), payload.c_str(), retained);

#     ifdef HA_MQTT_DISCOVERY_PRINT_FUNC
      if (result) {
        HA_MQTT_DISCOVERY_PRINT_FUNC("MQTT publish: topic=");
        HA_MQTT_DISCOVERY_PRINT_FUNC(topic);
        HA_MQTT_DISCOVERY_PRINT_FUNC("   payload=");
        HA_MQTT_DISCOVERY_PRINT_FUNC(payload);
        HA_MQTT_DISCOVERY_PRINT_FUNC("\n");     
      } else {
        HA_MQTT_DISCOVERY_PRINT_FUNC(error_message_prefix + "MQTT publish failure: topic=");
        HA_MQTT_DISCOVERY_PRINT_FUNC(topic);
        HA_MQTT_DISCOVERY_PRINT_FUNC("\n");     
      }
#     endif

      return result;
    }

    bool publishMqttState(bool retained = false) {
      if (mqtt_adaptor == NULL) return false;
      if (!mqtt_adaptor->connected()) return false;

      if (state_topic.isEmpty()) return true; // nothing configured

      if (!state.isDirty())
        return true; // nothing to do

      const char * topic = state_topic.c_str();

      bool result = false;
      bool is_binary_payload =  state.isBinary();
      bool is_string_payload = !is_string_payload;

      if (is_string_payload) {
        const char * payload = state.getStringValue().c_str();
        result = mqtt_adaptor->publish(topic, payload, retained);

#       ifdef HA_MQTT_DISCOVERY_PRINT_FUNC
        if (result) {
          HA_MQTT_DISCOVERY_PRINT_FUNC("MQTT publish: topic=");
          HA_MQTT_DISCOVERY_PRINT_FUNC(topic);
          HA_MQTT_DISCOVERY_PRINT_FUNC("   payload=");
          HA_MQTT_DISCOVERY_PRINT_FUNC(payload);
          HA_MQTT_DISCOVERY_PRINT_FUNC("\n");     
        } else {
          HA_MQTT_DISCOVERY_PRINT_FUNC(error_message_prefix + "MQTT publish failure: topic=");
          HA_MQTT_DISCOVERY_PRINT_FUNC(topic);
          HA_MQTT_DISCOVERY_PRINT_FUNC("\n");     
        }
#       endif
      } else {
        // binary payload
        const MqttState::Buffer & bin_payload = state.getBinaryValue();
        result = mqtt_adaptor->publish(topic, bin_payload.buffer, bin_payload.size, retained);

#       ifdef HA_MQTT_DISCOVERY_PRINT_FUNC
        if (result) {
          HA_MQTT_DISCOVERY_PRINT_FUNC("MQTT publish: topic=");
          HA_MQTT_DISCOVERY_PRINT_FUNC(topic);
          HA_MQTT_DISCOVERY_PRINT_FUNC("   (binary payload) size=");
          HA_MQTT_DISCOVERY_PRINT_FUNC(bin_payload.size);
          HA_MQTT_DISCOVERY_PRINT_FUNC("\n");     
        } else {
          HA_MQTT_DISCOVERY_PRINT_FUNC(error_message_prefix + "MQTT publish failure: topic=");
          HA_MQTT_DISCOVERY_PRINT_FUNC(topic);
          HA_MQTT_DISCOVERY_PRINT_FUNC("\n");     
        }
#       endif
      }

      if (result)
        state.clear();

      return result;
    }

    bool subscribe() {
      if (mqtt_adaptor == NULL) return false;
      if (!mqtt_adaptor->connected()) return false;

      if (command_topic.isEmpty()) return true; // nothing configured

      const char * topic = command_topic.c_str();
      bool result = mqtt_adaptor->subscribe(topic);

#     ifdef HA_MQTT_DISCOVERY_PRINT_FUNC
      if (result) {
        HA_MQTT_DISCOVERY_PRINT_FUNC("MQTT subscribe: topic=");
        HA_MQTT_DISCOVERY_PRINT_FUNC(topic);
        HA_MQTT_DISCOVERY_PRINT_FUNC("\n");     
      } else {
        HA_MQTT_DISCOVERY_PRINT_FUNC("MQTT subscribe failure: topic=");
        HA_MQTT_DISCOVERY_PRINT_FUNC(topic);
        HA_MQTT_DISCOVERY_PRINT_FUNC("\n");     
      }
#     endif

      return result;
    }

  private:
    MqttAdaptor * mqtt_adaptor;
    HaMqttDevice * device;
    MqttState state;

    HA_MQTT_INTEGRATION_TYPE type;
    String name;
    String unique_id;
    String object_id;
    String command_topic;
    String state_topic;

    struct KEY_VALUE_PAIR {
      String key;
      String value;
    };
    typedef std::vector<KEY_VALUE_PAIR> KeyValuePairVector;
    KeyValuePairVector more_string_values;

    struct STATIC_CSTR_ARRAY_ENTRY {
      String key;
      STATIC_CSTR_ARRAY the_array;
    };
    typedef std::vector<STATIC_CSTR_ARRAY_ENTRY> StaticCStrArrayVector;
    StaticCStrArrayVector more_static_cstr_entries;
    
};

}; // namespace HaMqttDiscovery

#endif // HA_MQTT_DISCOVERY_ENTITY
