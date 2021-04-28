---
title: < mail > Task
weight: 1108
---
## Syntax
```xml
<mail from="" tolist="" cclist="" bcclist="" mailhost="" message="" subject="" format="" files="" attachments="" async="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
A task to send email.

## Remarks ##
Text and text files to include in the message body may be specified as well as binary attachments.

### Example ###
Sends an email from nant@sourceforge.net to three recipients with a subject about the
attachments.  The body of the message will be the combined contents of body1.txt through
body4.txt.  The body1.txt through body3.txt files will also be included as attachments.
The message will be sent using the smtpserver.anywhere.com SMTP server.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <target name="sendmail" description="Send email">
        <mail
            from="nant@sourceforge.net"
            tolist="recipient1@sourceforge.net"
            cclist="recipient2@sourceforge.net"
            bcclist="recipient3@sourceforge.net"
            subject="Msg 7: With attachments"
            files="body1.txt,body2.txt;body3.txt,body4.txt"
            attachments="body1.txt,body2.txt;,body3.txt"
            mailhost="smtpserver.anywhere.com"
        />
    </target>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| from | Email address of sender  | String | True |
| tolist | Comma- or semicolon-separated list of recipient email addresses | String | True |
| cclist | Comma- or semicolon-separated list of CC: recipient email addresses  | String | False |
| bcclist |  Comma- or semicolon-separated list of BCC: recipient email addresses | String | False |
| mailhost | Host name of mail server. Defaults to &quot;localhost&quot; | String | False |
| message | Text to send in body of email message. At least one of the fields &#39;files&#39; or &#39;message&#39; must be provided. | String | False |
| subject | Text to send in subject line of email message. | String | False |
| format | Format of the message body. Valid values are &quot;Html&quot; or &quot;Text&quot;.  Defaults to &quot;Text&quot;. | String | False |
| files | Name(s) of text files to send as part of body of the email message.<br>Multiple file names are comma- or semicolon-separated. At least one of the fields &#39;files&#39; or &#39;message&#39; must be provided. | String | False |
| attachments | Name(s) of files to send as attachments to email message.<br>Multiple file names are comma- or semicolon-separated. | String | False |
| async | Sends the message asynchronously rather than waiting for the task to send the message before proceeding | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<mail from="" tolist="" cclist="" bcclist="" mailhost="" message="" subject="" format="" files="" attachments="" async="" failonerror="" verbose="" if="" unless="" />
```
