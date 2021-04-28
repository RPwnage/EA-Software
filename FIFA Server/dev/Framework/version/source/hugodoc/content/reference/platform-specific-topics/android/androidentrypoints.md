---
title: Android Entry Points
weight: 274
---

An overview of the differences between entry point types. Framework allows you to specify whether to use native or java entry points for
your packages and generates the relevant glue code. If your package sets the property runtime.android-skip-default-entry-point to true
Framework expects you to provide most of the entry point code.

## Java Entry Points ##

Many things you might wish to do on the Android platform are often only accessible from java which requires java code or JNI code to manipulate the
relevant java objects. Writing java code is usually easier so Framework defaults to using java glue code.

<a name="Framework Generated Entry Point"></a>
### Framework Generated Entry Point ###

If your package has no java code Framework will generate all the necessary glue for a java entry point. All that needs to be provided is a
native side entry point that matches the following signature:


```
extern int main(int argc, char** argv);
```
If your package has java code Framework will assume that you have implemented your own Activity class but that you still wish to use the generated
java to native main transition code. To use a standard main function with command line parameter support in a custom
java Activity modify the onCreate function in the following ways:

 - Declare native entry point function: &quot;public native int runEntryPoint(Activity activity);&quot;.
 - Call native entry point function passing Activity object: runEntryPoint(this);

###### Example ######

```
public class MyActivity extends Activity {

    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
  ...
        int ret = runEntryPoint(this);
  ...
    }
	
 ...

    public native int runEntryPoint(Activity activity);
}
```
<a name="No Framework Generated Entry Point"></a>
### No Framework Generated Entry Point ###

If you don&#39;t wish to use Framework&#39;s entry point generation set the following property:


```xml
<property name="runtime.android-skip-default-entry-point" value="true" />
```
For java entry points this will not add any generated code to the build and it is left to the user to handle any java to native transitions.

## Native Entry Points ##

If you wish to have a java free package for Android you can use a native entry point. This might be more convenient for smaller test packages or
debugging environments that only allow debugging of native code. To use native entry point flow set the following property in your package&#39;s .build file:

###### Syntax ######

```xml
<property name="[group].[ModuleName].android-glue" value="native" />
```
###### Example ######

```xml
<property name="runtime.MyModule.android-glue" value="native" />
```
<a name="Framework Generated Entry Point"></a>
### Framework Generated Entry Point ###

All that needs to be provided is a native main function that matches the following signature:


```
extern int main(int argc, char** argv);
```
Note that as no Activity class has been defined you should declare activity &quot;android.app.NativeActivity&quot; if using a custom manifest file.

<a name="No Framework Generated Entry Point"></a>
### No Framework Generated Entry Point ###

If you don&#39;t wish to use Framework&#39;s entry point generation set the following property:


```xml
<property name="runtime.android-skip-default-entry-point" value="true" />
```
Framework will add android_native_app_glue.c from the Android NDK to the source files and add the associated include paths
to assist in writing custom native entry point. You will need to provide an android_main function:


```
void android_main(struct android_app* app)
```

##### Related Topics: #####
-  [Android Command Line Arguments]( {{< ref "reference/platform-specific-topics/android/androidcommandline.md" >}} ) 
