---
title: DateTime Functions
weight: 1004
---
## Summary ##
Collection of time and date manipulation routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String DateTimeNow()]({{< relref "#string-datetimenow" >}}) | Gets the date and time that is the current local date and time on this computer.  NOTE: The output format will depend on your current OS&#39;s locale setting! |
| [String DateTimeUtcNow()]({{< relref "#string-datetimeutcnow" >}}) | Gets the date and time that is the current local date and time on this computer expressed as coordinated universal time (UTC).  NOTE: The output format will depend on your current OS&#39;s locale setting! |
| [String DateTimeNowAsFiletime()]({{< relref "#string-datetimenowasfiletime" >}}) | Gets the date and time that is the current local date and time on this computer expressed as a file timestamp.  NOTE: The output format will depend on your current OS&#39;s locale setting! |
| [String DateTimeToday()]({{< relref "#string-datetimetoday" >}}) | Gets the current date.  NOTE: The output format will depend on your current OS&#39;s locale setting! |
| [String DateTimeDayOfWeek()]({{< relref "#string-datetimedayofweek" >}}) | Returns the current day of the week.  NOTE: This function always returns the name in English. |
| [String DateTimeCompare(DateTime t1,DateTime t2)]({{< relref "#string-datetimecompare-datetime-t1-datetime-t2" >}}) | Compares two datetime stamps.  NOTE: The input DateTime format is expected to match your OS&#39;s locale setting or in ISO 8601 format that DateTime.Parse can handle. |
## Detailed overview ##
#### String DateTimeNow() ####
##### Summary #####
Gets the date and time that is the current local date and time on this computer.  NOTE: The output format will depend on your current OS&#39;s locale setting!

###### Parameter:  project ######


###### Return Value: ######
The current local date and time on this computer.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="@{DateTimeNow()}" />
</project>

<!-- Your output on en-US locale will look something like:
[echo] 1/29/2015 2:51:55 PM
-->

<!-- Your output on en-CA locale will look something like:
[echo] 29/01/2015 2:51:55 PM
-->

<!-- Your output on en-GB locale will look something like:
[echo] 29/01/2015 14:51:55
-->

```





#### String DateTimeUtcNow() ####
##### Summary #####
Gets the date and time that is the current local date and time on this computer expressed as coordinated universal time (UTC).  NOTE: The output format will depend on your current OS&#39;s locale setting!

###### Parameter:  project ######


###### Return Value: ######
The current local date and time on this computer expressed as UTC.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="@{DateTimeUtcNow()}" />
</project>

<!-- Your output on en-US locale will look something like:
[echo] 1/29/2015 10:51:55 PM
-->

<!-- Your output on en-CA locale will look something like:
[echo] 29/01/2015 10:51:55 PM
-->

<!-- Your output on en-GB locale will look something like:
[echo] 29/01/2015 22:51:55
-->

```





#### String DateTimeNowAsFiletime() ####
##### Summary #####
Gets the date and time that is the current local date and time on this computer expressed as a file timestamp.  NOTE: The output format will depend on your current OS&#39;s locale setting!

###### Parameter:  project ######


###### Return Value: ######
The current local date and time on this computer expressed as UTC.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="@{DateTimeNowAsFiletime()}" />
</project>

<!-- Your output on en-US locale will look something like:
[echo] 132344629031570819
-->

```





#### String DateTimeToday() ####
##### Summary #####
Gets the current date.  NOTE: The output format will depend on your current OS&#39;s locale setting!

###### Parameter:  project ######


###### Return Value: ######
The current date and time with the time part set to 00:00:00.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="@{DateTimeToday()}" />
</project>

<!-- Your output on en-US locale will look something like:
[echo] 1/29/2015 12:00:00 AM
-->

<!-- Your output on en-CA locale will look something like:
[echo] 29/01/2015 12:00:00 AM
-->

<!-- Your output on en-GB locale will look something like:
[echo] 29/01/2015 00:00:00
-->

```





#### String DateTimeDayOfWeek() ####
##### Summary #####
Returns the current day of the week.  NOTE: This function always returns the name in English.

###### Parameter:  project ######


###### Return Value: ######
The current day of the week.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message='@{DateTimeDayOfWeek()}' />
</project>

<!-- Your output may look something like this
[echo] Thursday
-->

```





#### String DateTimeCompare(DateTime t1,DateTime t2) ####
##### Summary #####
Compares two datetime stamps.  NOTE: The input DateTime format is expected to match your OS&#39;s locale setting or in ISO 8601 format that DateTime.Parse can handle.

###### Parameter:  project ######


###### Parameter:  t1 ######
The first datetime stamp.

###### Parameter:  t2 ######
The second datetime stamp.

###### Return Value: ######
Less than zero - t1 is less than t2.  &lt;br/&gt;
Zero - t1 equals t2. &lt;br/&gt;
Greater than zero - t1 is greater than t2. &lt;br/&gt;

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <!-- Compare file time -->
    <echo file="testfile1.txt" message="test file 1"/>
    <property name='t1' value="@{FileGetLastWriteTime('testfile1.txt')}" />
    <echo message="LastWriteTime of testfile1.txt = ${t1}"/>
    <sleep milliseconds="1000"/>
    <echo file="testfile2.txt" message="test file 2"/>
    <property name='t2' value="@{FileGetLastWriteTime('testfile2.txt')}" />
    <echo message="LastWriteTime of testfile2.txt = ${t2}"/>
    <echo message="DateTimeCompare result = @{DateTimeCompare('${t1}', '${t2}')}"/>

    <!-- Example Output
          [echo] LastWriteTime of testfile1.txt = 1/29/2015 3:20:34 PM
          [sleep] Sleeping for 1000 milliseconds
          [echo] LastWriteTime of testfile2.txt = 1/29/2015 3:20:35 PM
          [echo] DateTimeCompare result = -1
    -->


    <!-- Hard coding input time in specific format -->
    <property name="t1De" value="29.01.2015 09:00:00"/>
    <echo message="t1De = ${t1De}"/>
    <!-- Need to convert that to local OS format first and save to property t1 -->
    <script language="C#" compile="true">
        <code>
            
                public static void ScriptMain(Project project)
                {
                    string deDateTimeStr = project.Properties["t1De"];
                    DateTime localCultureDateTime = DateTime.Parse(deDateTimeStr, new System.Globalization.CultureInfo("de-DE", true));
                    project.Properties.Add("t1", localCultureDateTime.ToString() );
                }
            ] ]>
        </code>
    </script>
    <echo message="t1 = ${t1}"/>
    <property name="t2" value="@{DateTimeNow()}"/>
    <echo message="t2 = ${t2}"/>
    <echo message="DateTimeCompare result = @{DateTimeCompare('${t1}', '${t2}')}"/>

    <!-- Example Output
          [echo] t1De = 29.01.2015 09:00:00
          [echo] t1 = 1/29/2015 9:00:00 AM
          [echo] t2 = 1/29/2015 2:51:55 PM
          [echo] DateTimeCompare result = -1
    -->


    <!-- Making sure that your input format is universal (ie ISO 8601) -->
    <property name="t1" value="2015-01-29T15:00:00"/>
    <echo message="t1 = ${t1}"/>
    <property name="t2" value="@{DateTimeNow()}"/>
    <echo message="t2 = ${t2}"/>
    <echo message="DateTimeCompare result = @{DateTimeCompare('${t1}', '${t2}')}"/>

    <!-- Example Output
          [echo] t1 = 2015-01-29T15:00:00
          [echo] t2 = 1/29/2015 2:51:55 PM
          [echo] DateTimeCompare result = 1
    -->

</project>

```





