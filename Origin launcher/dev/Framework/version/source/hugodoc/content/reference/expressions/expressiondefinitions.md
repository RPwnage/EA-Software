---
title: Expression Definitions
weight: 239
---

This topics contains NAnt expression definitions

<a name="ExpressionDefinitions"></a>
## Expression Definitions ##

The expression parser evaluates string and Boolean expressions down to a true or false value.
Boolean values are denoted by the strings true and false. Most of the standard C comparison and logical operators are defined. Here is the complete set of operators in precedence order.
Those on the same lines are equal precedence.


```

    ()
    ! not
    < <= >= > lt lte gte gt
    ==  != eq neq
    && and
    || or

```

{{% panel theme = "info" header = "Note:" %}}
Notice the use of English like operators. These are useful to get around the fact that XML restricts the use &lt;, &gt;, and &amp; characters. These operators need to prefixed with whitespace.
{{% /panel %}}
All strings are case sensitive, with this slight exception; the strings true and false are special cased to be Boolean values,
so case sensitivity is ignored. Strings can be free form or quoted. In other words, they do not require quotes to delimit them; however quotes can be used if desired.
Any of the following can be used as quote characters, &#39; (single quote), &quot; (double quote), or ` (back-tick).
If a quote character is used to start a string, the same character must be used to finish the string.
If a quote character is not used, the first operator character or Boolean string detected will terminate the string.
For freeform strings, white space on is trimmed from both the start and end.


```

    string :: '[any chars]' |
              "[any chars]" |
              `[any chars]` |
               [any chars that don't include an operator or a Boolean.]

```
In addition, any of the quote-delimited forms can include the quote delimiter character if it is escaped by the backslash character, for example:


```

  “this \“ has a quote“ == ‘this has “ a quote‘

  “this is not a legal string`     (mismatched quotes)
  “this string is not legal\“      (end quote is escaped)
  “A legal string, finally“        (legal)
  ‘A legal string \‘too\‘‘         (legal)
  `A plethora of “legality“`       (legal)
  Here is a free-form string.      (legal)
  'Case Matters' != 'case matters' (legal)

```
Expressions are defined as shown. String comparisons are accomplished using the C# String.CompareTo() function, so follow those rules for all the comparison operators.


```

  expression :: boolean                     |
                ( expression )              |
                unary_operator expression   |
                expression binary_operator expression

```
An empty expression or a null expression generates an exception. In fact, any expression that does not resolve down to a Boolean will generate an exception.
Booleans and strings won’t intermix. For example, the expression abc != true will generate an exception since the left side is a string and the right side is a Boolean.


{{% panel theme = "warning" header = "Warning:" %}}
Property references will be expanded before the expression is evaluated.
For example, if the property nant.junk has the value &#39;this is junk&#39;, the expression &#39;${nant.junk} == test&#39;
will be expanded to &#39;this is junk == test&#39;, then evaluated to false.
{{% /panel %}}
## Example ##

Here are a few random expressions:


```

    abc == abc               (amazingly true)
    ' abc ' == ' abc '       (use quotes for strings with spaces)
    de  != abc               (once again true)
    a lt d                   (true)
    a lt d == true           (true, a < d evaluates to true, then true == true)
    a lt (d == true)         (generates an exception, mixing strings and booleans)
    (a lt d) and (z > x)     (true)
    abc == true              (not only false, but causes an exception)
    !true                    (false)
    '${nant.version} >= 0.2' (depends on the value of property nant.version)

```

##### Related Topics: #####
-  [Expression Evaluation]( {{< ref "reference/expressions/expressionevaluation.md" >}} ) 
-  [?? Operator]( {{< ref "reference/expressions/nullcoalescingoperator.md" >}} ) 
