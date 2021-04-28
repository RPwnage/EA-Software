Some packages include various items that are not
Unfortunately the ugly solution to this is to stick the items here and change the pathing in the modules to point to this folder. Usually the packages seperate the code that searches for these files in their packages so they're easy to spot and edit.

Files changed:
requests/certs.py
dateutil/zoneinfo/__init__.py