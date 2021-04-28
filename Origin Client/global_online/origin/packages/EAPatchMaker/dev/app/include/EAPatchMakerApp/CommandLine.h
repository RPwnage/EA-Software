/////////////////////////////////////////////////////////////////////////////
// CommandLine.h
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
//
// Implements basic command line parsing.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHMAKER_COMMANDLINE_H
#define EAPATCHMAKER_COMMANDLINE_H


#include <EAPatchClient/Base.h>
#include <EAPatchMaker/Config.h>
#include <EASTL/vector.h>
#include <EASTL/core_allocator_adapter.h>
#include <coreallocator/icoreallocator_interface.h>


namespace EA
{
    namespace Patch
    {
        /// CommandLine
        /// 
        /// This class implements basic command line parsing. Most command line processing is very similar
        /// between applications, so we wrap up this functionality here and solve the problems that often
        /// trip up custom command line processing code.
        ///
        /// Design Considerations
        /// To be consistent with the rest of the system this module is a part of, strings are 16 bit.
        /// Some of the member functions take char8_t* as arguments while others take String arguments or 
        /// return string16 references. The reason for this is that the char8_t arguments are arguments that
        /// are input-only and thus the user could use either char8_t* or string16 as sources for the argument.
        /// Arguments of type String are arguments that are written to by this class and thus could be of 
        /// variable size. It would be rather cumbersome to try to make these functions take char8_t* when 
        /// string16 is so much more powerful and simple.
        ///
        /// Notes
        /// Switch symbols in front of arguments can be either '-' or '/'.
        /// Typical command lines parsed by this class:
        ///    -x
        ///    /y
        ///    -x:Test
        ///    -SomeKey:"Some Value"
        ///    -Name:
        ///    blah
        ///    "quoted string"
        ///
        /// Examples
        /// In order to simply iterate through the command line arguments, you would do this:
        ///     for(int i = 0, iEnd = cmdLine.GetArgumentCount(); i < iEnd; ++i)
        ///         cmdLine[i];
        ///
        /// In order to look for the -x in this command line:
        ///     hello -a "some string" -xray -x
        /// You would do this:
        ///     if(cmdLine.FindArgument("-x") >= 0) 
        ///
        /// In order to look for the file name specified in the -File argument here:
        ///     hello -f -File:"Some Path.txt"
        /// You would do this:
        ///     String filePath;
        ///     if(cmdLine.FindSwitch("-File", false, filePath))
        ///         <use filePath>
        ///
        class CommandLine
        {
        public:
            typedef EA::Patch::String String;
            typedef eastl::vector<String, CoreAllocator> StringArray;


            /// enum Index
            /// Returns to an argument index. Defines special index values.
            enum Index
            {
                kIndexUnknown = -1,  /// Used in return values to indicate that the requested index was not found.
                kIndexAppend  = -1   /// Used in arguments to indicate that the index is meant to be one past the end.
            };

            /// CommandLine
            /// Default constructor which creates an empty command line. 
            /// The argument count is zero and the argument array is empty.
            CommandLine(EA::Allocator::ICoreAllocator* pCoreAllocator = NULL);

            /// CommandLine
            /// Constructor for a command line that is passed in as a single string.
            /// Quoting can be used to separate arguments with whitespace in them.
            /// If the argument is NULL then an empty command line is assumed.
            ///
            /// Example usage:
            ///     EACommandLine cmdLine("-x -b -locale:en-us -test:all");
            ///
            CommandLine(const char8_t* pCommandLineString, EA::Allocator::ICoreAllocator* pCoreAllocator = NULL);

            /// CommandLine
            /// Constructor for a command line that is passed in as argc/argv as with
            /// the C/C++ main() function. If the ppArgumentArray argument is NULL, 
            /// then the command line is expanded to nArgumentCount empty entries.
            ///
            /// Note that some platforms don't provide proper argc/argv arguments,
            /// and using CommandLine to read the argc/argv values may result in 
            /// incorrect arguments from what is expected. The AppCommandLine class
            /// resolves this by assuming the command line is an application command
            /// line and fixing it if so. CommandLine is a lean parsing class and
            /// doesn't try to have higher intelligence than that.
            ///
            /// Example usage:
            ///     int wmain(int argc, wchar_t** argv) {
            ///         EACommandLine cmdLine(argc, argv);
            ///         ...
            ///     }
            ///
            ///     int main(int argc, char** argv) {
            ///         EACommandLine cmdLine(argc, argv);
            ///         ...
            ///     }
            ///
            CommandLine(int nArgumentCount, char8_t** ppArgumentArray, EA::Allocator::ICoreAllocator* pCoreAllocator = NULL);

