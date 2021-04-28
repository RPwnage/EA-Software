The certs in these folders have been removed from perforce to ESS.
If you need the certs and keys for local development purposes you can pull them the eadp-gs-dirtysock-np namespace on https://ess.ea.com

This readme guide is meant to deal with the client certs used to communicate with various services. There also exist
stunnel certs that are used with CC mode of operation but is not covered in this guide as it is not typically run for
local development.

For non-local development our hardware and clusters are provisioned by the operations team. Please reach out if you are
running into certificate related errors as these instructions do not apply there.

1. On the login screen, make sure the namespace is not set and the method is set to "Okta"
2. Use your work email and password
3. Press the Sign In notification and wait for the push notification on your phone.
4. After login click the down arrow icon on the top left hand corner of the page to change the namespace to "eadp-gs-dirtysock-np".
5. You will see new folders show up where you can pull from.
   The locations for local development are as follows:
       OTP: secrets/kv/syseng/dirycast/otp/dev
       CC: secrets/kv/syseng/dirtycast/cc
       QoS: secrets/kv/syseng/dirtycast/qos
6. To copy the cert and key from ESS you have a few options. After the name of the secret (ex: cert), there is a copy
   button (looks like a clipboard) you can use. There is also an unhide button (looks like an eye) which shows you the
   secret you can copy/paste from your browser. The final option is use the "Copy Secret > Copy JSON" which takes the
   entire secret as a JSON response. The last option is not the most ideal but you can use it if you find it easier.

   Once you have this secret you need to create a files, one for cert and other for key in DirtyCast/run/certs based on
   the mode of DirtyCast you are running.
   For OTP it is DirtyCast/run/certs/default.crt and DirtyCast/run/certs/default.key
   For CC it is DirtyCast/run/certs/GS-DS-CCS.int.crt and DirtyCast/run/certs/GS-DS-CCS.int.key
   For QoS it is DirtyCast/run/certs/GS-DS-QOS.int.crt and DirtyCast/run/certs/GS-DS-QOS.int.key
   Note: if you run into any trouble please validate that the names have not changed in voipserverconfig.c when loading
         "voipserver.certfilename" and "voipserver.keyfilename" configuration options.
