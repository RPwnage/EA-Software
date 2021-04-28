using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using EA.FrameworkTasks.Model;


namespace EA.Eaconfig.Core
{
    public class PublicData :IPublicData
    {
        public IEnumerable<string> Defines
        {
            get { return _defines; }
            set { _defines = value; }
        }

        public IEnumerable<PathString> IncludeDirectories 
        { 
            get { return _includedirectories; }
            set { _includedirectories = value; }
        }

        public IEnumerable<PathString> UsingDirectories
        {
            get { return _usingdirectories; }
            set { _usingdirectories = value; }
        }

        public IEnumerable<PathString> LibraryDirectories
        {
            get { return _librarydirectories; }
            set { _librarydirectories = value; }
        }


        public FileSet Libraries
        {
            get { return _libraries; }
            set { _libraries = value; }
        }

        public FileSet Assemblies
        {
            get { return _assemblies; }
            set { _assemblies = value; }
        }


        public FileSet DynamicLibraries
        {
            get { return _dynamiclibraries; }
            set { _dynamiclibraries = value; }
        }

        private IEnumerable<string> _defines;
        private IEnumerable<PathString> _includedirectories;
        private IEnumerable<PathString> _usingdirectories;
        private IEnumerable<PathString> _librarydirectories;
        private FileSet _libraries;
        private FileSet _dynamiclibraries;
        private FileSet _assemblies;
    }
}
