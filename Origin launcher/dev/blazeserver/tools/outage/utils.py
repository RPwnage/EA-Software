#!/usr/bin/env python
import logging
import re
from errors import InputError, BlazeError

MULT = {'d':86400, 'h':3600, 'm':60, 's':1}

def default_logger():
    logger = logging.getLogger("outage")
    logger.setLevel(logging.INFO)
    ch = logging.StreamHandler()
    ch.setLevel(logging.INFO)
    formatter = logging.Formatter("%(asctime)s:%(name)s:%(levelname)s:%(message)s")
    ch.setFormatter(formatter)
    logger.addHandler(ch)
    return logger

def time_to_secs(timestr):
    pat = re.compile(r'(\d+)([dhms])')
    group = pat.search(timestr)
    if group:
        m = MULT[group.group(2)]
        return int(group.group(1)) * m
    else:
        raise InputError('Invalid input %s' % timestr)

def secs_to_fractions(secs):
    years, s = divmod(secs, 31556952)
    min, s = divmod(s, 60)
    h, min = divmod(min, 60)
    d, h = divmod(h, 24)
    str = '%i days %ih:%im:%is' % (d, h, min, s)
    return str
    
def get_blaze_error(dom):
    elements = dom.getElementsByTagName('errorCode')
    if len(elements) > 0:
        errorCode = elements[0].childNodes[0].nodeValue
        errorName = 'Unknown'
        elements = dom.getElementsByTagName('errorName')
        if len(elements) > 0:
            errorName = elements[0].childNodes[0].nodeValue
        raise BlazeError(errorCode, errorName,'')
''
class TablePrinter:
    def __init__(self, fields):
        self.fields = fields
        self.field_length = []
        self.rows = []
        for f in range(0, len(fields)):
            self.field_length.append(len(fields[f])+2)
        self.total_length = sum(self.field_length)
                
    def addrow(self, row):
        self.rows.append(row)
        for r in range(0, len(row)):
            if self.field_length[r] < len(row[r]):
                self.field_length[r] = len(row[r]) + 2
        self.total_length = sum(self.field_length)
                
    def print_table(self):
        print self._row_to_string(self.fields)
        sep = self._row_sep()
        print sep
        for r in self.rows:
            print self._row_to_string(r)
        print ""
    
    def _row_to_string(self, row):
        rowstring = []
        for r in range(0,len(row)):
            rowstring.append(str(row[r]).center(self.field_length[r]))
        
        return "|".join(rowstring)
        
    def _row_sep(self):
        row = ""
        for i in range(0,self.total_length):
            row += '-'
        return row
    
    
