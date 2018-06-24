import serial
import pynmea2
import time

ser = serial.Serial("/dev/ttyAMA0",9600)

longitude = None
latitude = None

while True:
    line = ser.readline()
    if line.startswith('$GNRMC'):
        rmc = pynmea2.parse(line)
        latitude = float(rmc.lat)/100
        longitude = float(rmc.lon)/100
        latitude_dec = (((float(rmc.lat)*10000)%1000000)/10000)/60+int(float(rmc.lat)/100)
        longitude_dec = (((float(rmc.lon)*10000)%1000000)/10000)/60+int(float(rmc.lon)/100)
        print "Latitude in 10:  ", latitude_dec
        
        print "Longitude in 10: ", longitude_dec

        print "Latitude in 60:  ", latitude
        
        print "Longitude in 60: ", longitude
        
        print('Got GPS data...')
        break
