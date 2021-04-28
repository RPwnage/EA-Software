#!/usr/bin/env python
import logging

def default_logger():
    logger = logging.getLogger("accesscontrol")
    logger.setLevel(logging.INFO)
    ch = logging.StreamHandler()
    ch.setLevel(logging.INFO)
    formatter = logging.Formatter("%(asctime)s:%(name)s:%(levelname)s:%(message)s")
    ch.setFormatter(formatter)
    logger.addHandler(ch)
    return logger


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
    
    
