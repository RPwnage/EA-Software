This package is a helper package that gives access to python in addition to python modules commonly used by the origin team not usually included with python.
It will call ActivePython package (with a dependency) on Windows and use the built-in python on Mac.

To use this package:

<dependency name="OriginPython"/>
<task name="OriginPython.exec" script="myscript.py" args="-a a -b b"/>

You can also use these variables if you prefer to use the <exec> yourself
$package.OriginPython.exe - path to the acceptable exe 
$package.OriginPython.pythonpath - PYTHONPATH environment variable to pass to the environment if you want access to this package's modules