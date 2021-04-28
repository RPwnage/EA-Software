---
title: ?? Operator
weight: 241
---

The ?? operator is called the null-coalescing operator and is used to define a default value for NAnt properties.

<a name="Section1"></a>
## ?? Operator ##

Operator ?? can be used to define default value for NAnt property. It provides shorter and faster alternative to `@{PropertyExists()}` function.

Operator ?? is used inside property evaluation operator. The result of property evaluation will be a property value or `default-property-value` if property does not exist:


```

   ${property-name??default-property-value}

```
The string after ?? operation can contain other property evaluation operators or functions:


```

   <property name="result" value=${some-property??${other-property??Default Result Value}}

```
Property `result`  will contain value of property  `some-property` , in case  `some-property`  does not exist property  `result` will contain value of the property `other-property` , and when  `other-property`  does not exist property  `result` will be assigned string `"Default Result Value"` .

To set empty string as a default value:


```

   ${property-name??}

```

{{% panel theme = "warning" header = "Warning:" %}}
The default value string specified after ?? operator is expanded regardless of existence of the property. I.e.
operator `${some-property??${other-property}}`  will fail in case property  *some-property* exists, but property *other-property* does not.

To avoid such failures use second ?? operator: `${some-property??${other-property??}}`
{{% /panel %}}
