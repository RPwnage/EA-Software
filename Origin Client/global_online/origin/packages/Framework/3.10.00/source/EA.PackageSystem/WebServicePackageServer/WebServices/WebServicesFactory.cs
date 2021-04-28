// Copyright (C) Electronic Arts Canada Inc. 2010.  All rights reserved.

using System;
using System.Net;
using System.Security.Principal;
using NAnt.Core.Util;

using PackageServer = EA.PackageSystem.PackageCore.Services;

namespace EA.PackageSystem.PackageCore.Services
{

    public static class WebServicesFactory
    {
        static public PackageServer.WebServices Generate()
        {
            ICredentials credentials = null;

            credentials = System.Net.CredentialCache.DefaultCredentials;

            string net_login = Environment.GetEnvironmentVariable("EAT_FRAMEWORK_NET_LOGIN");
            string net_pswd = Environment.GetEnvironmentVariable("EAT_FRAMEWORK_NET_PASSWORD");
            string net_domain = Environment.GetEnvironmentVariable("EAT_FRAMEWORK_NET_DOMAIN");
            if (net_login != null && net_pswd != null && net_domain != null)
            {
                credentials = new NetworkCredential(net_login, net_pswd, net_domain);
            }

            PackageServer.WebServices services = new PackageServer.WebServices();
            string newURL = PackageServer.WebServicesURL.GetWebServicesURL();
            if (newURL != null)
            {
                services.Url = newURL;
            }
            services.Credentials = credentials;
            return services;
        }
    }
}
