#include <ESP8266WiFi.h>    // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <PubSubClient.h>   // https://www.arduino.cc/reference/en/libraries/pubsubclient/
#include <SoftTimers.h>     // https://www.arduino.cc/reference/en/libraries/softtimers/
#include <ArduinoJson.h>    // https://www.arduino.cc/reference/en/libraries/arduinojson/
#include <Button.h>         // https://www.arduino.cc/reference/en/libraries/button/
#include <anyrtttl.h>       // https://www.arduino.cc/reference/en/libraries/anyrtttl/
#include <binrtttl.h>       // https://www.arduino.cc/reference/en/libraries/anyrtttl/
#include <pitches.h>        // https://www.arduino.cc/reference/en/libraries/anyrtttl/


#include <strings.h>  // for strcasecmp
#include <cctype>     //for isprint

#include "arduino_secrets.h"

#include "HaMqttDiscovery/HaMqttEntity.hpp"
#include "HaMqttDiscovery/HaMqttDevice.hpp"
#include "HaMqttDiscovery/MqttAdaptorPubSubClient.hpp"

using namespace HaMqttDiscovery;

struct LED_STATE {
  bool is_on;
  uint8_t brightness;
};
struct SMART_LED {
  uint8_t pin;
  HaMqttEntity entity;
  LED_STATE state;
};

struct BELL_SENSOR_STATE {
  bool is_pressed;
};
struct SMART_BELL_SENSOR {
  HaMqttEntity entity;
  BELL_SENSOR_STATE state;
};

struct MELODY_STATE {
  size_t selected_melody;
};
struct SMART_MELODY_SELECTOR {
  HaMqttEntity entity;
  MELODY_STATE state;
};

