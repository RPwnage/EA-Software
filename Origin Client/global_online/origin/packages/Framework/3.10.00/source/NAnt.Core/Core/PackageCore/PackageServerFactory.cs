using System;
using System.Linq;
using System.Xml;
using System.Reflection;
using System.Collections.Generic;

using NAnt.Core;
using System.IO;
using NAnt.Core.Util;
using NAnt.Core.Reflection;

namespace NAnt.Core.PackageCore
{
    public static class PackageServerFactory
    {
        public class PackageServerFactoryParameters
        {
            public enum ServerTypes { WebServices, Perforce }

            public ServerTypes ServerType = ServerTypes.WebServices;
        }


        public static IPackageServer CreatePackageServer(PackageServerFactoryParameters parameters = null)
        {
            IPackageServer server = null;

            if (parameters == null)
            {
                parameters = new PackageServerFactoryParameters();
            }

            switch (parameters.ServerType)
            {
                case PackageServerFactoryParameters.ServerTypes.WebServices:
                default:
                    server = LoadFromAssembly("WebServicePackageServer.dll");
                    break;
            }

            return server;
        }

        private static IPackageServer LoadFromAssembly(string assemblyFile)
        {
            try
            {
                var assembly = AssemblyLoader.Get(assemblyFile);

                foreach (Type type in assembly.GetTypes())
                {
                    if (typeof(IPackageServer).IsAssignableFrom(type) && !type.IsAbstract && !type.IsInterface)
                    {
                        return Activator.CreateInstance(type) as IPackageServer;
                    }
                }
            }
            catch (Exception ex)
            {
                string msg = String.Format("Failed to create PackageServerInstance from assembly '{0}', details: {1}", assemblyFile, ex.Message);
                throw new BuildException(msg, ex);
            }
            throw new BuildException(String.Format("Failed to create PackageServerInstance from assembly '{0}', can not find implementation of 'IPackageServer' type", assemblyFile));
        }


    }
}
