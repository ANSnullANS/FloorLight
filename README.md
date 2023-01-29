# FloorLight

Arduino-Nano based automatic LED-Light with passive IR for motion detection.

I'm using it to have some minimal light outside the bedroom in case I got to step out ;)

# BOM
- Arduino Nano
- 2x LED Stripe (WS28xx), 10cm - everything compatible with Adafruit Neopixel Library will do
- 2x SR602 PIR Sensor Module
- 1x DS3231M RTC Module - used for calculating Sunrise/Sunset
- 1x 5V 1,5A (USB-)Powersupply
- 1x USB-A to Mini-USB Cable, ~15cm (90° angled connectors preferably)
- Breadboard
- Wires

Optional:
- 3D Printer if you want some nice case (STL is made for the powersupply I had lying around)

# Credits
FloorLight is using following Libraries - Thanks to their respective creators!

- RTCLib by Adafruit (https://github.com/adafruit/RTClib)
- Dusk2Dawn by DM Kishi (https://github.com/dmkishi/Dusk2Dawn)
- NeoPixel by Adafruit (https://github.com/adafruit/Adafruit_NeoPixel)
