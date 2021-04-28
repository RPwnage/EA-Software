using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;

namespace EA.Tools
{
    class Program
    {
        static private bool SupressError = false;
        static private bool Silent = false;
        static private bool PrintLogo = false;
        static private bool Help = false;

        static private int ret = 0;

        static int Main(string[] args)
        {
            try
            {
                ParseCommandLine(args);

                if (!Silent && PrintLogo)
                {
                    ShowBanner();
                }

                if (Help)
                {
                    PrintHelp();
                }

                foreach (var dir in Directories)
                {
                    try
                    {
                        Directory.CreateDirectory(dir);
                    }
                    catch (Exception er)
                    {
                        SetError(er);
                    }
                }
            }
            catch (Exception ex)
            {
                SetError(ex);
            }
            return ret;
        }

        private static void ParseCommandLine(string[] args)
        {
            foreach (var arg in args)
            {
                if (arg[0] == '-' || arg[0] == '/')
                {
                    for (int i = 1; i < arg.Length; i++)
                    {
                        SetOption(arg[i]);
                    }
                }
                else if (arg[0] == '@')
                {
                    var file = arg.Substring(1);
                    using (var reader = new StreamReader(file))
                    {
                        string line = null;
                        
                        while ((line = reader.ReadLine()) != null)
                        {
                            Directories.Add(line.Trim(TRIM_CHARS));
                        }
                    }
                }
                else
                {
                    Directories.Add(arg.Trim(TRIM_CHARS));
                }
            }
        }

        private static void SetOption(char o)
        {
            switch (o)
            {
                case 'e':
                    SupressError = true;
                    break;
                case 's':
                    Silent = true;
                    break;
                case 'v':
                    PrintLogo = true;
                    break;
                case 'h':
                    Help = true;
                    break;
                default:
                    if (!Silent)
                    {
                        Console.WriteLine("mkdir: unknown option '-{0}'", o);
                    }
                    break;
            }
        }

        private static void SetError(Exception e)
        {
            if (!SupressError)
            {
                if (!Silent)
                {
                    Console.WriteLine("fw_mkdir error: '{0}'", e.Message);
                }
                ret = 1;
            }
        }

        private static void ShowBanner()
        {
            // Get version information directly from assembly.  This takes more work but keeps the version 
            // numbers being displayed in sync with what the assembly is marked with.
            string version = Assembly.GetExecutingAssembly().GetName().Version.ToString();

            string title = Assembly.GetExecutingAssembly().GetName().Name;
            object[] titleAttribute = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyTitleAttribute), true);
            if (titleAttribute.Length > 0)
                title = ((AssemblyTitleAttribute)titleAttribute[0]).Title;

            string copyright = "";
            object[] copyRightAttribute = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyCopyrightAttribute), true);
            if (copyRightAttribute.Length > 0)
                copyright = ((AssemblyCopyrightAttribute)copyRightAttribute[0]).Copyright;

            string link = "";
            object[] companyAttribute = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyCompanyAttribute), true);
            if (companyAttribute.Length > 0)
                link = ((AssemblyCompanyAttribute)companyAttribute[0]).Company;

            System.Console.WriteLine(String.Format("{0}-{1} {2}", title, version, copyright));
            System.Console.WriteLine(link);
            System.Console.WriteLine();
        }


        private static void PrintHelp()
        {
            Console.WriteLine("Usage: mkdir [-esvh] dir1 [dir2, dir3 ...] [@responsefile]");
            Console.WriteLine();
            Console.WriteLine("  -e  Do not return error code");
            Console.WriteLine("  -s  Silent");
            Console.WriteLine("  -v  Print version (logo)");
            Console.WriteLine("  @[filename] Responsefile can contain list of directory names, each directory on a separate line.");
        }

        private static List<string> Directories = new List<string>();

        private static readonly char[] TRIM_CHARS = new char[] { '"', ' ', '\n', '\r' };

    }
}
