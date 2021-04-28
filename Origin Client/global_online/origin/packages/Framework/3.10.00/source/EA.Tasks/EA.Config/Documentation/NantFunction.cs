using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using System.Reflection;

namespace EA.Eaconfig
{
    public class NantFunction
    {
        public string Name;
        public XmlNode Description;
        public MethodInfo MethodInfo;

        public override string ToString()
        {
            string parameters = "";
            foreach (ParameterInfo param in MethodInfo.GetParameters())
            {
                if (parameters != String.Empty)
                {
                    parameters += ",";
                }
                parameters += param.ParameterType.FullName;
            }

            return String.Format("{0}.{1}({2})",
                MethodInfo.DeclaringType.FullName, MethodInfo.Name, parameters)
                .ToLowerInvariant();
        }

        public string GetShortString()
        {
            string parameters = "";
            foreach (ParameterInfo param in MethodInfo.GetParameters())
            {
                if (param.ParameterType.Name.ToLowerInvariant() != "project")
                {
                    if (parameters != String.Empty)
                    {
                        parameters += ", ";
                    }
                    parameters += param.ParameterType.Name;
                }
            }

            return String.Format("{0}({1})",
                MethodInfo.Name, parameters);
        }

        public string GetShortStringWithNames()
        {
            string parameters = "";
            foreach (ParameterInfo param in MethodInfo.GetParameters())
            {
                if (param.ParameterType.Name.ToLowerInvariant() != "project")
                {
                    if (parameters != String.Empty)
                    {
                        parameters += ", ";
                    }
                    parameters += param.ParameterType.Name + " " + param.Name;
                }
            }

            return String.Format("{0}({1})",
                MethodInfo.Name, parameters);
        }
    }
}
