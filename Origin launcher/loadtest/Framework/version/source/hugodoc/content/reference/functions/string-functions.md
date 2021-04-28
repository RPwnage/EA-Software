---
title: String Functions
weight: 1017
---
## Summary ##
Collection of string manipulation routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String StrLen(String strA)]({{< relref "#string-strlen-string-stra" >}}) | Gets the number of characters in a string. |
| [String StrEndsWith(String strA,String strB)]({{< relref "#string-strendswith-string-stra-string-strb" >}}) | Determines whether the end of a string matches a string. |
| [String StrStartsWith(String strA,String strB)]({{< relref "#string-strstartswith-string-stra-string-strb" >}}) | Determines whether the start of a string matches a string. |
| [String StrCompare(String strA,String strB)]({{< relref "#string-strcompare-string-stra-string-strb" >}}) | Compares two specified strings. |
| [String StrCompareVersions(String strA,String strB)]({{< relref "#string-strcompareversions-string-stra-string-strb" >}}) | Compares two specified version strings.  Version strings should be numbers<br>separated by any non digit separator. Separators are used in comparison as well.<br>Dot separator will take a precedence over dash separator.<br>Numbers are always considered newer than letters, version strings that are starting with &quot;dev&quot; or &quot;work&quot; are considered a special case, dev and work takes precedence over numbers<br>            <br>For example, versions will be sorted this way in descending order:<br>test-09<br>test-8-1<br>test-8.1-alpha<br>test-8.1<br>test-8<br>Work and dev will be newer than numeric versions, but only if package name is not included:<br>dev<br>1.01.00<br>This function assumes that separators used in both version strings are the same. |
| [String StrVersionLess(String strA,String strB)]({{< relref "#string-strversionless-string-stra-string-strb" >}}) | Compares two specified version strings in the same way as StrCompareVersions.<br>However returns a boolean value of true if strA is less than strB. |
| [String StrVersionLessOrEqual(String strA,String strB)]({{< relref "#string-strversionlessorequal-string-stra-string-strb" >}}) | Compares two specified version strings in the same way as StrCompareVersions.<br>However returns a boolean value of true if strA is less than or equal to strB. |
| [String StrVersionGreater(String strA,String strB)]({{< relref "#string-strversiongreater-string-stra-string-strb" >}}) | Compares two specified version strings in the same way as StrCompareVersions.<br>However returns a boolean value of true if strA is greater than strB. |
| [String StrVersionGreaterOrEqual(String strA,String strB)]({{< relref "#string-strversiongreaterorequal-string-stra-string-strb" >}}) | Compares two specified version strings in the same way as StrCompareVersions.<br>However returns a boolean value of true if strA is greater than or equal to strB. |
| [String StrLastIndexOf(String strA,String strB)]({{< relref "#string-strlastindexof-string-stra-string-strb" >}}) | Reports the index position of the last occurrence of a string within a string. |
| [String StrIndexOf(String strA,String strB)]({{< relref "#string-strindexof-string-stra-string-strb" >}}) | Reports the index of the first occurrence of a string in a string. |
| [String StrContains(String strA,String strB)]({{< relref "#string-strcontains-string-stra-string-strb" >}}) | Reports if the first string contains the second string. |
| [String StrRemove(String strA,Int32 startIndex,Int32 count)]({{< relref "#string-strremove-string-stra-int32-startindex-int32-count" >}}) | Deletes a specified number of characters from the specified string beginning at a specified position. |
| [String StrReplace(String strA,String oldValue,String newValue)]({{< relref "#string-strreplace-string-stra-string-oldvalue-string-newvalue" >}}) | Replaces all occurrences of a string in a string, with another string. |
| [String StrInsert(String strA,Int32 startIndex,String value)]({{< relref "#string-strinsert-string-stra-int32-startindex-string-value" >}}) | Inserts a string at a specified index position in a string. |
| [String StrSubstring(String strA,Int32 startIndex)]({{< relref "#string-strsubstring-string-stra-int32-startindex" >}}) | Retrieves a substring from a string. The substring starts at a specified character position. |
| [String StrSubstring(String strA,Int32 startIndex,Int32 length)]({{< relref "#string-strsubstring-string-stra-int32-startindex-int32-length" >}}) | Retrieves a substring from a string. The substring starts at a specified character position. |
| [String StrConcat(String strA,String strB)]({{< relref "#string-strconcat-string-stra-string-strb" >}}) | Concatenates two strings. |
| [String StrPadLeft(String strA,Int32 totalWidth)]({{< relref "#string-strpadleft-string-stra-int32-totalwidth" >}}) | Right-aligns the characters a string, padding with spaces on the left for a total length. |
| [String StrPadRight(String strA,Int32 totalWidth)]({{< relref "#string-strpadright-string-stra-int32-totalwidth" >}}) | Left-aligns the characters in a string, padding with spaces on the left for a total length. |
| [String StrTrim(String strA)]({{< relref "#string-strtrim-string-stra" >}}) | Removes all occurrences of white space characters from the beginning and end of a string. |
| [String StrTrimWhiteSpace(String strA)]({{< relref "#string-strtrimwhitespace-string-stra" >}}) | Removes all occurrences of white space characters (including tabs and new lines)from the beginning and end of a string. |
| [String StrToUpper(String strA)]({{< relref "#string-strtoupper-string-stra" >}}) | Returns a copy of the given string in uppercase. |
| [String StrToLower(String strA)]({{< relref "#string-strtolower-string-stra" >}}) | Returns a copy of the given string in lowercase. |
| [String StrPascalCase(String strA)]({{< relref "#string-strpascalcase-string-stra" >}}) | Takes a space separated list of words and joins them with only the first letter of each capitalized |
| [String StrEcho(String strA)]({{< relref "#string-strecho-string-stra" >}}) | Echo a string. |
| [String StrIsEmpty(String strA)]({{< relref "#string-strisempty-string-stra" >}}) | Returns true if the specified string is the Empty string. |
| [String StrSelectIf(String strCondition,String strTrue,String strFalse)]({{< relref "#string-strselectif-string-strcondition-string-strtrue-string-strfalse" >}}) | Returns &quot;strTrue&quot; or &quot;strFalse&quot; depending on value of conditional |
| [String DistinctItems(String str)]({{< relref "#string-distinctitems-string-str" >}}) | Eliminates duplicates from the list of space separated items. |
| [String DistinctItemsCustomSeparator(String str,String sep)]({{< relref "#string-distinctitemscustomseparator-string-str-string-sep" >}}) | Eliminates duplicates from the list of items. |
## Detailed overview ##
#### String StrLen(String strA) ####
##### Summary #####
Gets the number of characters in a string.

