// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
using System.Management.Automation.Host;

namespace NAnt.NuGet.Deprecated
{
	// a collection of classes centrered around NugetVSPowerShellEnv - 
	// this class creates a mock environment in which nuget powershell
	// post install script can run after we generate a solution with
	// nuget dependencies

	// implements the raw ui interface but swallows all calls
	internal class PSHostRawUISink : PSHostRawUserInterface
	{
		private const string RawUIFailureMessage = "Framework powershell host does not support raw UI operations.";

		public override ConsoleColor BackgroundColor
		{
			get { return Console.BackgroundColor; } // nuget usage encountered in JQuery package
			set { throw new NotImplementedException(RawUIFailureMessage); }
		}

		public override ConsoleColor ForegroundColor
		{
			get { return Console.ForegroundColor; } // nuget usage encountered in JQuery package
			set { throw new NotImplementedException(RawUIFailureMessage); }
		}

		// unsupported properties
		public override bool KeyAvailable { get { throw new NotImplementedException(RawUIFailureMessage); } }
		public override Coordinates CursorPosition { get { throw new NotImplementedException(RawUIFailureMessage); } set { throw new NotImplementedException(RawUIFailureMessage); } }
		public override Coordinates WindowPosition { get { throw new NotImplementedException(RawUIFailureMessage); } set { throw new NotImplementedException(RawUIFailureMessage); } }
		public override int CursorSize { get { throw new NotImplementedException(RawUIFailureMessage); } set { throw new NotImplementedException(RawUIFailureMessage); } }
		public override Size BufferSize { get { throw new NotImplementedException(RawUIFailureMessage); } set { throw new NotImplementedException(RawUIFailureMessage); } }
		public override Size MaxPhysicalWindowSize { get { throw new NotImplementedException(RawUIFailureMessage); } }
		public override Size MaxWindowSize { get { throw new NotImplementedException(RawUIFailureMessage); } }
		public override Size WindowSize { get { throw new NotImplementedException(RawUIFailureMessage); } set { throw new NotImplementedException(RawUIFailureMessage); } }
		public override string WindowTitle { get { throw new NotImplementedException(RawUIFailureMessage); } set { throw new NotImplementedException(RawUIFailureMessage); } }

		// unsupported functions
		public override BufferCell[,] GetBufferContents(Rectangle rectangle) { throw new NotImplementedException(RawUIFailureMessage); }
		public override KeyInfo ReadKey(ReadKeyOptions options) { throw new NotImplementedException(RawUIFailureMessage); }
		public override void FlushInputBuffer() { throw new NotImplementedException(RawUIFailureMessage); }
		public override void ScrollBufferContents(Rectangle source, Coordinates destination, Rectangle clip, BufferCell fill) { throw new NotImplementedException(RawUIFailureMessage); }
		public override void SetBufferContents(Coordinates origin, BufferCell[,] contents) { throw new NotImplementedException(RawUIFailureMessage); }
		public override void SetBufferContents(Rectangle rectangle, BufferCell fill) { throw new NotImplementedException(RawUIFailureMessage); }
	}
}