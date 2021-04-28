// (c) Electronic Arts Inc.
using System.Collections.Generic;

namespace EA.EAConfig
{    
    public class CommonRun
    {
        public static void PrintErrorMessages(NAnt.Core.Logging.Log log, List<string> errorMessages)
        {
            if (errorMessages.Count > 0)
            {
                log.Minimal.WriteLine("============================================================================");
                log.Minimal.WriteLine();
                log.Minimal.WriteLine("Errors:");
                log.Minimal.WriteLine();
            }
            else
			{
                return;
			}
            for (int i = 0; i < errorMessages.Count; i++)
			{
                string error = errorMessages[i];
                log.Minimal.WriteLine(error);
                log.Minimal.WriteLine();
                if (i < errorMessages.Count - 1)
				{
                log.Minimal.WriteLine("----------------------------------------------------------------------------");
                }
            }
            if (errorMessages.Count > 0)
            {
                log.Minimal.WriteLine("============================================================================");
            }            
        }
	}
}
