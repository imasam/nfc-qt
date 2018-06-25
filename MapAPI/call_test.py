# coding=utf-8
# 
# test.py @ MapAPI
# Created by benjaminzhang on 2018/6/24.
# Copyright © 2018 benjaminzhang.
# All rights reserved.
from __future__ import print_function
from __future__ import unicode_literals
import json, traceback, codecs, os

import MapAPI

KEY = 'fdcdbaa4503bbc4ec96101b88febfe49'

api = MapAPI.API(KEY)

longitude_10 = None
latitude_10 = None
longitude_60 = None
latitude_60 = None

json_obj = {}
data = {}
location_gps_10 = {}
location_gps_60 = {}
location_amap = {}


def write_data(str):
    try:
        f = codecs.open('map_data.json', 'w', 'utf-8')
        f.write(str)
        f.close()
        print('File write SUCCESS!')
        os._exit(0)
    except:
        # traceback.print_exc()
        exc_type, exc_value, exc_tb = sys.exc_info()
        print(exc_type)
        print(exc_value)
        print(exc_tb)
        os._exit(-1)

try:
    import serial
    import pynmea2
    import time
    import sys

    reload(sys)
    sys.setdefaultencoding('utf-8')

    ser = serial.Serial("/dev/ttyAMA0", 9600)

    print('Getting GPS data...')
    while True:
        line = ser.readline()
        #line = '$GNRMC,034404.00,A,3640.48056,N,11707.96443,E,0.04,0.000,271017,0.0,0,D*68'
        #Dorm 9
        #line = '$GNRMC,041210.000,A,3031.7083,N,11421.1661,E,0.00,0.00,250618,,,A*74'
        if line.startswith('$GNRMC'):
            rmc = pynmea2.parse(line)
            print("Latitude:  ", float(rmc.lat) / 100)
            latitude_60 = float(rmc.lat) / 100
            print("Longitude: ", float(rmc.lon) / 100)
            longitude_60 = float(rmc.lon) / 100
            latitude_10 = (((float(rmc.lat) * 10000) % 1000000) / 10000) / 60 + int(float(rmc.lat) / 100)
            longitude_10 = (((float(rmc.lon) * 10000) % 1000000) / 10000) / 60 + int(float(rmc.lon) / 100)
            location_gps_10['latitude'] = latitude_10
            location_gps_10['longitude'] = longitude_10
            data['location_gps_10'] = location_gps_10
            location_gps_60['latitude'] = latitude_60
            location_gps_60['longitude'] = longitude_60
            data['location_gps_60'] = location_gps_60
                print('Got GPS data')
except:
    traceback.print_exc()
    json_obj['code'] = -1
    json_obj['msg'] = 'Get GPS data ERROR!'
    json_obj['data'] = {}
    json_str = json.dumps(json_obj)
    write_data(json_str)


try:
    gps_location_string = str(round(longitude_10, 6)) + ',' + str(round(latitude_10, 6))

    ret1 = api.assistant.coordinate.convert(locations=gps_location_string, coordsys='gps')
    amap_location_list = ret1.get('locations').split(',')
    amap_location_string = str(round(float(amap_location_list[0]), 6)) + ',' + str(round(float(amap_location_list[1]), 6))

    # amap_location_string = '114.358025,30.528546'
    location_amap['latitude'] = amap_location_string.split(',')[1]
    location_amap['longitude'] = amap_location_string.split(',')[0]
    data['location_amap'] = location_amap

    ret2 = api.place.around(types='150500|150600', location=amap_location_string, radius=200)
    ret3 = api.place.around(types='150700', location=amap_location_string, radius=200)

    data['metroStationCount'] = int(ret2.get('count'))
    data['busStationCount'] = int(ret3.get('count'))
    metroStationData = ret2.get('pois')
    if data.get('metroStationCount') > 0:
        for metroStation in metroStationData:
            metroStation.pop('address')
            metroStation.pop('poiweight')
            metroStation.pop('biz_ext')
            metroStation.pop('id')
            metroStation.pop('shopid')
            metroStation.pop('type')
            metroStation.pop('shopinfo')
            metroStation.pop('tel')
            metroStation.pop('biz_type')
            metroStation.pop('importance')
    busStationData = ret3.get('pois')
    if data.get('busStationCount') > 0:
        for busStation in busStationData:
            busStation.pop('address')
            busStation.pop('poiweight')
            busStation.pop('biz_ext')
            busStation.pop('id')
            busStation.pop('shopid')
            busStation.pop('type')
            busStation.pop('shopinfo')
            busStation.pop('tel')
            busStation.pop('biz_type')
            busStation.pop('importance')
    data['metroStationData'] = metroStationData
    data['busStationData'] = busStationData

    print('Call API SUCCESS!')

    json_obj['code'] = 0
    json_obj['msg'] = ''
    json_obj['data'] = data
    json_str = json.dumps(json_obj, ensure_ascii=False)
    write_data(json_str)
except:
    traceback.print_exc()
    json_obj['code'] = -2
    json_obj['msg'] = 'Calling Amap API ERROR!'
    json_obj['data'] = {}
    json_str = json.dumps(json_obj)
    write_data(json_str)
