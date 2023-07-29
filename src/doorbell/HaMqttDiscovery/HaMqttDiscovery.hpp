#pragma once

static String ha_discovery_prefix = "homeassistant";
static String ha_online_string = "online";
static String ha_offline_string = "offline";

static void setHomeAssistantDiscoveryPrefix(const String & value) {
    ha_discovery_prefix = value;
}
static void setHomeAssistantDiscoveryPrefix(const char * value) {
    ha_discovery_prefix = value;
}
static const String & getHomeAssistantDiscoveryPrefix() {
    return ha_discovery_prefix;
}

static void setHomeAssistantOnlineString(const String & value) {
    ha_online_string = value;
}
static void setHomeAssistantOnlineString(const char * value) {
    ha_online_string = value;
}
static const String & getHomeAssistantOnlineString() {
    return ha_online_string;
}

static void setHomeAssistantOfflineString(const String & value) {
    ha_offline_string = value;
}
static void setHomeAssistantOfflineString(const char * value) {
    ha_offline_string = value;
}
static const String & getHomeAssistantOfflineString() {
    return ha_offline_string;
}


enum HA_MQTT_INTEGRATION_TYPE {
    HA_MQTT_ALARM_CONTROL_PANEL ,
    HA_MQTT_BINARY_SENSOR       ,
    HA_MQTT_BUTTON              ,
    HA_MQTT_CAMERA              ,
    HA_MQTT_COVER               ,
    HA_MQTT_DEVICE_TRACKER      ,
    HA_MQTT_DEVICE_TRIGGER      ,
    HA_MQTT_FAN                 ,
    HA_MQTT_HUMIDIFIER          ,
    HA_MQTT_IMAGE               ,
    HA_MQTT_CLIMATE_HVAC        ,
    HA_MQTT_LIGHT               ,
    HA_MQTT_LOCK                ,
    HA_MQTT_NUMBER              ,
    HA_MQTT_SCENE               ,
    HA_MQTT_SELECT              ,
    HA_MQTT_SENSOR              ,
    HA_MQTT_SIREN               ,
    HA_MQTT_SWITCH              ,
    HA_MQTT_UPDATE              ,
    HA_MQTT_TAG_SCANNER         ,
    HA_MQTT_TEXT                ,
    HA_MQTT_VACUUM              ,
    HA_MQTT_WATER_HEATER        ,
};

const char * toString(const HA_MQTT_INTEGRATION_TYPE & type) {
    switch(type) {
        case HA_MQTT_ALARM_CONTROL_PANEL : return "alarm_control_panel";
        case HA_MQTT_BINARY_SENSOR       : return "binary_sensor";
        case HA_MQTT_BUTTON              : return "button";
        case HA_MQTT_CAMERA              : return "camera";
        case HA_MQTT_COVER               : return "cover";
        case HA_MQTT_DEVICE_TRACKER      : return "device_tracker";
        case HA_MQTT_DEVICE_TRIGGER      : return "device_automation";
        case HA_MQTT_FAN                 : return "fan";
        case HA_MQTT_HUMIDIFIER          : return "humidifier";
        case HA_MQTT_IMAGE               : return "image";
        case HA_MQTT_CLIMATE_HVAC        : return "climate";
        case HA_MQTT_LIGHT               : return "light";
        case HA_MQTT_LOCK                : return "lock";
        case HA_MQTT_NUMBER              : return "number";
        case HA_MQTT_SCENE               : return "scene";
        case HA_MQTT_SELECT              : return "select";
        case HA_MQTT_SENSOR              : return "sensor";
        case HA_MQTT_SIREN               : return "siren";
        case HA_MQTT_SWITCH              : return "switch";
        case HA_MQTT_UPDATE              : return "update";
        case HA_MQTT_TAG_SCANNER         : return "tag";
        case HA_MQTT_TEXT                : return "text";
        case HA_MQTT_VACUUM              : return "vacuum";
        case HA_MQTT_WATER_HEATER        : return "boiler";
        default: return "";
    };
}