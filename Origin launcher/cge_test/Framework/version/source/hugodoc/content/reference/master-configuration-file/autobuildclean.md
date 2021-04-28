---
title: Auto Build Clean
weight: 257
---

Without auto-build/clean, a user has to manually build or clean each dependent package of a package.
Framework introduces automatic building and cleaning of dependent packages. For example, if your
application depends on package A, which depends on packages B and C, then building the application should
automatically build A, and also build packages B and C.

To support auto-build/clean, Framework 2 introduces three attributes to [<masterversions/package>]( {{< ref "reference/master-configuration-file/masterversions/package.md" >}} ) in [Master Configuration Files]( {{< ref "reference/master-configuration-file/_index.md" >}} ) :
autobuildclean, autobuildtarget, and autocleantarget. Autobuildclean indicates if a package
participates in auto-build/clean or not, and has value of either true or false. The default value
is true for buildable Framework packages, and false otherwise. The other two attributes specify
the names of targets for auto-building and cleaning the package. autocleantarget has &quot;clean&quot; as default,
and autobuildtarget has &quot;build&quot; if the package lacks a default target.

And more importantly, Framework introduces a new attribute to &lt;target&gt;: style. Its values are:
&quot;build&quot;, &quot;clean&quot;, and &quot;use&quot;. Targets named &quot;build&quot; or &quot;buildall&quot; have &quot;build&quot; style by default.
Targets named &quot;clean&quot; or &quot;cleanall&quot; have the &quot;clean&quot; style by default. Other targets by default will have
the &quot;use&quot; style if the target is a parent target (i.e. it&#39;s not in another target&#39;s &#39;depends&#39; list), or
if the target is a child target (i.e. it&#39;s in another target&#39;s &#39;depends&#39; list) then its default style is
inherited from the parent target.

## Example of default inherited style: ##


```xml
<target name="build" depends="dep-list">
   </target>

   <target name="myclean" depends="dep-list" style="clean">
   </target>

   <!--
       dep-list inherits style from the parent target: either "build" style
       from target "build" or "clean" style from target "myclean"
   -->
   <target name="dep-list">
<dependent name="mypackage"/>
   </target>
```
## Example of OVERRIDING default inherited style: ##


```xml
<target name="mybuild" depends="myuse" style="build">
</target>

<!--
    child target "myuse" overrides default inherited style "build", from the above parent target
    'mybuild', with style "use"
-->
<target name="myuse" style="use">
    <dependent name="usepackage"/>
</target>
```
In Framework 1.x, a &lt;dependent&gt; can be put anywhere in the build script. This freedom is not allowed
if the &lt;dependent&gt; is a latest Framework package that auto builds and cleans. In such case, the
&lt;dependent&gt; must be nested in a target whose style is either &#39;build&#39; or &#39;clean&#39;. Otherwise,
a verbose warning will result and the &lt;dependent&gt; will not auto build nor clean.

## Master configuration and build file examples: ##

###### masterconfig.xml: ######

```xml
<project xmlns="schemas/ea/framework3.xsd">
    <masterversions>
        <!-- A specifies "build-me" as the target for auto-build -->
        <package name="A" version="1.00.00" autobuildtarget="build-me"/>
        <!-- B specifies "clean-me" as the target for auto-clean -->
        <package name="B" version="1.00.00" autocleantarget="clean-me"/>
        <!-- The build engineer turned off C's autobuildclean once he had built C and
        doesn't want it to be auto-built again -->
        <package name="C" version="1.00.00" autobuildclean="false"/>
    </masterversions>
</project>
```
###### App.build: ######

```xml
<project default="build">
    <package name="App" targetversion="1.00.00"/>
    <!-- This target has "build" style, and will pass it to child target init -->
    <target name="build" description="Builds the application" depends="init"/>
    <!-- This target has "use" style by default, and will pass it to init -->
    <target name="run" description="Runs the application" depends="init"/>
    <!-- This target will inherit "build" from build, and "use" from run -->
    <target name="init">
        <!--
        1. It must be inside a target now
        2. It'll build package A when target 'build' runs
        3. It won't build A when target 'run' executes
        -->
        <dependent name="A"/>
    </target>
</project>
```
###### A.build: ######

```xml
<project default="build-me">
    <package name="A" targetversion="1.00.00"/>
    <!-- This target specifies "build" style, and is listed in the master configuration file as
    the auto-build target for A. When NAnt runs App.build to build App, it'll find out this target and
    build A -->
    <target name="build-me" description="Builds package A" style="build" depends="init"/>
    <!-- This target inherits "build" style from build-me -->
    <target name="init">
        <dependent name="B"/>
        <dependent name="C"/>
    </target>
</project>
```
###### B.build: ######

```xml
<project default="build">
    <package name="B" targetversion="1.00.00"/>
    <!-- This target specifies "clean" style, and is listed in the master configuration file as
    the auto-clean target for B. When NAnt runs App.build to clean App, it'll find out this target and
    clean B -->
    <target name="clean-me" description="Cleans package B" style="clean"/>
    <target name="build" description="Builds package B"/>
</project>
```
###### C.build: ######

```xml
<project default="build">
    <package name="C" targetversion="1.00.00"/>
    <target name="build" description="Builds package C"/>
</project>
```
If non-buildable packages are listed in the master configuration file, their autobuildclean
attribute must be set to false explicitly. Failing to do so results in error. Non-buildable
packages include proxy packages, Framework 1.x packages, and Framework 2 (or higher) packages
that have &lt;buildable&gt; set to false in their manifest.xml.

If your top-level package is Framework 2 or higher, and it depends on Framework 1.x packages,
you&#39;ll have to build the Framework 1.x packages either manually, or by using tags like &lt;nant&gt;.
Framework doesn&#39;t support the mixture of a Framework 1.x top-level package and latest Framework
packages at lower levels.

