// NAnt - A .NET build tool
// Copyright (C) 2001 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// 
// 
// Jay Turpin (jayturpin@hotmail.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Text;
using System.Net.Mail;
using System.IO;

using NAnt.Core.Attributes;
using NAnt.Core;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks { 

	/// <summary>A task to send email.</summary>
    /// <remarks>
    /// Text and text files to include in the message body may be specified as well as binary attachments.
    /// </remarks>
    /// <include file='Examples/Mail/Mail.example' path='example'/>
   [TaskName("mail")]
    public class MailTask : Task
    {
        
        string _from = "";      
        string _toList = "";        
        string _ccList = "";        
        string _bccList = "";        
        string _mailHost = "localhost";
        string _subject = "";        
        string _message = "";        
        string _files = "";        
        string _attachments = "";       
        bool _isFormatHtml = false;
  
        /// <summary>Email address of sender </summary>
        [TaskAttribute("from", Required=true)]
        public string From { get { return _from; } set { _from = value; } }
        
        /// <summary>Comma- or semicolon-separated list of recipient email addresses</summary>
        [TaskAttribute("tolist", Required=true)]
        public string ToList {
            // convert to semicolon delimited
            get { return _toList.Replace(";" , ","); }
            set { _toList = value.Replace(";" , ","); }
        }

        /// <summary>Comma- or semicolon-separated list of CC: recipient email addresses </summary>
        [TaskAttribute("cclist")]
        public string CcList { 
            // convert to semicolon delimited
            get { return _ccList.Replace(";" , ","); } 
            set { _ccList = value.Replace(";" , ","); }
        }

        /// <summary> Comma- or semicolon-separated list of BCC: recipient email addresses</summary>
        [TaskAttribute("bcclist")]
        public string BccList { 
            // convert to semicolon delimited
            get { return _bccList.Replace(";" , ","); } 
            set { _bccList = value.Replace(";" , ","); }
        }

        /// <summary>Host name of mail server. Defaults to "localhost"</summary>
        [TaskAttribute("mailhost")]
        public string Mailhost { get { return _mailHost; } set { _mailHost = value; } }

        /// <summary>Text to send in body of email message. At least one of the fields 'files' or 'message' must be provided.</summary>
        [TaskAttribute("message")]
        public string Message { get { return _message; } set { _message = value; } }

        /// <summary>Text to send in subject line of email message.</summary>
        [TaskAttribute("subject")]
        public string Subject { get { return _subject; } set { _subject = value; } }
       
        /// <summary>Format of the message body. Valid values are "Html" or "Text".  Defaults to "Text".</summary>
        [TaskAttribute("format")]
        public string Format {
           get { return _isFormatHtml ? "Html" : "Text"; }
            set { _isFormatHtml = (value != null && 0 == String.Compare("Html", value)) ? true : false; } 
        }
       
       /// <summary>Name(s) of text files to send as part of body of the email message. 
        /// Multiple file names are comma- or semicolon-separated. At least one of the fields 'files' or 'message' must be provided.
        /// </summary>
        [TaskAttribute("files")]
        public string Files { 
            // convert to semicolon delimited
            get { return _files.Replace("," , ";"); } 
            set { _files = value.Replace("," , ";"); }
        }

        /// <summary>Name(s) of files to send as attachments to email message.
        /// Multiple file names are comma- or semicolon-separated.
        /// </summary>
        [TaskAttribute("attachments")]
        public string Attachments { 
            // convert to semicolon delimited
            get { return _attachments.Replace("," , ";"); } 
            set { _attachments = value.Replace("," , ";"); }
        }

        ///<summary>Initializes task and ensures the supplied attributes are valid.</summary>
        ///<param name="taskNode">Xml node used to define this task instance.</param>
        protected override void InitializeTask(System.Xml.XmlNode taskNode) {

            if (From.Length == 0) {
                throw new BuildException("Mail attribute \"from\" is required.", Location);
            }

            if (ToList.Length == 0 && CcList.Length == 0 && BccList.Length == 0) {
                throw new BuildException("Mail must provide at least one of these attributes: \"tolist\", \"cclist\" or \"bcclist\".", Location);
            }

        }

        /// <summary>
        /// This is where the work is done
        /// </summary>
        protected override void ExecuteTask() {

            MailMessage mailMessage = new MailMessage();
            
            mailMessage.From = new MailAddress(this.From);
            mailMessage.To.Add(this.ToList);
            if (!String.IsNullOrEmpty(this.BccList))
            {
                mailMessage.Bcc.Add(this.BccList);
            }
            if (!String.IsNullOrEmpty(this.CcList))
            {
                mailMessage.CC.Add(this.CcList);
            }
            mailMessage.Subject = this.Subject;
            mailMessage.IsBodyHtml = this._isFormatHtml;

            // Begin build message body
            StringWriter bodyWriter = new StringWriter();
            if (Message.Length > 0) {
                bodyWriter.WriteLine(Message);
                bodyWriter.WriteLine();
            }

            

            // Append file(s) to message body
            if (Files.Length > 0) {
                string[] fileList = Files.Split(new char[]{';'});
                string content;
                if ( fileList.Length == 1 ) {
                   content = ReadFile(fileList[0]);
                   if ( content != null )
                     bodyWriter.Write(content);
                } else {
                  foreach (string fileName in fileList) {
                     content = ReadFile(fileName);
                     if ( content != null ) {
                        bodyWriter.WriteLine(fileName);
                        bodyWriter.WriteLine(content);
                        bodyWriter.WriteLine("");
                     }
                  }
                }
            }

            // add message body to mailMessage
            string bodyText = bodyWriter.ToString();
            if (bodyText.Length != 0) {
                mailMessage.Body = bodyText;
            } else {
                throw new BuildException("Mail attribute \"file\" or \"message\" is required.");
            }

            // Append file(s) to message body
            if (Attachments.Length > 0) {
                string[] attachList = Attachments.Split(new char[]{';'});
                foreach (string fileName in attachList) {
                    try {
                        // MailAttachment likes fully-qualified file names, use FileInfo to get them
                        FileInfo fileInfo = new FileInfo(fileName);
                        Attachment attach = new Attachment(fileInfo.FullName);
                        mailMessage.Attachments.Add(attach);
                    } catch {
                        string msg = "WARNING! File \"" + fileName + "\" NOT attached to message. File does not exist or cannot be accessed. Check: " + Location.ToString() + "attachments=\"" + Attachments + "\"";
                        Log.Warning.WriteLine(LogPrefix + msg);
                    }
                }
            }

            // send message
            try {
                SmtpClient client = new SmtpClient(this.Mailhost);
                client.Send(mailMessage);

                // Create log entry:
                Log.Info.WriteLine(LogPrefix + String.Format("Sent mail TO={0}; CC={1}; BCC={2}", mailMessage.To.ToString(), mailMessage.CC.ToString(), mailMessage.Bcc.ToString()));
                Log.Info.WriteLine(LogPrefix + "Subject: " + mailMessage.Subject);
                foreach(Attachment item in mailMessage.Attachments)
                {
                    Log.Info.WriteLine(LogPrefix + String.Format("   Attachment: {0}", item.Name));
                }                

            } catch (Exception e) {
                StringBuilder msg = new StringBuilder();
                msg.Append(LogPrefix + "Error enountered while sending mail message.\n");
                msg.Append(LogPrefix + "Make sure that mailhost=" + this.Mailhost + " is valid\n");
                msg.Append(LogPrefix + "Internal Message: " + e.Message + "\n");
                msg.Append(LogPrefix + "Stack Trace:\n");
                msg.Append(e.ToString() + "\n");
                throw new BuildException("Error sending mail:\n " + msg.ToString());
            }
        }

       /// <summary>
       /// Reads a text file and returns the contents
       /// in a string
       /// </summary>
       /// <param name="filename"></param>
       /// <returns></returns>
       private string ReadFile(string filename)
       {
          StreamReader reader = null;
          try {
             reader = new StreamReader(File.OpenRead(filename));
             return reader.ReadToEnd();
          } catch {
             string msg = "WARNING! File \"" + filename + "\" NOT added to message body. File does not exist or is not readable. Check: " + Location.ToString() + "files=\"" + Files + "\"";
             Log.Status.WriteLine(LogPrefix + msg);
             return null;
          } finally {
             if ( reader != null )
                reader.Close();
          }
       }

    }
}
