---
title: Build Error Related Questions
weight: 306
---

This page lists some commonly asked questions related to build error messages that you might see during a build.

<a name="Section1"></a>
## Why do I see error message: unrecognized command line option &quot;-std=c++0x&quot;? ##

This is happening because the GCC compiler that is pre-installed on your system is an older version that does not support the C++0x (ISO C++ 2011 standard) yet.
You need to update your system&#39;s GCC to version 4.3.  If that is not feasible, you can also try suppressing this build option by providing a global property in
your masterconfig `cc.cpp11=false` .

