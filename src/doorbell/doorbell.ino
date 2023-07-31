#include <ESP8266WiFi.h>    // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <PubSubClient.h>   // https://www.arduino.cc/reference/en/libraries/pubsubclient/
#include <SoftTimers.h>     // https://www.arduino.cc/reference/en/libraries/softtimers/
#include <ArduinoJson.h>    // https://www.arduino.cc/reference/en/libraries/arduinojson/
#include <Button.h>         // https://www.arduino.cc/reference/en/libraries/button/

#include <strings.h>  // for strcasecmp
#include <cctype>     //for isprint

#include "arduino_secrets.h"

#include "HaMqttDiscovery/HaMqttEntity.hpp"
#include "HaMqttDiscovery/HaMqttDevice.hpp"
#include "HaMqttDiscovery/MqttAdaptorPubSubClient.hpp"

using namespace HaMqttDiscovery;

struct LIGHT_STATE {
  bool is_on;
  uint8_t brightness;
};

struct SMART_LED {
  uint8_t pin;
  HaMqttEntity entity;
  LIGHT_STATE state;
};

struct DOORBELL_STATE {
  bool is_pressed;
};

struct SMART_DOORBELL {
  uint8_t pin;
  HaMqttEntity entity;
  DOORBELL_STATE state;
};



//************************************************************
//   Predeclarations
//************************************************************

void setup();
void setup_wifi();
void setup_device();
void setup_led(size_t led_index);
void setup_mqtt();
void led_turn_on(size_t led_index, uint8_t brightness);
void led_turn_off(size_t led_index);
size_t print_cstr_without_terminating_null(const char* str, size_t length, size_t max_chunk_size);
void mqtt_subscription_callback(const char* topic, const byte* payload, unsigned int length);
void mqtt_reconnect();
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
static const uint8_t DOORBELL_PIN = D5;

WiFiClient wifi_client;
SoftTimer publish_timer; //millisecond timer

static const char * device_identifier_prefix = "doorbell";
String device_identifier; // defined as device_identifier_prefix followed by device mac address without ':' characters.

// MQTT variables
PubSubClient mqtt_client(wifi_client);
MqttAdaptorPubSubClient publish_adaptor;
HaMqttDevice this_device;

SMART_LED led0;
SMART_LED led1;
SMART_LED * leds[] = {&led0, &led1};
size_t leds_count = sizeof(leds)/sizeof(leds[0]);

Button doorbell_button(DOORBELL_PIN);
SMART_DOORBELL doorbell;

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
  led.entity.setState(String() + "{\"state\":\"ON\",\"brightness\":" + String(led.state.brightness) + "}");
}

