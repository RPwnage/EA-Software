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
using System.Linq;
using System.Net;
using NAnt.Shared.Properties;

namespace EA.PackageSystem.ConsolePackageManager
{
	// TODO logging clean up
	// dvaliant 2016/11/08
	/* logging in eapm is a mixture of Console and nant's Log class, should unify */

	public class ConsolePackageManager
	{
		public const string LogPrefix = "[eapm] ";

		public static int Main(string[] args)
		{
			ConsolePackageManager app = new ConsolePackageManager();
			return app.Start(args);
		}

		public int Start(string[] args)
		{
			try
			{
				Command command = ParseCommandLine(args);
				command.Execute();
				return ReturnCode.Ok;
			}
			catch (ApplicationException e)
			{
				ShowUserError(e);
				return ReturnCode.Error;
			}
			catch (WebException e)
			{
				ShowUserError(new ApplicationException("Unable to download package", e));
				return ReturnCode.Error;
			}
			catch (Exception e)
			{
				ShowInternalError(e);
				return ReturnCode.InternalError;
			}
		}

		private Command ParseCommandLine(string[] args)
		{
			Command command = null;

			// parse command line arguments
			if (args.Any())
			{
				// parse command
				string commandName = args.First();
				command = CommandFactory.Instance.CreateCommand(commandName);

				// let the command parse the rest by passing it the remaining arguments
				command.Arguments = args.Skip(1);

				// once we find a command stop parsing (the command will parse the rest)
			}

			// if no command given, display about help topic
			if (command == null)
			{
				command = new HelpCommand();
				command.Arguments = new string[] { "about" };
			}

			return command;
		}

		private bool HandleCredentialRejection(Exception e)
		{
			Exception baseEx = e.GetBaseException();
			if (baseEx is WebException && baseEx.Message == "The request failed with HTTP status 401: Unauthorized.")
			{
				// special case for handling authorization problem messages.
				Console.WriteLine(CMProperties.PackageServerAuthenticationError);
				return true;
			}
			return false;
		}

		private void ShowUserError(ApplicationException e)
		{
			if (!HandleCredentialRejection(e))
			{
				Exception current = e;
				while (current != null && current.Message.Length > 0)
				{
					Console.WriteLine(current.Message);
					current = current.InnerException;
				}
				Console.WriteLine();
				Console.WriteLine("Try 'eapm help' for more information.");
			}
		}

		private void ShowInternalError(Exception e)
		{
			if (!HandleCredentialRejection(e))
			{
				Console.WriteLine("INTERNAL ERROR");
				Console.WriteLine(e.ToString());
				Console.WriteLine();
				Console.WriteLine("Please send bug reports to {0}.", CMProperties.CMSupportChannel);
			}
		}
	}
}
