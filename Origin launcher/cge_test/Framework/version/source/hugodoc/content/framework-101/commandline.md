---
title: Running NAnt on the Command Line
weight: 11
---

Now that we have an understanding of how to describe a package in a build script we should know how we can run nant on these scripts.
This page will introduce you to the nant command line and the most commonly used arguments.

<a name="BuildingAPackage"></a>
## Building a Package ##

Likely the thing you are most interested in using nant for at this point is building a package.
To do this you must run nant.exe which is located in the bin folder of the Framework package.
It is recommended that you add the Framework bin folder to your PATH environment variable for convenience
so that you can just type nant.exe regardless of the directory you are in.
You could type &quot;nant -help&quot; to get a full list of all of the options, but what ones do you really need?


```
nant build -configs:pc64-vc-dev-debug -masterconfigfile:D:\masterconfig.xml -buildpackage:mypackage
```

```
D:/packages/mypackage/1.00.00> nant build
```
Above are two different ways you could do this. You can either be completely explicit or you can rely on nant defaults to have a shorter command line.
These days users tend to be more explicit because the nant defaults don&#39;t always line up with their workflows but knowing what happens when you leave out a field is important.

The most important argument on the nant command line is called a target.
In this case the target is called &quot;build&quot;.
A Target is like a function, it is a named procedure defined in build scripts that can be run by framework.
Targets are the only argument on the nant command line that does not start with a dash.
You can provide as many targets as you want to nant and they will execute one after another.
You may remember from the page about build scripts that Targets are one of the 6 fundamental Framework primatives.
The eaconfig package defines a large number of default targets in its build scripts that are loaded when Framework executes a packages package task.

The next most important thing that nant needs to know is what package we want to execute the target on.
For example if we are building an executable that depends on a bunch of library packages we need to tell nant to run the build target on the package with the executable module.
We can do this simply buy running nant.exe from with the root folder of the package and nant will look for a .build file.
Or if we don&#39;t want to navigate to the package&#39;s directory we can be explicit and specify the location of the build file with &quot;-buildfile&quot; or the name of the package with &quot;-buildpackage&quot;.

We have previously talked about the masterconfig file, and nant cannot execute without one, but you can choose to be explicit or implicit with the location of the masterconfigfile just like with the build file.
You can provide the path to the masterconfig file with the arguement &quot;-masterconfigfile&quot;.
Or if you choose to omit this property framework will look for a file with the name masterconfig.xml in the directory where it is executed from.
Increasingly users want to have a single global masterconfig rather than one in each package so it is more common these days to be explicit about the location of the masterconfig.

Lastly, if you are performing a build you need to specify a config with the argument &quot;-configs:pc64-vc-dev-debug&quot;.
This tells Framework that you want to build for the 64 bit pc platform with the debug configuration.
The eaconfig package has an xml file for each of the configs that it provides, and it provides configs for pc and unix platforms.
Other platforms are defined in platform config packages like android_config, which are loaded when the config can&#39;t be found in eaconfig.
Each platform has a number of different types, for example debug and opt (short for optimization, sometimes called release).
The build command takes a single config because it is what is know as a single config target and only builds one thing at a time.


{{% panel theme = "info" header = "Note:" %}}
The command line option -configs is quite new and does not exist in Framework 7.13.01 or older.
If you are a new framework user you should be aware of the old config arguments because you will likely
see them used a lot and may need to use them yourself if you are working with an old version of Framework.

The argument -D:package.configs=&quot;pc64-vc-dev-debug pc64-vc-dev-opt&quot; is exactly identical to -configs:&quot;pc64-vc-dev-debug pc64-vc-dev-opt&quot;.
However as you can tell the newer syntax is shorter and does not use the global property syntax (-D:).

There is a second argument -D:config= which is similar to package.configs but only accepts a single config, it is considered the &quot;startup config&quot; by Framework.
If you use -configs or package.configs and leave out config, Framework will set config to be the first config listed in package.configs.
If you use both config and package.configs, and config is not in package.configs, some targets like gensln may combine the two, so watch out.

Our recommendation going forward is to always use -configs, or on older frameworks to always use -D:package.configs, if you want to avoid some of the more confusing behavior related to this arguments.
Keep in mind framework will take the first config in configs as the start up config. In build script you can see the value of the startup config with (${config}), and a list of all of the configs with (${package.configs}).
{{% /panel %}}
<a name="VisualStudioBuilds"></a>
## Visual Studio Builds ##

The build target is quite simple but in practice it is not used as much anymore.
The build target runs what is called a &quot;native nant build&quot;, which means that Framework calls the compiler and linker executables directly
and builds up the arguments to provide them.
Native nant builds more work to maintain for the CM team and often other build tools like visual studio can take care of this.
Framework has the ability to generate Visual Studio projects based on a packages build script.
This allows users to more easily work on their packages with Visual Studio but it also allows Framework to let Visual Studio
take care of most of the complexities of building especially for more complicated platforms like Xb1 or android.

Unlike native nant builds, Building with visual studio is a two step procedure.
Framework must run one target to generate the solution based on your build scripts.
Then it must run a second target to build the solution using msbuild.
The first target is &quot;gensln&quot;, this generates the solution.
The second target is &quot;buildsln&quot;, this builds the generated solution.
There is also a third target &quot;opensln&quot; which can be used to open the solution and edit it in visual studio.


```
nant gensln buildsln -configs:"pc64-vc-dev-debug pc64-vc-dev-opt" -masterconfigfile:D:\masterconfig.xml -buildpackage:mypackage
```
Notice how we provide two configs this time in the config argument.
This means that the generated visual studio solution will have two different configs selectable from the config dialog when working in visual studio.
The buildsln target however is a single config target like the build target, so in that case only the first config listed will be used when doing then build.

