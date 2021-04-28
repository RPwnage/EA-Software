using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EA.FrameworkTasks.Model
{
    public class Configuration : IEquatable<Configuration>
    {
        public Configuration(string name, string system, string subsystem, string compiler, string platform, string type)
        {
            Name = name;
            System = system;
            SubSystem = subsystem ?? String.Empty;
            Compiler = compiler;
            Platform = platform;
            Type = type;
        }

        /// <summary>
        /// Full configuration name
        /// </summary>
        public readonly string Name;
        /// <summary>
        /// pc, ps3, etc 
        /// </summary>
        public readonly string System;
        /// <summary>
        /// Variation (spu, etc)
        /// </summary>
        public readonly string SubSystem;
        /// <summary>
        /// Compiler
        /// </summary>
        public readonly string Compiler;
        /// <summary>
        /// System+Compiler
        /// </summary>
        public readonly string Platform;
        /// <summary>
        /// debug, release, etc
        /// </summary>
        public readonly string Type;

        public override bool Equals(object other)
        {
            return Equals(other as Configuration);
        }


        public bool Equals(Configuration other)
        {
            if (Object.ReferenceEquals(other, null)) return false;

            if (Object.ReferenceEquals(this, other)) return true;

            return Name.Equals(other.Name);
        }

        public override int GetHashCode()
        {
            return Name.GetHashCode();
        }

        public static bool operator ==(Configuration a, Configuration b)
        {
            if (Object.ReferenceEquals(a, b))
            {
                return true;
            }

            if (((object)a == null) || ((object)b == null))
            {
                return false;
            }

            return a.Name == b.Name;
        }

        public static bool operator !=(Configuration a, Configuration b)
        {
            return !(a == b);
        }
    }

}