            /// CommandLine
            /// Standard copy constructor.
            CommandLine(const CommandLine& commandLine);

            /// ~CommandLine
            /// This is a basic destructor. It is virtual so that you can create subclasses
            /// of CommandLine and they will destruct properly when destroyed through this 
            /// base class. This class is not performance-driven and so having virtual functions
            /// is not likely to matter significantly.
            virtual ~CommandLine() {}

            /// operator=
            /// Standard assignment operator.
            CommandLine& operator=(const CommandLine& commandLine);

            /// GetArgumentCount
            /// Returns the number of arguments found in the command line.
            int GetArgumentCount() const;

            /// operator[] (const)
            /// Returns the argument at the given index.
            /// The index must be in range or else the behaviour is undefined.
            ///
            /// Example usage:
            ///     const String& s = cmdLine[3];
            const String& operator[](int nIndex) const;

            /// operator[] (non-const)
            /// Returns the argument at the given index.
            /// The index must be in range or else the behaviour is undefined.
            ///
            /// Example usage:
            ///     const String& s = cmdLine[3];
            String& operator[](int nIndex);

            /// GetCommandLineText
            /// Returns the command line as a single string. Quoting will be added to arguments
            /// as necessary to preserve argument boundaries where they have internal whitespace.
            const String& GetCommandLineText() const;

            /// SetCommandLineText
            /// Sets the command line text. This text replaces any existing command line.
            virtual void SetCommandLineText(const char8_t* pCommandLineString);

            /// InsertArgument
            /// Inserts the given argument at the given index. If the index refers to a position
            /// that is outside the bounds of the current array, then the array is resized to 
            /// include the given index. If input nIndex is kIndexAppend, then the argument is appended.
            /// If the index is out of range, the element is appended.
            /// The input pArgument is expected to be null-terminated.
            void InsertArgument(const char8_t* pArgument, int nIndex = kIndexAppend);

            /// EraseArgument
            /// Erases the argument at the given index. If input bResize is true, then the argument 
            /// is removed and the array is resized by -1, else it is cleared and the array size stays constant.
            /// If the index is out of range, nothing is done and the return value is false. 
            /// Otherwise the return value is true.
            bool EraseArgument(int nIndex, bool bResize = true);

            /// FindArgument
            /// This is a generic argument finding function.
            /// It searches all arguments starting with input nStartingIndex for the given argument
            /// indicated by input pArgument. If bFindCompleteString is true, then the found argument
            /// must match input pArgument completely. If bCaseSensitive is true, then the found 
            /// argument must match input pArgument case-sensitively. If input pResult is non-null
            /// the found argument is copied to it. If the the argument is not found, then pResult
            /// is cleared if non-null. The input pArgument is expected to be null-terminated.
            /// 
            /// Returns the index of the switch or kIndexUnknown if not found.
            ///
            /// Example usage:
            ///     int result = cmdLine.FindArgument("-x");
            ///
            int FindArgument(const char8_t* pArgument, bool bFindCompleteString = true, bool bCaseSensitive = false, String* pResult = NULL, int nStartingIndex = 0) const;

            /// GetSwitch
            /// This is a generic multi-purpose switch getting function.
            /// You would use it if you are looping through all arguments and wanted 
            /// to find out which command each one in turn is.
            /// The returned switch string doesn't include the prefix, as it may vary.
            /// Expects switches of the form: -<option>[:<result>] where option is the 
            /// switch and [:<result>] is optional.
            /// The <result> portion can be surrounded by quotes if it has whitespace.
            /// The <result> portion is returned in output pResult if pResult is non-null.
            ///
            /// Examples of switch usage include:
            ///    -x
            ///    /x:Test
            ///    -File:"Some File.txt"
            ///    -Name:
            ///
            /// Example usage:
            ///     String sSwitch, sResult;
            ///     for(int i = 0; i < cmdLine.GetArgumentCount(); ++i){
            ///         if(cmdLine.GetSwitch(i, &sSwitch){
            ///              if(sSwitch == "x")
            ///                  ...
            ///         }
            ///     }
            ///
            bool GetSwitch(int nIndex, String* pSwitch = NULL, String* pResult = NULL) const;

