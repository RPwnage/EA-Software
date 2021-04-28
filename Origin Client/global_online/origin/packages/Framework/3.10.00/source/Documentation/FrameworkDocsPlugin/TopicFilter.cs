using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;

using SandcastleBuilder.Utils.ConceptualContent;
using SandcastleBuilder.Utils.BuildEngine;
using SandcastleBuilder.Utils;

namespace FrameworkDocsPlugin
{
    public class TopicFilter
    {
        public class TopicWithLink
        {
            internal Topic Topic;
            internal string Link;
            internal TopicWithLink(Topic topic, string link)
            {
                Topic = topic;
                Link = link;
            }
        }
        private readonly List<Topic> Files = new List<Topic>();
        private readonly List<string> FilePatterns = new List<string>();

        internal readonly Topic Topic;
        internal readonly List<TopicFilter> ChildFilters = new List<TopicFilter>();

        // Provide fileItem parameter if folder has associated file
        internal TopicFilter(string title, FileItem fileItem = null, IEnumerable<string> filePatterns = null, string filePattern = null)
        {
            Topic = new Topic();
            Topic.Title = title;
            if (fileItem != null)
            {
                Topic.TopicFile = new TopicFile(fileItem);
            }
            else
            {
                //Important to assign null. This will set flags that this topic is a folder with no files
                Topic.TopicFile = null;
            }
            if (filePatterns != null)
            {
                FilePatterns.AddRange(filePatterns);
            }
            if (filePattern != null)
            {
                FilePatterns.Add(filePattern);
            }
        }

        internal bool FilterFiles(BuildProcess builder, string itemPath, string itemName, string itemTitle, out TopicWithLink topicInfo)
        {
            //First look if any child filter has match
            foreach (var childFilter in ChildFilters)
            {
                if (childFilter.FilterFiles(builder, itemPath, itemName, itemTitle, out topicInfo))
                {
                    return true;
                }
            }
            topicInfo = null;
            foreach (var pattern in FilePatterns)
            {
                if (Regex.IsMatch(itemName, pattern))
                {
                    string linkPath;
                    var topic = CreateTopic( builder, itemPath, itemName, itemTitle, out linkPath);
                    if (topic != null)
                    {
                        Files.Add(topic);
                        topicInfo = new TopicWithLink(topic, linkPath);
                    }

                    return true;
                }
            }
            return false;
        }

        internal static Topic CreateTopic(BuildProcess builder, string itemPath, string itemName, string itemTitle, out string linkPath)
        {
            Topic topic = null;

            // builder.CurrentProject.FindFile(itemPath); returns null if itemPath is not under project dir. 
            // Use MSBuild Link path uder the project dir (which is some nonexistent unique logical location) to get around this limitation

            linkPath = Path.Combine(builder.ProjectFolder, "Generated", itemName + ".aml");
            var msbuildItem = builder.CurrentProject.MSBuildProject.AddItem("None", itemPath, new Dictionary<string, string>() { { "Link", linkPath } });

            builder.CurrentProject.MSBuildProject.ReevaluateIfNecessary();
            if (msbuildItem == null)
            {
                builder.ReportProgress("ERROR in FrameworkDocsPlugin: Can not add file '{0}' to MSbuild", itemPath);
            }

            var sfhbItem = builder.CurrentProject.FindFile(linkPath);

            if (sfhbItem == null)
            {
                builder.ReportProgress("ERROR in FrameworkDocsPlugin: builder.CurrentProject.FindFile({0}) returned null", itemPath);
            }
            else
            {
                topic = new Topic();
                topic.Title = itemTitle;

                topic.TopicFile = new TopicFile(sfhbItem);
            }
            return topic;
        }

        internal int SetRelations()
        {
            foreach(var fileTopic in Files.OrderBy(fi=>fi.Title, StringComparer.InvariantCultureIgnoreCase))
            {
                Topic.Subtopics.Add(fileTopic);
            }

            int count = Files.Count;

            foreach (var childFilter in ChildFilters)
            {
                int childCount = childFilter.SetRelations();
                if(childCount > 0)
                {
                    Topic.Subtopics.Add(childFilter.Topic);
                    count += childCount;
                }
            }

            return count;
        }
    }
}
