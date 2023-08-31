#! /usr/bin/python3
'''
Use Home Assistant REST API to set presence toggle
'''
#System imports
import dbus
import sys
from datetime import datetime
from gi.repository import GLib
from dbus.mainloop.glib import DBusGMainLoop
from requests import get, post

def signal_handler(*args, **kwargs):
	base = "http://homessistant.local:8123/api"
	headers = {
		"Authorization": "Bearer <insert your Long-Lived Access Token here>",
		"content-type": "application/json",
	}
	data = {"entity_id": "input_boolean.item_to_toggle"}
	
	if args[0]:
		sys.stdout.flush()
		url = f"{base}/services/input_boolean/turn_off"
		response = post(url, headers=headers, json=data)
        if (response.status_code == 200):
			print("screensaverwatch: Set presence toggle to 'Off'")
	else:
		sys.stdout.flush()
		url = f"{base}/services/input_boolean/turn_on"
		response = post(url, headers=headers, json=data)
        if (response.status_code == 200):
			print("screensaverwatch: Set presence toggle to 'On'")

def main():
    DBusGMainLoop(set_as_default=True)
    bus = dbus.SessionBus()

    #register your signal callback
    bus.add_signal_receiver(signal_handler,
	        				bus_name='org.xfce.ScreenSaver',
			        		interface_keyword='org.xfce.ScreenSaver',
					        member_keyword='ActiveChanged')

    loop = GLib.MainLoop()

    try:
	    loop.run()
    except (KeyboardInterrupt):
	    time = datetime.now()
    	print("screensaverwatch: exiting on keyboard interrupt")
	    sys.stdout.flush()

if __name__ == "__main__":
	main()
