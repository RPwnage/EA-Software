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
    /// Proxy exception used to preserve stack trace of orinal BuildException. This exception should be transparent in the output.
    /// </summary>
    [Serializable]
    public class BuildExceptionStackTraceSaver : BuildException
    {
        public BuildExceptionStackTraceSaver(BuildException e)
            : base(String.Empty, e)
        {
        }
    }

    /// <summary>
    /// Proxy exception used to preserve stack trace of orinal Syatem.Exception. This exception should be transparent in the output.
    /// </summary>
    [Serializable]
    public class ExceptionStackTraceSaver : System.Exception
    {
        public ExceptionStackTraceSaver(System.Exception e)
            : base(String.Empty, e)
        {
        }
    }

}
