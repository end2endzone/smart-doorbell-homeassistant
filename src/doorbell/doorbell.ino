#include <ESP8266WiFi.h>    // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <PubSubClient.h>   // https://www.arduino.cc/reference/en/libraries/pubsubclient/
#include <SoftTimers.h>     // https://www.arduino.cc/reference/en/libraries/softtimers/
#include <ArduinoJson.h>    // https://www.arduino.cc/reference/en/libraries/arduinojson/

#include <strings.h>  // for strcasecmp
#include <cctype>     //for isprint
#include "arduino_secrets.h"

struct HA_MQTT_DEVICE {
  String * identifiers; // an array of 'identifiers_count' elements of string identifiers
  uint16_t identifiers_count;
  String name;
  String manufacturer;
  String model;
  String hw_version;
  String sw_version;
};

struct HA_MQTT_ENTITY {
  String name;
  String unique_id;
  String object_id;
  String command_topic;
  String state_topic;
  String more_json;
  HA_MQTT_DEVICE * device;
};

struct HA_MQTT_DISCOVERY {
  HA_MQTT_ENTITY * entity;	// the entity for Home Assistant to discover
  String topic;  // topic where Home Assistant will discover the associated entity
  String payload;
};

struct HA_MQTT_STATE {
  String payload; // payload of the mqtt state topic, if is_dirty is set
  bool is_dirty;  // true if the state has changed and the mqtt payload must be publshed to topic
};

struct HA_LIGHT_STATE {
  bool is_on;
  uint8_t brightness;
};

//************************************************************
//   Predeclarations
//************************************************************

void mqtt_subscription_callback(const char* topic, const byte* payload, unsigned int length);

void build_ha_mqtt_device_json(String & s, HA_MQTT_DEVICE * device);
void build_ha_mqtt_entity_discovery_payload(String & s, HA_MQTT_ENTITY * device);

void publish_ha_mqtt_discovery(HA_MQTT_DISCOVERY * discovery);
void mqtt_entity_subscribe(HA_MQTT_ENTITY * entity);

bool is_empty(const char * s);
bool is_empty(const String & s) { return is_empty(s.c_str()); }
bool is_empty(const String * s) { return is_empty(s->c_str()); }
bool parse_boolean(const char * value);
uint8_t parse_uint8(const char * value);

//************************************************************
//   Variables
//************************************************************

const char * wifi_ssid = SECRET_WIFI_SSID;
const char * wifi_pass = SECRET_WIFI_PASS;
const char* mqtt_server = SECRET_MQTT_HOST;
const char* mqtt_user = SECRET_MQTT_USER;
const char* mqtt_pass = SECRET_MQTT_PASS;

static const uint8_t LED0_PIN = 2;
static const uint8_t LED1_PIN = 16;

WiFiClient wifi_client;
SoftTimer publish_timer; //millisecond timer

static const String home_assistant_discovery_prefix = "homeassistant";
static const char * device_identifier_prefix = "doorbell";
String device_identifier_prefix_str = device_identifier_prefix;
String device_identifier; // defined as device_identifier_prefix followed by device mac address without ':' characters.

// MQTT variables
PubSubClient mqtt_client(wifi_client);
HA_MQTT_DEVICE this_device;

// LED-0 entity
HA_MQTT_ENTITY led0_entity;
HA_LIGHT_STATE led0_state;
HA_MQTT_STATE led0_mqtt;
HA_MQTT_DISCOVERY led0_discovery;

// LED-1 entity
HA_MQTT_ENTITY led1_entity;
HA_LIGHT_STATE led1_state;
HA_MQTT_STATE led1_mqtt;
HA_MQTT_DISCOVERY led1_discovery;

