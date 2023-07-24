#include <ESP8266WiFi.h>    // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <PubSubClient.h>   // https://www.arduino.cc/reference/en/libraries/pubsubclient/
#include <SoftTimers.h>     // https://www.arduino.cc/reference/en/libraries/softtimers/
#include <ArduinoJson.h>    // https://www.arduino.cc/reference/en/libraries/arduinojson/

#include <strings.h>  // for strcasecmp
#include <cctype>     //for isprint
#include "arduino_secrets.h"
#include "HaMqttDiscovery/HaMqttEntity.hpp"
#include "HaMqttDiscovery/HaMqttDevice.hpp"

struct MQTT_STATE {
  String payload; // payload of the mqtt state topic, if is_dirty is set
  bool is_dirty;  // true if the state has changed and the mqtt payload must be published to topic
};

struct LIGHT_STATE {
  bool is_on;
  uint8_t brightness;
  MQTT_STATE mqtt;
};

struct SMART_LED {
  uint8_t pin;
  HaMqttEntity entity;
  LIGHT_STATE state;
};


//************************************************************
//   Predeclarations
//************************************************************

void led_turn_on(size_t led_index, uint8_t brightness);
void led_turn_off(size_t led_index);
void setup();
void setup_wifi();
void setup_device_identifier();
void setup_led(size_t led_index);
void setup_mqtt();
size_t print_cstr_without_terminating_null(const char* str, size_t length, size_t max_chunk_size);
void mqtt_subscription_callback(const char* topic, const byte* payload, unsigned int length);
void mqtt_reconnect();
void show_error(uint16_t err);
bool is_empty(const char * s);
bool parse_boolean(const char * value);
uint8_t parse_uint8(const char * value);
void publish_entity_discovery(HaMqttEntity * entity);
void subscribe_to_entity_command_topic(HaMqttEntity * entity);


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

static const char * device_identifier_prefix = "doorbell";
String device_identifier; // defined as device_identifier_prefix followed by device mac address without ':' characters.

// MQTT variables
PubSubClient mqtt_client(wifi_client);
HaMqttDevice this_device;

SMART_LED led0;
SMART_LED led1;

SMART_LED * leds[] = {&led0, &led1};
size_t leds_count = sizeof(leds)/sizeof(leds[0]);

void led_turn_on(size_t led_index, uint8_t brightness = 255) {
  SMART_LED & led = *(leds[led_index]);

  if (brightness == 255) {
    // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
    digitalWrite(led.pin, LOW); // ON

    Serial.print("Set LED-");
    Serial.print(led_index);
    Serial.println(" on");
  }
  else {
    //LED pin brightness is inverted. 0 is 100% and 255 is 0%
    uint8_t pwm_brightness = map(brightness, 0, 255, 255, 0);
    
    analogWrite(led.pin, pwm_brightness);
    Serial.print("Set LED-");
    Serial.print(led_index);
    Serial.print(" brightness to ");
    Serial.println(pwm_brightness);
  }
  
  led.state.is_on = true;
  led.state.brightness = brightness;
  led.state.mqtt.is_dirty = true;
  led.state.mqtt.payload = "{\"state\":\"ON\",\"brightness\":" + String(led.state.brightness) + "}";
}

