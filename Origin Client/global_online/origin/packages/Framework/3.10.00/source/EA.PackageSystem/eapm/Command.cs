using System;
using System.Collections;
using System.Diagnostics;
using System.Reflection;

namespace EA.PackageSystem.ConsolePackageManager {

    public abstract class Command : IComparable {
        string _name;
        string[] _args = null;

        public string Name { 
            get { return _name; }
            set { _name = value; }
        }

        public string[] Arguments {
            get { return _args; }
            set { _args = value; }
        }

        public abstract string Summary { get; }
        public abstract string Usage { get; }
        public abstract string Remarks { get; }
        public abstract void Execute();

        public int CompareTo(object obj) {
            return CompareTo((Command) obj);
        }

        public int CompareTo(Command command) {
            return String.Compare(this.Name, command.Name);
        }
    }

    [AttributeUsage(AttributeTargets.Class, Inherited=false, AllowMultiple=false)]
    public class CommandAttribute : Attribute {

        string _name;

        public CommandAttribute(string name) {            
            _name = name;
        }

        public string Name {
            get { return _name; }
            set { _name = value; }
        }
    }

    class CommandCollection : ArrayList {
    }

    class CommandFactory {

        public static readonly CommandFactory Instance = new CommandFactory();

        CommandFactory() {
        }

        public Command CreateCommand(string commandName) {
            Assembly assembly = Assembly.GetExecutingAssembly();
            foreach (Type type in assembly.GetTypes()) {
                CommandAttribute attribute;
                if (IsCommand(type, out attribute)) {
                    if (attribute.Name == commandName) {
                        Command command = (Command) assembly.CreateInstance(type.FullName);
                        command.Name = attribute.Name;
                        return command;
                    }
                }
            }

            string msg = String.Format("Unknown command '{0}'.", commandName);
            throw new ApplicationException(msg);
        }

        public CommandCollection CreateAllCommands() {
            CommandCollection commands = new CommandCollection();

            Assembly assembly = Assembly.GetExecutingAssembly();
            foreach (Type type in assembly.GetTypes()) {
                CommandAttribute attribute;
                if (IsCommand(type, out attribute)) {
                    Command command = (Command) assembly.CreateInstance(type.FullName);
                    command.Name = attribute.Name;
                    commands.Add(command);
                }
            }
            commands.Sort();
            return commands;
        }

        bool IsCommand(Type type, out CommandAttribute attribute) {
            attribute = null;

            if (type.IsSubclassOf(typeof(Command)) && !type.IsAbstract) {
                object[] attributes = type.GetCustomAttributes(typeof(CommandAttribute), false);
                if (attributes.Length == 1) {
                    attribute = (CommandAttribute) attributes[0];
                    return true;
                }
            }
            return false;
        }
    }
}
