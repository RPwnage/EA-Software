---
title: Visual Studio Debugging and Deployment
weight: 269
---

Adding Visual Studio Debugging and Deployment Options

<a name="Section1"></a>
## Visual Studio Debugging and Deployment ##

Visual Studio debugging and deployment options can be added in Structured XML through platform extensions:


```xml

<Program name="Example">
. . . .
  <platforms>
    <xenon unless="${frostbite.xenon.enable_deployment}">
      <deployment enable="false" />
      <imagebuilder configfile="${frostbite.xenon.imagexexConfigFile}"/>
    </xenon>
    <xenon if="${frostbite.xenon.enable_deployment}">
      <deployment deploymenttype="HDD" enable="true">
        <deploymentfiles>
          $(RemoteRoot)=$(OutDir)$(TargetName).xex
          $(RemoteRoot)=$(GAME_DATA_DIR)\Scripts\XenonGame.cfg
        </deploymentfiles>
        <remoteroot>
          xe:\Frostbite\${frostbite.P4_CLIENT_NAME}
        </remoteroot>
      </deployment>
      <imagebuilder configfile="${frostbite.xenon.imagexexConfigFile}"/>
    </xenon>
    <ps3>
      <debugging
        executable-arguments=""
        debugger-command-line=""
        run-command-line=""
        homedir="$(TargetDir)"
        fileservdir="$(GAME_DATA_DIR)"
        />
    </ps3>
  </platforms>
</Program>

```
