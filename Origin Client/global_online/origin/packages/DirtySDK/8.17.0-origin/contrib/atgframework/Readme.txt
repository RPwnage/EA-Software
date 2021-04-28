This directory contains build framewok files required to build the Microsoft's "common framework" sample code, 
that is used by the DirtySDK Xenon samples.

These Microsoft source files and header files used to be archived directly in this directory and updated 
each time a new XDK was supported. It is not the case anymore. The build framework files are now referring
directly to the Microsoft XDK directory, thus eliminating the need for archiving the Microsoft
files in our depot. However, there is still a need to add/remove files from project/xenon/source.txt each time 
the supported XDK version is refreshed.
