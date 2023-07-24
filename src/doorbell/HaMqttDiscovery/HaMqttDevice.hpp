#pragma once

#include <vector>
#include <ArduinoJson.h>

class HaMqttDevice;
class HaMqttEntity;

class HaMqttDevice {
  public:
    typedef std::vector<String> StringVector;
    typedef std::vector<HaMqttEntity*> EntityPtrVector;

    HaMqttDevice() {
    }

    HaMqttDevice(const char * identifier, const char * name_) {
        identifiers.push_back(identifier);
        name = name_;
    }

    HaMqttDevice(const String& identifier, const String & name_) {
        identifiers.push_back(identifier);
        name = name_;
    }

    HaMqttDevice(const char * identifier, const char * name_, const char * manufacturer_, const char * model_) {
        identifiers.push_back(identifier);
        name = name_;
        manufacturer = manufacturer_;
        model = model_;
    }

    HaMqttDevice(const String& identifier, const String& name_, const String& manufacturer_, const String& model_) {
        identifiers.push_back(identifier);
        name = name_;
        manufacturer = manufacturer_;
        model = model_;
    }

    ~HaMqttDevice() {
    }

    size_t addEntity(HaMqttEntity * entity) {
      entities.push_back(entity);
      size_t entity_index = entities.size() - 1;
      return entity_index;
    }

    size_t getEntityIndex(HaMqttEntity * entity) {
      for(size_t i=0; i<entities.size(); i++) {
        const HaMqttEntity * e = entities[i];
        if (entity == e) {
          return i;
        }
      }
      return (size_t)-1;
    }

    void addIdentifier(const char * value) {
        identifiers.push_back(value);
    }

    void addIdentifier(const String & value) {
        identifiers.push_back(value);
    }

    size_t getIdentifiersCount() const {
        return identifiers.size();
    }

    const String * getIdentifier(size_t index) const {
        if (index < identifiers.size())
            return &identifiers[index];
        return NULL;
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

    void setManufacturer(const String & value) {
        manufacturer = value;
    }

    void setManufacturer(const char * value) {
        manufacturer = value;
    }

    const String & getManufacturer() const {
        return manufacturer;
    }

    void setModel(const String & value) {
        model = value;
    }

    void setModel(const char * value) {
        model = value;
    }

    const String & getModel() const {
        return model;
    }

    void setHardwareVersion(const String & value) {
        hw_version = value;
    }

    void setHardwareVersion(const char * value) {
        hw_version = value;
    }

    const String & getHardwareVersion() const {
        return hw_version;
    }

    void setSoftwareVersion(const String & value) {
        sw_version = value;
    }

    void setSoftwareVersion(const char * value) {
        sw_version = value;
    }

    const String & getSoftwareVersion() const {
        return sw_version;
    }

    void setConfigurationUrl (const String & value) {
        configuration_url  = value;
    }

    void setConfigurationUrl(const char * value) {
        configuration_url  = value;
    }

    const String & getConfigurationUrl() const {
        return configuration_url ;
    }
    void setSuggestedArea(const String & value) {
        suggested_area = value;
    }

    void setSuggestedArea(const char * value) {
        suggested_area = value;
    }

    const String & getSuggestedArea() const {
        return suggested_area;
    }

    void serializeTo(JsonObject json_object) const {
        JsonArray json_identifiers = json_object.createNestedArray("identifiers");
        for(size_t i=0; i<identifiers.size(); i++) {
            const String & identifier = identifiers[i];
            json_identifiers.add(identifier);
        }
        
        json_object["name"] = name;

        if (!manufacturer.isEmpty())
            json_object["manufacturer"] = manufacturer;
        if (!model.isEmpty())
            json_object["model"] = model;
        if (!hw_version.isEmpty())
            json_object["hw_version"] = hw_version;
        if (!sw_version.isEmpty())
            json_object["sw_version"] = sw_version;
        if (!configuration_url.isEmpty())
            json_object["configuration_url"] = configuration_url;
        if (!suggested_area.isEmpty())
            json_object["suggested_area"] = suggested_area;
        if (!via_device.isEmpty())
            json_object["via_device"] = via_device;
    }

  private:
    EntityPtrVector entities;
    StringVector identifiers;
    String name;
    String manufacturer;
    String model;
    String hw_version;
    String sw_version;
    String configuration_url;
    String suggested_area;
    String via_device;
};
