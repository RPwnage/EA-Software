using System;
using System.Diagnostics;
using System.Reflection;
using System.Xml.XPath;

using SandcastleBuilder.Utils;
using SandcastleBuilder.Utils.BuildEngine;
using SandcastleBuilder.Utils.PlugIn;
using SandcastleBuilder.Utils.ConceptualContent;
using System.IO;
using System.Collections.Generic;
using System.Xml.Xsl;
using EA.Eaconfig;

using NAnt.Core.Logging;

namespace FrameworkDocsPlugin
{
    public class TestPlugin : SandcastleBuilder.Utils.PlugIn.IPlugIn
    {
        #region Private Data Members

        private ExecutionPointCollection executionPoints;

        private BuildProcess builder;

        private IEnumerable<TopicFilter> taskFilters;
        private TopicCollection nantElementTopics;
        private TopicCollection functionTopics;

        private Log Log
        {
            get 
            {
                if (_log == null)
                {
                    throw new Exception("Log is not initialized in TaskPlugin");
                }
                return _log; 
            }
        } private Log _log;

        private static readonly string LogPrefix = "[TaskPlugin] ";


        #endregion

        #region IPlugIn implementation

        public string Name { get { return "Framework Task Generator"; } }

        public Version Version
        {
            get
            {
                Assembly asm = Assembly.GetExecutingAssembly();
                FileVersionInfo fvi = FileVersionInfo.GetVersionInfo(asm.Location);
                return new Version(fvi.ProductVersion);
            }
        }

        public string Copyright
        {
            get
            {
                Assembly asm = Assembly.GetExecutingAssembly();
                AssemblyCopyrightAttribute copyright =
                    (AssemblyCopyrightAttribute)Attribute.GetCustomAttribute(
                    asm, typeof(AssemblyCopyrightAttribute));
                return copyright.Copyright;
            }
        }

        public string Description { get { return "Uses Nant assemblies and comments to generate documentation"; } }

        public bool RunsInPartialBuild { get { return false; } }

        public ExecutionPointCollection ExecutionPoints
        {
            get
            {
                if (executionPoints == null)
                {
                    executionPoints = new ExecutionPointCollection {
                        new ExecutionPoint(BuildStep.GenerateSharedContent, ExecutionBehaviors.Before),
                        new ExecutionPoint(BuildStep.CopyConceptualContent, ExecutionBehaviors.After),
                        
                    };
                }
                return executionPoints;
            }
        }

        public string ConfigurePlugIn(SandcastleProject project, string currentConfig)
        {
            return currentConfig;
        }

        public void Initialize(BuildProcess buildProcess, XPathNavigator configuration)
        {
            builder = buildProcess;
            builder.ReportProgress("{0} Version {1}\r\n{2}\r\n",
                this.Name, this.Version, this.Copyright);

            var logLevel = Log.LogLevel.Normal;
            logLevel = Log.LogLevel.Diagnostic; // IM for now use diagnostyic for debugging. Later me may configure this

            _log = new Log(logLevel);
            _log.Listeners.Add(new PluginLogListener(buildProcess));

            taskFilters = TopicFilterDefinitions.GetTaskFilters();

            functionTopics = new TopicCollection(null);
            nantElementTopics = new TopicCollection(null);

        }

        public void Execute(ExecutionContext context)
        {
            if (context.BuildStep == BuildStep.GenerateSharedContent)
            {
                GenerateReflectionInfo(builder);

                builder.ExecuteAfterStepPlugIns();
                context.Executed = true;
            }
            else if (context.BuildStep == BuildStep.CopyConceptualContent)
            {
                UpdateNantElementsHiddenTOC();
                UpdateTasksTOC();
                UpdateFunctionsTOC();
            }
        }

        #endregion

        #region Private Methods

