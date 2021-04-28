#!/usr/bin/env python

class NetworkError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)
        
class AccessControlError(Exception):
    def __init__(self, code, value, msg):
        self.code = code
        self.value = value
        self.msg = msg
    def __str__(self):
        return repr('%s (%s:%s)' % (self.msg, self.code, self.value))