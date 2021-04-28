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
using System.IO;
using System.Collections.Generic;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Writers;

using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules;

using EA.Make.MakeItems;
using NAnt.Core.Events;

namespace EA.Make.MakeTools
{
	public class MakeBuildToolBase<BT>: MakeToolBase
	{
		// TODO unify templating code
		// -dvaliant 2017/09/28
		/*
			this const, and many of the template functions are repeated in CompilerBase generation,
			be nice to refactor all the templating code into a single utility class / namespace / thing
		*/
		public const string DefaultCompilerCommandLineTemplate = "%defines% %system-includedirs% %includedirs% %usingdirs% %options%"; // used for both assembler and compiler


		protected readonly Module_Native Module;

		public bool UseResponseFile;

		public virtual string OptionsSeparator
		{
			get
			{
				return UseResponseFile ? (_responsefile_separator ?? DEFAULT_RSP_OPTIONS_SEPARATOR) : DEFAULT_OPTIONS_SEPARATOR;
			}
		}
		private readonly string _responsefile_separator;


		protected MakeBuildToolBase(MakeModuleBase makeModule, Module_Native module, Tool tool)
			: base(makeModule, module.Package.Project, tool)
		{
			Module = module;

			UseResponseFile = GetToolParameter("useresponsefile", "false").OptionToBoolean();

			_responsefile_separator = GetToolParameter("responsefile.separator", defaultValue:null);
		}


		protected string AddVariable(MakeFileBase makefile, string name, PathString value, FileItem fileItem = null)
		{
			var variable = new MakeVariableScalar(GetKeyedName(name, fileItem), MakeModule.ToMakeFilePath(value));
			makefile.Items.Add(variable);

			return variable.Label;
		}

		/// <summary>Returns the @ symbol if the log level is at or below the level provided as the argument.</summary>
		/// <param name="level">The level at which to return the quite symbol. (Defaults to Minimal)</param>
		protected string GetQuietSymbol(NAnt.Core.Logging.Log.LogLevel level = NAnt.Core.Logging.Log.LogLevel.Minimal)
		{
			if (Project.Log.Level <= level)
			{
				return "@";
			}
			return "";
		}

		protected string AddVariable(MakeFileBase makefile, string name, string value, FileItem fileItem = null)
		{
			var variable = new MakeVariableScalar(GetKeyedName(name, fileItem), value);
			MakeModule.MakeFile.Items.Add(variable);

			return variable.Label;
		}

		public string ReplaceTemplate(string template, string variable, IEnumerable<string> items, Func<string, string> func= null)
		{
			var options = new StringBuilder();
			foreach (string item in items)
			{
				var itemval = item;
				if (func != null)
				{
					itemval = func(itemval);
				}
				var option = template.Replace(variable.Quote(), itemval).Replace(variable, itemval);
				options.Append(option);
				options.Append(OptionsSeparator);
			}
			return options.ToString();
		}

		private void OnMakeFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				string message = string.Format("{0}{1} MakeFile file '{2}'", "[makegenerator] ", e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
				if (e.IsUpdatingFile)
					Project.Log.Minimal.WriteLine(message);
				else
					Project.Log.Status.WriteLine(message);
			}
		}


		protected StringBuilder GetResponseFileCommandLine(PathString responsefile, StringBuilder commandLine, StringBuilder responseFileCommandline)
		{
			using (var writer = new MakeWriter())
			{
				writer.CacheFlushed += new CachedWriterEventHandler(OnMakeFileUpdate);
				writer.FileName = responsefile.Path;

				writer.Write(commandLine.ToString());

				if (responseFileCommandline.Length == 0)
				{
					responseFileCommandline.Append("@\"%responsefile%\"");
				}

				responseFileCommandline.Replace("%responsefile%", MakeModule.ToMakeFilePath(writer.FileName, QuotedPath.NotQuoted));

				return responseFileCommandline;
			}
		}


		public string ReplaceTemplate(string template, string variable, IEnumerable<PathString> items, QuotedPath quoteOverride = QuotedPath.NotQuoted, PathType pathTypeOverride = PathType.UseDefault)
		{
			var options = new StringBuilder();
			foreach (var item in items)
			{
				options.Append(template.Replace(variable, MakeModule.ToMakeFilePath(item, quoteOverride, pathTypeOverride)));
				options.Append(OptionsSeparator);
			}

			return options.ToString();
		}


		public  string GetTemplate(string templatename, string defaultTemplate = null, Tool tool = null)
		{
			string template = null;
			if (UseResponseFile)
			{
				template = GetToolParameter("template.responsefile." + templatename, null, tool);

			}
			if (String.IsNullOrEmpty(template))
			{
				template = GetToolParameter("template." + templatename, defaultTemplate, tool);
			}
			/*
			if (template == null)
			{
				throw new BuildException("[makegen] Template '" + GetToolParameterName(templatename) + "' not defined.");
			}*/

			return template.LinesToArray().ToString(" ");
			 
		}

		public StringBuilder ExtractTemplateFieldWithOption(StringBuilder template, string templateField)
		{
			var templateWithOption = new StringBuilder();
			int ind = -1;
			do
			{
				var buffer = template.ToString();
				ind = buffer.IndexOf(templateField);
				var lastTemplateInd = buffer.IndexOfAny(WHITE_SPACE_CHARS, ind + templateField.Length - 1);
				if (lastTemplateInd == -1)
				{
					lastTemplateInd = buffer.Length;
				}

				if (ind != -1)
				{
					int opind = ind - 1;
					while (opind >= 0 && buffer[opind] != '%')
					{
						opind--;
					}
					opind = buffer.IndexOfAny(WHITE_SPACE_CHARS, opind + 1, ind - opind - 1);
					templateWithOption.Append(buffer.Substring(opind, lastTemplateInd - opind));
					templateWithOption.Append(" ");
					template.Remove(opind, lastTemplateInd - opind);
				}
			} while (ind != -1);

			return templateWithOption;
		}

		private static readonly char[] WHITE_SPACE_CHARS = new char[] { ' ', '\t', '\n', 'r' };

		private static readonly string DEFAULT_OPTIONS_SEPARATOR = Environment.NewLine;
		private static readonly string DEFAULT_RSP_OPTIONS_SEPARATOR = Environment.NewLine;

	}
}
