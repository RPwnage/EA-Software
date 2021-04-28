// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using System.IO;

using NAnt.Core.Logging;

namespace NAnt.Core.Util
{
	public class CoreDump
	{
		public enum Format { Xml, Text };

		internal bool DumpProperties;
		internal bool DumpFilesets;
		internal bool DumpOptionsets;
		internal bool ExpandAll;

		private readonly Project _project;
		private readonly Log _log;

		private int IndentLevel
		{
			get { return _indentLevel; }
			set 
			{
				if (value >= 0)
				{
					_indentLevel = value;
				}
				_indent = String.Empty.PadLeft(2*_indentLevel);
			}
		}
		private int _indentLevel;
		private string _indent = String.Empty;

		internal CoreDump(Project project, bool dumpProperties = true, bool dumpFilesets = true, bool dumpOptionsets = true, bool expandAll = true, Log log = null)
		{
			_project = project;
			_log = log;

			DumpProperties = dumpProperties;
			DumpFilesets = dumpFilesets;
			DumpOptionsets = dumpOptionsets;
			ExpandAll = expandAll;

			IndentLevel = 0;
		}

		internal void Write(string path, Format format)
		{
			try
			{
				path = path.TrimWhiteSpace();
				if (!String.IsNullOrEmpty(path))
				{
					path = _project.GetFullPath(path);

					if (format == Format.Text)
					{
						using (var writer = new StreamWriter(path))
						{
							WriteCoreDump(writer: writer);
						}
					}
					else
					{
						XmlWriterSettings writerSettings = new XmlWriterSettings();
						writerSettings.Encoding = Encoding.UTF8;
						writerSettings.Indent = true;
						writerSettings.IndentChars = String.Empty.PadLeft(2);
						writerSettings.NewLineOnAttributes = false;
						writerSettings.NewLineChars = Environment.NewLine;
						writerSettings.CloseOutput = true;

						using (var xmlwriter = new XmlTextWriter(path, null))
						{
							WriteCoreDump(xmlwriter: xmlwriter);
						}

					}
				}
				else
				{
					WriteCoreDump();
				}
			}
			catch (Exception e)
			{
				if (_log != null)
				{
					_log.Error.WriteLine("CoreDump error: {0}", e.Message);
				}
			}

		}

		internal void Write(XmlTextWriter xmlwriter)
		{
			   WriteCoreDump(xmlwriter: xmlwriter);
		}

		internal void Write(TextWriter writer)
		{
			WriteCoreDump(writer: writer);
		}


		private void WriteCoreDump(TextWriter writer = null, XmlWriter xmlwriter = null)
		{
			try
			{
				if (xmlwriter != null)
				{
					xmlwriter.WriteStartElement("NAntProjectDump");
					xmlwriter.WriteAttributeString("dir", _project.BuildFileLocalName);
				}
				if (writer != null)
				{
					writer.WriteLine("NAntProjectDump dir={0}", _project.BuildFileLocalName);
				}
				if (_log != null)
				{
					_log.Status.WriteLine("NAntProjectDump dir={0}", _project.BuildFileLocalName);
				}

				WriteProperties(writer, xmlwriter);
				WriteFilesets(writer, xmlwriter);
				WriteOptionsets(writer, xmlwriter);

				if (xmlwriter != null)
				{
					xmlwriter.WriteEndElement();
				}

			}
			catch (Exception e)
			{
				if (_log != null)
				{
					_log.Error.WriteLine("CoreDump error: {0}",e.Message);
				}
			}
		}

		private void WriteProperties(TextWriter writer, XmlWriter xmlwriter)
		{
			if (DumpProperties)
			{
				WriteStartSection(writer, xmlwriter, "Properties");
				WriteStartSection(writer, xmlwriter, "Global");

				var selectednames = new HashSet<string>(Project.GlobalProperties.EvaluateExceptions(_project).Select(def=>def.Name));

				foreach (var prop in selectednames.Select(pname=> _project.Properties.SingleOrDefault(p=>p.Name == pname)).Where(prop=> prop!=null).OrderBy(p=>p.Name))
				{
					WriteProperty(writer, xmlwriter, prop);
				}

				WriteEndSection(writer, xmlwriter); //Global

				WriteStartSection(writer, xmlwriter, "Local");

				foreach (var prop in _project.Properties.Where(p => _project.Properties.IsPropertyLocal(p.Name)).OrderBy(p => p.Name))
				{
					WriteProperty(writer, xmlwriter, prop);
					selectednames.Add(prop.Name);
				}

				WriteEndSection(writer, xmlwriter); //Local

				WriteStartSection(writer, xmlwriter, "Regular");

				foreach (var prop in _project.Properties.Where(p => !selectednames.Contains(p.Name)).OrderBy(p => p.Name))
				{
					WriteProperty(writer, xmlwriter, prop);
				}

				WriteEndSection(writer, xmlwriter); //Regular
				WriteEndSection(writer, xmlwriter); //Properties
			}
		}

		private void WriteFilesets(TextWriter writer, XmlWriter xmlwriter)
		{
			if (DumpFilesets)
			{
				WriteStartSection(writer, xmlwriter, "Filesets");
				foreach (var fileset in _project.NamedFileSets.OrderBy(f=>f.Key))
				{
					WriteFileset(writer, xmlwriter, fileset.Key, fileset.Value);
				}
				WriteEndSection(writer, xmlwriter); //Filesets
			}
		}

