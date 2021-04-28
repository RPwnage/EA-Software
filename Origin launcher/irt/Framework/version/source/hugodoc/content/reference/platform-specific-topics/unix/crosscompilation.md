---
title: Compiling and Debugging for Unix on Windows
weight: 285
---

Support has been added for cross compiling Unix binaries on windows, this page explains the different level of support provided.

<a name="Section1"></a>
## Doing Cross Compile on Windows ##

### Prerequisites ###

To do cross compile for Linux platform on Windows host machine using Visual Studio, the following prerequisites need to be setup:

1. Your Visual Studio needs to have "Linux Development with C++" component installed.
2. Your masterconfig needs to include packages UnixCrossTools (use a version that support the clang version for your UnixClang version number) and vsimake.
```
<masterversions>
    <package name="UnixClang" version="9.0.0-proxy"/>
    <package name="UnixCrossTools" version="2.06.00"/>  <!-- Changelog indicates it is packaged for clang 9.0.0 -->
    <package name="vsimake" version="3.80"/>
</masterversions>
```
The UnixCrossTools contains a version of clang that runs on PC and all other necessary Linux build tools and libraries that has been cross compiled to run on PC.

### Generate Solution and Build ###

Once above prerequisites are satisifed, all you need is to do a regular solution generation ('gensln' nant target) for the unix configs.
The generated Visual Studio project will create make style vcxproj projects and GNU style .mak files and uses vsimake.exe (in vsimake package)
to launch the build.  And because it actually relies on an external vsimake.exe to drive the build, a no-change build in Visual Studio will 
still trigger a vsimake.exe execution and it is up to the makefiles files that determines if a build is actually needed.

However, if you are a developer and you intend to do some debugging with this build, then you may need extra command line options to properly setup
"deployment" information during solution generation.  And this deployment setup depends on whether you want to debug using Windows Subsystem for Linux (WSL) or
on an external Linux machine (or other VM).  The following sections describe differences with each debugging methods.

<a name="DebuggingWithWSL"></a>
## Debugging Linux build with WSL ##

### Prequisites ###

In order to allow your build to debug using WSL, you also need to have the following setup:

1. Make sure your Windows features have turned on "Windows Subsystem for Linux (WSL)"
2. After WSL is turned on and rebooted Windows, you also need to install Ubuntu (from Microsoft Store and search for Ubuntu)
3. After Ubuntu is installed, make sure you have proper SSH setup.  In this file: 
   ```
   /etc/ssh/sshd_config
   ```
   make sure you have following fields updated (or added):
   *  PermitRootLogin no
   *  AllowUsers &lt;Unix username used to setup your Ubuntu login&gt;
   *  PasswordAuthentication yes
   *  PubkeyAuthentication yes
   *  Port &lt;#&gt;     (use a new port number that you will use in Visual Studio connection setup)
4. Then restart your ssh service with: 
   ```
   sudo service ssh --full-restart
   ```
5. Make sure you have setup a connection to this WSL in your Visual Studio.  In options dialog box (menu -&gt; Tools -&gt; Options), 
   go to "Cross Platform" and "Connection Manager". Then setup your Linux connection information (use the same port number in previous step).
6. Note that since most of our library requires using clang's libc++, so make sure your Linux install has also downloaded 
   clang's libc++ system dynamic libraries as well: 
   ```
   sudo apt-get install libc++-dev libc++abi-dev
   ```

### Changes to Solution Generation ###

To generate a solution that take advantage of using your local WSL, you just need to set a global property 'eaconfig.unix.vcxproj-debug-on-wsl'
to true by passing that in through nant.exe command line (as -G:eaconfig.unix.vcxproj-debug-on-wsl=true") or set it up in globalproperties section
in masterconfig.  For example:

```
nant.exe -D:config=unix-x64-clang-debug -D:package.configs="unix-x64-clang-debug unix-x64-clang-opt" 
         -G:eaconfig.unix.vcxproj-debug-on-wsl=true 
         gensln
```

The generated solution should setup your project output path directly using unix path syntax with /mnt/[windows drive]/...
instead of the Windows path syntax [windows drive]:\\...

After re-creating a new Visual Studio solution and do a build, you should be able to just debug directly in your Visual Studio IDE.

<a name="DebuggingWithRemoteLinuxHost"></a>
## Debugging Linux build using a remote Linux host (or other VM) ##

### Prequisites ###

In order to allow your build to debug on a remote Linux host, you need to have the following setup:

1. Make sure your Linux host has proper SSH setup (same procedure as the WSL setup).
2. Make sure you have setup a connection to this remote Linux host in your Visual Studio (same steps as WSL setup).
3. Again, make sure you have clang's libc++ system dynamic libraries installed.
4. You need to be using Visual Studio 2017 or 2019.

### Changes to Solution Generation ###

In order to debug on a remote Linux host, we need to deploy the build output to remote Linux host machine during build (and therefore would
affect solution generation).  So we need to provide the remote deploy location (through global property 'eaconfig.unix.deploymentroot') and 
tell Framework to create instructions to deploy during build (through global property 'eaconfig.unix.vcxproj-deploy-during-build'). For example:

```
nant.exe -D:config=unix-x64-clang-debug -D:package.configs="unix-x64-clang-debug unix-x64-clang-opt" 
         -G:eaconfig.unix.vcxproj-deploy-during-build=true 
         -G:eaconfig.unix.deploymentroot=/tmp/deployroot
         gensln
```

The generated solution will add a "Post-Build Event" to your program modules and setup "Additional Files to Copy" which just map your 
local build output (including your runtime dependencies) to the remote deployment path.  Visual Studio will perform the deployment right after
it finishes building.  Then you should be able to just debug directly in your Visual Studio IDE.

