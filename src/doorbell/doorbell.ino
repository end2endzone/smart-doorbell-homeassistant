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

//************************************************************
//   Constants
//************************************************************

static const char * wifi_ssid = SECRET_WIFI_SSID;
static const char * wifi_pass = SECRET_WIFI_PASS;
static const char* mqtt_server = SECRET_MQTT_HOST;
static const char* mqtt_user = SECRET_MQTT_USER;
static const char* mqtt_pass = SECRET_MQTT_PASS;

static const uint8_t LED0_PIN = 2;
static const uint8_t LED1_PIN = 16;
static const uint8_t DOORBELL_PIN = D5;
static const uint8_t BUZZER_PIN = D1;

#define ERROR_MESSAGE_PREFIX "*** --> "
#define MAX_PUBLISH_RETRY 5
#define DELAY_BETWEEN_MQTT_TRANSACTIONS 100

//************************************************************
//   Variables
//************************************************************

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
  bool detected;
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

struct SWITCH_STATE {
  size_t is_on;
};
struct SMART_SWITCH {
  HaMqttEntity entity;
  SWITCH_STATE state;
};

struct BUTTON_STATE {
  size_t is_pressed;
};
struct SMART_BUTTON {
  HaMqttEntity entity;
  BUTTON_STATE state;
  BUTTON_STATE previous;
};

WiFiClient wifi_client;
SoftTimer hello_timer; //millisecond timer
SoftTimer test_timer; //millisecond timer
SoftTimer doorbell_ringer_timer; //millisecond timer, to delay between each doorbell ring
SoftTimer identify_delay_timer; //millisecond timer, to delay between each play of the identify RTTTL melody.

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

Button bell_sensor_reader(DOORBELL_PIN);
SMART_BELL_SENSOR bell_sensor;

SMART_MELODY_SELECTOR melody_selector;

SMART_BUTTON test_button;

SMART_SWITCH identify;
size_t identify_melody_index = 0;

HaMqttEntity * entities[] = {
  &led0.entity,
  &led1.entity,
  &bell_sensor.entity,
  &melody_selector.entity,
  &test_button.entity,
  &identify.entity,
};
size_t entities_count = sizeof(entities)/sizeof(entities[0]);

