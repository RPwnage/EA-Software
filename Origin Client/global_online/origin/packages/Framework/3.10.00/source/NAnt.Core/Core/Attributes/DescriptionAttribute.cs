using System;

namespace NAnt.Core.Attributes
{
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Class, Inherited = true)]
    public sealed class DescriptionAttribute : Attribute
    {
        public DescriptionAttribute(string description)
        {
            _description = description;
        }

        public string Description { get { return _description; } } string _description;
    }
}
