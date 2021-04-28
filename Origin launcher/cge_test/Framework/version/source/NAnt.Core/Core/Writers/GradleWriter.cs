// Copyright (C) 2019 Electronic Arts Inc.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace NAnt.Core.Writers
{
	public class GradleWriter : CachedWriter
	{
		private TextWriter m_writer;
		private GradleWriterFormat m_format;
		private int m_indentLevel;

		public GradleWriter(GradleWriterFormat format = null)
		{
			m_writer = new StreamWriter(Data, format.Encoding, 1024, leaveOpen: true);
			m_format = format ?? GradleWriterFormat.Default;
			m_indentLevel = 0;
		}

		public void WriteLine(string text)
		{
			if (m_indentLevel >= 1)
			{
				m_writer.Write(string.Concat(Enumerable.Repeat(m_format.IndentString, m_indentLevel * m_format.IndentAmount)));
			}
			m_writer.Write(text + System.Environment.NewLine);
		}

		public void Indent()
		{
			m_indentLevel++;
		}

		public void Unindent()
		{
			m_indentLevel--;
			if (m_indentLevel < 0)
			{
				throw new BuildException("Indentation mismatch in Gradlewriter");
			}
		}
		
		public void WriteStartBlock(string text)
		{
			WriteLine(String.Format("{0} {{", text));
			Indent();
		}

		public void WriteEndBlock()
		{
			Unindent();
			WriteLine("}");
		}

		public void WriteStartTask(string taskName)
		{
			WriteLine(String.Format("task {0} {{", taskName));
			Indent();
		}

		public void WriteEndTask()
		{
			Unindent();
			WriteLine("}");
		}

		public void WriteKeyValue(string key, string value)
		{
			WriteLine(String.Format("{0} = {1}", key, value));
		}

		public void WriteList(string listName, IList<string> listItems)
		{
			m_writer.Write(String.Format("{0} = [", listName));
			for (int i = 0; i < listItems.Count; ++i)
			{
				m_writer.Write(String.Format("'{0}'", listItems[i]));
				if (i < listItems.Count - 1)
				{
					m_writer.Write(", ");
				}
			}
			m_writer.WriteLine("]");
		}

		public void WriteDependency(string dependencyType, string dependency)
		{
			WriteLine($"{dependencyType} {dependency}");
		}

		internal void WriteDependencyString(string dependencyType, string dependencyString, string version = null)
		{
			dependencyString = $"'{dependencyString + (version != null ? ':' + version : String.Empty)}'";
			WriteDependency(dependencyType, dependencyString);
		}

		public void WriteRepository(string repositoryName)
		{
			WriteLine(String.Format("{0}()", repositoryName));
		}

		public void WriteApplyGradleFile(string gradleFile)
		{
			WriteLine(String.Format("apply from: '{0}'", gradleFile));
		}

		public void WriteApplyPlugin(string pluginName)
		{
			WriteLine(String.Format("apply plugin: '{0}'", pluginName));
		}

		public void WriteComment(string comment)
		{
			WriteLine(String.Format("// {0}", comment));
		}

		public void Close()
		{
			m_writer.Close();
		}

		public new void Flush()
		{
			m_writer.Flush();
			base.Flush();
		}

		protected override void Dispose(bool disposing)
		{
			m_writer.Dispose();
			base.Dispose(disposing);
		}
	}

	public class GradleWriterFormat
	{
		public string IndentString { get; }
		public int IndentAmount { get; }
		public System.Text.Encoding Encoding { get; }

		public readonly static GradleWriterFormat Default = new GradleWriterFormat
		(
			indentString:" ",
			indentAmount: 4,
			encoding: new System.Text.UTF8Encoding(false)
		);

		public GradleWriterFormat(string indentString, int indentAmount, System.Text.Encoding encoding)
		{
			IndentString = indentString;
			IndentAmount = indentAmount;
			Encoding = encoding;
		}
	}
}