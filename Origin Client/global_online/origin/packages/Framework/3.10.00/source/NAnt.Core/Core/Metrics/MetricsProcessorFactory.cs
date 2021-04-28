using System;
using System.IO;
using System.Reflection;
using NAnt.Core.Logging;

namespace NAnt.Core.Metrics
{
    internal class MetricsProcessorFactory
    {
        private static System.Reflection.Assembly _metricsAssembly = GetMetricsAssembly();

        private static string metricsAssemblyName = "";

        internal static IMetricsProcessor Create(bool verbose)
        {
            IMetricsProcessor metricsProcessor = null;

            try
            {

                System.Reflection.Assembly metricsAssembly = GetMetricsAssembly();

                if (metricsAssembly != null)
                {
                    metricsProcessor = metricsAssembly.CreateInstance("EA.MetricsProcessor.DefaultMetricsProcessor") as IMetricsProcessor;

                    if (metricsProcessor == null)
                    {
                        //_project.Log.Debug.Write("Hello");//.WriteLineIf(verbose, "Failed to create MetricsProcessor instance from assembly {0}", metricsAssembly);
                    }

                }
            }
            catch (Exception /*ex*/)
            {
                // Log.WriteLineIf(verbose, "Error in MetricsProcessorFactory.Create(): {0}", ex.Message);
            }

            return metricsProcessor;
        }

        private static System.Reflection.Assembly GetMetricsAssembly()
        {
            if (_metricsAssembly == null)
            {
                //metricsAssemblyName = "EA.MetricsProcessor.dll";
                metricsAssemblyName = "DefaultMetricsProcessor.dll";

                if (!String.IsNullOrEmpty(metricsAssemblyName))
                {
                    string path = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                    path = Path.Combine(path, metricsAssemblyName);
                    _metricsAssembly = System.Reflection.Assembly.LoadFrom(path);
                }
            }

            return _metricsAssembly;
        }
    }
}
