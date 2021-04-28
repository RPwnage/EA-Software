using NAnt.Core.Attributes;
using NAnt.Core.Structured;

namespace EA.Eaconfig.Structured
{
	[ElementName("android-gradle", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public sealed class AndroidGradle : ConditionalElementContainer
	{
		[TaskAttribute("gradle-directory", Required = false)]
		public string GradleDirectory
		{
			get { return m_gradleFilePath; }
			set { m_gradleFilePath = Project.GetFullPath(value); }
		}

		/// <summary>Project name for this module in Gradle context If omitted the module name will be used.</summary>
		[TaskAttribute("gradle-project", Required = false)]
		public string GradleProject { get; set; } = null;

		/// <summary>Value for the org.gradle.jvmargs property in gradle.properties</summary>
		[TaskAttribute("gradle-jvm-args", Required = false)]
		public string GradleJvmArgs { get; set; } = null;

		/// <summary>Java files to display in Visual Studio for debugging purposes (not used for build)</summary>
		[FileSet("javasourcefiles", Required = false)]
		public StructuredFileSet JavaSourceFiles { get; set; } = new StructuredFileSet();

		/// <summary>True by default. Includes the necessary files for using a native activity. Set to false if your program 
		/// implement its own activity class.</summary>
		[TaskAttribute("native-activity", Required = false)]
		public bool IncludeNativeActivityGlue { get; internal set; } = true;

		/// <summary>Extra values that should be added to the gradle.properties file when building this project.</summary>
		[Property("gradle-properties")]
		public ConditionalPropertyElement GradleProperties { get; } = new ConditionalPropertyElement();

		private string m_gradleFilePath;
	}
}