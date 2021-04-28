using System;
using System.Diagnostics;
using System.Reflection;
using System.Text;

namespace EA.PackageSystem.ConsolePackageManager {

    [Command("help")]
    class HelpCommand : Command {

        public override string Summary {
            get { return "get program help"; } 
        }
        public override string Usage {
            get { return "[<command>]"; } 
        }

        public override string Remarks {
            get { return 
@"Description
    Get help about the program or a command.  If no command name is given,
    print program usage help.

Examples
    Show help on the list command:
    > eapm help list";
            } 
        }

        const string CommandHelpFormat =
@"{0} -- {1}

Usage: eapm {0} {2}

{3}
";
        const string BannerFormat = 
@"EAPM-{0}.{1}.{2} (C) 2003 Electronic Arts

";

        const string AboutHelpString  =
@"EAPM is the EA Package Manager for the command line.  Try:

    eapm help           get command list
    eapm help <command> get help on a specific command
";

        const string UsageHelpFormat =
@"Usage: eapm <command> [arg ...]

Commands
{0}";


        public override void Execute() {
            string help;

            if (Arguments.Length == 0) {
                help = GetUsageHelpText();
            } else {
                string commandName = Arguments[0];
                switch (commandName) {
                    case "about":
                        help = GetAboutHelpText();
                        break;

                    case "usage":
                        help = GetUsageHelpText();
                        break;

                    default:
                        help = GetCommandHelpText(CommandFactory.Instance.CreateCommand(commandName));
                        break;
                }
            }

            Console.Write(help);
        }


        string GetCommandHelpText(Command command) {
            return String.Format(CommandHelpFormat, command.Name, command.Summary, command.Usage, command.Remarks);
        }

        string GetBannerText() { 
            FileVersionInfo info = FileVersionInfo.GetVersionInfo(Assembly.GetExecutingAssembly().Location);
            return String.Format(BannerFormat, info.FileMajorPart, info.FileMinorPart, info.FileBuildPart);
        }

        string GetAboutHelpText() {
            return GetBannerText() + AboutHelpString;
        }

        string GetUsageHelpText() {
            StringBuilder commandList = new StringBuilder();
            foreach (Command command in CommandFactory.Instance.CreateAllCommands()) {
                commandList.AppendFormat("    {0,-15} {1}\n", command.Name, command.Summary);
            }

            return GetBannerText() + String.Format(UsageHelpFormat, commandList.ToString());
        }
    }
}
