using System;

namespace EA.PackageSystem.ConsolePackageManager {

    class InvalidCommandArgumentsException : ApplicationException {
        Command _command;
        string _details;

        public InvalidCommandArgumentsException(Command command) {
            _command = command;
            _details = "Missing/wrong number of arguments.";
        }

        public InvalidCommandArgumentsException(Command command, string details) {
            _command = command;
            _details = details;
        }

        public override string Message {
            get {
                return String.Format("Usage: eapm {0} {1}\n{2}", _command.Name, _command.Usage, _details);
            }
        }
    }
}
