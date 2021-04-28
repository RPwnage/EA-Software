using System.Collections.Generic;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Writers;

using EA.Eaconfig.Core;
using EA.FrameworkTasks.Model;
using NAnt.Core.Util;

namespace EA.Eaconfig.Backends.VisualStudio
{
	internal sealed class VSAndroidProject : VSProjectBase
	{
		internal VSAndroidProject(VSSolutionBase solution, IEnumerable<IModule> modules)
			: base(solution, modules, ANDROID_PACKAGING_GUID)
		{
		}

		public override string ProjectFileName => ProjectFileNameWithoutExtension + ".androidproj";

		protected override IXmlWriterFormat ProjectFileWriterFormat => XmlFormat.Default;
		protected override string UserFileName => ProjectFileName + ".user";

		internal override string RootNamespace => Name;

		protected override void WriteProject(IXmlWriter writer)
		{
			AndroidProjectWriter.WriteProject(writer, Modules, this, WriteJavaFiles);
		}

		protected override void WriteUserFile()
		{
			// GRADLE-TODO
		}

		protected override IEnumerable<KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>> GetUserData(ProcessableModule module)
		{
			return Enumerable.Empty<KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>>(); // GRADLE-TODO
		}

		private void WriteJavaFiles(IXmlWriter writer)
		{
			var modulesToJavaFileSets = Modules
				.Select(module => new { Module = module, JavaFiles = module.PropGroupFileSet("javasourcefiles") })
				.Where(cTJ => cTJ.JavaFiles != null);
			if (modulesToJavaFileSets.Any())
			{
				// add generated files to solution explorer
				using (writer.ScopedElement("ItemGroup"))
				{
					foreach (var moduleToJavaFileSet in modulesToJavaFileSets)
					{
						foreach (FileItem javaFile in moduleToJavaFileSet.JavaFiles.FileItems)
						{
							using (writer.ScopedElement("None")
								.Attribute("Include", GetProjectPath(javaFile.FullPath))
								.Attribute("Condition", GetConfigCondition(moduleToJavaFileSet.Module.Configuration)))
							{
								string baseDir = javaFile.BaseDirectory ?? moduleToJavaFileSet.JavaFiles.BaseDirectory ?? moduleToJavaFileSet.Module.Package.Dir.NormalizedPath;
								using (writer.ScopedElement("Link"))
								{
									writer.WriteString(PathUtil.RelativePath(javaFile.FullPath, baseDir));
								}
							}
						}
					}
				}
			}
		}
	}
}