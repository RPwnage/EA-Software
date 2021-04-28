using System;
using System.Collections.Specialized;
using System.Text;

using NAnt.Core.PackageCore;

namespace EA.PackageSystem.ConsolePackageManager {

    /*
        eapm install PackageName
        eapm install PackageName-1.2.3
        eapm remove PackageName-1.2.3
        eapm list
        eapm list Framework
        eapm post build/PackageName-1.2.3.zip
        eapm help
        eapm describe [--contact] [--homepageurl] [--requires] [--requiredby] PackageName
        eapm describe PackageName-1.2.3
    */

    public class ConsolePackageManager {

        public static int Main(string[] args) {
            ConsolePackageManager app = new ConsolePackageManager();
            return app.Start(args);
        }

        public ConsolePackageManager() 
        {
            PackageMap.Init(null);
        }

        public int Start(string[] args) {
            try {
                Command command = ParseCommandLine(args);
                command.Execute();
                return ReturnCode.Ok;

            } catch (ApplicationException e) {
                ShowUserError(e);
                return ReturnCode.Error;

            } catch (Exception e) {
                ShowInternalError(e);
                return ReturnCode.InternalError;
            }
        }

        Command ParseCommandLine(string[] args) {
            Command command = null;

            // parse command line arguments
            if (args.Length >= 1) {
                // parse command
                string commandName = args[0];
                command = CommandFactory.Instance.CreateCommand(commandName);

                // let the command parse the rest by passing it the remaining arguments
                string[] commandArguments = new string[args.Length - 1];
                Array.Copy(args, 1, commandArguments, 0, commandArguments.Length);
                command.Arguments = commandArguments;

                // once we find a command stop parsing (the command will parse the rest)
            }

            // if no command given, display about help topic
            if (command == null) {
                command = new HelpCommand();
                command.Arguments = new string[] { "about" };
            }

            return command;
        }

        void ShowUserError(ApplicationException e) {
            Exception current = e;
            while (current != null && current.Message.Length > 0) {
                Console.WriteLine(current.Message);
                current = current.InnerException;
            }
            Console.WriteLine();
            Console.WriteLine("Try 'eapm help' for more information.");
        }

        void ShowInternalError(Exception e) {
            Console.WriteLine("INTERNAL ERROR");
            Console.WriteLine(e.ToString());
            Console.WriteLine();
            Console.WriteLine("Please send bug reports to framework2users@ea.com.");
        }
    }
}
