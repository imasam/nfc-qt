# Documentation of MapAPI

> doc.md @ MapAPI  
> Created by benjaminzhang on 2018/6/20.  
> Copyright © 2018 benjaminzhang.  
> All rights reserved.

## IP定位

IP定位是一个简单的HTTP接口，根据用户输入的IP地址，能够快速的帮用户定位IP的所在位置。

[文档](http://lbs.amap.com/api/webservice/guide/api/ipconfig)

## 逆地理编码

将经纬度转换为详细结构化的地址，且返回附近周边的POI、AOI信息。  
例如：116.480881,39.989410 转换地址描述后：北京市朝阳区阜通东大街6号

[文档](http://lbs.amap.com/api/webservice/guide/api/georegeo)

## 坐标转换

坐标转换是一类简单的HTTP接口，能够将用户输入的非高德坐标（GPS坐标、mapbar坐标、baidu坐标）转换成高德坐标。

[文档](http://lbs.amap.com/api/webservice/guide/api/convert)

## 搜索

搜索服务API是一类简单的HTTP接口，提供多种查询POI信息的能力，其中包括关键字搜索、周边搜索、多边形搜索、ID查询四种筛选机制。

[文档](http://lbs.amap.com/api/webservice/guide/api/search)

- 关键字搜索：通过用POI的关键字进行条件搜索，例如：肯德基、朝阳公园等；同时支持设置POI类型搜索，例如：银行
- 周边搜索：在用户传入经纬度坐标点附近，在设定的范围内，按照关键字或POI类型搜索；
- 多边形搜索：在多边形区域内进行搜索
- ID查询：通过POI ID，查询某个POI详情，建议可同输入提示API配合使用

## Demo

``` python
# coding=utf-8
# 
# call_test.py @ MapAPI
# Created by benjaminzhang on 2018/6/20.
# Copyright © 2018 benjaminzhang.
# All rights reserved.

import MapAPI

KEY = 'fdcdbaa4503bbc4ec96101b88febfe49'

api = MapAPI.API(KEY)

# 逆地理编码
ret = api.geocode.regeo(location='114.354941,30.526491', extensions='all')
# IP定位
ret = api.ip()
# print ret
# {u'status': u'1', u'info': u'OK', u'province': u'\u6e56\u5317\u7701', u'city': u'\u6b66\u6c49\u5e02', u'adcode': u'420100', u'infocode': u'10000', u'rectangle': u'114.0169501,30.36460893;114.6261227,30.77017755'}

# 坐标转换
ret = api.assistant.coordinate.convert(locations='116.481499,39.990475', coordsys='gps')
# 搜索
ret = api.place.around(location='114.355167,30.526417', extensions='all')

# ret 是dict类型