        private void UpdateTasksTOC()
        {
            Log.Status.WriteLine(LogPrefix + "Updating TOC with generated NAnt Task topics");

            var taskTopics = new TopicCollection(null);
            var structuredTopics = new TopicCollection(null);

            // Collect non empty directory filters into topic collection
            foreach (var filter in taskFilters)
            {
                filter.SetRelations();
                if (filter.Topic.Subtopics.Count > 0)
                {
                    if (filter.Topic.Title == "Structured XML")
                    {
                        structuredTopics.Add(filter.Topic);
                    }
                    else
                    {
                        taskTopics.Add(filter.Topic);
                    }
                }
            }

            // SHFB does this step. We need to do it for added topic files:
            //Copy the topic files
            string folder = builder.WorkingFolder + "ddueXml";
            taskTopics.GenerateConceptualTopics(folder, builder);
            structuredTopics.GenerateConceptualTopics(folder, builder);

            // Add all new topics to the "Reference/Tasks"
            var tasksParentTopic = FindTopicByID("44df9582-e180-4d2f-bdaa-636ef7322d7b");


            if (tasksParentTopic != null)
            {
                foreach (var topic in taskTopics)
                {
                    topic.Keywords.Add(new MSHelpKeyword("K", topic.Title));
                    tasksParentTopic.Subtopics.Add(topic);
                    Log.Status.WriteLine(LogPrefix + "      + '{0}'", topic.Title);
                }
            }
            else
            {
                Log.Error.WriteLine(LogPrefix + "Can cot find Reference/Tasks element, Id='{0}'", "44df9582-e180-4d2f-bdaa-636ef7322d7b");
            }

            var structuredXMLParentTopic = FindTopicByID("7ea54569-39f6-4dfd-a5c1-e5bc6e78c36f");

            if (structuredXMLParentTopic != null)
            {
                foreach (var topic in structuredTopics)
                {
                    topic.Keywords.Add(new MSHelpKeyword("K", topic.Title));
                    // Do not add Filter folder. Add tasks directly to the parent
                    foreach (var item in topic.Subtopics)
                    {
                        item.Keywords.Add(new MSHelpKeyword("K", item.Title));
                        structuredXMLParentTopic.Subtopics.Add(item);
                    }
                    Log.Status.WriteLine(LogPrefix + "      + '{0}'", topic.Title);
                }
            }
            else
            {
                Log.Error.WriteLine(LogPrefix + "Can cot find \"Structured Tasks Reference\" element, Id='{0}'", "7ea54569-39f6-4dfd-a5c1-e5bc6e78c36f");
            }


            Log.Status.WriteLine(LogPrefix + "Updating TOC done");

        }

        private void UpdateNantElementsHiddenTOC()
        {
            Log.Status.WriteLine(LogPrefix + "Updating TOC with generated NAnt Elements topics");

            // SHFB does this step. We need to do it for added topic files:
            //Copy the topic files
            string folder = builder.WorkingFolder + "ddueXml";
            nantElementTopics.GenerateConceptualTopics(folder, builder);

            // Add all new topics to the "Reference/NantElements(Hidden)"
            var elementsParentTopic = FindTopicByID("ff33fd32-eeeb-4892-b3b7-c91a9b0f4b4c");

            if (elementsParentTopic != null)
            {

                foreach (var topic in nantElementTopics)
                {
                    elementsParentTopic.Subtopics.Add(topic);
                    Log.Status.WriteLine(LogPrefix + "      + '{0}'", topic.Title);
                }
            }
            else
            {
                Log.Error.WriteLine(LogPrefix + "Can cot find Reference/Reference/NantElements(Hidden) element, Id='{0}'", "ff33fd32-eeeb-4892-b3b7-c91a9b0f4b4c");
            }

            Log.Status.WriteLine(LogPrefix + "Updating TOC done");

        }