void led_turn_off(size_t led_index) {
  SMART_LED & led = *(leds[led_index]);

  // Turn the LED off by making the voltage HIGH
  digitalWrite(led.pin, HIGH); // OFF
  led.state.is_on = false;
  led.state.mqtt.is_dirty = true;
  led.state.mqtt.payload = "{\"state\":\"OFF\",\"brightness\":" + String(led.state.brightness) + "}";

  Serial.print("Set LED-");
  Serial.print(led_index);
  Serial.println(" off");
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

void setup_led(size_t led_index) {
  SMART_LED & led = *(leds[led_index]);

  String led_index_str = String(led_index);

  // Configure the LED-0 entity attributes
  static const String NAME_PREFIX = "LED ";
  led.entity.setIntegrationType(HA_MQTT_LIGHT);
  led.entity.setName(NAME_PREFIX + led_index_str);
  led.entity.setCommandTopic( device_identifier + "/led" + led_index_str + "/set");
  led.entity.setStateTopic(   device_identifier + "/led" + led_index_str + "/state");
  led.entity.addKeyValue("schema","json");
  led.entity.addKeyValue("brightness","true");
  led.entity.setDevice(&this_device); // this also adds the entity to the device and generates a unique_id based on the first identifier of the device.
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
  this_device.addIdentifier(device_identifier);
  this_device.setName("Smart doorbell");
  this_device.setManufacturer("end2endzone");
  this_device.setModel("ESP8266");
  this_device.setHardwareVersion("2.2");
  this_device.setSoftwareVersion("0.1");

  setup_led(0);
  setup_led(1);
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

  // is this a LED command topic ?
  for(size_t i=0; i<leds_count; i++) {
    SMART_LED & led = *leds[i];

    if (led.entity.getCommandTopic() == topic) {
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
      uint8_t brightness = led.state.brightness; // if brightness is not supplied, use current value
      if (doc.containsKey("brightness"))
        brightness = doc["brightness"];
      
      // Apply command
      bool turn_on = parse_boolean(state);
      if (turn_on)
        led_turn_on(i, brightness);
      else
        led_turn_off(i);

      return; // this topic is handled
    }
  }

  Serial.print("MQTT error: unknown topic: ");
  Serial.println(topic);
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
      
      for(size_t i=0; i<leds_count; i++) {
        SMART_LED & led = *leds[i];

        // Publish Home Assistant mqtt discovery topic
        publish_entity_discovery(&led.entity);
    
        // Subscribe to receive entity state change notifications
        subscribe_to_entity_command_topic(&led.entity);
      }
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
      led_turn_on(0, 255);
      delay(50);
  
      led_turn_off(0);
      delay(50);
    }

    delay(500);
    
    // Make x long blinks
    for(uint16_t i=0; i<err; i++) {
      led_turn_on(0, 255);
      delay(500);
      led_turn_off(0);
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
  else if (strcmp("TRUE", value) == 0)
    return true;
  else if (strcmp("true", value) == 0)
    return true;
  else if (strcmp("YES", value) == 0)
    return true;
  else if (strcmp("yes", value) == 0)
    return true;
  else if (strcmp("1", value) == 0)
    return true;
  return false;
}

uint8_t parse_uint8(const char * value) {
  if (value == NULL)
    return 0;
  uint8_t numeric_value = (uint8_t)strtoul(value, NULL, 10);
  return numeric_value;
}

void publish_entity_discovery(HaMqttEntity * entity) {
  if (!mqtt_client.connected())
    return;

  String topic;
  String payload;
  entity->getDiscoveryTopic(topic);
  entity->getDiscoveryPayload(payload);

  if (topic.isEmpty() || payload.isEmpty())
    return;

  static const bool retained = true;
  mqtt_client.publish(topic.c_str(), payload.c_str(), retained);

  Serial.print("MQTT publish: topic=");
  Serial.print(topic);
  Serial.print("   payload=");
  Serial.println(payload);
}

void subscribe_to_entity_command_topic(HaMqttEntity * entity) {
  const char * topic = entity->getCommandTopic().c_str();
  if (is_empty(topic))
    return;

  mqtt_client.subscribe(topic);
  Serial.print("MQTT subscribe: topic=");
  Serial.println(topic);
}

void setup() {
  pinMode(LED0_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  
  led0.pin = LED0_PIN;
  led1.pin = LED1_PIN;

  // Force turn OFF all leds.
  for(size_t i=0; i<leds_count; i++) {
    SMART_LED & led = *leds[i];

    led.state.brightness = 255;
    led_turn_off(i); // for initializing led's internal variables
  }
  
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

  // should we update a LED mqtt state ?
  for(size_t i=0; i<leds_count; i++) {
    SMART_LED & led = *leds[i];
    if (led.state.mqtt.is_dirty) {
      if (mqtt_client.connected()) {
        const String & topic = led.entity.getStateTopic();
        const String & payload = led.state.mqtt.payload;
        if ( !topic.isEmpty() && !payload.isEmpty() ) {
          mqtt_client.publish(topic.c_str(), payload.c_str());
          Serial.print("MQTT publish: topic=");
          Serial.print(topic);
          Serial.print("   payload=");
          Serial.println(payload);
          
          led.state.mqtt.is_dirty = false;
        }
      }
    }
  }
}
