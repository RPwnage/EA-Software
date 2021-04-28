This is a test of a "three-location patch," which is the case of patching a read-only directory to a separate writable location. It's important to exercise that there is no attempt to write to the read-only directory, and that the diffing code does the right thing by using the contents of the read-only directory.

We have four directories here: local, patchSource, patch, and expected. 
The local directory is a directory that is to be patched. It has files that 
    are named the same as the patch directory, but whose contents are in fact different.
The patchSource directory is what the patch is built from.
The patch directory is the patch itself, built from patchSource.
The expected directory is the local directory should look like after it's patched.

