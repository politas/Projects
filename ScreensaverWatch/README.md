# Screensaver Watch
Licensed under the [Unlicense](./LICENSE)

watch_screensaver.py 

This program is designed for an XFCE desktop environment and a Homeassistant server which has a toggle helper to indicate presence in the room where the computer is located. It turns off the specified input_boolean entity when the screensaver is activated, and turns it off when the screensaver is deactivated. 

Very handy if you have a heater and a thermometer rigged up to keep things warm when you pop into your office for some late-night coding.

Also included is a systemd service file you can put in ~/.config/systemd/user and enable with

```
systemd --user enable screensaverwatch.service
```

Obviously, you'll need to make sure you have the program location correct in the service file.