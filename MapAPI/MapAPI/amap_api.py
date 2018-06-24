# coding=utf-8
# 
# amap_api.py @ MapAPI
# Created by benjaminzhang on 2018/6/20.
# Copyright © 2018 benjaminzhang.
# All rights reserved.

__all__ = ['APIError', 'API']

DEBUG_LEVEL = 1

import json
import socket
import sys
import time
import urllib2
from collections import Iterable


class APIError(Exception):
    code = None
    """HTTP status code"""

    url = None
    """request URL"""

    body = None
    """server response body; or detailed error information"""

    def __init__(self, code, url, body):
        self.code = code
        self.url = url
        self.body = body

    def __str__(self):
        return 'code={s.code}\nurl={s.url}\n{s.body}'.format(s=self)

    __repr__ = __str__


class API(object):
    key = None
    server = 'http://restapi.amap.com/v3/'

    decode_result = True
    timeout = None
    max_retries = None
    retry_delay = None

    def __init__(self, key, srv=None,
                 decode_result=True, timeout=3, max_retries=3,
                 retry_delay=1):
        """:param srv: The MapAPI server address
        :param decode_result: whether to json_decode the result
        :param timeout: HTTP request timeout in seconds
        :param max_retries: maximal number of retries after catching URL error
            or socket error
        :param retry_delay: time to sleep before retrying"""
        self.key = key
        if srv:
            self.server = srv
        self.decode_result = decode_result
        assert timeout >= 0 or timeout is None
        assert max_retries >= 0
        self.timeout = timeout
        self.max_retries = max_retries
        self.retry_delay = retry_delay

        _setup_apiobj(self, self, [])

    def update_request(self, request):
        """overwrite this function to update the request before sending it to
        server"""
        pass


def _setup_apiobj(self, api, path):
    if self is not api:
        self._api = api
        self._urlbase = api.server + '/'.join(path)

    lvl = len(path)
    done = set()
    for i in _APIS:
        if len(i) <= lvl:
            continue
        cur = i[lvl]
        if i[:lvl] == path and cur not in done:
            done.add(cur)
            setattr(self, cur, _APIProxy(api, i[:lvl + 1]))


class _APIProxy(object):
    _api = None
    """underlying :class:`MapAPI` object"""

    _urlbase = None

    def __init__(self, api, path):
        _setup_apiobj(self, api, path)

    def __call__(self, *args, **kargs):
        if len(args):
            raise TypeError('Only keyword arguments are allowed')

        url = self._urlbase + '?'
        for k, v in self._mkarg(kargs).iteritems():
            url += k + '=' + v + '&'

        request = urllib2.Request(url)
        request.add_header('Content-type', 'application/json')
        # request.add_header('Content-length', str(len(body)))

        self._api.update_request(request)

        retry = self._api.max_retries
        while True:
            retry -= 1
            try:
                ret = urllib2.urlopen(request, timeout=self._api.timeout).read()
                break
            except urllib2.HTTPError as e:
                raise APIError(e.code, url, e.read())
            except (socket.error, urllib2.URLError) as e:
                if retry < 0:
                    raise e
                _print_debug('caught error: {}; retrying'.format(e))
                time.sleep(self._api.retry_delay)

        if self._api.decode_result:
            try:
                ret = json.loads(ret)
            except:
                raise APIError(-1, url, 'json decode error, value={0!r}'.format(ret))
        return ret

    def _mkarg(self, kargs):
        """change the argument list (encode value, add api key)
        :return: the new argument list"""

        def enc(x):
            if isinstance(x, unicode):
                return x.encode('utf-8')
            return str(x)

        kargs = kargs.copy()
        kargs['key'] = self._api.key
        for (k, v) in kargs.items():
            if isinstance(v, Iterable) and not isinstance(v, basestring):
                kargs[k] = ','.join([enc(i) for i in v])
            elif v is None:
                del kargs[k]
            else:
                kargs[k] = enc(v)

        return kargs


def _print_debug(msg):
    if DEBUG_LEVEL:
        sys.stderr.write(str(msg) + '\n')


_APIS = [
    '/ip',
    '/geocode/regeo',
    '/place/around',
    '/assistant/coordinate/convert'
]

_APIS = [i.split('/')[1:] for i in _APIS]
