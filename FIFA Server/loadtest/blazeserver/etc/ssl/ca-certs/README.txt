This directory contains all the root CA certficates that the Blaze server will load on startup.
This default set of roots included are those provided by the OpenSSL package.  If you need to
add new root CAs, please just add them to this directory in PEM format with a .pem extension
and the Blaze server will pick them up automatically on startup.
