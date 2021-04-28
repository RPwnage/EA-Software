// (c) Electronic Arts. All rights reserved.

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace EA.Eaconfig
{
	public class VC_Common_Options : GeneratePlatformOptions
	{
		protected bool OptionsetHasLinkTask()
		{
			bool optionSetHasLinktask = false;
			if (ConfigOptionSet.Options.Contains("build.tasks"))
			{
				// We split the tasks into individual component first instead of doing a string search
				// of "link" in case we have any custom build tasks and don't want to be confused
				// with any user custom tasks.
				if (new List<string>(ConfigOptionSet.Options["build.tasks"].Split()).Contains("link"))
				{
					optionSetHasLinktask = true;
				}
			}
			return optionSetHasLinktask;
		}

		protected string GetCompilerPdbTemplate()
		{
			if (OptionsetHasLinkTask())
			{
				// If this option set has a link task (ie building program/dll/assembly), we need the compiler
				// to output the pdb to an intermediate folder because the linker (with the -PDB switch)
				// will also create one at the final outputdir.  As of Visual Studio 2015, the pdb format
				// between linker and compiler are no longer compatible if -debug:fastlink option is used.
				// Also, check the following link regarding the difference between linker's pdb vs compiler's pdb.
				// They technically contains different output and for build that has link task, we want to use
				// linker generated pdb:
				// http://blogs.msdn.com/b/yash/archive/2007/10/12/pdb-files-what-are-they-and-how-to-generate-them.aspx
				return "%intermediatedir%\\%outputname%.pdb";
			}
			else
			{
				// If this option set doesn't have link task (such as static library build), we will revert to the
				// old behaviour and have the pdb generate directly at the outputdir.  Unlike the linker,
				// the librarian tool doesn't have option to create the pdb and we want the pdb to be side
				// by side with the .lib file.
				return "%outputdir%\\%outputname%.pdb";
			}
		}

		protected void Common_VC_PCH_Options()
		{
			if (Properties.GetBooleanPropertyOrDefault("eaconfig.enable-pch", true))
			{
				if (ConfigOptionSet.Options["pch-header-file"] != null)
					AddOptionIf("create-pch", "on", OptionType.cc_Options, "/Yc\"" + ConfigOptionSet.Options["pch-header-file"] + "\"");
				else
					AddOptionIf("create-pch", "on", OptionType.cc_Options, "/Yc\"%pchheaderfile%\"");

				AddOptionIf("create-pch", "on", OptionType.cc_Options, "/Fp\"%pchfile%\"");
				AddOptionIf("use-pch", "on", OptionType.cc_Options, "/Yu\"%pchheaderfile%\"");
				AddOptionIf("use-pch", "on", OptionType.cc_Options, "/Fp\"%pchfile%\"");
				//In case precompiled header is defined directly through cc options
				if (ContainsOption(OptionType.cc_Options, "/Yc") || ContainsOption(OptionType.cc_Options, "-Yc"))
				{
					// Need to set this flag so that nant build can schedule the file with this flag to be built first.
					ConfigOptionSet.Options["create-pch"] = "on";
				}
				else if (ContainsOption(OptionType.cc_Options, "/Yu") || ContainsOption(OptionType.cc_Options, "-Yu"))
				{
					// Need to make sure this is set so that module set data will setup the PrecompiledFile variable.
					ConfigOptionSet.Options["use-pch"] = "on";
				}
			}
		}

		protected void Common_VC_Warning_Options()
		{
			AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-WX"); // Treat warnings as errors 
			AddOptionIf("warningsaserrors", "off", OptionType.cc_Options, "-WX-"); // Disable Treat warnings as errors 

			if (FlagEquals("warnings", "on"))
			{
				PrependOption(OptionType.link_Options, "-ignore:4089");  //Disable warning: dll discarded by OPT:ref option

				if (DefaultWarningSuppression == WarningSuppressionModes.On)
				{
					if (FlagEquals("warninglevel", "pedantic"))
					{
						// pendantic maps to /Wall but /Wall turns some warnings on that we never want 
						// references for /Wall warnings (below 5000): https://msdn.microsoft.com/en-us/library/23k5d385.aspx

						PrependOption(OptionType.cc_Options, "-wd4061");   //  enumerator 'abc' in switch of enum 'xyz' is not explicitly handled by a case label
						PrependOption(OptionType.cc_Options, "-wd4062");   //  enumerator 'abc' in switch of enum 'xyz' is not handled				
						PrependOption(OptionType.cc_Options, "-wd4371");   //  "layout of class may have changed from a previous version of the compiler due to better packing of member", this warning can't be fixed by changes in code
						PrependOption(OptionType.cc_Options, "-wd4514");   //  unreferenced inline function removed			
						PrependOption(OptionType.cc_Options, "-wd4571");   //  Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught					
						PrependOption(OptionType.cc_Options, "-wd4619");   //  #pragma warning : there is no warning number '####'			
						PrependOption(OptionType.cc_Options, "-wd4640");   //  construction of local static object is not thread safe				
						PrependOption(OptionType.cc_Options, "-wd4668");   //  use of undefined preprocessor macro (default value is zero)				
						PrependOption(OptionType.cc_Options, "-wd4692");   //  'function': signature of non-private member contains assembly private native type 'native_type'
						PrependOption(OptionType.cc_Options, "-wd4710");   //  inline function not inlined (compiler's discretion)			
						PrependOption(OptionType.cc_Options, "-wd4738");   //  storing 32-bit float result in memory, possible loss of performance				
						PrependOption(OptionType.cc_Options, "-wd4820");   //  padding at end of structure by compiler			
						PrependOption(OptionType.cc_Options, "-wd4826");   //  Conversion from 'char *' to 'uint64_t' is sign-extended.			
						PrependOption(OptionType.cc_Options, "-wd4987");   //  nonstandard extension used: 'throw (...)'
					}
					else if (FlagEquals("warninglevel", "high"))
					{
						// medium and high both map to /W4, however there's a few warnings we want on by default from /Wall, but ONLY if warning level is "high"

						// syntax is -w4XXXX, XXXX is promoted to -w4 level. This is a little confusing in MS terminology since these warnings actually do have levels at or lower than w4 but are defaulted
						// to off and only turned on in wAll, promoting them to 4 changes their 'level' to 4 but also enables them which is what we want

						EnabledDefaultOffVCWarningUnlessExplicitlyDisabled("4242", warninglevel: '4');   // 'identifier': conversion from 'type1' to 'type2', possible loss of data
						EnabledDefaultOffVCWarningUnlessExplicitlyDisabled("4254", warninglevel: '4');   // 'operator': conversion from 'type1' to 'type2', possible loss of data
						EnabledDefaultOffVCWarningUnlessExplicitlyDisabled("4265", warninglevel: '4');   // 'class': class has virtual functions, but destructor is not virtual
						EnabledDefaultOffVCWarningUnlessExplicitlyDisabled("4287", warninglevel: '4');   // 'operator': unsigned/negative constant mismatch
						EnabledDefaultOffVCWarningUnlessExplicitlyDisabled("4296", warninglevel: '4');   // 'operator': expression is always false
						EnabledDefaultOffVCWarningUnlessExplicitlyDisabled("4312", warninglevel: '4');   // 'operation' : conversion from 'type1' to 'type2' of greater size
						EnabledDefaultOffVCWarningUnlessExplicitlyDisabled("4555", warninglevel: '4');   // expression has no effect; expected expression with side-effect
						EnabledDefaultOffVCWarningUnlessExplicitlyDisabled("4946", warninglevel: '4');   // reinterpret_cast used between related classes: 'class1' and 'class2'

						// potential candiates for "useful" warnings that we traditionally disable even at /Wall level, could consider adding these in a future release

						// EnabledDefaultOffVCWarningUnlessExplicitlyDisabled("4365", warninglevel: '4');   // 'action': conversion from 'type_1' to 'type_2', signed/unsigned mismatch (NOTE: probably never want this on capilano due to xdk header issues)
						// EnabledDefaultOffVCWarningUnlessExplicitlyDisabled("4571", warninglevel: '4');   // Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught	
						// EnabledDefaultOffVCWarningUnlessExplicitlyDisabled("4574", warninglevel: '4');   // identifier' is defined to be '0': did you mean to use '#if identifier'?
						// EnabledDefaultOffVCWarningUnlessExplicitlyDisabled("4668", warninglevel: '4');   // 'symbol' is not defined as a preprocessor macro, replacing with '0' for 'directives'
					}

					// default disables from /W4 and below
					PrependOption(OptionType.cc_Options, "-wd4100");   //  unreferenced formal parameter -->
					PrependOption(OptionType.cc_Options, "-wd4127");   //  conditional expression is constant
					PrependOption(OptionType.cc_Options, "-wd4251");   // 'xxx': struct 'yyy' needs to have dll-interface to be used by clients of class 'zzz'
					PrependOption(OptionType.cc_Options, "-wd4275");   //  non dll-interface class 'xxx' used as base for dll-interface class 'yyy'
					PrependOption(OptionType.cc_Options, "-wd4324");   //  padding due to alignment statement -->
					PrependOption(OptionType.cc_Options, "-wd4350");   //   behavior change.  "warning is "Microsoft's fault" and we have no choice but to disable the warning somehow. We may want to disable that warning (it's a -wall warning anyway) in eaconfig, as it has no benefit for us"
					PrependOption(OptionType.cc_Options, "-wd4464");   //  relative include path contains '..'
					PrependOption(OptionType.cc_Options, "-wd4482");   //  nonstandard extension used: enum used in qualified name
					PrependOption(OptionType.cc_Options, "-wd4577");   //  'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed
					PrependOption(OptionType.cc_Options, "-wd4711");   //  function selected for automatic inline expansion -->
					PrependOption(OptionType.cc_Options, "-wd4746");   //   volatile access order isn't guaranteed.
					PrependOption(OptionType.cc_Options, "-wd4986");   //  'operator new[]': exception specification does not match previous declaration
					PrependOption(OptionType.cc_Options, "-wd4987");   //  nonstandard extension used: 'throw (...)'

					//Disable warning: an ANSI source file is compiled on a system with a codepage that
					//cannot represent all characters in the file.    This is a workaround for a long-standing
					//IncrediBuild bug.   Their virtual environment doesn't capture the machine's codebase,
					//so if source files with special characters in their comments are distributed to
					//machines with Chinese or Japanese code-pages, then the distributed build can fail.
					//This warning is exceedingly unlikely to represent a real issue, so we're disabling
					//it to avoid the risk of hitting the IncrediBuild bug. -->
					PrependOption(OptionType.cc_Options, "-wd4819");

					AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig." + ConfigPlatform + ".warningsuppression", ""));
				}
				else if (DefaultWarningSuppression == WarningSuppressionModes.Strict)
				{
					PrependOption(OptionType.cc_Options, "-wd4514");   //  Disable warning: unreferenced inline function removed -->
					PrependOption(OptionType.cc_Options, "-wd4371");   //  VS2013 Disable warning: "layout of class may have changed from a previous version of the compiler due to better packing of member", this warning can't be fixed by changes in code
					PrependOptionIf("optimization", "off", OptionType.cc_Options, "-wd4710");   //  This warning only makes sense when inlining is enabled 
				}
				else if (DefaultWarningSuppression == WarningSuppressionModes.Custom)
				{
					AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig." + ConfigPlatform + ".warningsuppression", ""));
				}

				if (FlagEquals("warninglevel", "medium") || FlagEquals("warninglevel", "high"))
				{
					// a slightly lower warning level
					PrependOption(OptionType.cc_Options, "-W4");
				}
				else if (FlagEquals("warninglevel", "pedantic"))
				{
					// turn on all warnings
					PrependOption(OptionType.cc_Options, "-Wall");
				}
			}
			else if (FlagEquals("warnings", "off"))
			{
				PrependOption(OptionType.cc_Options, "-W0"); // Disable warnings
			}
		}

		protected void SetCommonOptions()
		{
			Common_VC_PCH_Options();
			// Enable BuildTiming Data
			AddOptionIf("buildtiming", "on", OptionType.cc_Options, "-Bt+");
			AddOptionIf("buildtiming", "on", OptionType.lib_Options, "-time+");
			AddOptionIf("buildtiming", "on", OptionType.link_Options, "-time+");

			AddOptionIf("runtimeerrorchecking", "on", OptionType.cc_Options, "-RTC1"); // Generate run-time error checking code

			AddOptionIf("debugsymbols", "on", OptionType.cc_Options, string.Format("-Fd\"{0}\"", GetCompilerPdbTemplate()));

			if (FlagEquals("debugsymbols", "on"))
			{
				if (FlagEquals("c7debugsymbols", "on"))
				{
					AddOption(OptionType.cc_Options, "-Z7"); // Generate C7-style embedded debug information
				}
				else
				{
					if (FlagEquals("managedcpp", "off"))
					{
						if (FlagEquals("optimization", "on"))
						{
							if (FlagEquals("editandcontinue", "on"))
							{
								Log.Warning.WriteLine("[GenerateBuildOptionset] Optionset '{0}': -O2 optimization option is incompatible with edit-and-continue -ZI option. Edit-and-continue will be ignored.", ConfigSetName);
							}

							AddOption(OptionType.cc_Options, "-Zi");
						}
						else
						{
							if (FlagEquals("editandcontinue", "on"))
							{
								if (ContainsOption(OptionType.cc_Options, "-Zi"))
								{
									Log.Warning.WriteLine("[GenerateBuildOptionset] Optionset '{0}': -Zi option is already present, it is incompatible with edit-and-continue -ZI option. Edit-and-continue will be ignored", ConfigSetName);
								}
								else
								{
									AddOption(OptionType.cc_Options, "-ZI");
								}
							}
							AddOptionIf("editandcontinue", "off", OptionType.cc_Options, "-Zi");
						}
					}
					else
					{
						AddOption(OptionType.cc_Options, "-Zi");
					}
				}
			}
			else
			{
				// Add warning to help detecting inconsistency
				if (Log.WarningLevel >= Log.WarnLevel.Advise && FlagEquals("c7debugsymbols", "on"))
				{
					Log.Warning.WriteLine("[GenerateBuildOptionset] Optionset '{0}': c7debugsymbols='on', but debugsymbols is 'off', no -Z7 option will be set, no debug symbols generated.", ConfigSetName);
				}
			}

			if (FlagEquals("clanguage-strict", "on"))
			{
				AddOptionIf("clanguage", "on", OptionType.cc_Options, "-TC");
			}
			if (FlagEquals("cpplanguage-strict", "on"))
			{
				AddOptionIf("clanguage", "off", OptionType.cc_Options, "-TP");
			}

			if (FlagEquals("exceptions", "on"))
			{
				// A sanity test to make sure user didn't already try to create their custom config option set and applied their
				// own switches.
				if (!ContainsOption(OptionType.cc_Options, "EHa") && !ContainsOption(OptionType.cc_Options, "EHsc")
					&& !ContainsOption(OptionType.cc_Options, "EHr") && !ContainsOption(OptionType.cc_Options, "EHs"))
				{
					//In VS 2005, using /clr (Common Language Runtime Compilation) implies /EHa
					//(/clr /EHa is redundant). The compiler will generate an error if /EHs[c]
					//is used after /clr.
					//See http://msdn2.microsoft.com/en-us/library/1deeycx5.aspx for more
					//details on the various Visual C++ exception handling modes.
					if (FlagEquals("toolconfig", "on") || ContainsOption(OptionType.cc_Options, "clr"))
					{
						AddOption(OptionType.cc_Options, "-EHa");
					}
					else
					{
						AddOption(OptionType.cc_Options, "-EHsc");
					}
				}

				AddOption(OptionType.cc_CompilerInternalDefines, "_CPPUNWIND");
			}
			// test for excetions is 'off' even in 'else case' to allow user setting exceptions to custom for their own settings.
			else if (FlagEquals("exceptions", "off"))
			{
				// _HAS_EXCEPTIONS=0 is required for vs2010 and later, otherwise system headers
				// such as xlocale will throw out a bunch of warnings.
				AddOption(OptionType.cc_Defines, "_HAS_EXCEPTIONS=0");
			}

			// RTTI
			if (FlagEquals("rtti", "on"))
			{
				AddOption(OptionType.cc_Options, "-GR");
				AddOption(OptionType.cc_CompilerInternalDefines, "_CPPRTTI");
			}
			else if (!ContainsOption(OptionType.cc_Options, "clr")) // disable RTTI unless clr, can't specify -GR- with clr flag
			{
				AddOption(OptionType.cc_Options, "-GR-");
			}

			AddOptionIf("iteratorboundschecking", "off", OptionType.cc_Defines, "_ITERATOR_DEBUG_LEVEL=0");
			AddOptionIf("iteratorboundschecking", "on", OptionType.cc_Defines, "_ITERATOR_DEBUG_LEVEL=2");

			AddOptionIf("optimization", "off", OptionType.cc_Options, "-Od");

			if (FlagEquals("debugsymbols", "on"))
			{
				if (FlagEquals("debugfastlink", "on") && FlagEquals("managedcpp", "off"))
				{
					//if (FlagEquals("profile","on"))
					//{
					//	// It has been reported that if /PROFILE switch is being used it will cause -debug:FASTLINK not to work and revert back to -debug.
					//	// However, looks like eaconfig doesn't actually set the /PROFILE switch when "profile" option is turned on.  So the following
					//	// comment is commented out for now.  We keep this commented code for now so that people know about this incompatibility.
					//	Log.Warning.WriteLine("The 'debugfastlink' option may not have effect when 'profile' is turned on!");
					//}
					AddOption(OptionType.link_Options, "-debug:FASTLINK");
				}
				else
				{
					// Visual Studio 2015 uses /debug to generate normal non-fastlink pdbs.In Visual Studio 2017 and beyond, /debug will generate fastlink pdbs if building a debug build but will generate full pdbs when building release build.  We don't want this variability so we force DebugFull setting if people are using /debug in Visual Studio 2017.
					if (Properties["config-vs-version"].StrCompareVersions("15.0") >= 0)
						AddOption(OptionType.link_Options, "-debug:full");
					else
						AddOption(OptionType.link_Options, "-debug");
				}
				AddOption(OptionType.link_Options, "-PDB:\"%linkoutputpdbname%\"");  //Need to explicitly set output location - otherwise the default PDB location for VCPROJ builds is to the 'build' dir, not 'bin' dir.
				AddOptionIf("managedcpp", "on", OptionType.link_Options, "-ASSEMBLYDEBUG");  //Debuggable Assembly: Runtime tracking and disable optimizations 
			}

			// ----- GENERAL OPTIONS ----- 
			AddOptionIf("pcconsole", "on", OptionType.link_Options, "-subsystem:console");
			AddOptionIf("pcconsole", "off", OptionType.link_Options, "-subsystem:windows");
			// This prints timing out for each stage of the linker. Which given horrible link times on xb1, this can be helpful
			// in optimizing.
			AddOptionIf("print_link_timings", "on", OptionType.link_Options, "-time");
			AddOptionIf("print_link_timings", "on", OptionType.link_Options, "-verbose:incr");

			var useMpThreads = ConfigOptionSet.Options["multiprocessorcompilation"] ?? Properties["eaconfig.build-MP"];

			if (useMpThreads != null)
			{
				useMpThreads = useMpThreads.TrimWhiteSpace();

				if (String.IsNullOrEmpty(useMpThreads) || (useMpThreads.IsOptionBoolean() && useMpThreads.OptionToBoolean()))
				{
					AddOption(OptionType.cc_Options, "-MP");
				}
				else if (!useMpThreads.IsOptionBoolean())
				{
					int threads;
					if (!Int32.TryParse(useMpThreads, out threads))
					{
						Log.Warning.WriteLine("eaconfig.build-MP='{0}' value is invalid, it can be empty or contain integer number of threads to use with /MP option : /MP${{eaconfig.build-MP}}. Adding '/MP' option with no value.", useMpThreads);

						useMpThreads = String.Empty;
					}
					AddOption(OptionType.cc_Options, "-MP" + useMpThreads);
				}
			}

			if (FlagEquals("misc", "on"))
			{
				AddOption(OptionType.cc_Options, "-c");             // Compile only
				AddOption(OptionType.cc_Options, "-Zc:forScope");   // Enforce Standard C++ for scoping
			}

			// Adding features added to VS2017 
			if (Project.Properties["config-vs-version"].StrCompareVersions("15.0") >= 0)
			{
				// changes how error and warning information is displayed
				string msvcDiagnosticsProp = Project.Properties["eaconfig.msvc.diagnostics"];
				if (!String.IsNullOrEmpty(msvcDiagnosticsProp))
				{
					AddOption(OptionType.cc_Options, "/diagnostics:" + msvcDiagnosticsProp);
				}
				else if (ConfigOptionSet.Options.Contains("msvc.diagnostics"))
				{
					AddOption(OptionType.cc_Options, "/diagnostics:" + ConfigOptionSet.Options["msvc.diagnostics"]);
				}
				else
				{
					// if no user setting, default to the latest supported language specification.
                    if (Project.Properties["config-vs-version"].StrCompareVersions("16.0") >= 0)
                        AddOption(OptionType.cc_Options, "/diagnostics:column");
                    else
                        AddOption(OptionType.cc_Options, "/diagnostics:classic");
                }

				// Enable MSVC permissive mode, which removes Microsoft specific extensions that violate the c++ standard.
				AddOptionIf("msvc.permissive", "on", OptionType.cc_Options, "/permissive");
				AddOptionIf("msvc.permissive", "off", OptionType.cc_Options, "/permissive-");
				
				// MSVC char8_t mode new type in MSVC C++latest standard.
				AddOptionIf("msvc.char8_t", "on", OptionType.cc_Options, "/Zc:char8_t");
				AddOptionIf("msvc.char8_t", "off", OptionType.cc_Options, "/Zc:char8_t-");

				Common_CppLanguage_Version_Options(vcCompiler: true);

				// Enable /Zf (Faster PDB generation) in VS 15.1 and beyond this flag was exposed to optimize toolchain RPC communication with mspdbsrv.exe
				// See https://docs.microsoft.com/en-us/cpp/build/reference/zf 
				if (FlagEquals("fastpdbgeneration", "on"))
				{
					if (ContainsOption(OptionType.cc_Options, "-Zi") || ContainsOption(OptionType.cc_Options, "-ZI"))
					{
						AddOption(OptionType.cc_Options, "/Zf");
					}
				}
			}

			// ----- LINK OPTIONS ----- 
			if (FlagEquals("usedebuglibs", "off"))
			{
				AddOptionIf("multithreadeddynamiclib", "off", OptionType.cc_Options, "-MT");  // link with multithreaded library  
				AddOptionIf("multithreadeddynamiclib", "on", OptionType.cc_Options, "-MD");   // link with multithreaded dll library
			}
			else
			{
				AddOptionIf("multithreadeddynamiclib", "off", OptionType.cc_Options, "-MTd");  // link with multithreaded library  (debug version)
				AddOptionIf("multithreadeddynamiclib", "on", OptionType.cc_Options, "-MDd");   // link with multithreaded dll library (debug version)
			}

			// ----- OUTPUT OPTIONS ----- 
			AddOptionIf("banner", "off", OptionType.cc_Options, "-nologo");    // Suppress copyright message
			AddOptionIf("banner", "off", OptionType.link_Options, "-nologo");  // Suppress copyright message
			AddOptionIf("banner", "off", OptionType.lib_Options, "-nologo");  // Suppress copyright message
			AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-map:\"%linkoutputmapname%\"");
			AddOptionIf("generatedll", "on", OptionType.link_Options, "-dll");
			AddOption(OptionType.link_Options, "-out:\"%linkoutputname%\"");
			AddOption(OptionType.lib_Options, "-out:\"%outputdir%/" + Properties["lib-prefix"] + "%outputname%" + Properties["lib-suffix"] + "\"");

			// ----- WARNING OPTIONS ----- 
			Common_VC_Warning_Options();

			AddOptionIf("incrementallinking", "off", OptionType.link_Options, "-incremental:no ");  // Disable incremental linking
			AddOptionIf("incrementallinking", "on", OptionType.link_Options, "-incremental");       // Enable incremental linking


			if (FlagEquals("optimization", "on"))
			{

                // Since Visual Studio 2015 Update 3 Microsoft introduced a new optimizer that
                // makes aggressive choices in the presence of perceived undefined behavior. 
                // Some of these choices are correct, others are not. The incorrect choices are 
                // causing unexpected code generation failures.
                //
                // To mitigate, this new optimizer is disabled. 
                //
                // In the future we may wish to re-enable it either unconditionally
                // or perhaps guarded with some eaconfig.unsafeoptimizations feature 
                // flag.
                // -mburke 2016-08-11
                // -bmay 2019-02-28 -> Verified this is still required and our code breaks with this optmizer enabled.
                AddOption(OptionType.cc_Options, "-d2SSAOptimizer-");


				AddOption(OptionType.cc_Options, "-O2");// Maximize speed (equivalent to /Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy) 
				AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-Zo");

				if (Properties["eaconfig.disable_framepointer_optimization"] != null)
				{
					AddOption(OptionType.cc_Options, "-Oy-");// Disable runtime buffer security checking
				}

                if (ConfigOptionSet.Options["buildset.name"] != "ManagedCppAssembly")
                {
					AddOptionIf("disable_reference_optimization", "off", OptionType.link_Options, "-opt:ref"); // eliminates functions and/or data that are never referenced
					AddOptionIf("disable_reference_optimization", "on", OptionType.link_Options, "-opt:noref");

					// Allow Explicit disabling of comdat folding. 
					// This can save in link time at a cost of executable size + runtime memory.
					// Comdat folding is finding functions and constants with duplicate values and merging (folding) them. 
					// As a side effect builds with "-OPT:noicf" have improved debugging in common functions because it can be
					// unsettling to step into a different function in a different module while debugging (even if that function 
					// generates the same executable code)
					AddOptionIf("disable_comdat_folding", "on", OptionType.link_Options, "-opt:noicf");
					AddOptionIf("disable_comdat_folding", "off", OptionType.link_Options, "-opt:icf");
                }
            }

			// link time code generation:
			if (FlagEquals("optimization.ltcg", "on"))
			{
				AddOption(OptionType.cc_Options, "-GL");        //enable link-time code generation
				AddOption(OptionType.lib_Options, "-LTCG");     //enable link-time code generation
				ReplaceOption(OptionType.cc_Options, "-Zc:inline", String.Empty);

				string mode = Properties["eaconfig.optimization.ltcg.mode"] ?? ConfigOptionSet.Options["optimization.ltcg.mode"];
				string fileName = Properties["eaconfig.optimization.ltcg.fileName"] ?? ConfigOptionSet.Options["optimization.ltcg.fileName"];
				string pgdoption = String.IsNullOrEmpty(fileName) ? "" : (":PGD=" + fileName);

				switch (mode)
				{
					// Reference info on linker switch for Link-time Code Generation:
					// https://docs.microsoft.com/en-us/cpp/build/reference/ltcg-link-time-code-generation?view=vs-2017
					case "incremental":
						AddOption(OptionType.link_Options, "-LTCG:INCREMENTAL");
						break;
					case "nostatus":
						AddOption(OptionType.link_Options, "-LTCG:NOSTATUS");
						break;
					case "status":
						AddOption(OptionType.link_Options, "-LTCG:STATUS");
						break;
					case "off":
						AddOption(OptionType.link_Options, "-LTCG:OFF");
						break;

					// The followings are specific to Profile Guided Optimization
					// https://docs.microsoft.com/en-us/cpp/build/reference/profile-guided-optimizations
					// The old /LTCG:PG* options are deprecated.  The followings uses the new syntax for VS 2015 and up
					case "instrument":
						AddOption(OptionType.link_Options, "-LTCG");
						AddOptionIf("generatedll", "off", OptionType.link_Options, "-GENPROFILE" + pgdoption);
						break;
					case "fastinstrument":
						AddOption(OptionType.link_Options, "-LTCG");
						AddOptionIf("generatedll", "off", OptionType.link_Options, "-FASTGENPROFILE" + pgdoption);
						break;
					case "useprofile":
						AddOption(OptionType.link_Options, "-LTCG");
						AddOptionIf("generatedll", "off", OptionType.link_Options, "-USEPROFILE" + pgdoption);
						break;

					default:
						// Default will be no profile guided optimization and no special options.
						AddOption(OptionType.link_Options, "-LTCG");
						break;
				}
			}

			if (FlagEquals("clanguage", "off"))
			{
				AddOptionIf("windowsruntime", "on", OptionType.cc_Options, "-ZW"); // Enable Windows Runtime extensions
			}
		}

		protected override void SetPlatformOptions()
		{				
			AddOptionIf("runtimeerrorchecking", "on", OptionType.cc_CompilerInternalDefines, "__MSVC_RUNTIME_CHECKS");

			// ----- LINK OPTIONS ----- 
			if (ContainsOption(OptionType.cc_Options, "clr"))
			{
				// force all clr modules to use MDd, since MTd is incompatible with clr, VS2010 and earlier would correct this automatically but VS2012 and later fail
				if (FlagEquals("multithreadeddynamiclib", "off"))
				{
					Log.Warning.WriteLine("multithreadeddynamiclib is set to off on module '{0}', but clr is set to on. These two settings are incompatible, switching multithreadeddynamiclib to on", ConfigOptionSet.Options["name"]);
					ConfigOptionSet.Options["multithreadeddynamiclib"] = "on";
				}
			}
			SetCommonOptions();

			if (FlagEquals("usedebuglibs", "off"))
			{
				AddOption(OptionType.cc_CompilerInternalDefines, "_MT");

				if (FlagEquals("managedcpp", "on"))
				{
					AddOptionIf("multithreadeddynamiclib", "on", OptionType.link_Options, "-nodefaultlib:libcmt.lib");   // In case MCPP is linked with static lib that uses -MTd
				}
			}
			else
			{
				AddOption(OptionType.cc_CompilerInternalDefines, "_MT");

				if (FlagEquals("managedcpp", "on"))
				{
					AddOptionIf("multithreadeddynamiclib", "on", OptionType.link_Options, "-nodefaultlib:libcmtd.lib");   // In case MCPP is linked with static lib that uses -MTd
				}
			}

			if (FlagEquals("multithreadeddynamiclib", "on"))
			{
				AddOption(OptionType.cc_CompilerInternalDefines, "_DLL");
			}

			if (FlagEquals("clanguage", "off"))
			{
				// Enable Windows Runtime extensions
				if (FlagEquals("windowsruntime", "on"))
				{
					AddOption(OptionType.cc_CompilerInternalDefines, "__cplusplus_winrt=201009L");
				}

				if (FlagEquals("managedcpp", "on"))
				{
					AddOption(OptionType.cc_CompilerInternalDefines, "__cplusplus_cli=200406L");
					AddOption(OptionType.cc_CompilerInternalDefines, "_M_CEE");
					AddOption(OptionType.cc_CompilerInternalDefines, "_MANAGED=1");
				}

				// MSVC up through 2015 defines __cplusplus to be 199711L
				AddOption(OptionType.cc_CompilerInternalDefines, "__cplusplus=199711L");
			}

			AddOption(OptionType.cc_CompilerInternalDefines, "_WIN32");

			string processor = Properties["config-processor"];
			switch (processor)
			{
				case "x64":
					AddOption(OptionType.cc_CompilerInternalDefines, "_M_AMD64");
					AddOption(OptionType.cc_CompilerInternalDefines, "_WIN64");
					break;
				case "x86":
					AddOption(OptionType.cc_CompilerInternalDefines, "_M_IX86");
					break;
				case "arm":
					AddOption(OptionType.cc_CompilerInternalDefines, "_M_ARM");
					break;
				case "arm64":
					AddOption(OptionType.cc_CompilerInternalDefines, "_M_ARM64");
					break;
				default:
					throw new Exception(string.Format("Unknown processor type '{0}', please update eaconfig's vc-common-options.cs with required defines.", processor));
			}

			string vsVersion = Properties["config-vs-version"];
			string vsMajorVersion = vsVersion.Substring(0, vsVersion.IndexOf("."));
			switch (vsMajorVersion)
			{
				case "14":
					AddOption(OptionType.cc_CompilerInternalDefines, "_MSC_VER=1900");
					break;
				case "15":
					AddOption(OptionType.cc_CompilerInternalDefines, "_MSC_VER=1910");
					break;
				case "16":
					AddOption(OptionType.cc_CompilerInternalDefines, "_MSC_VER=1920");
					break;
				default:
					throw new Exception(string.Format("Unknown Visual Studio major version '{0}', please update eaconfig", vsMajorVersion));
			}
		}
	}
}