###### Parameter:  project ######


###### Parameter:  strA ######
The string to count.

###### Return Value: ######
The number of characters in the specified string.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{StrLen('nant.exe')} == 8" />
</project>

```





#### String StrEndsWith(String strA,String strB) ####
##### Summary #####
Determines whether the end of a string matches a string.

###### Parameter:  project ######


###### Parameter:  strA ######
The string to search.

###### Parameter:  strB ######
The string to match.

###### Return Value: ######
True if the specified string strA ends with the specified string strB.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{StrEndsWith('nant.exe', '.exe')}" />
</project>

```





#### String StrStartsWith(String strA,String strB) ####
##### Summary #####
Determines whether the start of a string matches a string.

###### Parameter:  project ######


###### Parameter:  strA ######
The string to search.

###### Parameter:  strB ######
The string to match.

###### Return Value: ######
True if the specified string, strA, starts with the specified string, strB.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{StrStartsWith('nant.exe', 'nant')}" />
</project>

```





#### String StrCompare(String strA,String strB) ####
##### Summary #####
Compares two specified strings.

###### Parameter:  project ######


###### Parameter:  strA ######
The first String.

###### Parameter:  strB ######
The second String.

###### Return Value: ######
Less than zero - strA is less than strB. &lt;br/&gt;
Zero - strA is equal to strB. &lt;br/&gt;
Greater than zero - strA is greater than strB. &lt;br/&gt;

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{StrCompare('nant', 'nant')} == 0" />
</project>

```





