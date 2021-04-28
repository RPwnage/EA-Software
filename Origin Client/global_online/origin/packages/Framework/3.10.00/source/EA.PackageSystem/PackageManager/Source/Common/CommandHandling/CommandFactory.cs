using System;
using System.Reflection;

namespace EA.Common.CommandHandling {

    public class CommandFactory {

        // Singleton pattern 
        // See "Exploring the Singleton Design Pattern" on MSDN for implementation details of Singletons in .NET
        public static readonly CommandFactory Instance = new CommandFactory();

        private readonly CommandBuilderCollection Builders = new CommandBuilderCollection();

        private CommandFactory() {
            AddCommandClasses(Assembly.GetExecutingAssembly());
        }

        public void AddCommandClasses(Assembly assembly) {
            foreach (Type type in assembly.GetTypes()) {
                if (type.IsSubclassOf(typeof(Command)) && !type.IsAbstract) {
                    Builders.Add(new CommandBuilder(type.FullName, assembly));
                }
            }
        }

        public Command CreateCommand(string commandName) {
            Command command = null;
            CommandBuilder builder = Builders.GetBuilder(commandName);
            if (builder != null) {
                command = builder.CreateCommand();
            }
            return command;
        }

        public void ExecuteCommand(string commandName) {
            Command command = CommandFactory.Instance.CreateCommand(commandName);
            command.Do();
        }
    }
}