            /// FindSwitch
            /// This is a generic multi-purpose switch finding function.
            /// Expects switches of the form: -<option>[:<result>] where option is the 
            /// switch and [:<result>] is optional.
            /// The <result> portion can be surrounded by quotes if it has whitespace.
            /// The <result> portion is returned in output pResult if pResult is non-null.
            /// If the the argument is not found, then pResult is cleared if non-null.
            /// The switch prefix can begin with either of '-' and '/'.
            /// The input pSwitch is string that refers to the part of the switch
            /// after the prefix. The switch string matching is done by whole string
            /// and not by substring. Thus, searching for switch "x" fails for argument "-xa".
            /// The input bCaseSensitive specifies whether the input switch must 
            /// match in a case-sensitive way.
            /// The input pSwitch is expected to be null-terminated.
            /// The input pSwitch doesn't need to include a prefix (e.g. '-'), but will 
            /// work if one is present.
            ///
            /// Examples include:
            ///    -x
            ///    /x:Test
            ///    -File:"Some File.txt"
            ///    -Name:
            ///
            /// Returns the index of the switch or kIndexUnknown if not found.
            ///
            /// Example usage:
            ///     String s;
            ///     int result = cmdLine.FindSwitch("file", false, &s);
            ///     if(s == "Some File.txt") ...
            ///
            int FindSwitch(const char8_t*  pSwitch, bool bCaseSensitive = false, String* pResult = NULL, int nStartingIndex = 0) const;

            /// EASTLCoreAllocator
            /// To do: Use the typedef in eastl/core_allocator_adapter.h after ~March 2008.
            typedef EA::Allocator::CoreAllocatorAdapter<EA::Allocator::ICoreAllocator> EASTLCoreAllocator;

            /// ConvertStringToStringArray
            /// Takes an input string and parses it to an array of strings based on
            /// whitespace separators in the input string. Quoting in the input string
            /// is respected; surrounding quotes are removed as the string is copied
            /// into the array.
            static void ConvertStringToStringArray(const String& sInput, StringArray& stringArray);

            /// ConvertStringArrayToString
            /// This is the opposite of ConvertStringToStringArray. Quoting is added as
            /// necessary in order to preserve argument boundaries.
            static void ConvertStringArrayToString(const StringArray& stringArray, String& sResult);

        protected:
            StringArray    mStringArray;
            mutable String msCommandLine;
            String         msEmpty;

        }; // class CommandLine




        /// AppCommandLine
        /// 
        /// Implements an application command line that acts the same as 
        /// CommandLine except that it guarantees that the first argument 
        /// is a full path to the currently running application. 
        /// Some platforms (notably Win32) don't guarantee that main's 
        /// argv[0] is a path to the application yet this is something 
        /// that you will want to rely on being consistently present.
        /// Other platforms (notably PS2) screw up the argc/argv arguments
        /// when there are no arguments.
        class AppCommandLine : public CommandLine
        {
        public:
            /// AppCommandLine
            /// Default constructor which creates a command line with only one argument: the application path. 
            AppCommandLine();

            /// AppCommandLine
            /// Constructor for a string of char or for a C++ string via the c_str function.
            AppCommandLine(const char8_t* pCommandLineString);

            /// AppCommandLine
            /// Constructor for argc/argv style construction as with C/C++ main().
            AppCommandLine(int nArgumentCount, char8_t** ppArgumentArray);

            /// AppCommandLine
            /// Standard copy constructor
            AppCommandLine(const AppCommandLine& appCommandLine);

            /// operator=
            AppCommandLine& operator=(const AppCommandLine& appCommandLine);

            /// SetCommandLineText
            /// Sets the command line text. This text replaces any existing command line.
            virtual void SetCommandLineText(const char8_t* pCommandLineString);

        public:
            /// FixCommandLine
            /// This is a static helper function which fixes a given EA::CommandLine to 
            /// act like a proper and consistent application command line.
            static void FixCommandLine(CommandLine& cmdLine, const char8_t* pAppPath = NULL);

        }; // class AppCommandLine

    }
} // namespace EA



#endif // Header include guard






