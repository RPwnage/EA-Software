using System;
using System.Reflection;

namespace EA.Common.CommandHandling {

    /// <summary>Indicates that class should be treated as a command.</summary>
    /// <remarks>
    /// Attach this attribute to a subclass of Command to have the CommandHandler 
    /// be able to reconize it.  The name should be short but must not confict
    /// with any other command already in use.
    /// </remarks>
    [AttributeUsage(AttributeTargets.Class, Inherited=false, AllowMultiple=false)]
    public class CommandAttribute : Attribute {

        string _commandName;

        public CommandAttribute(string commandName) {            
            CommandName = commandName;
        }

        public string CommandName {
            get { return _commandName; }
            set { _commandName = value; }
        }
    }
}