const char melody00[] PROGMEM = "None:d=4,o=5,b=125:1p";
const char melody01[] PROGMEM = "Axel F:d=4,o=5,b=125:g,8a#.,16g,16p,16g,8c6,8g,8f,g,8d.6,16g,16p,16g,8d#6,8d6,8a#,8g,8d6,8g6,16g,16f,16p,16f,8d,8a#,2g,p,16f6,8d6,8c6,8a#,g,8a#.,16g,16p,16g,8c6,8g,8f,g,8d.6,16g,16p,16g,8d#6,8d6,8a#,8g,8d6,8g6,16g,16f,16p,16f,8d,8a#,2g";
const char melody02[] PROGMEM = "Cantina:d=4,o=5,b=250:8a,8p,8d6,8p,8a,8p,8d6,8p,8a,8d6,8p,8a,8p,8g#,a,8a,8g#,8a,g,8f#,8g,8f#,f.,8d.,16p,p.,8a,8p,8d6,8p,8a,8p,8d6,8p,8a,8d6,8p,8a,8p,8g#,8a,8p,8g,8p,g.,8f#,8g,8p,8c6,a#,a,g";
const char melody03[] PROGMEM = "Coca cola:d=4,o=5,b=125:8f#6,8f#6,8f#6,8f#6,g6,8f#6,e6,8e6,8a6,f#6,d6,2p";
const char melody04[] PROGMEM = "Final Countdown:d=4,o=5,b=125:p,8p,16b,16a,b,e,p,8p,16c6,16b,8c6,8b,a,p,8p,16c6,16b,c6,e,p,8p,16a,16g,8a,8g,8f#,8a,g.,16f#,16g,a.,16g,16a,8b,8a,8g,8f#,e,c6,2b.,16b,16c6,16b,16a,1b";
const char melody05[] PROGMEM = "Doom map:d=4,o=5,b=80:16e4,16g4,16e4,16e4,16e4,16f#4,16e4,16e4,16e4,16a#4,16e4,16a4,16e4,16g4,16e4,16e4,16e4,16g4,16e4,16e4,16e4,16f#4,16e4,16e4,16e4,16a#4,16e4,16a4,16e4,16g4,16e4,16f#4,16g4,16a#4,16g4,16g4,16g4,16c,16g4,16g4,16g4,16c#,16g4,16c,16g4,16a#4,16g4,16g4,16g4,16a#4,16g4,16g4,16g4,16c,16g4,16g4,16g4,16c#,16g4,16c,16g4,8a#4";
const char melody06[] PROGMEM = "Duke Nukem:d=4,o=5,b=90:16f#4,16a4,16p,16b4,8p,16f#4,16b4,16p,16c#,8p,16f#4,16c#,16p,16d,16p,16f#4,16p,8c#,16b4,16a4,16f#4,16e4,16f4,16f#4,16g4,16b4,16a4,16g4,16f#4,16a4,16p,16f#4,16a4,16p,16b4,8p,16f#4,16b4,16p,16c#,8p,16f#4,16c#,16p,16d,16p,16f#4,16p,8c#,16b4,16a4,16f#4,16e4,16f4,16f#4,16g4,16b4,16a4,16g4,16f#4,16a4";
const char melody07[] PROGMEM = "Entertainer:d=4,o=5,b=140:8d,8d#,8e,c6,8e,c6,8e,2c.6,8c6,8d6,8d#6,8e6,8c6,8d6,e6,8b,d6,2c6,p,8d,8d#,8e,c6,8e,c6,8e,2c.6,8p,8a,8g,8f#,8a,8c6,e6,8d6,8c6,8a,2d6";;
const char melody08[] PROGMEM = "Flintstones:d=4,o=5,b=200:g#,c#,8p,c#6,8a#,g#,c#,8p,g#,8f#,8f,8f,8f#,8g#,c#,d#,2f,2p,g#,c#,8p,c#6,8a#,g#,c#,8p,g#,8f#,8f,8f,8f#,8g#,c#,d#,2c#";
const char melody09[] PROGMEM = "Funky Town:d=4,o=4,b=125:8c6,8c6,8a#5,8c6,8p,8g5,8p,8g5,8c6,8f6,8e6,8c6,2p,8c6,8c6,8a#5,8c6,8p,8g5,8p,8g5,8c6,8f6,8e6,8c6";
const char melody10[] PROGMEM = "GoodBad:d=4,o=5,b=56:32p,32a#,32d#6,32a#,32d#6,8a#.,16f#.,16g#.,d#,32a#,32d#6,32a#,32d#6,8a#.,16f#.,16g#.,c#6,32a#,32d#6,32a#,32d#6,8a#.,16f#.,32f.,32d#.,c#,32a#,32d#6,32a#,32d#6,8a#.,16g#.,d#";
const char melody11[] PROGMEM = "Itchy:d=4,o=5,b=160:8c6,8a,p,8c6,8a6,p,8c6,8a,8c6,8a,8c6,8a6,p,8p,8c6,8d6,8e6,8p,8e6,8f6,8g6,p,8d6,8c6,d6,8f6,a#6,a6,2c7";
const char melody12[] PROGMEM = "Mission 2:d=4,o=6,b=100:32d,32d#,32d,32d#,32d,32d#,32d,32d#,32d,32d,32d#,32e,32f,32f#,32g,16g,8p,16g,8p,16a#,16p,16b,16p,16g,8p,16g,8p,16f,16p,16f#,16p,16g,8p,16g,8p,16a#,16p,16b,16p,16g,8p,16g,8p,16f,16p,16f#,16p,16a#,16g,2d,32p,16a#,16g,2c#,32p,16a#,16g,2c,16p,16a#5,16c";
const char melody13[] PROGMEM = "MissionImp:d=16,o=6,b=95:32d,32d#,32d,32d#,32d,32d#,32d,32d#,32d,32d,32d#,32e,32f,32f#,32g,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,a#,g,2d,32p,a#,g,2c#,32p,a#,g,2c,a#5,8c,2p,32p,a#5,g5,2f#,32p,a#5,g5,2f,32p,a#5,g5,2e,d#,8d";
const char melody14[] PROGMEM = "Super Mario Bros. 1:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16c7,16p,16c7,16c7,p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16d#6,8p,16d6,8p,16c6";
const char melody15[] PROGMEM = "Super Mario Bros. 3 Level 1:d=4,o=5,b=80:16g,32c,16g.,16a,32c,16a.,16b,32c,16b,16a.,32g#,16a.,16g,32c,16g.,16a,32c,16a,4b.,32p,16c6,32f,16c.6,16d6,32f,16d.6,16e6,32f,16e6,16d.6,32c#6,16d.6,16c6,32f,16c.6,16d6,32f,16d6,4e.6,32p,16g,32c,16g.,16a,32c,16a.,16b,32c,16b,16a.,32g#,16a.,16c6,8c.6,32p,16c6,4c.6";
const char melody16[] PROGMEM = "Super Mario Bros. Death:d=4,o=5,b=90:32c6,32c6,32c6,8p,16b,16f6,16p,16f6,16f.6,16e.6,16d6,16c6,16p,16e,16p,16c";
const char melody17[] PROGMEM = "Super Mario Bros. Underground:d=16,o=6,b=100:c,c5,a5,a,a#5,a#,2p,8p,c,c5,a5,a,a#5,a#,2p,8p,f5,f,d5,d,d#5,d#,2p,8p,f5,f,d5,d,d#5,d#,2p,32d#,d,32c#,c,p,d#,p,d,p,g#5,p,g5,p,c#,p,32c,f#,32f,32e,a#,32a,g#,32p,d#,b5,32p,a#5,32p,a5,g#5";
const char melody18[] PROGMEM = "Super Mario Bros. Water:d=8,o=6,b=250:4d5,4e5,4f#5,4g5,4a5,4a#5,b5,b5,b5,p,b5,p,2b5,p,g5,2e.,2d#.,2e.,p,g5,a5,b5,c,d,2e.,2d#,4f,2e.,2p,p,g5,2d.,2c#.,2d.,p,g5,a5,b5,c,c#,2d.,2g5,4f,2e.,2p,p,g5,2g.,2g.,2g.,4g,4a,p,g,2f.,2f.,2f.,4f,4g,p,f,2e.,4a5,4b5,4f,e,e,4e.,b5,2c.";
const char melody19[] PROGMEM = "Smoke:d=4,o=5,b=112:c,d#,f.,c,d#,8f#,f,p,c,d#,f.,d#,c,2p,8p,c,d#,f.,c,d#,8f#,f,p,c,d#,f.,d#,c,p";
const char melody20[] PROGMEM = "StarWars:d=4,o=5,b=45:32p,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#.6,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#6";
const char melody21[] PROGMEM = "Sweet Child:d=8,o=5,b=140:d,d6,a,g,g6,a,f#6,a,d,d6,a,g,g6,a,f#6,a,e,d6,a,g,g6,a,f#6,a,e,d6,a,g,g6,a,f#6,a";
const char melody22[] PROGMEM = "TopGun:d=4,o=4,b=31:32p,16c#,16g#,16g#,32f#,32f,32f#,32f,16d#,16d#,32c#,32d#,16f,32d#,32f,16f#,32f,32c#,16f,d#,16c#,16g#,16g#,32f#,32f,32f#,32f,16d#,16d#,32c#,32d#,16f,32d#,32f,16f#,32f,32c#,g#";
const char melody23[] PROGMEM = "20thCenFox:d=16,o=5,b=140:b,8p,b,b,2b,p,c6,32p,b,32p,c6,32p,b,32p,c6,32p,b,8p,b,b,b,32p,b,32p,b,32p,b,32p,b,32p,b,32p,b,32p,g#,32p,a,32p,b,8p,b,b,2b,4p,8e,8g#,8b,1c#6,8f#,8a,8c#6,1e6,8a,8c#6,8e6,1e6,8b,8g#,8a,2b";
const char melody24[] PROGMEM = "XFiles:d=4,o=5,b=125:e,b,a,b,d6,2b.,1p,e,b,a,b,e6,2b.,1p,g6,f#6,e6,d6,e6,2b.,1p,g6,f#6,e6,d6,f#6,2b.,1p,e,b,a,b,d6,2b.,1p,e,b,a,b,e6,2b.,1p,e6,2b.";