void led_turn_off(size_t led_index) {
  SMART_LED & led = *(leds[led_index]);

  // Turn the LED off by making the voltage HIGH
  digitalWrite(led.pin, HIGH); // OFF
  led.state.is_on = false;
  led.entity.setState(String() + "{\"state\":\"OFF\",\"brightness\":" + String(led.state.brightness) + "}");

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

void setup_device() {
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

  // Configure this device attributes
  this_device.addIdentifier(device_identifier);
  this_device.setName("Smart doorbell");
  this_device.setManufacturer("end2endzone");
  this_device.setModel("ESP8266");
  this_device.setHardwareVersion("2.2");
  this_device.setSoftwareVersion("0.1");
  this_device.setMqttAdaptor(&publish_adaptor);

  setup_led(0);
  setup_led(1);

  // Configure DOORBELL entity attributes
  doorbell.pin = DOORBELL_PIN;
  doorbell.entity.setIntegrationType(HA_MQTT_BINARY_SENSOR);
  doorbell.entity.setName("BELL");
  doorbell.entity.setStateTopic(   device_identifier + "/doorbell/state");
  doorbell.entity.addKeyValue("device_class","sound");
  doorbell.entity.setDevice(&this_device); // this also adds the entity to the device and generates a unique_id based on the first identifier of the device.
  doorbell.entity.setMqttAdaptor(&publish_adaptor);

}

void setup_led(size_t led_index) {
  SMART_LED & led = *(leds[led_index]);

  String led_index_str = String(led_index);

  // Configure the LED entity attributes
  static const String NAME_PREFIX = "LED ";
  led.entity.setIntegrationType(HA_MQTT_LIGHT);
  led.entity.setName(NAME_PREFIX + led_index_str);
  led.entity.setCommandTopic( device_identifier + "/led" + led_index_str + "/set");
  led.entity.setStateTopic(   device_identifier + "/led" + led_index_str + "/state");
  led.entity.addKeyValue("schema","json");
  led.entity.addKeyValue("brightness","true");
  led.entity.setDevice(&this_device); // this also adds the entity to the device and generates a unique_id based on the first identifier of the device.
  led.entity.setMqttAdaptor(&publish_adaptor);
}

void setup_mqtt() {
  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setCallback(mqtt_subscription_callback);
  mqtt_client.setKeepAlive(30);
  
  // Changing default buffer size. If buffer is too small, publishing and notifications are discarded.
  uint16_t default_buffer_size = mqtt_client.getBufferSize();
  Serial.print("PubSubClient.buffer_size: ");
  Serial.println(default_buffer_size);

  mqtt_client.setBufferSize(4*default_buffer_size);
  Serial.print("PubSubClient.buffer_size set to ");
  Serial.println(mqtt_client.getBufferSize());
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
    Serial.print("Attempting MQTT connection... ");

    bool connect_success = false;

    // Attempt to connect
    MqttLastWillAndTestament lwt;
    if (this_device.getLastWillAndTestamentInfo(lwt)) {
      // Connect and setup an MQTT Last Will and Testament
      connect_success = mqtt_client.connect(
          device_identifier.c_str(),
          mqtt_user,
          mqtt_pass,
          lwt.topic.c_str(),
          lwt.qos,
          lwt.retain,
          lwt.payload.c_str());
    } else {
      // Connect normally
      connect_success = mqtt_client.connect(device_identifier.c_str(), mqtt_user, mqtt_pass);
    }
    
    if (connect_success) {
      Serial.print("connected as '");
      Serial.print(device_identifier.c_str());
      Serial.println("'.");
    } else {
      Serial.print("failed, mqtt-state=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }

    // Do initialization stuff when we first connect.
    if (mqtt_client.connected()) {
      
      this_device.publishMqttDeviceStatus(false);

      for(size_t i=0; i<leds_count; i++) {
        SMART_LED & led = *leds[i];

        // Publish Home Assistant mqtt discovery topic
        led.entity.publishMqttDiscovery();
    
        // Publish entity's state to initialize Home Assistant UI
        led.entity.getState().setDirty();
        led.entity.publishMqttState();

        // Subscribe to receive entity state change notifications
        led.entity.subscribe();
      }

      doorbell.entity.publishMqttDiscovery();
      doorbell.state.is_pressed = false;
      doorbell.entity.setState("OFF");
      doorbell.entity.publishMqttState();

      this_device.publishMqttDeviceStatus(true);
    }
    
  }
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

void setup() {
  pinMode(LED0_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  
  doorbell_button.begin();
  doorbell.pin = DOORBELL_PIN;

  led0.pin = LED0_PIN;
  led1.pin = LED1_PIN;

  publish_adaptor.setPubSubClient(&mqtt_client);

  Serial.begin(115200);
  
  // Force turn OFF all leds.
  for(size_t i=0; i<leds_count; i++) {
    SMART_LED & led = *leds[i];

    led.state.brightness = 255;
    led_turn_off(i); // for initializing led's internal variables
  }
  
  setup_wifi();
  setup_device();
  setup_mqtt();
}

void loop() {
  // make sure mqtt is working
  if (!mqtt_client.connected()) {
    mqtt_reconnect();
  }
  mqtt_client.loop();

  // Read DOORBELL pin
  if (doorbell_button.toggled()) {
    if (doorbell_button.read() == Button::PRESSED) {
      doorbell.state.is_pressed = true;
    } else {
      doorbell.state.is_pressed = false;
    }
    doorbell.entity.setState(doorbell.state.is_pressed ? "ON" : "OFF");
  }

  // should we update a LED mqtt state ?
  for(size_t i=0; i<leds_count; i++) {
    SMART_LED & led = *leds[i];
    if (led.entity.getState().isDirty()) {
      led.entity.publishMqttState();
    }
  }

  if (doorbell.entity.getState().isDirty()) {
    doorbell.entity.publishMqttState();
  }
}
