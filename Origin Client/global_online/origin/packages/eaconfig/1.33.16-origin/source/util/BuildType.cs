using NAnt.Core;

using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;

namespace EA.Eaconfig
{
    public class BuildType
    {
        public enum Types { Generic=0, CSharp = 1, Managed = 2, EASharp = 4, MakeStyle=8, FSharp = 16 }

        public readonly String Name;
        public readonly String BaseName;
        public readonly String SubSystem;
        public readonly Types Type;

        public bool IsCLR
        {
            get
            {
                return IsCSharp || IsFSharp || IsManaged;
            }
        }

        public bool IsCSharp
        {
            get
            {
                return ((Type & Types.CSharp) == Types.CSharp);
            }
        }

        public bool IsFSharp
        {
            get
            {
                return ((Type & Types.FSharp) == Types.FSharp);
            }
        }

        public bool IsEASharp
        {
            get
            {
                return ((Type & Types.EASharp) == Types.EASharp);
            }
        }

        public bool IsMakeStyle
        {
            get
            {
                return ((Type & Types.MakeStyle) == Types.MakeStyle);
            }
        }



        public bool IsManaged
        {
            get
            {
                return ((Type & Types.Managed) == Types.Managed);
            }
        }

        public BuildType(string name, string basename, string subsystem, bool isEasharp, bool isMakestype)
        {
            if (name == "Program")
            {
                name = "StdProgram";
            }
            else if (name == "Library")
            {
                name = "StdLibrary";
            }

            Name = name;
            BaseName = basename;
            if(!String.IsNullOrEmpty(subsystem))
            {
                SubSystem = subsystem;
            }
            else
            {
                SubSystem = string.Empty;
            }
            if (BaseName.Contains("CSharp"))
            {
                Type = Types.CSharp;
            }
            else if (BaseName.Contains("FSharp"))
            {
                Type = Types.FSharp;
            }
            else if (BaseName.Contains("Managed"))
            {
                Type = Types.Managed;
            }
            else
            {
                Type = Types.Generic;
            }
            if (isEasharp)
            {
                Type = Type | Types.EASharp;
            }
            if (isMakestype)
            {
                Type = Type | Types.MakeStyle;
            }


        }
    }
}


