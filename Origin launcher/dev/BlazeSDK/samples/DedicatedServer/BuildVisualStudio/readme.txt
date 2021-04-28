These files demonstrate how to build the dedicated server as an independent VS sln.

Steps:

1. Copy these files to the parent directory
2. Edit DedicatedServer.vsprops to set your packages root and package versions (use the package versions that BlazeSDK uses)
3. Install BlazeSDK from the package server
4. Build the BlazeSDK sln for PC with samples enabled:
   nant slnruntime -D:samples=true -D:package.configs="pc-vc-dev-debug pc-vc-dev-opt"
5. Open BlazeSDK.sln and then Rebuild Solution
6. Open DedicatedServer.sln and build it

For the sake of convenience I have allowed BlazeSDK to do all the grunt work of installing and building the other packages we need; DedicatedServer reuses those libs.

Note that these buildfiles are for illustrative purposes only, and are not maintained.