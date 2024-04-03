# Clever Clock
![IMG_20210413_090921__01_crop](https://github.com/politas/Projects/assets/2109967/183e2c67-d30d-4a14-a87a-eef30bd32683)

This project is put together using three "Feather" boards from Adafruit, in a custom 3D printed case along with a silvered perspex window cut down from an old clock radio.

 - [HUZZAH32 - ESP32 Feather](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/overview)
 - [Adafruit 0.56" 4-Digit 7-Segment FeatherWing Display](https://www.adafruit.com/product/3108)
 - [FeatherWing Proto](https://www.adafruit.com/product/2884)

![Clever Clock internals](https://github.com/politas/Projects/assets/2109967/6f45ad7e-7451-450b-9384-0488bc662a1e)
 
It also uses an IR distance sensor I had lying around, which is connected to power along with a pullup resister on the protoboard in between the HUZZAH32 and the Featherwing display, with the data pin connected to PIN34 on the ESP32.

[ESPHome](https://esphome.io) is installed on the HUZZAH32 to allow it to work with Home Assistant easily. It previously had heavily customised C++ code which embedded much of the desired functionality in concert with Openhab - See [clock_openhab](https://github.com/politas/Projects/clock_openhab) If that's what you're after.

The [ESPHome configuration file](https://github.com/politas/Projects/clock_esphome/clever-clock.yaml) here Looks for two entities from the host Home Assistant server:

- ``sensor.clever_clock_time`` - This should be a template sensor using ``timestamp_custom('%H%M')`` to provide 24-hour time as a simple number. Zero padding is done in the lambda that sets the display.
- ``input_number.clever_clock_brightness`` - Numeric value from 0 to 10 which controls the brightness level of the LED display.

What worked for me to get the external libraries installed was to download the entire libraries into a subdirectory on my Home Assistant server and include all the needed files.

IF you are just wanting a clock, with no presence sensor, You'll have no need for the ``platform: adc`` sensor or the ``binary_sensor`` block, and if you are connecting up some kind of presence sensor, you will need to replace the adc sensor and the binary sensor's lambda with details for your presence detector.