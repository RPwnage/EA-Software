---
title: Math Functions
weight: 1009
---
## Summary ##
Collection of NAnt Math routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String MathPI()]({{< relref "#string-mathpi" >}}) | The ratio of the circumference of a circle to its diameter. |
| [String MathE()]({{< relref "#string-mathe" >}}) | The natural logarithmic base. |
| [String MathLT(Double a,Double b)]({{< relref "#string-mathlt-double-a-double-b" >}}) | Compare two numbers to determine if first number is less than second. |
| [String MathLTEQ(Double a,Double b)]({{< relref "#string-mathlteq-double-a-double-b" >}}) | Compare two numbers to determine if first number is less than or equal to second. |
| [String MathGT(Double a,Double b)]({{< relref "#string-mathgt-double-a-double-b" >}}) | Compare two numbers to determine if first number is greater than second. |
| [String MathGTEQ(Double a,Double b)]({{< relref "#string-mathgteq-double-a-double-b" >}}) | Compare two numbers to determine if first number is greater than or equal to second. |
| [String MathEQ(Double a,Double b)]({{< relref "#string-matheq-double-a-double-b" >}}) | Compare two numbers to determine if first number is equal to second. |
| [String MathAdd(Int32 a,Int32 b)]({{< relref "#string-mathadd-int32-a-int32-b" >}}) | Add two integer numbers. |
| [String MathAddf(Double a,Double b)]({{< relref "#string-mathaddf-double-a-double-b" >}}) | Add two floating-point numbers. |
| [String MathSub(Int32 a,Int32 b)]({{< relref "#string-mathsub-int32-a-int32-b" >}}) | Subtract two integer numbers. |
| [String MathSubf(Double a,Double b)]({{< relref "#string-mathsubf-double-a-double-b" >}}) | Subtract two floating-point numbers. |
| [String MathMul(Int32 a,Int32 b)]({{< relref "#string-mathmul-int32-a-int32-b" >}}) | Multiply two integer numbers. |
| [String MathMulf(Double a,Double b)]({{< relref "#string-mathmulf-double-a-double-b" >}}) | Multiply two floating-point numbers. |
| [String MathMod(Int32 a,Int32 b)]({{< relref "#string-mathmod-int32-a-int32-b" >}}) | Computes the modulo of two numbers. |
| [String MathModf(Double a,Double b)]({{< relref "#string-mathmodf-double-a-double-b" >}}) | Computes the modulo of two numbers. |
| [String MathDiv(Int32 a,Int32 b)]({{< relref "#string-mathdiv-int32-a-int32-b" >}}) | Divide two integer numbers. |
| [String MathDivf(Double a,Double b)]({{< relref "#string-mathdivf-double-a-double-b" >}}) | Divide two floating-point numbers. |
| [String MathAbs(Int32 a)]({{< relref "#string-mathabs-int32-a" >}}) | Calculates the absolute value of a specified number. |
| [String MathAbsf(Double a)]({{< relref "#string-mathabsf-double-a" >}}) | Calculates the absolute value of a specified number. |
| [String MathCeiling(Double a)]({{< relref "#string-mathceiling-double-a" >}}) | Returns the smallest whole number greater than or equal to the specified number. |
| [String MathFloor(Double a)]({{< relref "#string-mathfloor-double-a" >}}) | Returns the largest whole number less than or equal to the specified number. |
| [String MathPow(Double x,Double y)]({{< relref "#string-mathpow-double-x-double-y" >}}) | Raise the specified number to the specified power. |
| [String MathSqrt(Double a)]({{< relref "#string-mathsqrt-double-a" >}}) | Computes the square root of a specified number. |
| [String MathDegToRad(Int32 a)]({{< relref "#string-mathdegtorad-int32-a" >}}) | Converts an angle from degrees to radians. |
| [String MathSin(Double a)]({{< relref "#string-mathsin-double-a" >}}) | Calculates the sine of the specified angle. |
| [String MathCos(Double a)]({{< relref "#string-mathcos-double-a" >}}) | Calculates the cosine of the specified angle. |
| [String MathTan(Double a)]({{< relref "#string-mathtan-double-a" >}}) | Calculates the tangent of the specified angle. |
| [String MathChangePrecision(Double a,Int32 precision)]({{< relref "#string-mathchangeprecision-double-a-int32-precision" >}}) | Changes the precision of a specified floating-point number. |
## Detailed overview ##
#### String MathPI() ####
##### Summary #####
The ratio of the circumference of a circle to its diameter.

