using System;
using System.Text;

namespace NAnt.Core.Threading
{
    public static class ThreadUtil
    {
        public static Exception ProcessAggregateException(Exception e)
        {
            Exception res = e;
            if (e is AggregateException)
            {
                AggregateException ae = e as AggregateException;
                res = ae.GetBaseException();
                ae = res as AggregateException;
                if (ae != null)
                {
                    //Try to extract rootcause exception:
                    if (ae.InnerExceptions.Count > 0)
                    {
                        res = ae.InnerExceptions[0];
                        foreach (Exception ex in ae.InnerExceptions)
                        {
                            if (ex is BuildException)
                            {
                                res = ex;
                                break;
                            }
                            else if (ex is ContextCarryingException)
                            {
                                res = ex;
                                break;
                            }
                        }
                    }
                }
            }
            return res;
        }

        public static string GetStackTrace(Exception e)
        {
            var output = new StringBuilder();

            if (e is AggregateException)
            {
                AggregateException ae = e as AggregateException;
                ae = ae.Flatten();
                foreach (var inner in ae.InnerExceptions)
                {
                    if (!(inner is BuildException))
                    {
                        output.AppendLine();
                        output.AppendLine("********************************************************");
                        output.AppendLine("    " + inner.Message);
                        output.AppendLine("--------------------------------------------------------");
                        output.AppendLine(inner.StackTrace);
                        output.AppendLine();
                    }
                }
            }
            else if (!(e is BuildException))
            {
                output.AppendLine();
                output.AppendLine("********************************************************");
                output.AppendLine("    " + e.Message);
                output.AppendLine("--------------------------------------------------------");
                output.AppendLine(e.StackTrace);
                output.AppendLine();

            }
            return output.ToString();
        }


        public static void RethrowThreadingException(String message, Location location, Exception e)
        {
            Exception ex = ProcessAggregateException(e);

            if (typeof(BuildException).IsAssignableFrom(ex.GetType()))
            {
                BuildException be = ex as BuildException;
                if (be.Location == Location.UnknownLocation)
                {
                    be = new BuildException(be.Message, location, be.InnerException);
                }
                throw new BuildExceptionStackTraceSaver(be);
            }

            var new_be = new BuildException(message, location, ex);
            throw new BuildExceptionStackTraceSaver(new_be);

        }

        public static void RethrowThreadingException(String message, Exception e)
        {
            Exception ex = ProcessAggregateException(e);

            if (ex is BuildException)
            {
                throw new BuildExceptionStackTraceSaver(ex as BuildException);
            }

            var new_ex = new Exception(message, ex);
            throw new ExceptionStackTraceSaver(new_ex);
        }

    }
}