void led0_turn_on(uint8_t brightness = 255) {
  if (brightness == 255) {
    // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
    digitalWrite(LED0_PIN, LOW); // ON

    Serial.println("Set LED-0 on");
  }
  else {
    //LED pin brightness is inverted. 0 is 100% and 255 is 0%
    uint8_t pwm_brightness = map(brightness, 0, 255, 255, 0);
    
    analogWrite(LED0_PIN, pwm_brightness);
    Serial.print("Set LED-0 brightness to ");
    Serial.println(pwm_brightness);
  }
  
  led0_state.is_on = true;
  led0_state.brightness = brightness;
  
  led0_mqtt.is_dirty = true;
  led0_mqtt.payload = "{\"state\":\"ON\",\"brightness\":" + String(led0_state.brightness) + "}";
}

void led0_turn_off() {
  // Turn the LED off by making the voltage HIGH
  digitalWrite(LED0_PIN, HIGH); // OFF
  led0_state.is_on = false;
  led0_mqtt.is_dirty = true;
  led0_mqtt.payload = "{\"state\":\"OFF\",\"brightness\":" + String(led0_state.brightness) + "}";

  Serial.println("Set LED-0 off");
}

void led1_turn_on(uint8_t brightness = 255) {
  if (brightness == 255) {
    // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
    digitalWrite(LED1_PIN, LOW); // ON

    Serial.println("Set LED-1 on");
  }
  else {
    //LED pin brightness is inverted. 0 is 100% and 255 is 0%
    uint8_t pwm_brightness = map(brightness, 0, 255, 255, 0);
    
    analogWrite(LED1_PIN, pwm_brightness);
    Serial.print("Set LED-1 brightness to ");
    Serial.println(pwm_brightness);
  }
  
  led1_state.is_on = true;
  led1_state.brightness = brightness;
  
  led1_mqtt.is_dirty = true;
  led1_mqtt.payload = "{\"state\":\"ON\",\"brightness\":" + String(led0_state.brightness) + "}";
}

void led1_turn_off() {
  // Turn the LED off by making the voltage HIGH
  digitalWrite(LED1_PIN, HIGH); // OFF
  led1_state.is_on = false;
  led1_mqtt.is_dirty = true;
  led1_mqtt.payload = "{\"state\":\"OFF\",\"brightness\":" + String(led0_state.brightness) + "}";

  Serial.println("Set LED-1 off");
}

void setup_wifi() {
  delay(10);

  // Connecting to local WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(521); // 521 is a good prime number
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // init random number generator
  randomSeed(micros());
}

void setup_device_identifier() {
  device_identifier.clear();
  device_identifier.reserve(32);
  device_identifier = device_identifier_prefix;

  // append mac address characters
  String mac_addr = WiFi.macAddress();
  const char * input = mac_addr.c_str();
  uint16_t length = 0;
  while(input && input[0] != '\0' && length < 18) {
    if (*input != ':') {
      device_identifier += *input;
      length++;
    }
    input++;
  }
  Serial.print("Device identifier: ");
  Serial.println(device_identifier);
}

