using System;
using System.Reflection;

namespace EA.Common.CommandHandling {

    public class CommandBuilder {
        
        Assembly _assembly;
        string _className;
        string _commandName;

        public CommandBuilder(string className)
            : this(className, null) {
        }

        public CommandBuilder(string className, Assembly assembly) {
            _className = className;
            _assembly  = assembly;

            // get command name from attribute
            CommandAttribute commandAttribute = (CommandAttribute) Attribute.GetCustomAttribute(assembly.GetType(ClassName), typeof(CommandAttribute));
            _commandName = commandAttribute.CommandName;
        }

        public string ClassName {
            get { return _className; }
        }

        public Assembly Assembly {
            get { return _assembly; }
        }

        public string CommandName {
            get { return _commandName; }
        }

        public Command CreateCommand() {
            Command command;
            try {
                // create instance (ignore case)
                command = (Command) Assembly.CreateInstance(ClassName, true);
            } catch (Exception) {
                command = null;
            }
            return command;
        }
    }
}