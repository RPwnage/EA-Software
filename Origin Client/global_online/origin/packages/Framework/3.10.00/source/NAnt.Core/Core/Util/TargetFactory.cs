using System;
using System.Xml;
using System.Collections.Generic;
using System.Collections.Concurrent;

using NAnt.Core;
using NAnt.Core.Util;

namespace NAnt.Core.Reflection
{
    internal class TargetFactory
    {
        static TargetFactory()
        {
            _targets = new ConcurrentDictionary<string, Lazy<TargetInfo>>();
        }

        internal static Target CreateTarget(Project project, string buildFileDirectory, string filename, XmlNode targetNode, Location location)
        {
            if(String.IsNullOrEmpty(filename))
            {
                return CreateNewTarget(project, buildFileDirectory, targetNode, location);
            }

            string targetName = targetNode.Attributes["name"].Value;

            TargetInfo info = _targets.GetOrAddBlocking(GetKey(filename, targetName), key =>
                {
                   return new TargetInfo(CreateNewTarget(project, buildFileDirectory, targetNode, location), DateTime.Now);
                });

            return info.CachedTarget.Copy();
        }

        private static string GetKey(string filename, string targetName)
        {
            return filename + ":" + targetName;

        }

        private static Target CreateNewTarget(Project project, string buildFileDirectory, XmlNode targetNode, Location location)
        {
            Target target = new Target();
            target.Project = project;
            target.Parent = project;
            target.BaseDirectory = buildFileDirectory;
            if (location != null)
                target.Location = location;
            target.Initialize(targetNode);

            return target;
        }

        private class TargetInfo
        {
            public Target CachedTarget;
            public DateTime LastWriteTime;
            public TargetInfo(Target target, DateTime lastWriteTime)
            {
                CachedTarget = target;
                LastWriteTime = lastWriteTime;
            }
        };

        private static ConcurrentDictionary<string, Lazy<TargetInfo>> _targets;
    }
}
