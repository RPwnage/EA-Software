// Copyright (C) 2019 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Linq;

namespace EA.Tools
{
    class Program
    {
        static private bool Help = false;
        static private bool Verbose = false;
        static private string SrcFile = null;
        static private string ObjFile = null;
        static private string DepFile = null;
        static private string CLExePath = null;
        static List<string> CLExeArguments = new List<string>();

        static private int ret = 0;

        static int Main(string[] args)
        {
            try
            {
                ParseCommandLine(args);

                if (Help)
                {
                    PrintHelp();
                    return 0;
                }

                return EA.CPlusPlusTasks.VCCallerWithDependency.Execute(EA.CPlusPlusTasks.VCCallerWithDependency.ProgramType.COMPILER, CLExePath, CLExeArguments, SrcFile, ObjFile, DepFile, Verbose);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                ret = 99;
            }

            return ret;
        }

        private static void ParseCommandLine(string[] args)
        {
            int currentIndex = 0;
            foreach (string arg in args)
            {
                if (arg.StartsWith("/src:") || arg.StartsWith("-src:"))
                {
                    SrcFile = arg.Substring(5);
                }
                else if (arg.StartsWith("/obj:") || arg.StartsWith("-obj:"))
                {
                    ObjFile = arg.Substring(5);
                }
                else if (arg.StartsWith("/dep:") || arg.StartsWith("-dep:"))
                {
                    DepFile = arg.Substring(5);
                }
                else if (arg == "/verbose" || arg == "-verbose")
                {
                    Verbose = true;
                }
                else
                {
                    CLExePath = arg;
                }

                if (CLExePath != null)
                {
                    break;
                }

                ++currentIndex;
            }

            string errMsg = "";
            if (SrcFile == null)
            {
                errMsg += "Unable to get source file argument." + Environment.NewLine;
            }
            if (ObjFile == null)
            {
                errMsg += "Unable to get object file argument." + Environment.NewLine;
            }
            if (CLExePath == null)
            {
                errMsg += "Unable to get real compiler exe path." + Environment.NewLine;
            }

            if (!String.IsNullOrEmpty(errMsg))
            {
                PrintHelp();
                throw new System.ArgumentException("Invalid input arguments!" + Environment.NewLine + errMsg);
            }

            CLExeArguments = args.ToList().GetRange(currentIndex + 1, args.Length-(currentIndex + 1));
        }

        private static void PrintHelp()
        {
            Console.WriteLine("Usage: cl_wrapper.exe </src:<srcfile>> </obj:<objfile>> [/dep:<depfile>] [/verbose] <full_path to real compiler exe> <... real compiler exe pass through arguments ...>");
            Console.WriteLine();
        }

    }
}