###### Parameter:  project ######


###### Return Value: ######
PI

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathPI()} == 3.1415926535897931" />
</project>

```





#### String MathE() ####
##### Summary #####
The natural logarithmic base.

###### Parameter:  project ######


###### Return Value: ######
E

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathE()} == 2.7182818284590451" />
</project>

```





#### String MathLT(Double a,Double b) ####
##### Summary #####
Compare two numbers to determine if first number is less than second.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
True if less than, otherwise False

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathLT('1', '2')}" />
</project>

```





#### String MathLTEQ(Double a,Double b) ####
##### Summary #####
Compare two numbers to determine if first number is less than or equal to second.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
True if less than or equal, otherwise False

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathLTEQ('1', '2')}" />
</project>

```





#### String MathGT(Double a,Double b) ####
##### Summary #####
Compare two numbers to determine if first number is greater than second.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
True if greater than, otherwise False

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathGT('2', '1')}" />
</project>

```





#### String MathGTEQ(Double a,Double b) ####
##### Summary #####
Compare two numbers to determine if first number is greater than or equal to second.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
True if greater than or equal, otherwise False

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathGTEQ('2', '1')}" />
</project>

```





#### String MathEQ(Double a,Double b) ####
##### Summary #####
Compare two numbers to determine if first number is equal to second.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
True if equal, otherwise False




#### String MathAdd(Int32 a,Int32 b) ####
##### Summary #####
Add two integer numbers.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
The sum of two numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathAdd('1', '1')} == 2" />
</project>

```





#### String MathAddf(Double a,Double b) ####
##### Summary #####
Add two floating-point numbers.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
The sum of two numbers.

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathAddf('1.2', '1.2')} == 2.4" />
</project>

```





#### String MathSub(Int32 a,Int32 b) ####
##### Summary #####
Subtract two integer numbers.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
The difference of two numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathSub('4', '2')} == 2" />
</project>

```





#### String MathSubf(Double a,Double b) ####
##### Summary #####
Subtract two floating-point numbers.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
The difference of two numbers.

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathSubf('2.0', '1.0')} == 1" />
</project>

```





#### String MathMul(Int32 a,Int32 b) ####
##### Summary #####
Multiply two integer numbers.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
The product of two numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathMul('3', '3')} == 9" />
</project>

```





#### String MathMulf(Double a,Double b) ####
##### Summary #####
Multiply two floating-point numbers.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
The product of two numbers.

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathMulf('1.2', '2.0')} == 2.4" />
</project>

```





#### String MathMod(Int32 a,Int32 b) ####
##### Summary #####
Computes the modulo of two numbers.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
The modulo of two numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathMod('24', '18')} == 6" />
</project>

```





#### String MathModf(Double a,Double b) ####
##### Summary #####
Computes the modulo of two numbers.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
The modulo of two numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathModf('24.0', '18.0')} == 6" />
</project>

```





#### String MathDiv(Int32 a,Int32 b) ####
##### Summary #####
Divide two integer numbers.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
The division of two numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathDiv('6', '3')} == 2" />
</project>

```





#### String MathDivf(Double a,Double b) ####
##### Summary #####
Divide two floating-point numbers.

###### Parameter:  project ######


###### Parameter:  a ######
The first number.

###### Parameter:  b ######
The second number.

###### Return Value: ######
The division of two numbers.

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathDivf('3.0', '2.0')} == 1.5" />
</project>

```





#### String MathAbs(Int32 a) ####
##### Summary #####
Calculates the absolute value of a specified number.

###### Parameter:  project ######


###### Parameter:  a ######
The number.

###### Return Value: ######
The absolute value of a specified number.

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathAbs('-1')} == 1" />
</project>

