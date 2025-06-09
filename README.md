# Lunch Whistle

This project implements an automated break timer using a WeMos D1 mini (ESP8266) and a relay to trigger a train whistle. A small web interface allows configuration of the whistle times.

## Building
Use the Arduino IDE or PlatformIO with the ESP8266 core. Copy `src/main.cpp` into your sketch and provide your WiFi credentials.

## Usage
1. Flash the code to the WeMos D1 mini.
2. Connect the D1 mini to your WiFi network.
3. Browse to the device's IP address. The web page lets you enter comma separated times in `HH:MM` format.
4. At each configured time the relay will activate for one second, sounding the whistle.
