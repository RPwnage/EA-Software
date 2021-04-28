// Copyright (C) Electronic Arts Canada Inc. 2003.  All rights reserved.

using System;
using System.Collections;
using System.IO;
using System.Net;
using System.Security.Principal;

using NAnt.Core;

namespace EA.PackageSystem.PackageCore
{
    public class PackagePoster
    {
        public void Post(string path)
        {
            string fileName = Path.GetFileName(path);
            // upload to server
            string dstPath = Path.Combine(@"\\eac-as5.eac.ad.ea.com\incoming", Path.GetFileName(path));
            File.Copy(path, dstPath);

            try
            {
                WindowsPrincipal wp = new WindowsPrincipal(WindowsIdentity.GetCurrent());
                string uncName = wp.Identity.Name;

                // call web service
                Services.WebServices services = Services.WebServicesFactory.Generate();
                string errorMessage = services.PostPackage(fileName, uncName, null, 0, null, null);
                if (errorMessage != null)
                {
                    throw new ApplicationException(errorMessage);
                }
            }
            catch
            {
                // There was some error in posting, so let's delete the file in the incoming folder
                File.Delete(dstPath);
                throw;
            }
        }
    }
}
