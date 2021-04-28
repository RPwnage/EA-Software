This is a simple single-file patch test. It doesn't do much but it's easier to trace with a debugger.


We have four directories here: local, patchSource, patch, and expected. 
The local directory is a directory that is to be patched. It has files that 
    are named the same as the patch directory, but whose contents are in fact different.
The patchSource directory is what the patch is built from.
The patch directory is the patch itself, built from patchSource.
The expected directory is the local directory should look like after it's patched.

