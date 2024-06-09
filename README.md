![doorbell logo](pics/featured_image.jpg?raw=true)


# smart-doorbell-homeassistant
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Github Releases](https://img.shields.io/github/release/end2endzone/smart-doorbell-homeassistant.svg)](https://github.com/end2endzone/smart-doorbell-homeassistant/releases)

A project to convert a traditional electromagnetic chime doorbell into a smart doorbell compatible with Home Assistant.



# Purpose #

I do have a traditional electromagnetic chime doorbell ([how it works](https://home.howstuffworks.com/home-improvement/repair/doorbell3.htm)) already working at my house. We sometimes miss when people ring on the doorbell. It could be because we are downstairs watching a movie. But the chime is also weak and some times, we also miss it when the house is mostly silent. It could also be cultural but where I live, people usually press the button once and wait. I was looking for a solution to this problem.

There are a lot of smart doorbell on the market. Most of them can be easily integrated with smart home softwares. However, most of them requires to be connected to the internet and not subscribtion free. I favor privacy. I wanted to take this opportunity to go DIY and learn from this.

This project allowed me to learn so much new things. I learned to use the [ESP8266 microcontroller](https://en.wikipedia.org/wiki/ESP8266), learned about [MQTT protocol](https://mqtt.org/) and [Home Assistant's MQTT discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery).



# Features #

The main features of this smart doorbell are:

* Integrates easily with existing hardware.
  * The integration does not alter the original hardware.
  * The doorbell continues to work as usual.
* Audio feedback from an [electric piezo buzzer](https://www.google.com/search?q=piezo+buzzer).
  * Play short [RTTTL melodies](https://www.youtube.com/results?search_query=arduino+rtttl+melodies) when the doorbell rings.
  * User selectable melodies from [a great list of melodies](https://github.com/end2endzone/smart-doorbell-homeassistant/blob/main/src/doorbell/doorbell.ino#L129-L156) or integrate [more melodies](https://github.com/end2endzone/smart-doorbell-homeassistant/blob/main/src/doorbell/rtttl_melodies.txt) or [more ringtones](https://github.com/end2endzone/smart-doorbell-homeassistant/blob/main/src/doorbell/rtttl_ringtones.txt).
  * Play a looping RTTTL melody for device physical location identifier.
* Integrates with [Home Assistant](https://www.home-assistant.io/) through [MQTT protocol](https://mqtt.org/) taking advantage of [Home Assistant's MQTT discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) feature.
* Support for multiple devices running simultaneously on the same network.
  * The doorbell identify uniquely on the network using the last 4 digits of its MAC address.
  * Turn all physical doorbell chimes in your house into smart doorbell.
* Support for manual trigger with a physical button or with an "home assistant button" to trigger ringing detection and reporting.



# Requirements #

* A [Home Assistant](https://www.home-assistant.io/) instance.
* An existing [MQTT broker](https://aws.amazon.com/what-is/mqtt/#seo-faq-pairs#what-are-mqtt-comp) service running such as [mosquitto](https://mosquitto.org/).



# Components

The following components are required for the project:

* 1x [50mm x 70mm double sided perfboard](https://www.amazon.ca/s?k=5x7+double+sided+perfboard)
* 1x [ESP8266 microcontroller](https://www.amazon.ca/s?k=ESP8266+NodeMCU+microcontroller)
* 1x [momentary tactile tact push button](https://www.amazon.ca/s?k=Momentary+Tactile+Tact+Push+Button)
* 1x [NPN 2N2222 transistor](https://www.amazon.ca/s?k=NPM+2N2222+transistor)
* 1x [1K Ohm Carbon Film Resistor](https://www.amazon.ca/s?k=1K+Ohm+Carbon+Film+Resistor)
* 3x [2pin screw terminal block connector](https://www.amazon.ca/s?k=2pin+screw+terminal+block+connector)
* 1x [Glass Tube Reed Switch (normally open)](https://www.amazon.ca/s?k=Glass+Tube+Reed+Switch+normally+open)
* 1x [Piezo Electric Buzzer Alarm](https://www.amazon.ca/s?k=Piezo+Electric+Buzzer+Alarm)
* 1x [AC 12V to DC 5V Buck Voltage Regulator Step Down Converter](https://www.amazon.ca/s?k=AC+12V+24V+to+DC+5V+Buck+Voltage+Regulator+Step+Down+Converter)
* 1x [Step Up Module UPS Uninterruptible Power Supply for 18650 Lithium Battery](https://www.amazon.ca/s?k=18650+Lithium+Battery+Boost+Step+Up+Module+UPS+Uninterruptible+Power+Supply)
* 1x [18500 rechargeable li-ion battery](https://www.amazon.ca/s?k=18500+rechargeable+li-ion+battery)


# Circuit Diagram

![](Smart%20Doorbell%20(breadboard).png?raw=true)
![](Smart%20Doorbell%20(perfboard).png?raw=true)



# How it works

## Integration with Home Assistant
The device is exposed through [Home Assistant's MQTT discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery). Home Assistant detects the device automatically and adds it to the dashboard:

![](pics/home%20assistant%20dashboard%20card.png?raw=true)


## Power

The circuit is powered from the existing 12V AC power of the existing doorbell circuitry.

A battery connected to an Uninterruptible Power Supply (UPS) circuit is required when someone rings the doorbell. When ringing, the chime electromagnet is energized temporary cutting the 12V AC power to the step down converter. Without the UPS, the ESP8266 reboots everytime someone rings. The 18650 battery is powering the device when they do.


## Signal detection

When ringing, the chime electromagnet is energized. The electromagnetic field is picked up by the reed switch which trigger an interrupt in the ESP8266. The reed switch is held in place using sticky tack putty.

A tactile push button is connected in parallel to the reed switch to manually trigger the "ringing event" within the microcontroller. 


## MQTT transactions
 
The device is exposed through a single root topic. This topic is formatted as `doorbell-[4-LAST-BYTES-OF-MAC-ADDRESS]`. For example `doorbell-97BC`.

The device outputs its states and commands to the following topics:

| Topics                       | Value  |
|------------------------------|--------|
| doorbell-97BC/doorbell/state | OFF    |
| doorbell-97BC/identify/state | OFF    |
| doorbell-97BC/melody/state   | None   |
| doorbell-97BC/status         | online |
| doorbell-97BC/test/set       | OFF    |

The device exposes the following entities to _Home Assistant_:
* _binary sensor_, to get the state of the bell ring.
* _switch_, to enable identify mode which plays an audio tone. 
* _select_, to allow user selection of the melody played when the bell is detected.
* _button_, to force the device to report a bell detection for testing purposes.

Each entity names is prefixed with `doorbell-97BC_` allowing unique names.

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



# Pictures

[![](pics/thumbnails/IMG_20240519_131053.jpg?raw=true)](pics/IMG_20240519_131053.jpg)
[![](pics/thumbnails/IMG_20240519_131223.jpg?raw=true)](pics/IMG_20240519_131223.jpg)
[![](pics/thumbnails/IMG_20240519_131322.jpg?raw=true)](pics/IMG_20240519_131322.jpg)
[![](pics/thumbnails/IMG_20240519_131329.jpg?raw=true)](pics/IMG_20240519_131329.jpg)
[![](pics/thumbnails/IMG_20240519_131541.jpg?raw=true)](pics/IMG_20240519_131541.jpg)



# Authors #

* **Antoine Beauchamp** - *Initial work* - [end2endzone](https://github.com/end2endzone)



# License #

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details
