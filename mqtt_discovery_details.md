The following document describes the required MQTT topics for enabling _Home Assistant's MQTT discovery_ feature.

## MQTT transactions
 
The following topics are published to allow the device to be detected by Home Assistant:

**homeassistant/binary_sensor/doorbell-97BC_binary_sensor0/config :**

```json
{
  "device_class": "sound",
  "name": "Bell",
  "unique_id": "doorbell-97BC_binary_sensor0",
  "object_id": "doorbell-97BC_Bell",
  "state_topic": "doorbell-97BC/doorbell/state",
  "availability_topic": "doorbell-97BC/status",
  "payload_available": "online",
  "payload_not_available": "offline",
  "device": {
    "identifiers": [
      "doorbell-97BC"
    ],
    "name": "Smart doorbell 97BC",
    "manufacturer": "end2endzone",
    "model": "ESP8266",
    "hw_version": "1.0",
    "sw_version": "0.1"
  }
}
```

**homeassistant/button/doorbell-97BC_button0/config :**

```json
{
  "name": "Test",
  "unique_id": "doorbell-97BC_button0",
  "object_id": "doorbell-97BC_Test",
  "command_topic": "doorbell-97BC/test/set",
  "state_topic": "doorbell-97BC/test/set",
  "availability_topic": "doorbell-97BC/status",
  "payload_available": "online",
  "payload_not_available": "offline",
  "device": {
    "identifiers": [
      "doorbell-97BC"
    ],
    "name": "Smart doorbell 97BC",
    "manufacturer": "end2endzone",
    "model": "ESP8266",
    "hw_version": "1.0",
    "sw_version": "0.1"
  }
}
```

**homeassistant/select/doorbell-97BC_select0/config :**

```json
{
  "options": [
    "None",
    "Beethoven Fifth Symphony",
    "Coca Cola",
    "Duke Nukem (short)",
    "Entertaine (short)",
    "Flintstones (short)",
    "Intel",
    "Mission Impossible - Intro",
    "Mission Impossible (short)",
    "Mosaic-long",
    "Nokia",
    "Pacman",
    "Popeye (short)",
    "Star Wars - Cantina (short)",
    "Star Wars - Imperial March (short)",
    "Star Wars (short)",
    "Super Mario Bros. 1 (short)",
    "Super Mario Bros. 3 Level 1 (short)",
    "Super Mario Bros. Death",
    "Sweet Child",
    "The Good the Bad and the Ugly (short)",
    "The Itchy Scratchy Show",
    "Tones",
    "Trio",
    "Wolf Whistle",
    "X-Files (short)"
  ],
  "name": "Melody",
  "unique_id": "doorbell-97BC_select0",
  "object_id": "doorbell-97BC_Melody",
  "command_topic": "doorbell-97BC/melody/set",
  "state_topic": "doorbell-97BC/melody/state",
  "availability_topic": "doorbell-97BC/status",
  "payload_available": "online",
  "payload_not_available": "offline",
  "device": {
    "identifiers": [
      "doorbell-97BC"
    ],
    "name": "Smart doorbell 97BC",
    "manufacturer": "end2endzone",
    "model": "ESP8266",
    "hw_version": "1.0",
    "sw_version": "0.1"
  }
}
```

**homeassistant/switch/doorbell-97BC_switch0/config :**

```json
{
  "device_class": "switch",
  "name": "Identify",
  "unique_id": "doorbell-97BC_switch0",
  "object_id": "doorbell-97BC_Identify",
  "command_topic": "doorbell-97BC/identify/set",
  "state_topic": "doorbell-97BC/identify/state",
  "availability_topic": "doorbell-97BC/status",
  "payload_available": "online",
  "payload_not_available": "offline",
  "device": {
    "identifiers": [
      "doorbell-97BC"
    ],
    "name": "Smart doorbell 97BC",
    "manufacturer": "end2endzone",
    "model": "ESP8266",
    "hw_version": "1.0",
    "sw_version": "0.1"
  }
}
```
