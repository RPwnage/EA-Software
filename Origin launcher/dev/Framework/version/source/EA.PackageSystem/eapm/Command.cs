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
using System.Linq;
using System.Reflection;

namespace EA.PackageSystem.ConsolePackageManager
{

    internal abstract class Command
    {
        internal abstract string Summary { get; }
        internal abstract string Usage { get; }
        internal abstract string Remarks { get; }

        internal string Name;
        internal IEnumerable<string> Arguments;

        internal abstract void Execute();
	}

	[AttributeUsage(AttributeTargets.Class, Inherited=false, AllowMultiple=false)]
    internal class CommandAttribute : Attribute
    {
        internal readonly string Name;

        internal CommandAttribute(string name)
        {
            Name = name;
        }
	}

	internal class CommandFactory
    {
        internal static readonly CommandFactory Instance = new CommandFactory();

		private CommandFactory()
        {
		}

        internal Command CreateCommand(string commandName)
        {
			Assembly assembly = Assembly.GetExecutingAssembly();
			foreach (Type type in assembly.GetTypes())
            {
				CommandAttribute attribute;
				if (IsCommand(type, out attribute))
                {
					if (attribute.Name == commandName)
                    {
						Command command = (Command) assembly.CreateInstance(type.FullName);
						command.Name = attribute.Name;
						return command;
					}
				}
			}
			throw new ApplicationException(String.Format("Unknown command '{0}'.", commandName));
		}

		internal List<Command> CreateAllCommands()
        {
            List<Command> commands = new List<Command>();

			Assembly assembly = Assembly.GetExecutingAssembly();
			foreach (Type type in assembly.GetTypes())
            {
				CommandAttribute attribute;
				if (IsCommand(type, out attribute))
                {
					Command command = (Command) assembly.CreateInstance(type.FullName);
					command.Name = attribute.Name;
					commands.Add(command);
				}
			}
            commands = commands.OrderBy(com => com.Name).ToList();
			return commands;
		}

		private bool IsCommand(Type type, out CommandAttribute attribute)
        {
			if (type.IsSubclassOf(typeof(Command)) && !type.IsAbstract)
            {
				object[] attributes = type.GetCustomAttributes(typeof(CommandAttribute), false);
				if (attributes.Any())
                {
                    attribute = (CommandAttribute)attributes.First();
					return true;
				}
			}

            attribute = null;
			return false;
		}
	}
}
