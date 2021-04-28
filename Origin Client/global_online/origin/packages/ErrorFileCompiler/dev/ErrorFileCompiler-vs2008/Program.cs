using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ErrorFileCompiler
{
    class AreaCode
    {
        public string Area;
        public int Value;
        public string Comment;
    };

    class ErrorWarning
    {
        public string Code;
        public List<AreaCode> ErrorInfo;
        public int Value;
        public string Comment;
    };

    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length != 2)
            {
                Console.WriteLine("Usage: ErrorFileCompiler inputfile outputfile");
                return;
            }

            StreamReader reader = File.OpenText(args[0]);

            string line;

            bool bAreaDefs = true;

            List<AreaCode> AreaCodes = new List<AreaCode>();
            Dictionary<string, AreaCode> AreaCodeMap = new Dictionary<string, AreaCode>();
            List<ErrorWarning> ErrorCodes = new List<ErrorWarning>();


            while ((line = reader.ReadLine()) != null)
            {
                if(line.StartsWith("/// $errors"))
                {
                    bAreaDefs = false;
                    continue;
                }

                string[] parts = line.Split(new char[]{' ', '\t', '(', '+', ')'}, StringSplitOptions.RemoveEmptyEntries);

                if (parts.Length > 2)
                {
                    if (parts[0] == "#define")
                    {
                        if (bAreaDefs)
                        {
                            AreaCode ac = new AreaCode();

                            ac.Area = parts[1];

                            // Read the value
                            if (parts[2].StartsWith("0x"))
                            {
                                ac.Value = int.Parse(parts[2].Substring(2), System.Globalization.NumberStyles.HexNumber);
                            }
                            else if (parts[2].Contains("<<"))
                            {
                                string[] p = parts[2].Split(new string[] { "<<" }, StringSplitOptions.RemoveEmptyEntries);

                                if (p.Length == 2)
                                {
                                    ac.Value = int.Parse(p[0]) << int.Parse(p[1]);
                                }
                                else
                                {
                                    Console.WriteLine("Invalid Line: " + line);
                                    continue;
                                }
                            }

                            if (parts.Length > 3)
                            {
                                if (parts[3].StartsWith("//"))
                                {
                                    ac.Comment = string.Join(" ", parts.ToList().Skip(4).ToArray());
                                }
                            }

                            AreaCodes.Add(ac);
                            AreaCodeMap[ac.Area] = ac;
                        }
                        else
                        {
                            ErrorWarning e = new ErrorWarning();

                            e.ErrorInfo = new List<AreaCode>();

                            e.Code = parts[1];

                            for (int i = 2; i < parts.Length; i++)
                            {
                                AreaCode ac;
                                if (AreaCodeMap.TryGetValue(parts[i], out ac))
                                {
                                    e.ErrorInfo.Add(ac);
                                    e.Value += ac.Value;

                                }
                                else
                                {
                                    if (parts[i].StartsWith("0x"))
                                    {
                                        e.Value += int.Parse(parts[i].Substring(2), System.Globalization.NumberStyles.HexNumber);
                                    }
                                    else if (parts[i].StartsWith("//"))
                                    {
                                        e.Comment = string.Join(" ", parts.ToList().Skip(i+1).ToArray());
                                        break;
                                    }
                                    else
                                    {
                                        e.Value += int.Parse(parts[i]);
                                    }
                                }
                            }
                            ErrorCodes.Add(e);
                        }
                    }
                }
            }

            StreamWriter writer = File.CreateText(args[1]);

            writer.WriteLine(" /// Do not change this file. This file is generated from: " + args[0]);
            writer.WriteLine();
            writer.WriteLine("#include \"stdafx.h\"");
            writer.WriteLine("#include <eastl/map.h>");
            writer.WriteLine("#include \"../impl/EbisuErrorFunctions.h\"");
            writer.WriteLine("#include \"EbisuSDK/EbisuTypes.h\"");
            writer.WriteLine();
            writer.WriteLine("namespace Ebisu");
            writer.WriteLine("{");
            writer.WriteLine();
            writer.WriteLine("ErrorRecord ErrorList[] = ");
            writer.WriteLine("{");

            for (int i = 0; i < ErrorCodes.Count; i++)
            {
                writer.WriteLine("\t{\"" + ErrorCodes[i].Code + "\", \"" + (ErrorCodes[i].ErrorInfo.Count >= 1 ? ErrorCodes[i].ErrorInfo[1].Area : "EBISU_ERROR_AREA_GENERAL") + "\", \"" + ErrorCodes[i].Comment + "\"}" + (i < ErrorCodes.Count - 1 ? "," : ""));
            }

            writer.WriteLine("};");
            writer.WriteLine();
            writer.WriteLine();
            writer.WriteLine("eastl::map<int, int> ErrorToRecordMap;");
            writer.WriteLine();
            writer.WriteLine("struct ErrorToRecordMapInitializer");
            writer.WriteLine("{");
            writer.WriteLine("    ErrorToRecordMapInitializer()");
            writer.WriteLine("    {");
            for(int i=0; i<ErrorCodes.Count; i++)
            {
                writer.WriteLine("        ErrorToRecordMap[0x" + ErrorCodes[i].Value.ToString("X8") + "] = " + i + ";\t// " + ErrorCodes[i].Code + "\t: " + ErrorCodes[i].Comment);
            }
            writer.WriteLine("    }");
            writer.WriteLine("};");

            writer.WriteLine();
            writer.WriteLine("ErrorRecord * GetErrorInfo(EbisuErrorT err)");
            writer.WriteLine("{");
            writer.WriteLine("  static ErrorToRecordMapInitializer InitErrorToRecordMap;");
            writer.WriteLine("	eastl::map<int, int>::iterator i = ErrorToRecordMap.find(err);");
            writer.WriteLine();
            writer.WriteLine("	if(i != ErrorToRecordMap.end())");
            writer.WriteLine("	{");
            writer.WriteLine("		return &ErrorList[i->second];");
            writer.WriteLine("	}");
            writer.WriteLine("	else");
            writer.WriteLine("	{");
            writer.WriteLine("		return NULL;");
            writer.WriteLine("	}");
            writer.WriteLine("}");
            writer.WriteLine();
            writer.WriteLine("} // namespace ebisu");
            writer.WriteLine();

            writer.Close();

        }
    }
}