#### String StrCompareVersions(String strA,String strB) ####
##### Summary #####
Compares two specified version strings.  Version strings should be numbers
separated by any non digit separator. Separators are used in comparison as well.
Dot separator will take a precedence over dash separator.
Numbers are always considered newer than letters, version strings that are starting with &quot;dev&quot; or &quot;work&quot; are considered a special case, dev and work takes precedence over numbers
            
For example, versions will be sorted this way in descending order:
test-09
test-8-1
test-8.1-alpha
test-8.1
test-8
Work and dev will be newer than numeric versions, but only if package name is not included:
dev
1.01.00
This function assumes that separators used in both version strings are the same.

###### Parameter:  project ######


###### Parameter:  strA ######
The first version String.

###### Parameter:  strB ######
The second version String.

###### Return Value: ######
Less than zero - strA is less than strB. &lt;br/&gt;
Zero - strA is equal to strB. &lt;br/&gt;
Greater than zero - strA is greater than strB. &lt;br/&gt;
   Comparison algorithm takes into account version formatting, rather than doing simple lexicographical comparison.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{StrCompareVersions('1.00.00', '1.00.00')} == 0" />
</project>

```





#### String StrVersionLess(String strA,String strB) ####
##### Summary #####
Compares two specified version strings in the same way as StrCompareVersions.
However returns a boolean value of true if strA is less than strB.

###### Parameter:  project ######


###### Parameter:  strA ######
The first version String.

###### Parameter:  strB ######
The second version String.

###### Return Value: ######
true if strA is less than strB, false otherwise

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{StrVersionLess('1.05.00', '1.10.00')}" />
</project>

```





#### String StrVersionLessOrEqual(String strA,String strB) ####
##### Summary #####
Compares two specified version strings in the same way as StrCompareVersions.
However returns a boolean value of true if strA is less than or equal to strB.

###### Parameter:  project ######


###### Parameter:  strA ######
The first version String.

###### Parameter:  strB ######
The second version String.

###### Return Value: ######
true if strA is less than or equal to strB, false otherwise

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{StrVersionLessOrEqual('1.05.00', '1.10.00')}" />
</project>

```





#### String StrVersionGreater(String strA,String strB) ####
##### Summary #####
Compares two specified version strings in the same way as StrCompareVersions.
However returns a boolean value of true if strA is greater than strB.

###### Parameter:  project ######


###### Parameter:  strA ######
The first version String.

###### Parameter:  strB ######
The second version String.

###### Return Value: ######
true if strA is greater than strB, false otherwise

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{StrVersionGreater('1.10.00', '1.05.00')}" />
</project>

```





#### String StrVersionGreaterOrEqual(String strA,String strB) ####
##### Summary #####
Compares two specified version strings in the same way as StrCompareVersions.
However returns a boolean value of true if strA is greater than or equal to strB.

###### Parameter:  project ######


###### Parameter:  strA ######
The first version String.

###### Parameter:  strB ######
The second version String.

###### Return Value: ######
true if strA is greater than or equal to strB, false otherwise

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{StrVersionGreaterOrEqual('1.10.00', '1.05.00')}" />
</project>

```





#### String StrLastIndexOf(String strA,String strB) ####
##### Summary #####
Reports the index position of the last occurrence of a string within a string.

###### Parameter:  project ######


###### Parameter:  strA ######
The string to search.

###### Parameter:  strB ######
The string to seek.

###### Return Value: ######
The last index position of string strB in string strA if found; otherwise -1 if not found.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{StrLastIndexOf('nant.exe.config', '.')} == 8" />
</project>

```





#### String StrIndexOf(String strA,String strB) ####
##### Summary #####
Reports the index of the first occurrence of a string in a string.

###### Parameter:  project ######


###### Parameter:  strA ######
The string to search.

###### Parameter:  strB ######
The string to seek.

###### Return Value: ######
The index position of string strB in string strA if found; otherwise -1 if not found.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{StrIndexOf('nant.exe', '.')} == 4" />
</project>

