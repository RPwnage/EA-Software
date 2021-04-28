using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
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

                foreach (var data in SrcDst)
                {
                    try
                    {
                        int err_count = Copy.Execute(data.Src, data.Dst);

                        if (err_count > 0)
                        {
                            if (!SupressError)
                            {
                                ret = 1;
                            }
                        }
                    }
                    catch (Exception er)
                    {
                        SetError(er);
                    }
                }

                if (Copy.Summary)
                {
                    Console.WriteLine("copy: files copied={0}, hard linked={1}, up-to-date={2}.", Copy.copied, Copy.linked, Copy.uptodate); 
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
            string src = null;
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
                        src = null;
                        while ((line = reader.ReadLine()) != null)
                        {
                            if (String.IsNullOrWhiteSpace(line))
                            {
                                continue;
                            }
                            if (src == null)
                            {
                                src = line.Trim(TRIM_CHARS);
                            }
                            else
                            {
                                SrcDst.Add(new Data(src, line.Trim(TRIM_CHARS) ));
                                src = null;
                            }
                        }
                        if (src != null)
                        {
                            throw new ArgumentException(String.Format("Invalid response file '{0}': source '{1}' does not have corresponding destination", file, src));
                        }
                    }
                }
                else
                {
                    if (src == null)
                    {
                        src = arg.Trim(TRIM_CHARS);
                    }
                    else
                    {

                        SrcDst.Add(new Data( src, arg.Trim(TRIM_CHARS)));
                        src = null;
                    }
                }
            }
            if (src != null)
            {
                throw new ArgumentException(String.Format("Invalid command line: source '{0}' does not have corresponding destination", src));
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
                case 'r':
                    Copy.Recurcive = true;
                    break;
                case 'o':
                    Copy.OverwriteReadOnlyFiles = true;
                    break;
                case 'n':
                    Copy.OverwriteReadOnlyFiles = false;
                    break;
                case 't':
                    Copy.SkipUnchangedFiles = true;
                    break;
                case 'l':
                    Copy.UseHardlinksIfPossible = true;
                    break;
                case 'f':
                    Copy.CopyEmptyDir = true;
                    break;
                case 'i':
                    Copy.Info = true;
                    break;
                case 'w':
                    Copy.Warnings = true;
                    break;
                case 'm':
                    Copy.Summary = true;
                    break;
                case 'y':
                    Copy.IgnoreMissingInput = true;
                    break;


                default:
                    if (!Silent)
                    {
                        Console.WriteLine("copy: unknown option '-{0}'", o);
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
                    Console.WriteLine("copy error: '{0}'", e.Message);
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
            Console.WriteLine("Usage: copy [-esvh...] src dst [src1 dst1 ...] [@responsefile]");
            Console.WriteLine();
            Console.WriteLine("  -e  Do not return error code");
            Console.WriteLine("  -s  Silent");
            Console.WriteLine("  -v  Print version (logo)");
            Console.WriteLine("  -r  recursively copy directories");
            Console.WriteLine("  -o  overwrite readonly files (default)");
            Console.WriteLine("  -n  do not overwrite readonly files");
            Console.WriteLine("  -t  copy only newer files");
            Console.WriteLine("  -l  use hardlink if possible");
            Console.WriteLine("  -f  copy empty directories");
            Console.WriteLine("  -i  print list of copied files");
            Console.WriteLine("  -w  print warnings");
            Console.WriteLine("  -m  print summary");
            Console.WriteLine("  -y  ignore missing input (do not return error)");


            Console.WriteLine("  @[filename] Responsefile can contain list of source and destination, each path on a separate line.");
            Console.WriteLine("  file name portion of the source  can contain patterns '*' and '?' .");
        }

        private static List<Data> SrcDst = new List<Data>();

        struct Data
        {
            internal string Src;
            internal string Dst;

            internal Data(string src, string dst)
            {
                Src = src;
                Dst = dst;
            }
        }

        private static readonly char[] TRIM_CHARS = new char[] { '"', ' ', '\n', '\r' };

    }
}
