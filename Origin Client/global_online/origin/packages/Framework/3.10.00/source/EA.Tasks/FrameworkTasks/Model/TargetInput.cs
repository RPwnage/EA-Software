using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EA.FrameworkTasks.Model
{
    public class TargetInput
    {
        public enum InputItemType { Property, FileSet, OptionSet};

        public readonly List<TargetInputItem> Data = new List<TargetInputItem>();

        public class TargetInputItem
        {
            public TargetInputItem(string name, string sourcename, InputItemType type)
            {
                Name = name;
                SourceName = sourcename;
                Type = type;
            }
            public readonly string Name;
            public readonly string SourceName;
            public readonly InputItemType Type;
        }
    }
}