```





#### String StrContains(String strA,String strB) ####
##### Summary #####
Reports if the first string contains the second string.

###### Parameter:  project ######


###### Parameter:  strA ######
The string to search.

###### Parameter:  strB ######
The string to seek.

###### Return Value: ######
true if strB is found in  string strA; otherwise false if not found.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{StrContains('nant.exe', 'nant')}" />
</project>

```





#### String StrRemove(String strA,Int32 startIndex,Int32 count) ####
##### Summary #####
Deletes a specified number of characters from the specified string beginning at a specified position.

###### Parameter:  project ######


###### Parameter:  strA ######
The string.

###### Parameter:  startIndex ######
The position in the string to begin deleting characters.

###### Parameter:  count ######
The number of characters to delete.

###### Return Value: ######
A string having count characters removed from strA starting at startIndex.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="'@{StrRemove('nant.exe', '4', '1')}' == 'nantexe'" />
</project>

```





#### String StrReplace(String strA,String oldValue,String newValue) ####
##### Summary #####
Replaces all occurrences of a string in a string, with another string.

###### Parameter:  project ######


###### Parameter:  strA ######
The string.

###### Parameter:  oldValue ######
The string to be replaced.

###### Parameter:  newValue ######
The string to replace all occurrences of oldValue.

###### Return Value: ######
A string having every occurrence of oldValue in strA replaced with newValue.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="'@{StrReplace('nant.exe', '.exe', '.config')}' == 'nant.config'" />
</project>

```





#### String StrInsert(String strA,Int32 startIndex,String value) ####
##### Summary #####
Inserts a string at a specified index position in a string.

###### Parameter:  project ######


###### Parameter:  strA ######
The string to insert into.

###### Parameter:  startIndex ######
The index position of the insertion.

###### Parameter:  value ######
The string to insert.

###### Return Value: ######
A string equivalent to strA but with value inserted at position startIndex.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="'@{StrInsert('.exe', '0', 'nant')}' == 'nant.exe'" />
</project>

```





#### String StrSubstring(String strA,Int32 startIndex) ####
##### Summary #####
Retrieves a substring from a string. The substring starts at a specified character position.

###### Parameter:  project ######


###### Parameter:  strA ######
The string.

###### Parameter:  startIndex ######
The starting character position of a substring in this instance.

###### Return Value: ######
A substring of strA starting at startIndex.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="'@{StrSubstring('nant.exe', '4')}' == '.exe'" />
</project>

```





#### String StrSubstring(String strA,Int32 startIndex,Int32 length) ####
##### Summary #####
Retrieves a substring from a string. The substring starts at a specified character position.

###### Parameter:  project ######


###### Parameter:  strA ######
The string.

###### Parameter:  startIndex ######
The starting character position of a substring in this instance.

###### Return Value: ######
A substring of strA starting at startIndex.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="'@{StrSubstring('nant.exe', '4')}' == '.exe'" />
</project>

```





#### String StrConcat(String strA,String strB) ####
##### Summary #####
Concatenates two strings.

###### Parameter:  project ######


###### Parameter:  strA ######
The first string.

###### Parameter:  strB ######
The second string.

###### Return Value: ######
The concatenation of strA and strB.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="'@{StrConcat('nant', '.exe')}' == 'nant.exe'" />
</project>

```





#### String StrPadLeft(String strA,Int32 totalWidth) ####
##### Summary #####
Right-aligns the characters a string, padding with spaces on the left for a total length.

###### Parameter:  project ######


###### Parameter:  strA ######
The string to pad.

###### Parameter:  totalWidth ######
The number of characters in the resulting string, equal to the number of original characters plus
any additional padding characters.

###### Return Value: ######
A string that is equivalent to strA, but right-aligned and padded on the left with as many
spaces as needed to create a length of totalWidth.
If totalWidth is less than the length of strA, a string that is identical to strA is returned.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="'@{StrPadLeft('nant', '8')}' == '    nant'" />
</project>