void setup_mqtt() {
  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setCallback(mqtt_subscription_callback);
  
  // Changing default buffer size. If buffer is too small, publishing and notifications are discarded.
  uint16_t default_buffer_size = mqtt_client.getBufferSize();
  Serial.print("PubSubClient.buffer_size: ");
  Serial.println(default_buffer_size);

  mqtt_client.setBufferSize(4*default_buffer_size);
  Serial.print("PubSubClient.buffer_size set to ");
  Serial.println(mqtt_client.getBufferSize());

  // Configure this device attributes
  this_device.identifiers = new String();
  *this_device.identifiers = device_identifier;
  this_device.identifiers_count = 1;
  this_device.name = "Smart doorbell";
  this_device.manufacturer = "end2endzone";
  this_device.model = "ESP8266";
  this_device.hw_version = "2.2";
  this_device.sw_version = "0.1";

  // Configure the LED-0 entity attributes
  led0_entity.name = "LED 0";
  led0_entity.unique_id     = device_identifier + "_led0";
  led0_entity.command_topic = device_identifier + "/led0/set";
  led0_entity.state_topic   = device_identifier + "/led0/state";
  led0_entity.more_json = ",\"schema\":\"json\",\"brightness\":\"true\"";
  led0_entity.device = &this_device;

  led0_discovery.entity = &led0_entity;
  led0_discovery.topic = home_assistant_discovery_prefix + "/light/" + device_identifier + "_entity0/config";
  build_ha_mqtt_entity_discovery_payload(&led0_discovery.payload, led0_discovery.entity);




  // Configure the LED-1 entity attributes
  led1_entity.name = "LED 1";
  led1_entity.unique_id     = device_identifier + "_led1";
  led1_entity.command_topic = device_identifier + "/led1/set";
  led1_entity.state_topic   = device_identifier + "/led1/state";
  led1_entity.more_json = ",\"schema\":\"json\",\"brightness\":\"true\"";
  led1_entity.device = &this_device;

  led1_discovery.entity = &led1_entity;
  led1_discovery.topic = home_assistant_discovery_prefix + "/light/" + device_identifier + "_entity1/config";
  build_ha_mqtt_entity_discovery_payload(&led1_discovery.payload, led1_discovery.entity);
}

bool is_printable(const byte* payload, unsigned int length) {
  for(unsigned int i=0; i<length; i++) {
    if (!isprint((char)payload[i]))
      return false;
  }
  return true;
}

size_t print_cstr_without_terminating_null(const char* str, size_t length, size_t max_chunk_size) {
  // Allocate memory for chunks
  char * chunk_buffer = new char[max_chunk_size];
  if (chunk_buffer == NULL)
    return (size_t)-1;
  
  size_t printed_length = 0;
  while(printed_length < length) {
    // Compute how much byte remains to be printed
    size_t remains = length - printed_length;
    if (remains == 0)
      return printed_length;
    
    // Compute how much we can print as a single chumk
    size_t chunk_size = (max_chunk_size - 1); // default maximum chunk size
    if (remains < chunk_size) {
      chunk_size = remains;
    }
    
    // Grab next print chunk
    size_t offset = printed_length;
    memcpy(chunk_buffer, &str[offset], chunk_size);

    // Make it safe to print
    chunk_buffer[chunk_size] = '\0';

    // Print it
    Serial.print(chunk_buffer);
    printed_length += chunk_size;
  }

  return printed_length;
}

void mqtt_subscription_callback(const char* topic, const byte* payload, unsigned int length) {
  Serial.print("MQTT notify: ");
  Serial.print(length);
  Serial.print(" bytes. topic=");
  Serial.print(topic);

  if (!is_printable(payload, length)) {
    Serial.println();
  } else {
    Serial.print(" payload=");
    print_cstr_without_terminating_null((const char*)payload, length, 256);
    Serial.println();
  }

  // is this our LED-0 command topic ?
  if (led0_entity.command_topic == topic) {
    String json = (const char *)payload;

    // Parse json
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) {
      Serial.print("Failed parsing json payload: ");
      Serial.println(error.f_str());
      return;
    }
    
    // Get the key values from json
    if (!doc.containsKey("state"))
      return;
    const char* state = doc["state"];
    uint8_t brightness = led0_state.brightness; // if brightness is not supplied, use current value
    if (doc.containsKey("brightness"))
      brightness = doc["brightness"];
    
    // Apply command
    bool turn_on = parse_boolean(state);
    if (turn_on)
      led0_turn_on(brightness);
    else
      led0_turn_off();

  }
  // is this our LED-1 command topic ?
  else if (led1_entity.command_topic == topic) {
    String json = (const char *)payload;

    // Parse json
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) {
      Serial.print("Failed parsing json payload: ");
      Serial.println(error.f_str());
      return;
    }
    
    // Get the key values from json
    if (!doc.containsKey("state"))
      return;
    const char* state = doc["state"];
    uint8_t brightness = led1_state.brightness; // if brightness is not supplied, use current value
    if (doc.containsKey("brightness"))
      brightness = doc["brightness"];
    
    // Apply command
    bool turn_on = parse_boolean(state);
    if (turn_on)
      led1_turn_on(brightness);
    else
      led1_turn_off();

  }
  else {
    Serial.print("MQTT error: unknown topic: ");
    Serial.println(topic);
  }
}

