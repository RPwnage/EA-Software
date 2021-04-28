// Copyright (C) Electronic Arts Canada Inc. 2010.  All rights reserved.

using System;
using System.Net;

//NOTICE(RASHIN):
//This code should not be used unless you want to use
//the web services proxy that doesn't require user authentication to work.

namespace EA.PackageSystem.PackageCore.Services
{

    public class WebServices2URL
    {
        static public string GetWebServices2URL()
        {
            string url = System.Configuration.ConfigurationManager.AppSettings["WebServices2URL"];
            return url;
        }
    }
}
