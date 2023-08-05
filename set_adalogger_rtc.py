import time
import urtc

from machine import I2C, Pin
import sdcard, uos

def ftime():
    year, month, mday, hour, minute, second, weekday, yearday = time.localtime()
    return f"{year:04}-{month:02}-{mday:02} {hour:02}:{minute:02}:{second:02}"

def ftime_rtc(rtc):
    year, month, mday, weekday, hour, minute, second, millisecond = rtc.datetime()
    return f"{year:04}-{month:02}-{mday:02} {hour:02}:{minute:02}:{second:02}"

def set_rtc_time(rtc):
    year, month, mday, hour, minute, second, weekday, yearday = time.localtime()
    datetime = urtc.datetime_tuple(year=year, month=month, day=mday, weekday=weekday, hour=hour, minute=minute, second=second, millisecond=0)
    rtc.datetime(datetime)

i2c = I2C(0,scl=Pin(5), sda=Pin(4))
rtc = urtc.PCF8523(i2c)

print(ftime())
print(ftime_rtc(rtc))

set_rtc_time(rtc)

print(ftime())
print(ftime_rtc(rtc))