        private void UpdateFunctionsTOC()
        {
            Log.Status.WriteLine(LogPrefix + "Updating TOC with generated NAnt Funtion topics");


            // SHFB does this step. We need to do it for added topic files:
            //Copy the topic files
            string folder = builder.WorkingFolder + "ddueXml";
            functionTopics.GenerateConceptualTopics(folder, builder);

            // Add all new topics to the "Reference/Functions"
            var funcParentTopic = FindTopicByID("50e32339-f57f-402c-a92f-dcc3668d6c3d");

            if (funcParentTopic != null)
            {
                functionTopics.Sort();

                foreach (var topic in functionTopics)
                {
                    topic.Keywords.Add(new MSHelpKeyword("K", topic.Title));
                    funcParentTopic.Subtopics.Add(topic);
                    Log.Status.WriteLine(LogPrefix + "      + '{0}'", topic.Title);
                }
            }
            else
            {
                Log.Error.WriteLine(LogPrefix + "Can cot find Reference/Functions element, Id='{0}'", "50e32339-f57f-402c-a92f-dcc3668d6c3d");
            }

            Log.Status.WriteLine(LogPrefix + "Updating TOC done");

        }


        private Topic FindTopicByID(string id, TopicCollection collection = null)
        {
            if (collection != null)
            {
                foreach (var t in collection)
                {
                    if (t.Id == id)
                    {
                        return t;
                    }
                    var found = FindTopicByID(id, t.Subtopics);
                    if (found != null)
                    {
                        return found;
                    }
                }
            }
            else
            {
                foreach (var c in builder.ConceptualContent.Topics)
                {
                    var found = FindTopicByID(id, c);
                    if (found != null)
                    {
                        return found;
                    }
                }
            }
            return null;
        }

        private void GenerateReflectionInfo(BuildProcess buildProcess)
        {
            XslCompiledTransform transform = new XslCompiledTransform();
            transform.Load("CommentToMaml.xslt");

            SchemaMetadata schemadata = new SchemaMetadata(Log, LogPrefix);
            schemadata.XmlDocs.SetXsltTransform(transform);
            AddAssemblies(buildProcess, schemadata);
            schemadata.GenerateSchemaMetaData();

            TaskTopicGenerator TaskTopicGen = new TaskTopicGenerator(schemadata);
            TaskTopicGen.GenerateGuids();
            TaskTopicGen.GenerateTopics(builder, taskFilters, nantElementTopics);

            FunctionTopicGenerator FunctionTopicGen = new FunctionTopicGenerator(schemadata);
            FunctionTopicGen.GenerateGuids();
            FunctionTopicGen.GenerateTopics(builder, functionTopics);

            builder.CurrentProject.MSBuildProject.ReevaluateIfNecessary();
        }

        private void AddAssemblies(BuildProcess buildProcess, SchemaMetadata schemadata)
        {
            foreach (var item in buildProcess.CurrentProject.DocumentationSources)
            {
                string extension = Path.GetExtension(item.SourceFile.ExpandedPath).ToLower();

                if (extension == ".dll")
                {
                    buildProcess.ReportProgress("Ready to process {0}", item.SourceFile.ExpandedPath);

                    var asm = GetLoadedAssembly(item.SourceDescription);
                    
                    if(asm==null)
                    {
                        buildProcess.ReportProgress("Loading assembly {0}", item.SourceFile.ExpandedPath);
                        asm = Assembly.LoadFrom(item.SourceFile.ExpandedPath);
                    }
                    schemadata.AddAssembly(asm, item.SourceFile.ExpandedPath);

                    buildProcess.ReportProgress("Finished to processing {0}", item.SourceFile.ExpandedPath);
                }
            }
        }

        /// <summary>Checks to see if the assembly has already been loaded from another directory</summary>
        private Assembly GetLoadedAssembly(string name)
        {
            Assembly[] current_assemblies = AppDomain.CurrentDomain.GetAssemblies();
            foreach (Assembly asm in current_assemblies)
            {
                if (asm.ManifestModule.Name == name)
                {
                    return asm;
                }
            }
            return null;
        }

        #endregion

        #region IDisposable implementation

        ~TestPlugin()
        {
            this.Dispose(false);
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {

        }

        #endregion
    }
}