```





#### String StrPadRight(String strA,Int32 totalWidth) ####
##### Summary #####
Left-aligns the characters in a string, padding with spaces on the left for a total length.

###### Parameter:  project ######


###### Parameter:  strA ######
The string to pad.

###### Parameter:  totalWidth ######
The number of characters in the resulting string, equal to the number of original characters plus
any additional padding characters.

###### Return Value: ######
A string that is equivalent to strA, but left-aligned and padded on the right with as many
spaces as needed to create a length of totalWidth.
If totalWidth is less than the length of strA, a string that is identical to strA is returned.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="'@{StrPadRight('nant', '8')}' == 'nant    '" />
</project>

```





#### String StrTrim(String strA) ####
##### Summary #####
Removes all occurrences of white space characters from the beginning and end of a string.

###### Parameter:  project ######


###### Parameter:  strA ######
The string to trim.

###### Return Value: ######
A string equivalent to strA after white space characters are removed.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="'@{StrTrim('  nant  ')}' == 'nant'" />
</project>

```





#### String StrTrimWhiteSpace(String strA) ####
##### Summary #####
Removes all occurrences of white space characters (including tabs and new lines)from the beginning and end of a string.

###### Parameter:  project ######


###### Parameter:  strA ######
The string to trim.

###### Return Value: ######
A string equivalent to strA after white space characters are removed.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="'@{StrTrim('  nant  ')}' == 'nant'" />
</project>

```





#### String StrToUpper(String strA) ####
##### Summary #####
Returns a copy of the given string in uppercase.

###### Parameter:  project ######


###### Parameter:  strA ######
The string.

###### Return Value: ######
A string in uppercase.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="'@{StrToUpper('nant')}' == 'NANT'" />
</project>

```





#### String StrToLower(String strA) ####
##### Summary #####
Returns a copy of the given string in lowercase.

###### Parameter:  project ######


###### Parameter:  strA ######
The string.

###### Return Value: ######
A string in lowercase.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="'@{StrToLower('NANT')}' == 'nant'" />
</project>

```





#### String StrPascalCase(String strA) ####
##### Summary #####
Takes a space separated list of words and joins them with only the first letter of each capitalized

###### Parameter:  project ######


###### Parameter:  strA ######
A space separate list of words

###### Return Value: ######
A one word string composed of all of the input words joined and with only the starting letter of each capitalized

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{StrPascalCase('hello world')} eq 'HelloWorld'" />
</project>

```





#### String StrEcho(String strA) ####
##### Summary #####
Echo a string.

###### Parameter:  project ######


###### Parameter:  strA ######
The string.

###### Return Value: ######
The string.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="'@{StrEcho('nant')}' == 'nant'" />
</project>

```





#### String StrIsEmpty(String strA) ####
##### Summary #####
Returns true if the specified string is the Empty string.

###### Parameter:  project ######


###### Parameter:  strA ######
The string.

###### Return Value: ######
True of False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{StrIsEmpty('')}" />
</project>

```





#### String StrSelectIf(String strCondition,String strTrue,String strFalse) ####
##### Summary #####
Returns &quot;strTrue&quot; or &quot;strFalse&quot; depending on value of conditional

###### Parameter:  project ######


###### Parameter:  strCondition ######
The string to determine whether True or False.

###### Parameter:  strTrue ######
String to return when conditional evaluates to True

###### Parameter:  strFalse ######
String to return when conditional evaluates to False

###### Return Value: ######
Either strTrue or strFalse

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="'@{StrSelectIf('True', 'IsTrue', 'IsFalse')}' == 'IsTrue'" />
</project>

```





#### String DistinctItems(String str) ####
##### Summary #####
Eliminates duplicates from the list of space separated items.

###### Parameter:  project ######


###### Parameter:  str ######
The string to process.

###### Return Value: ######
A Space separated String containing distinct items.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{DistinctItems('a b f c d d e f g')} eq 'a b f c d e g'" />
</project>

```





#### String DistinctItemsCustomSeparator(String str,String sep) ####
##### Summary #####
Eliminates duplicates from the list of items.

###### Parameter:  project ######


###### Parameter:  str ######
The string to process.

###### Parameter:  sep ######
A custom separator string

###### Return Value: ######
String containing distinct items.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail unless="@{DistinctItemsCustomSeparator('a b f c d d e f g', '|')} eq 'a|b|f|c|d|e|g'" />
</project>

```