static const char* melodies_array[] = {
  melody00,
  melody01,
  melody02,
  melody03,
  melody04,
  melody05,
  melody06,
  melody07,
  melody08,
  melody09,
  melody10,
  melody11,
  melody12,
  melody13,
  melody14,
  melody15,
  melody16,
  melody17,
  melody18,
  melody19,
  melody20,
  melody21,
  melody22,
  melody23,
  melody24,
};
static const size_t melodies_array_count = sizeof(melodies_array) / sizeof(melodies_array[0]);
static const char* melody_names[melodies_array_count] = {0};
static const size_t INVALID_MELODY_INDEX = (size_t)-1;

//************************************************************
//   Predeclarations
//************************************************************

void setup();
void setup_melody_names();
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
size_t find_melody_by_name(const char * name);
size_t find_melody_by_name(const String & name);
size_t find_melody_by_name(const uint8_t * buffer, size_t length);
void extract_melody_name(const __FlashStringHelper* str, String & name);
void extract_melody_name(size_t index, String & name);

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
static const uint8_t BUZZER_PIN = D1;

WiFiClient wifi_client;
SoftTimer publish_timer; //millisecond timer
SoftTimer melody_disabled_timer; //millisecond timer

static const char * device_identifier_prefix = "doorbell";
String device_identifier; // defined as device_identifier_prefix followed by device mac address without ':' characters.

