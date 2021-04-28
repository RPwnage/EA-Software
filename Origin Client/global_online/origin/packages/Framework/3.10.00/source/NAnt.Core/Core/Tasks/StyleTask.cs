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
// Serge (serge@wildwestsoftware.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.IO;
using System.Xml;
using System.Xml.Xsl;
using System.Xml.XPath;
using System.Collections;
using System.Text.RegularExpressions;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks {

    /// <summary>
    /// Process a document with XSLT.
    /// This is useful for building views of XML based documentation, or in generating code.
    /// </summary>
    /// <include file='Examples/Style/Style.example' path='example'/>
    [TaskName("style")]
    public class StyleTask : Task {
                
        string _baseDir = null;        
        string _destDir = null;        
        string _extension = "html";        
        string _xsltFile = null;        
        string _srcFile = null;        
        string _outputFile = null;

        Hashtable _params = new Hashtable(); // TODO sort this out with an attribute

        /// <summary>Where to find the source XML file, default is the project's basedir.</summary>
        [TaskAttribute("basedir", Required=false)]
        public string BaseDir { 
            get { return _baseDir; } 
            set { _baseDir = value; } 
        }
        
        /// <summary>Directory in which to store the results. The default is the current directory.</summary>
        [TaskAttribute("destdir", Required=false)]
        public string DestDir { 
            get { return _destDir; } 
            set { _destDir = value; } 
        }
        
        /// <summary>Desired file extension to be used for the targets. The default is "html".</summary>
        [TaskAttribute("extension", Required=false)]
        public string Extension { 
            get { return _extension; } 
            set { _extension = value; } 
        }
        
        /// <summary>Name of the stylesheet to use - given either relative to the project's basedir or as an absolute path.</summary>
        [TaskAttribute("style", Required=true)]
        public string StyleSheet { 
            get { return _xsltFile; } 
            set { _xsltFile = value; } 
        }
        
        /// <summary>Specifies a single XML document to be styled. Should be used with the <c>out</c> attribute.</summary>
        [TaskAttribute("in", Required=true)]
        public string SrcFile { 
            get { return _srcFile; } 
            set { _srcFile = value; } 
        }
        
        /// <summary>Specifies the output name for the styled result from the in attribute. The default is the name specified by the in attribute but with an .html extension.</summary>
        [TaskAttribute("out", Required=false)]
        public string OutputFile { 
            get { return _outputFile; } 
            set { _outputFile = value; } 
        }

        XmlReader CreateXmlReader(string file) {
			XmlTextReader xmlReader = new XmlTextReader(file);
            return xmlReader;
        }

        XmlWriter CreateXmlWriter(string filepath) {
            string xmlPath = filepath;
            XmlWriter xmlWriter = null;

            string targetDir = Path.GetDirectoryName(Path.GetFullPath(xmlPath));
            if (targetDir != null && targetDir != "" && !Directory.Exists(targetDir)) {
                Directory.CreateDirectory(targetDir);
            }
            
            // UTF-8 encoding will be used
            //xmlWriter = new XmlTextWriter(xmlPath, null);
            // Create text writer first
            XmlTextWriter writer = new XmlTextWriter(xmlPath, null);
            writer.Formatting = Formatting.Indented; // make indenting formatted
            xmlWriter = writer;

            return xmlWriter;
        }

        ///<param name="taskNode"> taskNode used to define this task instance </param>
        protected override void InitializeTask(XmlNode taskNode) {

            // Load parameters
            foreach (XmlNode node in taskNode) {
                if(node.Name.Equals("param")) {
                    string paramname = node.Attributes["name"].Value;
                    string paramval = node.Attributes["expression"].Value;
                    _params[paramname] = Project.ExpandProperties(paramval);
                }
            }
        }

        protected override void ExecuteTask() {
            string destFile = OutputFile;
            // TODO handle filesets
            if (destFile == null || destFile == "") {
                // TODO: use System.IO.Path (gs)
                // append extension if necessary
                string ext = Extension[0]=='.'
                    ? Extension
                    : "." + Extension;

                int extPos = SrcFile.LastIndexOf('.');

                if (extPos == -1) {
                    destFile = SrcFile + ext;
                } else {
                    destFile = SrcFile.Substring(0, extPos) + ext;
                }
            }

            string basedirPath = Project.GetFullPath(BaseDir);
            string destdirPath = Project.GetFullPath(DestDir);
            string srcPath  = Path.GetFullPath(Path.Combine(basedirPath, SrcFile));
            string xsltPath = Path.GetFullPath(Path.Combine(basedirPath, StyleSheet));
            string destPath = Path.GetFullPath(Path.Combine(destdirPath, destFile));

            FileInfo srcInfo  = new FileInfo(srcPath);
            FileInfo destInfo = new FileInfo(destPath);
            FileInfo xsltInfo = new FileInfo(xsltPath);

            if (!srcInfo.Exists) {
                string msg = String.Format("Unable to find source XML file {0}", srcPath);
                throw new BuildException(msg, Location);
            }
            if (!xsltInfo.Exists) {
                string msg = String.Format("Unable to find stylesheet file {0}", xsltPath);
                throw new BuildException(msg, Location);
            }

            bool destOutdated = !destInfo.Exists
                || srcInfo.LastWriteTime  > destInfo.LastWriteTime
                || xsltInfo.LastWriteTime > destInfo.LastWriteTime;

            if (destOutdated) {
                XmlReader xmlReader = null;
                XmlReader xslReader = null;
                XmlWriter xmlWriter = null;

				bool isOutputOK = true;
                try {
                    xmlReader = CreateXmlReader(srcPath);
                    xslReader = CreateXmlReader(xsltPath);
                    xmlWriter = CreateXmlWriter(destPath);

                    Log.Info.WriteLine(LogPrefix + "Transforming into " + destdirPath);

                    XslCompiledTransform xslt = new XslCompiledTransform();
                    XPathDocument xml = new XPathDocument(xmlReader);
                    XsltArgumentList scriptargs = new XsltArgumentList();

                    Log.Info.WriteLine(LogPrefix + "Loading stylesheet " + Path.GetFullPath(xsltPath));

					// XmlResolver = null, external resources are not resolved
					// Evidence - trusted source
	                xslt.Load(xslReader);

	                // Load parameters
                    foreach (string key in _params.Keys) {
                        scriptargs.AddParam(key, "", (string) _params[key]);
                    }

                    Log.Status.WriteLine(LogPrefix + "Processing " + Path.GetFullPath(srcPath) + " to " + Path.GetFullPath(destPath));
                    xslt.Transform(xml, scriptargs, xmlWriter);

                } catch (Exception e) {
                    throw new BuildException("Could not perform XSLT transformation.", Location, e);
                } finally {
                    // Ensure file handles are closed
                    if (xmlReader != null) { xmlReader.Close(); }
                    if (xslReader != null) { xslReader.Close(); }
	                if (xmlWriter != null) {
	                	xmlWriter.Close();
	                	if (!isOutputOK)
	                		File.Delete(Path.GetFullPath(destPath));
	                }
                }
            }
        }
    }
}