void mqtt_reconnect() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt_client.connect(device_identifier.c_str(), mqtt_user, mqtt_pass)) {
      Serial.print("Connected as '");
      Serial.print(device_identifier.c_str());
      Serial.println("'.");
    } else {
      Serial.print("failed, mqtt-state=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }

    // Do initialization stuff when we fisrt connect.
    if (mqtt_client.connected()) {
      
      // Publish Home Assistant mqtt discovery topic
      publish_ha_mqtt_discovery(&led0_discovery);
      publish_ha_mqtt_discovery(&led1_discovery);
  
      // Subscribe to receive entity state change notifications
      mqtt_entity_subscribe(&led0_entity);
      mqtt_entity_subscribe(&led1_entity);
    }
    
  }
}

void show_error(uint16_t err) {
  // Setup a timer to fast blink for 5 seconds
  SoftTimer blink_timer; //millisecond timer
  blink_timer.setTimeOutTime(5000);
  
  while(true) {
    // Fast blink the LED
    blink_timer.reset(); //reset or start counting
    while(!blink_timer.hasTimedOut()) {
      led0_turn_on();
      delay(50);
  
      led0_turn_off();
      delay(50);
    }

    delay(500);
    
    // Make x long blinks
    for(uint16_t i=0; i<err; i++) {
      led0_turn_on();
      delay(500);
      led0_turn_off();
      delay(500);
    }
    
    // Wait again before restart
    delay(3000);
  }
}

bool is_empty(const char * s) {
  if (s == NULL)
    return true;
  bool empty = (s[0] == '\0');
  return empty;
}

bool parse_boolean(const char * value) {
  if (value == NULL)
    return false;
  if (strcmp("ON", value) == 0)
    return true;
  else if (strcmp("on", value) == 0)
    return true;
  else if (strcmp("YES", value) == 0)
    return true;
  else if (strcmp("yes", value) == 0)
    return true;
  else if (strcmp("O", value) == 0)
    return true;
  else if (strcmp("o", value) == 0)
    return true;
  return false;
}

uint8_t parse_uint8(const char * value) {
  if (value == NULL)
    return 0;
  uint8_t numeric_value = (uint8_t)strtoul(value, NULL, 10);
  return numeric_value;
}

void append_json_key_value_pair(String * s, const char * key, const String & value, bool is_first = false) {
  if (!is_first)
    *s += ",";
  *s += "\"";
  *s += key;
  *s += "\":";
  *s += "\"";
  *s += value;
  *s += "\"";  
}
void append_json_key_value_pair(String * s, const char * key, const char * value, bool is_first = false) {
  if (!is_first)
    *s += ",";
  *s += "\"";
  *s += key;
  *s += "\":";
  *s += "\"";
  *s += value;
  *s += "\"";
}

void build_ha_mqtt_device_json(String * s, HA_MQTT_DEVICE * dev) {
  s->clear();
  s->reserve(200);

  if (dev->identifiers == NULL ||
      dev->identifiers_count < 1 ||
      dev->name == NULL)
    return;

  *s += "{\"identifiers\":[";
  for(uint16_t i=0; i<dev->identifiers_count; i++) {
    *s += "\"";
    *s += dev->identifiers[i];
    *s += "\"";
    if (i+1 < dev->identifiers_count)
      *s += ",";
  }
  *s += "]";

  if (!is_empty(dev->name))
    append_json_key_value_pair(s, "name", dev->name);
  if (!is_empty(dev->manufacturer))
    append_json_key_value_pair(s, "manufacturer", dev->manufacturer);
  if (!is_empty(dev->model))
    append_json_key_value_pair(s, "model", dev->model);
  if (!is_empty(dev->hw_version))
    append_json_key_value_pair(s, "hw_version", dev->hw_version);
  if (!is_empty(dev->sw_version))
    append_json_key_value_pair(s, "sw_version", dev->sw_version);

  *s += "}";
}

