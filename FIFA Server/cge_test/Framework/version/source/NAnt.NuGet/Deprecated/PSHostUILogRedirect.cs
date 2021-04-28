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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Management.Automation;
using System.Management.Automation.Host;
using System.Security;

namespace NAnt.NuGet.Deprecated
{
	// if a nuget script tries to perform UI operations (for example Write-Host) we need an instance of a PSHostUserInterface
	// this subclass implements the non-interactive portions by directing to Framework's logger, interactive elements throw
	// not implemented exception (nothing stops nuget instances from using these elements but they tend not to)
	internal class PSHostUILogRedirect : PSHostUserInterface
	{
		public override PSHostRawUserInterface RawUI
		{
			get { return m_rawUI; }
		}

		private readonly PSHostRawUserInterface m_rawUI = new PSHostRawUISink();

		private const string InteractiveFailureMessage = "Framework powershell host does not support user prompts.";

		// unsupported user interactions methods
		public override Dictionary<string, PSObject> Prompt(string caption, string message, Collection<FieldDescription> descriptions) { throw new NotImplementedException(InteractiveFailureMessage); }
		public override int PromptForChoice(string caption, string message, Collection<ChoiceDescription> choices, int defaultChoice) { throw new NotImplementedException(InteractiveFailureMessage); }
		public override PSCredential PromptForCredential(string caption, string message, string userName, string targetName) { throw new NotImplementedException(InteractiveFailureMessage); }
		public override PSCredential PromptForCredential(string caption, string message, string userName, string targetName, PSCredentialTypes allowedCredentialTypes, PSCredentialUIOptions options) { throw new NotImplementedException(InteractiveFailureMessage); }
		public override SecureString ReadLineAsSecureString() { throw new NotImplementedException(InteractiveFailureMessage); }
		public override string ReadLine() { throw new NotImplementedException(InteractiveFailureMessage); }

		public override void WriteProgress(long sourceId, ProgressRecord record)
		{
			throw new NotImplementedException("Framework powershell host does not support progress indicators.");
		}

		public override void Write(ConsoleColor foregroundColor, ConsoleColor backgroundColor, string value)
		{
			// ignore colours
			Write(value);
		}

		public override void Write(string value) { }
		public override void WriteDebugLine(string message) { }
		public override void WriteErrorLine(string value) { }
		public override void WriteLine(string value) { }
		public override void WriteVerboseLine(string message) { }
		public override void WriteWarningLine(string message) { }
	}
}