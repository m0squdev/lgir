# lgir
Change input source and backlight of LG TVs via an ESP8266.
The IR blaster can be controlled both by using the encoder and by sending HTTP requests to the microcontroller.

## Setup
Create the file `wifi-config.h` and define `WIFI_SSID` and `WIFI_PASSWORD` in it.
Connect the ESP8266 to an encoder and an IR blaster, upload the code and you're done!

## Controls
On startup, press the switch within 5 seconds to navigate to the backlight entry in the menu. You need to do this every time you restart your TV.
After the startup:
- Use the switch to switch input source.
- Use the knob to adjust the backlight. Once you're done, press the switch to exit the menu.
