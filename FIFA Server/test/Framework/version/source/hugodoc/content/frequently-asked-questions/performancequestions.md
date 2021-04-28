---
title: Performance Related Questions
weight: 307
---

This page list frequently asked questions related to build performance.

<a name="BulkBuild"></a>
## How do I set up Bulk Build? ##

If you want to use Bulk Build, you can visit the [Bulk Build]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/_index.md" >}} ) page for more information on
how to set it up.

<a name="PreCompiledHeader"></a>
## How do I set up Precompiled Headers? ##

You just need to provide a &lt;pch&gt; child element under &lt;config&gt; element in your module definition.  Please see
the [How to define custom build options in a module]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md" >}} ) page details on the syntax and how to set it up.

<a name="ParallelBuild"></a>
## How Do I Control The NAnt Parallel Build? ##

Framework supports thread safe parallelization of builds.  That said, it still depends on
the fact that if you have custom build tasks (such as pre-build/post-build steps), they need to be thread safe as well.
Framework provided a few NAnt tasks to help you with writing your custom tasks that is thread safe. (&lt;parallel.do&gt;, &lt;parallel.foreach&gt;, &lt;namedlock&gt;)

If your current scripts is not thread safe and you need to turn off the parallelization feature, you can add `-noparallel` command line switch to turn it off.

<a name="DistributedBuild"></a>
## What Are My Options In Doing Distributed Builds? ##

Framework provided support for doing native NAnt distributed build or building with Incredibuild.  Please check these links for more information:

 - [Distributed Build Targets]( {{< ref "reference/targets/targetreference.md#DistributedBuildTargets" >}} )
 - [Incredibuild Build Targets]( {{< ref "reference/targets/targetreference.md#IncredibuildTargets" >}} )