```





#### String MathAbsf(Double a) ####
##### Summary #####
Calculates the absolute value of a specified number.

###### Parameter:  project ######


###### Parameter:  a ######
The number.

###### Return Value: ######
The absolute value of a specified number.

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathAbsf('-1.2')} == 1.2" />
</project>

```





#### String MathCeiling(Double a) ####
##### Summary #####
Returns the smallest whole number greater than or equal to the specified number.

###### Parameter:  project ######


###### Parameter:  a ######
The number.

###### Return Value: ######
The smallest whole number greater than or equal to the specified number.

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathCeiling('1.2')} == 2" />
</project>

```





#### String MathFloor(Double a) ####
##### Summary #####
Returns the largest whole number less than or equal to the specified number.

###### Parameter:  project ######


###### Parameter:  a ######
The number.

###### Return Value: ######
The largest whole number less than or equal to the specified number.

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathFloor('1.2')} == 1" />
</project>

```





#### String MathPow(Double x,Double y) ####
##### Summary #####
Raise the specified number to the specified power.

###### Parameter:  project ######


###### Parameter:  x ######
The number.

###### Parameter:  y ######
The power.

###### Return Value: ######
The number x raised to the power y.

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathPow('2.0', '2.0')} == 4" />
</project>

```





#### String MathSqrt(Double a) ####
##### Summary #####
Computes the square root of a specified number.

###### Parameter:  project ######


###### Parameter:  a ######
The specified number.

###### Return Value: ######
The square root of a specified number.

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{MathSqrt('25.0')} == 5" />
</project>

```





#### String MathDegToRad(Int32 a) ####
##### Summary #####
Converts an angle from degrees to radians.

###### Parameter:  project ######


###### Parameter:  a ######
The specified number of degrees.

###### Return Value: ######
The degree of a.

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name='pi' value='@{MathPI()}' />
    <property name='rad' value='@{MathDivf("${pi}", "180")}' />
    <property name='deg' value='@{MathMulf("90", "${rad}")}' />
    
    <echo message="@{MathDegToRad('90')} == ${deg}" />
</project>

```





#### String MathSin(Double a) ####
##### Summary #####
Calculates the sine of the specified angle.

###### Parameter:  project ######


###### Parameter:  a ######
The specified angle in radians.

###### Return Value: ######
The sine of a.

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="deg" value="@{MathDegToRad('90')}" />
    <fail unless="@{MathSin('${deg}')} == 1" />
</project>

```





#### String MathCos(Double a) ####
##### Summary #####
Calculates the cosine of the specified angle.

###### Parameter:  project ######


###### Parameter:  a ######
The specified angle in radians.

###### Return Value: ######
The cosine of a.

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="deg" value="@{MathDegToRad('90')}" />
    <fail unless="@{MathCos('${deg}')} == 6.1230317691118863E-17" />
</project>

```





#### String MathTan(Double a) ####
##### Summary #####
Calculates the tangent of the specified angle.

###### Parameter:  project ######


###### Parameter:  a ######
The specified angle in radians.

###### Return Value: ######
The tangent of a.

##### Remarks #####
Full precision ensures that numbers converted to strings will
have the same value when they are converted back to numbers.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="deg" value="@{MathDegToRad('180')}" />
    <fail unless="@{MathTan('${deg}')} == -1.2246063538223773E-16" />
</project>

```





#### String MathChangePrecision(Double a,Int32 precision) ####
##### Summary #####
Changes the precision of a specified floating-point number.

###### Parameter:  project ######


###### Parameter:  a ######
The specified floating-point number.

###### Parameter:  precision ######
The new precision.

###### Return Value: ######
The value of a using the specified precision.

##### Remarks #####
By default NAnt math functions return doubles using full precision (17 bits). Typically only
15 bits are needed.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <eval code="@{MathSubf('2.2', '1.2')}" property='p' type='Function'/>
    <fail unless='${p} == 1.0000000000000002' />

    <eval code="@{MathChangePrecision('${p}', '15')}" property='p' type='Function'/>
    <fail unless='${p} == 1' />
</project>

```