		private void WriteOptionsets(TextWriter writer, XmlWriter xmlwriter)
		{
			if (DumpOptionsets)
			{
				WriteStartSection(writer, xmlwriter, "Optionsets");
				foreach (var optionset in _project.NamedOptionSets.OrderBy(f => f.Key))
				{
					WriteOptionset(writer, xmlwriter, optionset.Key, optionset.Value);
				}
				WriteEndSection(writer, xmlwriter); //Optionsets
			}
		}


		private void WriteStartSection(TextWriter writer, XmlWriter xmlwriter, string val)
		{
			if (_log != null)
			{
				_log.Status.WriteLine(_indent+"---- {0} ----",val);
			}
			if (writer != null)
			{
				writer.WriteLine(_indent + "---- {0} ----", val);
			}
			if (xmlwriter != null)
			{
				xmlwriter.WriteStartElement(val);
			}

			IndentLevel++;

		}

		private void WriteEndSection(TextWriter writer, XmlWriter xmlwriter)
		{
			IndentLevel--;

			if (xmlwriter != null)
			{
				xmlwriter.WriteEndElement();
			}

		}

		private void WriteProperty(TextWriter writer, XmlWriter xmlwriter, Property prop)
		{
			if (_log != null || writer != null)
			{
				var val = String.Format(_indent + "{0}{1}{2}={3}", prop.Name, prop.ReadOnly ? "[readonly]" : String.Empty, prop.Deferred ? "[deferred]" : String.Empty, prop.Value);
				if (_log != null)
				{
					_log.Status.WriteLine(val);
				}

				if (writer != null)
				{
					writer.WriteLine(val);
				}
			}
			if(xmlwriter != null)
			{
					xmlwriter.WriteStartElement("property");
					xmlwriter.WriteAttributeString("name", prop.Name);
					if(prop.ReadOnly)
					{
						xmlwriter.WriteAttributeString("readonly", "true");
					}
					if(prop.Deferred)
					{
						xmlwriter.WriteAttributeString("deferred", "true");
					}

					xmlwriter.WriteString(prop.Value);
					xmlwriter.WriteEndElement();
			}

		}

		private void WriteFileset(TextWriter writer, XmlWriter xmlwriter, string name, FileSet fs)
		{
			if (_log != null || writer != null) 
			{
				var val = String.Format(_indent + "fileset name={0} {1}", name, String.IsNullOrEmpty(fs.BaseDirectory) ? String.Empty : "basedir=" + fs.BaseDirectory);
				if (_log != null)
				{
					_log.Status.WriteLine(val);
				}
				if (writer != null)
				{
					writer.WriteLine(val);
				}

				if (ExpandAll)
				{
					IndentLevel++;
					foreach (var file in fs.FileItems)
					{
						val = String.Format(_indent + "file{0}={1}", String.IsNullOrEmpty(file.OptionSetName) ? String.Empty : " [optionset=" + file.OptionSetName + "]", file.FullPath);
						if (_log != null)
						{
							_log.Status.WriteLine(val);
						}
						if (writer != null)
						{
							writer.WriteLine(val);
						}

					}
					IndentLevel--;
				}
			}

			if (xmlwriter != null)
			{
				xmlwriter.WriteStartElement("fileset");
				xmlwriter.WriteAttributeString("name", name);
				if (!String.IsNullOrEmpty(fs.BaseDirectory))
				{
					xmlwriter.WriteAttributeString("basedir", fs.BaseDirectory);
				}
				if (ExpandAll)
				{
					foreach (var file in fs.FileItems)
					{
						xmlwriter.WriteStartElement("file");
						if (!String.IsNullOrEmpty(file.OptionSetName))
						{
							xmlwriter.WriteAttributeString("optionset", file.OptionSetName);
						}
						xmlwriter.WriteString(file.FullPath);
						xmlwriter.WriteEndElement();
					}
				}
				xmlwriter.WriteEndElement();
			}

		}

		private void WriteOptionset(TextWriter writer, XmlWriter xmlwriter, string name, OptionSet os)
		{
			if (_log != null || writer != null)
			{
				var val = String.Format(_indent + "optionset name={0}", name);
				if (_log != null)
				{
					_log.Status.WriteLine(val);
				}
				if (writer != null)
				{
					writer.WriteLine(val);
				}

				if (ExpandAll)
				{
					IndentLevel++;
					foreach (var option in os.Options)
					{
						val = String.Format(_indent + "{0}={1}", option.Key, option.Value);

						if (_log != null)
						{
							_log.Status.WriteLine(val);
						}
						if (writer != null)
						{
							writer.WriteLine(val);
						}
					}
					IndentLevel--;
				}
			}


			if (xmlwriter != null)
			{

				xmlwriter.WriteStartElement("optionset");
				xmlwriter.WriteAttributeString("name", name);
				if (ExpandAll)
				{
					foreach (var option in os.Options)
					{
						xmlwriter.WriteStartElement("option");
						xmlwriter.WriteAttributeString("name", option.Key);
						xmlwriter.WriteString(option.Value);
						xmlwriter.WriteEndElement();
					}
				}
				xmlwriter.WriteEndElement();
			}

		}
	}
}