// MQTT support variables
PubSubClient mqtt_client(wifi_client);
MqttAdaptorPubSubClient publish_adaptor;

// Home Assistant support variables
HaMqttDevice this_device;

SMART_LED led0;
SMART_LED led1;
SMART_LED * leds[] = {&led0, &led1};
size_t leds_count = sizeof(leds)/sizeof(leds[0]);

Button doorbell_button(DOORBELL_PIN);
SMART_BELL_SENSOR doorbell;

SMART_MELODY_SELECTOR melody_selector;

HaMqttEntity * entities[] = {
  &led0.entity,
  &led1.entity,
  &doorbell.entity,
  &melody_selector.entity,
};
size_t entities_count = sizeof(entities)/sizeof(entities[0]);


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

void setup_melody_names() {
  for(size_t i=0; i<melodies_array_count; i++) {
    // Find the name of this rtttl melody
    String name;
    extract_melody_name(i, name);
    Serial.print("Found melody ");
    Serial.print(String(i));
    Serial.print(": ");
    Serial.println(name);

    melody_names[i] = strdup(name.c_str());
  }
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
  doorbell.entity.setIntegrationType(HA_MQTT_BINARY_SENSOR);
  doorbell.entity.setName("BELL");
  doorbell.entity.setStateTopic(device_identifier + "/doorbell/state");
  doorbell.entity.addKeyValue("device_class","sound");
  doorbell.entity.setDevice(&this_device); // this also adds the entity to the device and generates a unique_id based on the first identifier of the device.
  doorbell.entity.setMqttAdaptor(&publish_adaptor);

  // Configure MELODY_SELECTOR entity attributes
  melody_selector.entity.setIntegrationType(HA_MQTT_SELECT);
  melody_selector.entity.setName("Melody");
  melody_selector.entity.setCommandTopic(device_identifier + "/melody/set");
  melody_selector.entity.setStateTopic(device_identifier + "/melody/state");
  melody_selector.entity.addStaticCStrArray("options", melody_names, melodies_array_count);
  melody_selector.entity.setDevice(&this_device); // this also adds the entity to the device and generates a unique_id based on the first identifier of the device.
  melody_selector.entity.setMqttAdaptor(&publish_adaptor);

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

  bool printable = is_printable(payload, length);
  if (!printable) {
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

  // is this a MELODY_SELECTOR command topic ?
  if (melody_selector.entity.getCommandTopic() == topic) {
    if (printable) {
      size_t melody_name_index = find_melody_by_name(payload, length);
      if (melody_name_index != INVALID_MELODY_INDEX) {
        melody_selector.state.selected_melody = melody_name_index;
        melody_selector.entity.setState(melody_names[melody_selector.state.selected_melody]);

        return; // this topic is handled
      }
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
      
      // Set device as "offline" while we update all entity states.
      // This will "disable" all entities in Home Assistant while we update.
      this_device.publishMqttDeviceStatus(false);

      // Force all entities to update
      for(size_t i=0; i<entities_count; i++) {
        HaMqttEntity & entity = *(entities[i]);

        // Publish Home Assistant mqtt discovery topic
        entity.publishMqttDiscovery();

        // Publish entity's state to initialize Home Assistant UI
        entity.getState().setDirty();
        entity.publishMqttState();

        // Subscribe to receive entity state change notifications
        entity.subscribe();
      }

      // Set device as back "online"
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

size_t find_melody_by_name(const char * name) { return find_melody_by_name(String(name)); }
size_t find_melody_by_name(const String & name) {
  for(size_t i=0; i<melodies_array_count; i++) {
    const char * melody = melody_names[i];
    if (name == melody) {
      return i;
    }
  }
  return INVALID_MELODY_INDEX;
}
size_t find_melody_by_name(const uint8_t * buffer, size_t length) {
  if (length == 0)
    return INVALID_MELODY_INDEX;
  
  String test;
  test.reserve(length+1);
  
  for(size_t i=0; i<length; i++)
    test += (char)buffer[i];
  
  return find_melody_by_name(test);
}

void extract_melody_name(const __FlashStringHelper* str, String & name) {
  name.clear();
  if (str == NULL)
    return;

  const char * buffer = (const char *)str; // temporary storage type to allow buffer++
  char c;
  c = pgm_read_byte_near(buffer);
  while(c != '\0' && c != ':') {
    name += c;
    buffer++;
    c = pgm_read_byte_near(buffer);
  }
}

void extract_melody_name(size_t index, String & name) {
  name.clear();
  if (index >= melodies_array_count)
    return;
  const char * melody = melodies_array[index];
  extract_melody_name((const __FlashStringHelper*)melody, name);
}


void setup() {
  pinMode(LED0_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  doorbell_button.begin();

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

  // Set default values for other entities
  doorbell.state.is_pressed = false;
  doorbell.entity.setState("OFF");

  melody_selector.state.selected_melody = 0;
  melody_selector.entity.setState(melody_names[melody_selector.state.selected_melody]);

  setup_wifi();
  setup_melody_names();
  setup_device();
  setup_mqtt();

  // setup a timer to prevent starting a new melody
  melody_disabled_timer.setTimeOutTime(1);
  melody_disabled_timer.reset();  //start counting now
  delay(10); // set our timer to be in "timed out" state.
  melody_disabled_timer.setTimeOutTime(2500);
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

  // Should we start a melody?
  if (doorbell.entity.getState().isDirty() &&
      melody_selector.state.selected_melody < melodies_array_count &&
      melody_disabled_timer.hasTimedOut() &&
      !anyrtttl::nonblocking::isPlaying())
  {
    const char * selected_melodies_array = melodies_array[melody_selector.state.selected_melody];
    anyrtttl::nonblocking::begin_P(BUZZER_PIN, selected_melodies_array);

    // Update our timer
    melody_disabled_timer.reset();  //start counting now
  }

  // If we are playing something, keep playing! 
  if ( anyrtttl::nonblocking::isPlaying() )
  {
    anyrtttl::nonblocking::play();
  }

  // Publish dirty state of all entities
  for(size_t i=0; i<entities_count; i++) {
    HaMqttEntity & entity = *(entities[i]);
    if (entity.getState().isDirty()) {
      entity.publishMqttState();
    }
  }
}
