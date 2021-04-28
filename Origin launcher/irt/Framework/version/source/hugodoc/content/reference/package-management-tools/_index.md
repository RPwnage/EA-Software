---
title: Package Management Tools
weight: 294
---

Framework provides a number of package management related tools for handling operations related to your packages and the package server.

![package]( package.png )<a name="PackageManagementTools"></a>
## Package Management Tools ##

The following are tools used for package management:
 - [Package Server]( {{< ref "reference/package-management-tools/packageserver.md" >}} )
 - [eapm]( {{< ref "reference/package-management-tools/eapm.md" >}} )

<a name="credentials"></a>
## Package Server Credentials ##

Access to package server is secured. When running on Windows Framework will use your domain account to access package server.
On Unix, Linux, MAC systems, or when you are not logged into EA domain account you can use use eapm tool to store
credentials.

Calling &quot;eapm password&quot; will interactively prompt the user for their login, password and domain and store them
in a encrypted credential store file for that user.

```
eapm password
```

{{% panel theme = "warning" header = "Warning:" %}}
This password encryption feature is not designed to be secure, the passwords are unencrypted by Framework before communicating them to the package server and anyone with access to Framework source code can unencrypt the password.
This is only designed to help in situations where someone might be watching as you type in your password or inadvertantly access the credential store file.
{{% /panel %}}
For situations where the password encryption needs to be automated there is the option to pass the password as an argument to eapm.

```
eapm password <password>
          
-or-
          
eapm password <username> <password> <domain>
```

{{% panel theme = "info" header = "Note:" %}}
In the past, Framework uses environment variables EAT_FRAMEWORK_NET_LOGIN, EAT_FRAMEWORK_NET_DOMAIN, and EAT_FRAMEWORK_NET_PASSWORD.  These environment variables are no longer being used.  So adding or updating these environment variables will not have any effects.
{{% /panel %}}

##### Related Topics: #####
-  [What Is Framework Package?]( {{< ref "reference/packages/whatispackage.md" >}} ) 
-  [Package Server](http://packages.ea.com) 
