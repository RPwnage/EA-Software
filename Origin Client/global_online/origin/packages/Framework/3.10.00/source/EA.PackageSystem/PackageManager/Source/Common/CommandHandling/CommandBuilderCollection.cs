using System;
using System.Collections;

namespace EA.Common.CommandHandling {

    public class CommandBuilderCollection : DictionaryBase {

        public bool Contains(string commandName) {
            return Dictionary.Contains(commandName);
        }

        public void Add(CommandBuilder builder) {
            if (!Contains(builder.CommandName)) {
                Dictionary.Add(builder.CommandName, builder);
            }
        }

        public CommandBuilder GetBuilder(string commandName) {
            if (Contains(commandName)) {
                return (CommandBuilder) Dictionary[commandName];
            } else {
                return null;
            }
        }
    }
}