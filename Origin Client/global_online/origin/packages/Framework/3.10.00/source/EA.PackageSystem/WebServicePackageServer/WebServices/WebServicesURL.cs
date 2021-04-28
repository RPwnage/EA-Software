// Copyright (C) Electronic Arts Canada Inc. 2010.  All rights reserved.

using System;
using System.Net;

using PackageServer = EA.PackageSystem.PackageCore.Services;

namespace EA.PackageSystem.PackageCore.Services
{
    public class WebServicesURL
    {
        static public string GetWebServicesURL()
        {
            string url = System.Configuration.ConfigurationManager.AppSettings["WebServicesURL"];
            return url;
        }
    }
}
