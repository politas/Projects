import time
from math import ceil, floor

from machine import SPI, Pin
import sdcard, uos

from PiicoDev_SSD1306 import *
from PiicoDev_BME280 import PiicoDev_BME280
from PiicoDev_Unified import sleep_ms # cross-platform compatible sleep function

#Use Adafruit PiCowbell Adalogger JST socket I2C pins and SDCard SPI pins
SDA = 4
SCL = 5
SCK = 18
MOSI = 19
MISO = 16
CS = 17

FNAME = '/sd/bme280-readings.csv'

def ftime():
    year, month, mday, hour, minute, second, weekday, yearday = time.localtime()
    return f"{year:04}-{month:02}-{mday:02} {hour:02}:{minute:02}:{second:02}"

def fdate():
    year, month, mday, hour, minute, second, weekday, yearday = time.localtime()
    return f"{year:04}-{month:02}-{mday:02}"

class TemperatureWindow(object):
    """Creates a list of temperature readings with a defined size"""
    def __init__(self, size):
        self.Readings = []
        self.Size = size
    
    def __iter__(self):
        for val in self.Readings:
            yield val
    
    def add(self,temperature) -> None:
        """Adds a new temperature to the list
        
        Checks current list size and removes excess readings so that the list
        is not larger than the defined size when adding the new temperature. """
        while (len(self.Readings) >= self.Size):
            self.remove_first()
        self.Readings.append(temperature)
    
    def remove_first(self) -> None:
        """Removes first entry from the Readings list"""
        self.Readings.pop(0)
    
    def maxim(self) -> int:
        """Returns an integer that is above the highest stored value in Readings """
        return ceil(max(self.Readings))
    
    def minim(self) -> int:
        """Returns an integer that is below the lowest stored value in Readings """
        return floor(min(self.Readings))
    
    def last(self) -> float:
        """Returns the last value in the Readings list."""
        return self.Readings[-1]

def create_graph(display,max_value,min_value):
    """Creates a new graph with the provided minimum and maximum values."""
    graph = display.graph2D(originX=36,originY=62,width=90,height=61,minValue=min_value,maxValue=max_value,c=1)
    display.fill_rect(0,0,128,64,0)
    display.rect(35,0,93,64,1)
    display.text("{:}".format(max_value), 0,0, 1) 
    display.text("{:}".format(min_value), 0,56, 1) 
    display.show()
    return graph

def write_reading(reading):
    """Appends a time-stamped data reading to a CSV file on the SDCard."""
    temperature, pressure, humidity = reading
    with open (FNAME, "a") as f:
        f.write(ftime())
        f.write(",")
        f.write(str(temperature))
        f.write(",")
        f.write(str(pressure))
        f.write(",")
        f.write(str(humidity))
        f.write('\n') # A new line
        f.flush() # Force writing of buffered data to the SD card

def read_display_store(display,graph,readings,sensor):
    """Core loop continues taking readings while temperature remains within current graph boundaries."""
    max_value = readings.maxim()
    min_value = readings.minim()
    while (readings.maxim() == max_value) and (readings.minim() == min_value):
        reading = sensor.values()
        readings.add(reading[0])
        write_reading(reading)
        print(f"{ftime()} - {readings.last()}")
        display.fill_rect(36,1,90,62,0)
        display.updateGraph2D(graph,readings.last())
        display.fill_rect(0,28,35,10,1) # white box for current reading display
        display.text("{:00.1f}".format(readings.last()), 1,29,0)
        display.show()
        time.sleep(60.0)

def graph_old_values(display,graph,readings):
    """Redraws graph with old values when graph boundaries have changed."""
    for val in readings:
        display.fill_rect(36,1,90,62,0)
        display.updateGraph2D(graph,val)
    display.fill_rect(0,28,35,10,1) # white box for current reading display
    display.text("{:00.1f}".format(readings.last()), 1,29,0)
    display.show()
    
def main():
    # Initialise SDCard on Adafruit PiCowbell
    spi = SPI(0,sck=Pin(SCK), mosi=Pin(MOSI), miso=Pin(MISO))
    cs = Pin(CS)
    sd = sdcard.SDCard(spi, cs)
    uos.mount(sd, '/sd')
    print(uos.listdir('/sd'))

    # Initialise PiicoDev Display and Pimoroni BME280 sensor using Piicodev modules
    display = create_PiicoDev_SSD1306(bus=0,sda=machine.Pin(SDA),scl=machine.Pin(SCL))
    sensor = PiicoDev_BME280(bus=0,sda=machine.Pin(SDA),scl=machine.Pin(SCL),address=0x76)
    
    print(ftime())
    # Throw away initial reading from BME280, which is always wrong
    _ = sensor.values()
    time.sleep(1.0)
    
    # Setup list of temperatures so we can redraw the graph when temperatures cross the boundaries.
    readings = TemperatureWindow(90)
    # Get an initial reading to set the starting graph range
    reading = sensor.values()
    readings.add(reading[0])
    
    graph = create_graph(display,readings.maxim(),readings.minim())
    while True:
        read_display_store(display,graph,readings,sensor)
        graph = create_graph(display,readings.maxim(),readings.minim())
        graph_old_values(display,graph,readings)

if __name__ == "__main__":
	main()
