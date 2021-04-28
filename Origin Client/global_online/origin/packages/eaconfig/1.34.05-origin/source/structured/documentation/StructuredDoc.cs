using System;
using System.Collections.Generic;
using System.Text;
using System.Reflection;

using EA.Eaconfig.Structured;
using NAnt.Core.Attributes;
using NantPropertyAttribute = NAnt.Core.Attributes.PropertyAttribute;

namespace EA.Eaconfig.Structured.Documentation
{
    public class StructuredDoc
    {
        public List<Tag> RootTags = new List<Tag>();
        private Stack<Tag> TagStack = new Stack<Tag>();

        public class TagAttribute
        {
            public string Name = String.Empty;
            public bool Required = false;
            public string Description = String.Empty;
        }

        public enum TagType
        {
            Undefined,
            FileSet,
            OptionSet,
            Property,
            BuildElement
        }

        public class Tag
        {
            public List<Tag> Tags = new List<Tag>();
            public List<TagAttribute> Attributes = new List<TagAttribute>();

            public bool Required = false;
            public string Description = String.Empty;
            public string Name = String.Empty;
            public TagType Type = TagType.Undefined;
        }

        private void ReadProperty(PropertyInfo pi)
        {
            object[] attrs = pi.GetCustomAttributes(false);
            foreach (object obj in attrs)
            {
                if (obj is TaskAttributeAttribute)
                {
                    TagAttribute ta = new TagAttribute();
                    TaskAttributeAttribute tsa = obj as TaskAttributeAttribute;
                    ta.Name = tsa.Name;
                    ta.Required = tsa.Required;
                    if(ta.Required)
                        TagStack.Peek().Attributes.Insert(0, ta);
                    else
                        TagStack.Peek().Attributes.Add(ta);
                    continue;
                }

                Tag tag = new Tag();
                TagStack.Push(tag);
                if (obj is FileSetAttribute)
                {
                    tag.Type = TagType.FileSet;
                    FileSetAttribute fsa = obj as FileSetAttribute;
                    tag.Name = fsa.Name;
                    tag.Required = fsa.Required;
                }
                else if (obj is OptionSetAttribute)
                {
                    tag.Type = TagType.OptionSet;
                    OptionSetAttribute osa = obj as OptionSetAttribute;
                    tag.Name = osa.Name;
                    tag.Required = osa.Required;
                }
                else if (obj is NantPropertyAttribute)
                {
                    tag.Type = TagType.Property;
                    NantPropertyAttribute npa = obj as NantPropertyAttribute;
                    tag.Name = npa.Name;
                    tag.Required = npa.Required;
                }
                else if (obj is NAnt.Core.Attributes.DescriptionAttribute)
                {
                    NAnt.Core.Attributes.DescriptionAttribute da = obj as NAnt.Core.Attributes.DescriptionAttribute;
                    tag.Description = da.Description;
                }

                if (obj is BuildElementAttribute)
                {
                    if(tag.Type == TagType.Undefined) 
                        tag.Type = TagType.BuildElement;
                    ReadProperties(pi.PropertyType);
                    BuildElementAttribute bea = obj as BuildElementAttribute;
                    tag.Name = bea.Name;
                    tag.Required = bea.Required;
                }

                TagStack.Pop();
                if (tag.Type != TagType.Undefined)
                    TagStack.Peek().Tags.Add(tag);
            }
        }

        private void ReadProperties(Type type)
        {
            PropertyInfo[] props = type.GetProperties(BindingFlags.Public | BindingFlags.Instance);
            foreach (PropertyInfo prop in props)
            {
                ReadProperty(prop);
            }
        }

        public void ReadType(Type type)
        {
            TaskNameAttribute[] attributes = (TaskNameAttribute[])type.GetCustomAttributes(typeof(TaskNameAttribute), false);
            if (attributes.Length > 0)
            {
                TagStack.Push(new Tag());
                TagStack.Peek().Name = attributes[0].Name;
                ReadProperties(type);
                RootTags.Add(TagStack.Pop());
            }
        }
    }
}
