# Lunch Whistle

This project implements an automated break timer using a WeMos D1 mini ESP32 and a relay to trigger a train whistle. A small web interface allows configuration of the whistle times.

## Building

Use the Arduino IDE or PlatformIO with the ESP32 core. Copy `src/main.cpp` into your sketch and provide your WiFi credentials. Connect the relay to GPIO5 (labeled `D1` on the board).


## Usage
1. Flash the code to the WeMos D1 mini.
2. Connect the D1 mini to your WiFi network.

3. Browse to the device's IP address. The page now has a centered layout. It allows entry of comma separated times in `HH:MM` format, lets you adjust the duration of each blast, the pause between them and the timezone offset, and provides a **Test Whistle** button for manual testing without altering the schedule.
4. The configured schedule, durations and timezone offset are stored in non-volatile memory so they persist across power loss.
5. At each configured time the relay will sound two blasts using the configured lengths, pause and offset.

3. Browse to the device's IP address. The page now has a centered layout. It allows entry of comma separated times in `HH:MM` format, lets you adjust the duration of each blast and the pause between them, and provides a **Test Whistle** button for manual testing without altering the schedule.
4. The configured schedule and durations are stored in non-volatile memory so they persist across power loss.
5. At each configured time the relay will sound two blasts using the configured lengths and pause.

3. Browse to the device's IP address. The page allows entry of comma separated times in `HH:MM` format and lets you adjust the duration of each of the two whistle blasts. It also provides a **Test Whistle** button for manual testing without altering the schedule.
4. At each configured time the relay will sound two blasts using the configured lengths.