void build_ha_mqtt_entity_discovery_payload(String * s, HA_MQTT_ENTITY * entity) {
  s->clear();
  s->reserve(512);

  if (entity->name == NULL ||
      entity->unique_id == NULL)
    return;

  *s += "{";
  
  append_json_key_value_pair(s, "name", entity->name, true);

  if (!is_empty(entity->unique_id))
    append_json_key_value_pair(s, "unique_id", entity->unique_id);
  if (!is_empty(entity->object_id))
    append_json_key_value_pair(s, "object_id", entity->object_id);
  if (!is_empty(entity->command_topic))
    append_json_key_value_pair(s, "cmd_t", entity->command_topic);
  if (!is_empty(entity->state_topic))
    append_json_key_value_pair(s, "stat_t", entity->state_topic);
  if (!is_empty(entity->more_json))
    *s += entity->more_json;

  if (entity->device) {
    String device_json;
    build_ha_mqtt_device_json(&device_json, entity->device);
    *s += ",\"device\":";
    *s += device_json;
  }

  *s += "}";
}

void publish_ha_mqtt_discovery(HA_MQTT_DISCOVERY * discovery) {
  if (!mqtt_client.connected())
    return;
  if (discovery == NULL || discovery->entity == NULL)
    return;

  const char * topic = discovery->topic.c_str();
  const char * payload = discovery->payload.c_str();
  if (is_empty(topic) || is_empty(payload))
    return;

  static const bool retained = true;
  mqtt_client.publish(topic, payload, retained);

  Serial.print("MQTT publish: topic=");
  Serial.print(topic);
  Serial.print("   payload=");
  Serial.println(payload);
}

void mqtt_entity_subscribe(HA_MQTT_ENTITY * entity) {
  if (entity == NULL)
    return;

  const char * topic = entity->command_topic.c_str();
  if (is_empty(topic))
    return;

  mqtt_client.subscribe(topic);
  Serial.print("MQTT subscribe: topic=");
  Serial.println(topic);
}

void setup() {
  pinMode(LED0_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  
  led0_state.brightness = 255;
  led0_turn_off(); // for initializing internal variables
  
  led1_state.brightness = 255;
  led1_turn_off(); // for initializing internal variables
  
  Serial.begin(115200);
  
  setup_wifi();
  setup_device_identifier();
  setup_mqtt();
}

void loop() {
  // make sure mqtt is working
  if (!mqtt_client.connected()) {
    mqtt_reconnect();
  }
  mqtt_client.loop();

  // should we update the LED mqtt state ?
  if (led0_mqtt.is_dirty) {
    if (mqtt_client.connected()) {
      const char * topic = led0_entity.state_topic.c_str();
      const char * payload = led0_mqtt.payload.c_str();
      if (!is_empty(topic) && !is_empty(payload) ) {
        mqtt_client.publish(topic, payload);
        Serial.print("MQTT publish: topic=");
        Serial.print(topic);
        Serial.print("   payload=");
        Serial.println(payload);
        
        led0_mqtt.is_dirty = false;
      }
    }
  }
  if (led1_mqtt.is_dirty) {
    if (mqtt_client.connected()) {
      const char * topic = led1_entity.state_topic.c_str();
      const char * payload = led1_mqtt.payload.c_str();
      if (!is_empty(topic) && !is_empty(payload) ) {
        mqtt_client.publish(topic, payload);
        Serial.print("MQTT publish: topic=");
        Serial.print(topic);
        Serial.print("   payload=");
        Serial.println(payload);
        
        led1_mqtt.is_dirty = false;
      }
    }
  }
}
