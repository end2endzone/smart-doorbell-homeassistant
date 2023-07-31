#ifndef HA_MQTT_DISCOVERY_MQTT_STATE
#define HA_MQTT_DISCOVERY_MQTT_STATE

#include "HaMqttDiscovery.hpp"

namespace HaMqttDiscovery {

class MqttState {
  public:

    struct Buffer {
      uint8_t * buffer;
      size_t size;
    };

    MqttState() {
      memset(&bin_value, 0, sizeof(bin_value));
      is_binary = false;
      is_dirty = false;
    }
    virtual ~MqttState() {
      clear();
    }
  private:
    // Disable copy ctor
    MqttState(const MqttState & copy) {}
  public:

    virtual void clear() {
      if (bin_value.buffer) {
        free(bin_value.buffer);
        memset(&bin_value, 0, sizeof(bin_value));
      } else {
        str_value.clear();
      }
      is_binary = false;
      is_dirty = false;
    }

    virtual bool isBinary() {
      return is_binary;
    }

    virtual bool isDirty() {
      return is_dirty;
    }

    virtual void setDirty(bool value = true) {
      is_dirty = value;
    }

    virtual void set(const char * value) {
      if (is_binary)
        clear();
      is_binary = false;
      is_dirty = true;
      str_value = value;
    }
    
    virtual void set(const String & value) {
      if (is_binary)
        clear();
      is_binary = false;
      is_dirty = true;
      str_value = value;
    }
    
    virtual void set(const uint8_t * value, size_t length) {
      if (is_binary)
        clear();
      uint8_t * tmp = (uint8_t *)malloc(length);
      if (!tmp)
        return;
      memcpy(tmp, value, length);
      bin_value.buffer = tmp;
      bin_value.size = length;

      is_binary = true;
      is_dirty = true;
    }

    const String & getStringValue() const {
      return str_value;
    }

    const Buffer & getBinaryValue() const {
      return bin_value;
    }

  private:
    bool is_binary;
    bool is_dirty;

    String str_value;
    Buffer bin_value;
};

}; // namespace HaMqttDiscovery

#endif // HA_MQTT_DISCOVERY_MQTT_STATE
