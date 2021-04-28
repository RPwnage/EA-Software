---
title: Character Functions
weight: 1003
---
## Summary ##
Collection of character manipulation routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String CharIsDigit(String s,Int32 index)]({{< relref "#string-charisdigit-string-s-int32-index" >}}) | Indicates whether the character at the specified position in a string is a decimal digit. |
| [String CharIsNumber(String s,Int32 index)]({{< relref "#string-charisnumber-string-s-int32-index" >}}) | Indicates whether the character at the specified position in a string is categorized<br>as a decimal digit or hexadecimal number. |
| [String CharIsWhiteSpace(String s,Int32 index)]({{< relref "#string-chariswhitespace-string-s-int32-index" >}}) | Indicates whether the character at the specified position in a string is categorized<br>as white space. |
| [String CharIsLetter(String s,Int32 index)]({{< relref "#string-charisletter-string-s-int32-index" >}}) | Indicates whether a character is categorized as an alphabetic letter. |
| [String CharIsLetterOrDigit(String s,Int32 index)]({{< relref "#string-charisletterordigit-string-s-int32-index" >}}) | Indicates whether the character at the specified position in a string is categorized as an<br>alphabetic character or a decimal digit. |
| [String CharToLower(Char c)]({{< relref "#string-chartolower-char-c" >}}) | Converts the value of a character to its lowercase equivalent. |
| [String CharToUpper(Char c)]({{< relref "#string-chartoupper-char-c" >}}) | Converts the value of a character to its uppercase equivalent. |
| [String CharIsLower(String s,Int32 index)]({{< relref "#string-charislower-string-s-int32-index" >}}) | Indicates whether the character at the specified position in a string is categorized as a lowercase letter. |
| [String CharIsUpper(String s,Int32 index)]({{< relref "#string-charisupper-string-s-int32-index" >}}) | Indicates whether the character at the specified position in a string is categorized as an uppercase letter. |
## Detailed overview ##
#### String CharIsDigit(String s,Int32 index) ####
##### Summary #####
Indicates whether the character at the specified position in a string is a decimal digit.

###### Parameter:  project ######


###### Parameter:  s ######
The string.

###### Parameter:  index ######
The character position in s.

###### Return Value: ######
Returns true if the character at position index in s is a decimal digit; otherwise, false.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{CharIsDigit('5', '0')}" />
</project>

```





#### String CharIsNumber(String s,Int32 index) ####
##### Summary #####
Indicates whether the character at the specified position in a string is categorized
as a decimal digit or hexadecimal number.

###### Parameter:  project ######


###### Parameter:  s ######
The string.

###### Parameter:  index ######
The character position in s.

###### Return Value: ######
Returns true if the character at position index in s is a decimal digit or hexadecimal number; otherwise, false.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{CharIsNumber('9', '0')}" />
</project>

```





#### String CharIsWhiteSpace(String s,Int32 index) ####
##### Summary #####
Indicates whether the character at the specified position in a string is categorized
as white space.

###### Parameter:  project ######


###### Parameter:  s ######
The string.

###### Parameter:  index ######
The character position in s.

###### Return Value: ######
Returns true if the character at position index in s is white space; otherwise, false.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{CharIsWhiteSpace(' ', '0')}" />
</project>

```





#### String CharIsLetter(String s,Int32 index) ####
##### Summary #####
Indicates whether a character is categorized as an alphabetic letter.

###### Parameter:  project ######


###### Parameter:  s ######
The string.

###### Parameter:  index ######
The character position in s.

###### Return Value: ######
Returns true if the character at position index in s is an alphabetic character; otherwise, false.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{CharIsLetter('a', '0')}" />
</project>

```





#### String CharIsLetterOrDigit(String s,Int32 index) ####
##### Summary #####
Indicates whether the character at the specified position in a string is categorized as an
alphabetic character or a decimal digit.

###### Parameter:  project ######


###### Parameter:  s ######
The string.

###### Parameter:  index ######
The character position in s.

###### Return Value: ######
Returns true if the character at position index in s is an alphabetic character or a decimal digit; otherwise, false.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{CharIsLetterOrDigit('a', '0')}" />
</project>

```





#### String CharToLower(Char c) ####
##### Summary #####
Converts the value of a character to its lowercase equivalent.

###### Parameter:  project ######


###### Parameter:  c ######
The character.

###### Return Value: ######
The lowercase equivalent of c.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{CharToLower('A')} == a" />
</project>

```





#### String CharToUpper(Char c) ####
##### Summary #####
Converts the value of a character to its uppercase equivalent.

###### Parameter:  project ######


###### Parameter:  c ######
The character.

###### Return Value: ######
The uppercase equivalent of c.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{CharToUpper('a')} == A" />
</project>

```





#### String CharIsLower(String s,Int32 index) ####
##### Summary #####
Indicates whether the character at the specified position in a string is categorized as a lowercase letter.

###### Parameter:  project ######


###### Parameter:  s ######
The string.

###### Parameter:  index ######
The character position in s.

###### Return Value: ######
Returns true if the character at position index in s is a lowercase letter; otherwise, false.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{CharIsLower('a', '0')}" />
</project>

```





#### String CharIsUpper(String s,Int32 index) ####
##### Summary #####
Indicates whether the character at the specified position in a string is categorized as an uppercase letter.

###### Parameter:  project ######


###### Parameter:  s ######
The string.

###### Parameter:  index ######
The character position in s.

###### Return Value: ######
Returns true if the character at position index in s is an uppercase letter; otherwise, false.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{CharIsUpper('A', '0')}" />
</project>

```





