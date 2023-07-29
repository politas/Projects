# Clever Clock
![IMG_20210413_090921__01_crop](https://github.com/politas/Projects/assets/2109967/183e2c67-d30d-4a14-a87a-eef30bd32683)

This project is put together using three "Feather" boards from Adafruit, in a custom 3D printed case along with a silvered perspex window cut down from an old clock radio.

 - [HUZZAH32 - ESP32 Feather](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/overview)
 - [Adafruit 0.56" 4-Digit 7-Segment FeatherWing Display](https://www.adafruit.com/product/3108)
 - [FeatherWing Proto](https://www.adafruit.com/product/2884)

![Clever Clock internals](https://github.com/politas/Projects/assets/2109967/6f45ad7e-7451-450b-9384-0488bc662a1e)
 
It also uses an IR distance sensor I had lying around, which is connected to power along with a pullup resister on the protoboard in between the HUZZAH32 and the Featherwing display, with the data pin connected to PIN34 on the ESP32.

This code makes heavy use of the [example code](https://github.com/adafruit/Adafruit_LED_Backpack/tree/master/examples/clock_sevenseg_ds1307) provided by Adafruit for the [LED Backpack](https://github.com/adafruit/Adafruit_LED_Backpack).

Most of the code is just to keep the clock display running. Apart from that it does the following:

 - Automatically dims the screen according to time of day.
 - Sets the time ahead ten minutes for the two hours before a work shift.
 - Triggers a pair of scenes to toggle on and off some lights when the distance sensor detects an object nearby (such as a waved hand).

It also relies on custom jython code added to my OpenHab instance, which is not included, but can be inferred based on the REST API items used. Your requirements are likely quite different:

 - Is_Dark - True when the sun is down, False during daylight
 - Is_Work_Day - Set to True around 10:00pm when a calender entry exists for the next day indicating a daytime work shift. Set to false after start of shift.
 - Is_Work_Night - Set to True around 10:00am when a calender entry exists for the upcoming evening indicating a nighttime work shift. Set to false after start of shift.
 - Light_object - One of the controllable lights that is to be controlled by the distance sensor.
 - Lighting_Scene - openhab Item which triggers a change in various entities when set to a named scene. This is the most complicated jython code dependancy, which I wrote due to OpenHab not having built-in scenes with adjustable transition times.