static const char* melodies_array[] PROGMEM = {
  "None:d=4,o=5,b=900:32p",
  "Beethoven Fifth Symphony:d=4,o=5,b=125:8p,8g5,8g5,8g5,2d#5",
  "Coca Cola:d=4,o=5,b=125:8f#6,8f#6,8f#6,8f#6,g6,8f#6,e6,8e6,8a6,f#6,d6",
  "Duke Nukem (short):d=4,o=5,b=90:16f#4,16a4,16p,16b4,8p,16f#4,16b4,16p,16c#,8p",
  "Entertaine (short):d=4,o=5,b=140:8d,8d#,8e,c6,8e,c6,8e,2c.6,8c6,8d6,8d#6,8e6,8c6,8d6,e6,8b,d6,2c6",
  "Flintstones (short):d=4,o=5,b=200:g#,c#,8p,c#6,8a#,g#,c#,8p,g#,8f#,8f,8f,8f#,8g#,c#,d#,2f",
  "Intel:d=16,o=5,b=320:d,p,d,p,d,p,g,p,g,p,g,p,d,p,d,p,d,p,a,p,a,p,a,2p,d,p,d,p,d,p,g,p,g,p,g,p,d,p,d,p,d,p,a,p,a,p,a,2p",
  "Mission Impossible - Intro:d=16,o=6,b=95:32d,32d#,32d,32d#,32d,32d#,32d,32d#,32d,32d,32d#,32e,32f,32f#,32g,g",
  "Mission Impossible (short):d=16,o=6,b=95:a#,g,2d,32p,a#,g,2c#,32p,a#,g,2c,a#5,8c,2p,32p,a#5,g5,2f#,32p,a#5,g5,2f,32p,a#5,g5,2e,d#,8d",
  "Mosaic-long:d=8,o=6,b=400:c,e,g,e,c,g,e,g,c,g,c,e,c,g,e,g,e,c,p,c5,e5,g5,e5,c5,g5,e5,g5,c5,g5,c5,e5,c5,g5,e5,g5,e5,c5",
  "Nokia:d=4,o=4,b=180:8e5,8d5,f#,g#,8c#5,8b,d,e,8b,8a,c#,e,2a",
  "Pacman:d=4,o=5,b=112:32b,32p,32b6,32p,32f#6,32p,32d#6,32p,32b6,32f#6,16p,16d#6,16p,32c6,32p,32c7,32p,32g6,32p,32e6,32p,32c7,32g6,16p,16e6,16p,32b,32p,32b6,32p,32f#6,32p,32d#6,32p,32b6,32f#6,16p,16d#6,16p,32d#6,32e6,32f6,32p,32f6,32f#6,32g6,32p,32g6,32g#6,32a6,32p,32b.6",
  "Popeye (short):d=8,o=6,b=160:a5,c,c,c,4a#5,a5,4c",
  "Star Wars - Cantina (short):d=4,o=5,b=250:8a,8p,8d6,8p,8a,8p,8d6,8p,8a,8d6,8p,8a,8p,8g#,a,8a,8g#,8a,g,8f#,8g,8f#,f.,8d.,16p",
  "Star Wars - Imperial March (short):d=4,o=5,b=100:e,e,e,8c,16p,16g,e,8c,16p,16g,e",
  "Star Wars (short):d=4,o=5,b=45:32p,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#.6",
  "Super Mario Bros. 1 (short):d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g",
  "Super Mario Bros. 3 Level 1 (short):d=4,o=5,b=80:16g,32c,16g.,16a,32c,16a.,16b,32c,16b,16a.,32g#,16a.,16g,32c,16g.,16a,32c,16a,4b.",
  "Super Mario Bros. Death:d=4,o=5,b=90:32c6,32c6,32c6,8p,16b,16f6,16p,16f6,16f.6,16e.6,16d6,16c6,16p,16e,16p,16c",
  "Sweet Child:d=8,o=5,b=140:d,d6,a,g,g6,a,f#6,a,d,d6,a,g,g6,a,8f#6",
  "The Good the Bad and the Ugly (short):d=4,o=5,b=56:32p,32a#,32d#6,32a#,32d#6,8a#.,16f#.,16g#.,d#",
  "The Itchy Scratchy Show:d=4,o=5,b=160:8c6,8a,p,8c6,8a6,p,8c6,8a,8c6,8a,8c6,8a6,p,8p,8c6,8d6,8e6,8p,8e6,8f6,8g6,p,8d6,8c6,d6,8f6,a#6,a6,2c7",
  "Tones:d=8,o=5,b=500:b,16p,b,2p,g,16p,g,2p,d6,16p,d6,2p,d,16p,d.,1p,b,16p,b,2p,g,16p,g,2p,d6,16p,d6,2p,d,16p,d.",
  "Trio:d=32,o=6,b=320:d#,a#5,d#,a#5,d#,a#5,d#,a#5,d#,a#5,d#,p,g,d#,g,d#,g,d#,g,d#,g,d#,g,p,a#,g,a#,g,a#,g,a#,g,a#,g,a#",
  "Wolf Whistle:d=16,o=5,b=900:8a4,a#4,b4,c,c#,d,d#,e,f,f#,g,g#,a,a#,b,c6,8c#6,d6,d#6,e6,f6,4p,4p,a4,a#4,b4,c,c#,d,d#,e,f,f#,g,g#,a,a#,b,a#,a,g#,g,f#,f,e,d#,d,c#,c,b4,a#4,a4",
  "X-Files (short):d=4,o=5,b=125:e,b,a,b,d6,2b.,8p,e,b,a,b,d6,2b.",
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
void play_hello_animation();
void led_turn_on(size_t led_index, uint8_t brightness);
void led_turn_off(size_t led_index);
size_t print_cstr_without_terminating_null(const char* str, size_t length, size_t max_chunk_size);
void mqtt_subscription_callback(const char* topic, const byte* payload, unsigned int length);
void mqtt_reconnect();
bool parse_boolean(const char * value);
uint8_t parse_uint8(const char * value);
String parse_string_without_terminating_null(const uint8_t * buffer, size_t length);
size_t find_melody_by_name(const char * name);
size_t find_melody_by_name(const String & name);
size_t find_melody_by_name(const uint8_t * buffer, size_t length);
void extract_melody_name(const __FlashStringHelper* str, String & name);
void extract_melody_name(size_t index, String & name);
void increase_mqtt_buffer(uint16_t new_buffer_size = 0);
void timer_force_timed_out(SoftTimer & timer);

//************************************************************
//   Function definitions
//************************************************************

void play_hello_animation() {
  digitalWrite(LED0_PIN, HIGH); // OFF
  digitalWrite(LED1_PIN, HIGH); // OFF

  hello_timer.setTimeOutTime(500);
  hello_timer.reset();

  static const int ANIMATION_COUNT = 4;
  for(int i=0; i<ANIMATION_COUNT; i++) {
    int hello_pin = (i%2 == 0 ? LED0_PIN : LED1_PIN);
    hello_timer.reset();  //start counting now

    while(!hello_timer.hasTimedOut()) {
      digitalWrite(hello_pin, LOW); // ON
      delay(50);
      digitalWrite(hello_pin, HIGH); // OFF
      delay(50);
    }
    delay(500);
  }
}

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

  // Find our IDENTIFY RTTL melody by name
  identify_melody_index = find_melody_by_name("Trio");
  if (identify_melody_index == INVALID_MELODY_INDEX)
    identify_melody_index = 1;
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
  this_device.setHardwareVersion("1.0");
  this_device.setSoftwareVersion("0.1");
  this_device.setMqttAdaptor(&publish_adaptor);

  setup_led(0);
  setup_led(1);

  // Configure DOORBELL entity attributes
  bell_sensor.entity.setIntegrationType(HA_MQTT_BINARY_SENSOR);
  bell_sensor.entity.setName("Bell");
  bell_sensor.entity.setStateTopic(device_identifier + "/doorbell/state");
  bell_sensor.entity.addKeyValue("device_class","sound");
  bell_sensor.entity.setDevice(&this_device); // this also adds the entity to the device and generates a unique_id based on the first identifier of the device.
  bell_sensor.entity.setMqttAdaptor(&publish_adaptor);

  // Configure MELODY_SELECTOR entity attributes
  melody_selector.entity.setIntegrationType(HA_MQTT_SELECT);
  melody_selector.entity.setName("Melody");
  melody_selector.entity.setCommandTopic(device_identifier + "/melody/set");
  melody_selector.entity.setStateTopic(device_identifier + "/melody/state");
  melody_selector.entity.addStaticCStrArray("options", melody_names, melodies_array_count);
  melody_selector.entity.setDevice(&this_device); // this also adds the entity to the device and generates a unique_id based on the first identifier of the device.
  melody_selector.entity.setMqttAdaptor(&publish_adaptor);

  // Configure test_button entity attributes
  test_button.entity.setIntegrationType(HA_MQTT_BUTTON);
  test_button.entity.setName("Test");
  test_button.entity.setCommandTopic(device_identifier + "/test/set");
  test_button.entity.setStateTopic(  device_identifier + "/test/set");
  test_button.entity.setDevice(&this_device); // this also adds the entity to the device and generates a unique_id based on the first identifier of the device.
  test_button.entity.setMqttAdaptor(&publish_adaptor);

  // Configure identify switch entity attributes
  identify.entity.setIntegrationType(HA_MQTT_SWITCH);
  identify.entity.setName("Identify");
  identify.entity.setCommandTopic(device_identifier + "/identify/set");
  identify.entity.setStateTopic(device_identifier + "/identify/state");
  identify.entity.addKeyValue("device_class","switch");
  identify.entity.setDevice(&this_device); // this also adds the entity to the device and generates a unique_id based on the first identifier of the device.
  identify.entity.setMqttAdaptor(&publish_adaptor);
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
  increase_mqtt_buffer(2048);
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

  // is this a MELODY selector command topic ?
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

  // is this a TEST button command topic ?
  if (test_button.entity.getCommandTopic() == topic) {
    // Interrupt what ever we are playing.
    if (anyrtttl::nonblocking::isPlaying())
      anyrtttl::nonblocking::stop();

    // Apply command
    test_button.state.is_pressed = true;
    test_timer.reset(); // start test timer, when the timer expires, the state will change to false

    return; // this topic is handled
  }

  // is this the IDENTIFY switch command topic ?
  if (identify.entity.getCommandTopic() == topic) {
    // Interrupt what ever we are playing.
    if (anyrtttl::nonblocking::isPlaying())
      anyrtttl::nonblocking::stop();

    String value = parse_string_without_terminating_null(payload, length);

    // Apply command
    bool turn_on = parse_boolean(value.c_str());
    if (turn_on != identify.state.is_on) {
      // the state has changed
      identify.state.is_on = turn_on;
      identify.entity.setState((identify.state.is_on ? "ON" : "OFF"));
    }

    return; // this topic is handled
  }

  Serial.print(String(ERROR_MESSAGE_PREFIX) + "MQTT error: unknown topic: ");
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
      
      bool success = false;

      // Set device as "offline" while we update all entity states.
      // This will "disable" all entities in Home Assistant while we update.
      this_device.publishMqttDeviceStatus(false);

      // Force all entities to be discovered
      for(size_t i=0; i<entities_count; i++) {
        HaMqttEntity & entity = *(entities[i]);

        // Publish Home Assistant mqtt discovery topic
        success = false;
        for(size_t i=0; i<MAX_PUBLISH_RETRY && !success; i++) {
          success = entity.publishMqttDiscovery();
          if (!success) {
            // try to increase the mqtt buffer size and try again.
            increase_mqtt_buffer();
          }
        }

        // Allow time for Home Assistant to properly 'discover' the previous entity
        #ifdef DELAY_BETWEEN_MQTT_TRANSACTIONS
        delay(DELAY_BETWEEN_MQTT_TRANSACTIONS);
        #endif

        // Subscribe to receive entity state change notifications
        entity.subscribe();
      }

      // Allow time for Home Assistant to properly 'discover' the previous entity
      #ifdef DELAY_BETWEEN_MQTT_TRANSACTIONS
      delay(DELAY_BETWEEN_MQTT_TRANSACTIONS);
      #endif

      // Force all entities to be updated
      for(size_t i=0; i<entities_count; i++) {
        HaMqttEntity & entity = *(entities[i]);

        // Publish entity's state to initialize Home Assistant UI
        entity.getState().setDirty();
        success = false;
        for(size_t i=0; i<MAX_PUBLISH_RETRY && !success; i++) {
          success = entity.publishMqttState();
          if (!success) {
            // try to increase the mqtt buffer size and try again.
            increase_mqtt_buffer();
          }
        }

        // Allow time for Home Assistant to properly 'discover' the previous entity
        #ifdef DELAY_BETWEEN_MQTT_TRANSACTIONS
        delay(DELAY_BETWEEN_MQTT_TRANSACTIONS);
        #endif
      }

      // Set device as back "online"
      // Home assistant has issue detecting the 'online' state.
      // Update the status again a few times to prevent this issue as much as possible.
      for(size_t i=0; i<3; i++) {
        this_device.publishMqttDeviceStatus(true);
        
        #ifdef DELAY_BETWEEN_MQTT_TRANSACTIONS
        delay(DELAY_BETWEEN_MQTT_TRANSACTIONS);
        #endif
      }
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

String parse_string_without_terminating_null(const uint8_t * buffer, size_t length) {
  String str;
  str.reserve(length+1);
  for(size_t i=0; i<length; i++)
    str += (char)buffer[i];
  return str;
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
  
  // Build a valid String from the first 'length' bytes of the given buffer
  String test = parse_string_without_terminating_null(buffer, length);

  // Now search using a String
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

void increase_mqtt_buffer(uint16_t new_buffer_size) {
  uint16_t current_buffer_size = mqtt_client.getBufferSize();

  // If no size is requested, double the previous buffer size
  if (new_buffer_size == 0) {
    new_buffer_size = 2*current_buffer_size;
    
    // Check for overflows
    if (new_buffer_size < current_buffer_size)
      new_buffer_size = (uint16_t)-1; // set to maximum
  }

  bool success = mqtt_client.setBufferSize(new_buffer_size);
  if (success) {
    Serial.print("PubSubClient buffer_size increased from ");
    Serial.print(current_buffer_size);
    Serial.print(" bytes to ");
    Serial.print(mqtt_client.getBufferSize());
    Serial.println(" bytes.");
  } else {
    Serial.print(String(ERROR_MESSAGE_PREFIX) + "Failed increasing PubSubClient buffer_size to ");
    Serial.print(new_buffer_size);
    Serial.print(" bytes. PubSubClient buffer_size set to ");
    Serial.println(mqtt_client.getBufferSize());
  }
}

void timer_force_timed_out(SoftTimer & timer) {
  unsigned long current_time_out_time = timer.getTimeOutTime();
  timer.setTimeOutTime(1);
  timer.reset();
  delay(2);
  timer.setTimeOutTime(current_time_out_time);
}

void setup() {
  pinMode(LED0_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.begin(115200);
  Serial.println("READY!");

  play_hello_animation();

  bell_sensor_reader.begin();

  led0.pin = LED0_PIN;
  led1.pin = LED1_PIN;

  publish_adaptor.setPubSubClient(&mqtt_client);

  HaMqttDiscovery::error_message_prefix = ERROR_MESSAGE_PREFIX;  
  
  // Force turn OFF all leds.
  for(size_t i=0; i<leds_count; i++) {
    SMART_LED & led = *leds[i];

    led.state.brightness = 255;
    led_turn_off(i); // for initializing led's internal variables
  }

  // Set default values for other entities
  bell_sensor.state.detected = false;
  bell_sensor.entity.setState("OFF");

  identify.state.is_on = false;
  identify.entity.setState("OFF");

  // Set entity's state to publish an empty payload to the command/state topic (both are identical).
  // This will 'delete' the command topic until the button is pressed again.
  test_button.state.is_pressed = false;
  test_button.entity.getState().setDirty();

  setup_melody_names();

  melody_selector.state.selected_melody = 0;
  const char * default_selected_melody_name = melody_names[melody_selector.state.selected_melody];
  Serial.print(String() + "Set default melody selector state to '" + default_selected_melody_name + "'.");
  melody_selector.entity.setState(default_selected_melody_name);

  setup_wifi();
  setup_device();
  setup_mqtt();

  // setup a timer to prevent starting a new melody
  timer_force_timed_out(doorbell_ringer_timer);
  doorbell_ringer_timer.setTimeOutTime(2500);

  // setup a timer to prevent playing the identify melody as soon as it ends.
  identify_delay_timer.setTimeOutTime(2500);
  identify_delay_timer.reset();

  // setup a timer to compute how long do we force the bell sensor to be ON.
  timer_force_timed_out(test_timer);
  test_timer.setTimeOutTime(200);
}

void loop() {
  // make sure mqtt is working
  if (!mqtt_client.connected()) {
    yield();
    mqtt_reconnect();
  }
  mqtt_client.loop();

  // Read DOORBELL pin
  if (bell_sensor_reader.toggled()) {
    if (bell_sensor_reader.read() == Button::PRESSED) {
      bell_sensor.state.detected = true;
    } else {
      bell_sensor.state.detected = false;
    }
    bell_sensor.entity.setState(bell_sensor.state.detected ? "ON" : "OFF");
  }

  // Did we pressed the TEST button?
  if (test_button.previous.is_pressed == false && test_button.state.is_pressed) {
    // Force the state of the bell sensor to ON
    bell_sensor.state.detected = true;
    bell_sensor.entity.setState(bell_sensor.state.detected ? "ON" : "OFF");

    // Start a time to stop overriding the bell sensor
    test_timer.reset();
  }
  if (test_button.state.is_pressed && test_timer.hasTimedOut()) {
    // Force the state of the bell sensor to OFF
    bell_sensor.state.detected = false;
    bell_sensor.entity.setState(bell_sensor.state.detected ? "ON" : "OFF");

    test_button.state.is_pressed = false;
  }
  test_button.previous = test_button.state; // remember previous state

  // Should we start a doorbell melody?
  if (bell_sensor.entity.getState().isDirty() &&
      bell_sensor.state.detected && // do not trigger a melody when button is released 
      melody_selector.state.selected_melody < melodies_array_count &&
      doorbell_ringer_timer.hasTimedOut() &&
      !anyrtttl::nonblocking::isPlaying())
  {
    const char * selected_melody_buffer = melodies_array[melody_selector.state.selected_melody];
    Serial.print("Playing: ");
    Serial.println(melody_names[melody_selector.state.selected_melody]);
    anyrtttl::nonblocking::begin_P(BUZZER_PIN, selected_melody_buffer);

    // Update our timer
    doorbell_ringer_timer.reset();  //start counting now
  }

  // Should we start the identify melody?
  if (identify.state.is_on &&
      identify_melody_index != INVALID_MELODY_INDEX &&
      identify_delay_timer.hasTimedOut() &&
      !anyrtttl::nonblocking::isPlaying())
  {
    const char * melody_buffer = melodies_array[identify_melody_index];
    Serial.print("Playing: ");
    Serial.println(melody_names[identify_melody_index]);
    anyrtttl::nonblocking::begin_P(BUZZER_PIN, melody_buffer);

    // Update our timer
    identify_delay_timer.reset();  //start counting now
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

      bool success = false;
      for(size_t i=0; i<MAX_PUBLISH_RETRY && !success; i++) {
        success = entity.publishMqttState();
        if (!success) {
          // try to increase the mqtt buffer size and try again.
          increase_mqtt_buffer();
        }
      }

      // Limit publishing 1 entity state per loop.
      break;
    }
  }
}
