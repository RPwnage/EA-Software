// NAnt - A .NET build tool
// Copyright (C) 2001-2003 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// 
// 
// File Maintainer:
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Linq;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks {

    /// <summary>Set properties with system information.</summary>
	/// <remarks>
	///   <para>Sets a number of properties with information about the system environment.  The intent of this task is for nightly build logs to have a record of the system information that the build was performed on.</para>
	///   <list type="table">
	///     <listheader><term>Property</term>      <description>Value</description></listheader>
	///     <item><term>sys.clr.version</term>     <description>Common Language Runtime version number.</description></item>
	///     <item><term>sys.env.*</term>           <description>Environment variables, stored both in upper case or their original case.(e.g., sys.env.Path or sys.env.PATH).</description></item>
	///     <item><term>sys.os.folder.system</term><description>The System directory.</description></item>
	///     <item><term>sys.os.folder.temp</term>  <description>The temporary directory.</description></item>
	///     <item><term>sys.os.platform</term>     <description>Operating system platform ID.</description></item>
	///     <item><term>sys.os.version</term>      <description>Operating system version.</description></item>
    ///     <item><term>sys.os</term>              <description>Operating system version string.</description></item>
	///   </list>
	/// </remarks>
    /// <include file='Examples/SysInfo/DefaultPrefix.example' path='example'/>
    /// <include file='Examples/SysInfo/NoPrefix.example' path='example'/>
    /// <include file='Examples/SysInfo/Verbose.example' path='example'/>
    /// <include file='Examples/SysInfo/Enviro.example' path='example'/>
    [TaskName("sysinfo")]
    public class SysInfoTask : Task {
        string _prefix = "sys.";
       
        /// <summary>The string to prefix the property names with.  Default is "sys."</summary>
        [TaskAttribute("prefix", Required=false)]
        public string Prefix {
            get { return _prefix; }
            set { _prefix = value; }
        }


        protected override void ExecuteTask() 
        {
            Log.Debug.WriteLine(LogPrefix + "Setting system information properties under " + Prefix + "*");

            // set properties
            Properties.Add(Prefix + "clr.version", Environment.Version.ToString());
            Properties.Add(Prefix + "os.platform", Environment.OSVersion.Platform.ToString());
            Properties.Add(Prefix + "os.version", Environment.OSVersion.Version.ToString());
            Properties.Add(Prefix + "os.folder.system", Environment.GetFolderPath(Environment.SpecialFolder.System));
            Properties.Add(Prefix + "os.folder.temp", Path.GetTempPath());
            Properties.Add(Prefix + "os", Environment.OSVersion.ToString());

            // set environment variables
            IDictionary variables = Environment.GetEnvironmentVariables();
            foreach (string name in variables.Keys) {
                string value = (string) variables[name];
                if(Property.VerifyPropertyName(name))
                {
                    // add default name
                    Properties.Add(Prefix + "env." + name, value);
                    // add upper case name
                    Properties.Add(Prefix + "env." + name.ToUpper(), value);
                }
                else
                {
                    Log.Debug.WriteLine(LogPrefix + "Cannot create property for '{0}'.", name);
                }
            }
            // Ensure PATH property is always present.
            SetDefaultProperty("PATH", String.Empty);

            if (Log.Level >= Log.LogLevel.Diagnostic)
            {
                // display the properties
                Log.Debug.WriteLine(LogPrefix + "nant.version = " + Properties["nant.version"]);

                Properties.ForEach(p =>
                    {
                        if (p.Name.StartsWith(Prefix, StringComparison.Ordinal) && !p.Name.StartsWith(Prefix + "env.", StringComparison.Ordinal))
                            Log.Debug.WriteLine(LogPrefix + "{0} = {1}", p.Name, p.Value);
                    });
            }
        }

        private void SetDefaultProperty(string name, string defValue)
        {
            if (!Properties.Contains(Prefix + "env." + name.ToUpper()))
            {
                Properties.Add(Prefix + "env." + name.ToUpper(), defValue);
            }
        }
    }
}
