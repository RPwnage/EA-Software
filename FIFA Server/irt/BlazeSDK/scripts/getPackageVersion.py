#!/usr/bin/python

# Only stdout should be version of the package

import sys, os, StringIO, getopt

opts, extraparams = getopt.getopt(sys.argv[1:],'p:b:','package=base=');

packageName = "";
base = os.getcwd();
for o,p in opts:
    if o in ['-p','--package']:
        packageName = p;
    if o in ['-b','--base']:
        base = p;
        
if(packageName == ""):
    print "Usage:\n\tpython getPackageVersion.py -b <sdkDir> -p <packageName>";
    
def getPackageVersion(xml,package):

    key = '<package name="' + package + '"';
    start = xml.find(key);
    if(start == -1):
        return None;
    start += len(key);

    key = 'version="';
    start = xml.find(key,start);
    if(start == -1):
        return None;
    start += len(key);
    
    end = start;
    while(xml[end] != '"'):
        end += 1;
    
    sweetFilling = xml[start:end];
    return sweetFilling;

base = base.rstrip('/');
masterconfigfile = base + '/masterconfig.xml';

# Open masterconfig.xml
f = open(masterconfigfile,'r');

# Be gutsy and read the entire file into mem
xml = f.read();

# EABase
version = getPackageVersion(xml,packageName);
if(version != None):
    print version;
    
f.close();