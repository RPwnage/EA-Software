// NAnt - A .NET build tool
// Copyright (C) 2001 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
//
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)

using System;
using System.Text;
using System.Text.RegularExpressions;
using System.Linq;
using System.Diagnostics;
using System.Collections.Generic;
using System.Runtime.Serialization;

namespace NAnt.Core
{

    /// <summary>
    /// Thrown whenever an error occurs during the build.
    /// </summary>
    [Serializable]
    public class BuildException : ApplicationException
    {
        public enum StackTraceType { None, XmlOnly, Full };

        private Location _location = Location.UnknownLocation;
        private string _type = String.Empty;

        public readonly StackTraceType StackTraceFlags;

        public static string FormatStackTrace(Exception ex, int depth = 1)
        {
            if (ex == null)
            {
                return String.Empty;
            }
            var trace = new StackTrace(ex, true);
            var sb = new StringBuilder();

            if (trace.FrameCount > 0)
            {
                int maxCount = Math.Min(trace.FrameCount, depth > 0 ? depth : trace.FrameCount);

                for (int i = 0; i < maxCount; i++)
                {
                    var frame = trace.GetFrame(i);
                    sb.AppendFormat("{0} ({1}, {2}) Method: '{3}'{4}", frame.GetFileName(), frame.GetFileLineNumber(), frame.GetFileColumnNumber(), frame.GetMethod().Name, i < maxCount - 1 ? Environment.NewLine : String.Empty);
                }
            }
            else
            {
                sb.Append(ex.StackTrace);
            }
            return sb.ToString();
        }


        public static BuildException.StackTraceType GetBaseStackTraceType(Exception e)
        {
            BuildException.StackTraceType type = BuildException.StackTraceType.Full;

            while (e != null)
            {
                if (e is BuildException)
                {
                    type = ((BuildException)e).StackTraceFlags;

                }
                else
                {
                }
                e = e.InnerException;
            }
            return type;

        }



        /// <summary>
        /// Constructs a build exception with no descriptive information.
        /// </summary>
        public BuildException(StackTraceType stackTrace = StackTraceType.Full)
            : base()
        {
            _type = GetType().ToString();
            StackTraceFlags = stackTrace;
        }

        /// <summary>
        /// Constructs an exception with a descriptive message.
        /// </summary>
        public BuildException(String message, StackTraceType stackTrace = StackTraceType.Full)
            : base(message)
        {
            StackTraceFlags = stackTrace;
        }

        /// <summary>
        /// Constructs an exception with a descriptive message and an
        /// instance of the Exception that is the cause of the current Exception.
        /// </summary>
        public BuildException(String message, Exception e, StackTraceType stackTrace = StackTraceType.Full)
            : base(message, e)
        {
            StackTraceFlags = stackTrace;
        }

        /// <summary>
        /// Constructs an exception with a descriptive message and location
        /// in the build file that caused the exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="location">Location in the build file where the exception occured.</param>
        public BuildException(String message, Location location, StackTraceType stackTrace = StackTraceType.Full)
            : base(message)
        {
            _location = location ?? Location.UnknownLocation;
            StackTraceFlags = stackTrace;
        }

        /// <summary>
        /// Constructs an exception with the given descriptive message, the
        /// location in the build file and an instance of the Exception that
        /// is the cause of the current Exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="location">Location in the build file where the exception occured.</param>
        /// <param name="e">An instance of Exception that is the cause of the current Exception.</param>
        public BuildException(String message, Location location, Exception e, StackTraceType stackTrace = StackTraceType.Full)
            : base(message, e)
        {
            _location = location ?? Location.UnknownLocation;
            StackTraceFlags = stackTrace;
        }

        /// <summary>
        /// Constructs an exception with the given descriptive message, the
        /// location in the build file and an instance of the Exception that
        /// is the cause of the current Exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="location">Location in the build file where the exception occured.</param>
        /// <param name="e">An instance of Exception that is the cause of the current Exception.</param>
        /// <param name="type">Type of the build exception.</param>
        public BuildException(String message, Location location, Exception e, String type, StackTraceType stackTrace = StackTraceType.Full)
            : base(message, e)
        {
            _location = location ?? Location.UnknownLocation;
            _type = type;
            StackTraceFlags = stackTrace;
        }

        /// <summary>
        /// Constructs an exception with the given descriptive message and the
        /// type of the exception thrown
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="type">Type of the build exception.</param>
        public BuildException(String message, String type, StackTraceType stackTrace = StackTraceType.Full)
            : base(message)
        {
            _type = type;
            StackTraceFlags = stackTrace;
        }

        /// <summary>Initializes a new instance of the BuildException class with serialized data.</summary>
        public BuildException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
            /*
            string fileName  = info.GetString("Location.FileName");
            int lineNumber   = info.GetInt32("Location.LineNumber");
            int columnNumber = info.GetInt32("Location.ColumnNumber");
            */
            _location = info.GetValue("Location", _location.GetType()) as Location;
        }

        /// <summary>Sets the SerializationInfo object with information about the exception.</summary>
        /// <param name="info">The object that holds the serialized object data. </param>
        /// <param name="context">The contextual information about the source or destination. </param>
        /// <remarks>For more information, see SerializationInfo in the Microsoft documentation.</remarks>
        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            base.GetObjectData(info, context);
            info.AddValue("Location", _location);
            info.AddValue("Type", _type);
        }

        public Location Location
        {
            get { return _location; }
        }

        public string Type
        {
            get { return _type; }
        }

        public string BaseMessage
        {
            get { return base.Message; }
        }
    }
}
