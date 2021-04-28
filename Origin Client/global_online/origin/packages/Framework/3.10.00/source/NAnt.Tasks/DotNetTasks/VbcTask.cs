// NAnt - A .NET build tool
// Copyright (C) 2001-2002 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for details.
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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Mike Krueger (mike@icsharpcode.net)
// Aaron A. Anderson (aaron@skypoint.com | aaron.anderson@farmcreditbank.com)

using System;
using System.IO;

using NAnt.Core.Attributes;

namespace NAnt.DotNetTasks {

    /// <summary>Compiles Microsoft Visual Basic.NET programs using vbc.exe.</summary>
    // <include file='Examples/Vbc/Vbc.example' path='example'/>
    [TaskName("vbc")]
    public class VbcTask : CompilerBase {

        public VbcTask() : base("vbc") { }

        string _baseAddress     = null;
        string _imports         = null;
        string _optionCompare   = null;
        bool   _optionExplicit  = false;
        bool   _optionStrict    = false;
        bool   _optionOptimize  = false;
        bool   _removeintchecks = false;
        string _rootNamespace   = null;

        /// <summary>Specifies whether <c>/baseaddress</c> option gets passed to the compiler.</summary>
        /// <remarks><a href="ms-help://MS.NETFrameworkSDK/vblr7net/html/valrfbaseaddressspecifybaseaddressofdll.htm">See the Microsoft.NET Framework SDK documentation for details.</a></remarks>
        /// <value>The value of this property is a string that makes up a 32bit hexidecimal number.</value>
        [TaskAttribute("baseaddress")]
        public string BaseAddress   { get { return _baseAddress; } set {_baseAddress = value;}}

        /// <summary>Specifies whether the <c>/imports</c> option gets passed to the compiler</summary>
        /// <remarks><a href="ms-help://MS.NETFrameworkSDK/vblr7net/html/valrfImportImportNamespaceFromSpecifiedAssembly.htm">See the Microsoft.NET Framework SDK documentation for details.</a></remarks>
        /// <value>The value of this attribute is a string that contains one or more namespaces separated by commas.</value>
        /// <example>Example of an imports attribute
        /// <code><![CDATA[imports="Microsoft.VisualBasic, System, System.Collections, System.Data, System.Diagnostics"]]></code></example>
        [TaskAttribute("imports")]
        public string Imports         { get { return _imports; } set {_imports = value;}}

        /// <summary>Specifies whether <c>/optioncompare</c> option gets passed to the compiler</summary>
        /// <remarks><a href="ms-help://MS.NETFrameworkSDK/vblr7net/html/valrfOptioncompareSpecifyHowStringsAreCompared.htm">See the Microsoft.NET Framework SDK documentation for details.</a></remarks>
        /// <value>The value of this property must be either <c>text</c>, <c>binary</c>, or an empty string.  If the value is <c>false</c> or empty string, the switch is omitted.</value>
        [TaskAttribute("optioncompare")]
        public string OptionCompare   { get { return _optionCompare; } set {_optionCompare = value;}}

        /// <summary>Specifies whether the <c>/optionexplicit</c> option gets passed to the compiler.</summary>
        /// <remarks><a href="ms-help://MS.NETFrameworkSDK/vblr7net/html/valrfOptionexplicitRequireExplicitDeclarationOfVariables.htm">See the Microsoft.NET Framework SDK documentation for details.</a></remarks>
        /// <value>The value of this attribute must be either <c>true</c> or <c>false</c>.  If <c>false</c>, the switch is omitted.</value>
        [TaskAttribute("optionexplicit")]
        public bool   OptionExplicit  { get { return Convert.ToBoolean(_optionExplicit); } set {_optionExplicit = value;}}
        
        /// <summary>Specifies whether the <c>/optimize</c> option gets passed to the compiler.</summary>
        /// <remarks><a href="ms-help://MS.NETFrameworkSDK/vblr7net/html/valrfoptimizeenabledisableoptimizations.htm">See the Microsoft.NET Framework SDK documentation for details.</a></remarks>
        /// <value>The value of this attribute must be either <c>true</c> or <c>false</c>.  If <c>false</c>, the switch is omitted.</value>
        [TaskAttribute("optionoptimize")]
        public bool   OptionOptimize{ get { return Convert.ToBoolean(_optionOptimize); } set {_optionOptimize = value;}}

        /// <summary>Specifies whether the <c>/optionstrict</c> option gets passed to the compiler.</summary>
        /// <remarks><a href="ms-help://MS.NETFrameworkSDK/vblr7net/html/valrfOptionstrictEnforceStrictTypeSemantics.htm">See the Microsoft.NET Framework SDK documentation for details.</a></remarks>
        /// <value>The value of this attribute must be either <c>true</c> or <c>false</c>.  If <c>false</c>, the switch is omitted.</value>
        [TaskAttribute("optionstrict")]
        public bool   OptionStrict    { get { return Convert.ToBoolean(_optionStrict); } set {_optionStrict = value;}}

        /// <summary>Specifies whether the <c>/removeintchecks</c> option gets passed to the compiler.</summary>
        /// <remarks><a href="ms-help://MS.NETFrameworkSDK/vblr7net/html/valrfRemoveintchecksRemoveInteger-OverflowChecks.htm">See the Microsoft.NET Framework SDK documentation for details.</a></remarks>
        /// <value>The value of this attribute must be either <c>true</c> or <c>false</c>.  If <c>false</c>, the switch is omitted.</value>
        [TaskAttribute("removeintchecks")]
        public bool   RemoveIntChecks { get { return Convert.ToBoolean(_removeintchecks); } set {_removeintchecks = value;}}

        /// <summary>Specifies whether the <c>/rootnamespace</c> option gets passed to the compiler.</summary>
        /// <remarks><a href="ms-help://MS.NETFrameworkSDK/vblr7net/html/valrfRootnamespace.htm">See the Microsoft.NET Framework SDK documentation for details.</a></remarks>
        /// <value>The value of this attribute is a string that contains the root namespace of the project.</value>
        [TaskAttribute("rootnamespace")]
        public string RootNamespace   { get { return _rootNamespace; } set {_rootNamespace = value;}}

        /// <summary>
        /// Writes the compiler options to the specified TextWriter.
        /// </summary>
        /// <param name="writer"></param>
        /// <remarks></remarks>
        protected override void WriteOptions(TextWriter writer) {
            if (_baseAddress != null) {
                writer.WriteLine("/baseaddress:{0}", _baseAddress);
            }
            if (Debug) {
                writer.WriteLine("/debug");
                writer.WriteLine("/define:DEBUG=True,TRACE=True");
            }
            if (_imports != null) {
                writer.WriteLine("/imports:{0}", _imports);
            }
            System.Console.WriteLine("/imports:{0}", _imports);

            if (_optionCompare != null) {
                writer.WriteLine("/optioncompare:{0}", _optionCompare);
            }
            if (OptionExplicit) {
                writer.WriteLine("/optionexplicit");
            }
            if (OptionStrict) {
                writer.WriteLine("/optionstrict");
            }
            if (RemoveIntChecks) {
                writer.WriteLine("/removeintchecks");
            }	
            if (OptionOptimize) {
                writer.WriteLine("/optimize");
            }	
            if (_rootNamespace != null) {
                writer.WriteLine("/rootnamespace:{0}", _rootNamespace);
            }
        }        
        protected override string GetExtension(){ return "vb";}
    }
}
