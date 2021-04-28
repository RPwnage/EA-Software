// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Net;
using System.Xml;
using System.Linq;

using NAnt.Core.Util;

namespace NAnt.Perforce.ConnectionManagment
{
	public class ProxyMap
	{
		public class Location
		{
			public readonly string Name;
			public readonly ReadOnlyCollection<IPUtil.Subnet> Subnets;

			internal Location(string name, IEnumerable<IPUtil.Subnet> subnets)
			{
				Name = name;
				Subnets = new ReadOnlyCollection<IPUtil.Subnet>(new List<IPUtil.Subnet>(subnets));
			}

            public override string ToString()
            {
                return String.Format("{0} ({1})", Name, String.Join(", ", Subnets.Select(sub => String.Format("{0}/{1}", sub.Address, sub.Mask))));
            }
        }

		public class Server
		{
			/// <summary>The address and port of the central perforce server</summary>
			public readonly string Address;

			/// <summary>A human friendly name for the server so users can easily indentify it</summary>
			public readonly string Name;

			public readonly P4Server.P4CharSet CharSet;

			/// <summary>A list of all of the local proxy servers</summary>
			public readonly ReadOnlyDictionary<Location, string> Proxies;

			internal Server(string address, P4Server.P4CharSet charSet, string name = null)
			{
				Address = address;
                CharSet = charSet;
                Proxies = new ReadOnlyDictionary<Location, string>(new Dictionary<Location, string>());
				Name = name;
            }

			internal Server(string address, P4Server.P4CharSet charSet, Dictionary<Location, string> proxies, string name)
			{
				Address = address;
                CharSet = charSet;
                Proxies = new ReadOnlyDictionary<Location, string>(new Dictionary<Location, string>(proxies));
				Name = name;
			}
		}

		private readonly Dictionary<string, Location> s_locations = new Dictionary<string, Location>();
		private readonly Dictionary<string, Server> s_servers = new Dictionary<string, Server>();
		private readonly Dictionary<IPUtil.Subnet, Location> s_subnets = new Dictionary<IPUtil.Subnet, Location>();

		private ProxyMap(XmlDocument document, string fileName)
		{
			// parse document
			XmlNode mapNode = document.GetChildElementByName("map");

			// parse locations			
			XmlNode locationsNode = mapNode.GetChildElementByName("locations");
			foreach (XmlNode locationElement in locationsNode.GetChildElementsByName("location"))
			{
				List<IPUtil.Subnet> subnets = new List<IPUtil.Subnet>();
				string locationName = locationElement.GetAttributeValue("name");
				foreach (XmlNode subnetElement in locationElement.GetChildElementsByName("subnet"))
				{
					IPUtil.Subnet subnet = new IPUtil.Subnet
					(
						IPAddress.Parse(subnetElement.GetAttributeValue("address")),
						IPAddress.Parse(subnetElement.GetAttributeValue("mask"))
					);
					subnets.Add(subnet);
				}
				Location newLocation = new Location(locationName, subnets);
				s_locations.Add(locationName, newLocation);

				// map subnets back to locations, to make sure we're not assigning subnet to two locations
				foreach (IPUtil.Subnet subnet in subnets)
				{
					if (!s_subnets.ContainsKey(subnet))
					{
						s_subnets.Add(subnet, newLocation);
					}
					else
					{
						throw new FormatException(String.Format("Subnet '{0}' referenced multiple times in proxy map file ({1}). Each subnet should only be listed once.", subnet.Address.ToString(), fileName));
					}
				}
			}

			// parse servers
			XmlNode serversNode = mapNode.GetChildElementByName("servers");
			foreach (XmlNode serverElement in serversNode.GetChildElementsByName("server"))
			{
				P4Server.P4CharSet charSet = P4Server.P4CharSet.none; // by default, don't set server charset info

				Dictionary<Location, string> proxies = new Dictionary<Location, string>();
				string centralServerName = serverElement.GetAttributeValue("address");
				string centralServerCharset = serverElement.GetAttributeValue("charset", "none");
                if (!Enum.TryParse(centralServerCharset, out charSet))
                {
                    string validCharSets = String.Join(", ", Enum.GetValues(typeof(P4Server.P4CharSet)).Cast<P4Server.P4CharSet>().Select(s => "'" + s + "'"));
                    throw new FormatException(String.Format("Server '{0}' has invalid charset value '{1}'. Valid values are: {2}. ({3})", centralServerName, charSet, validCharSets, fileName));
                }

				// an optimal user friendly name to help users differentiate the servers
				string userFriendlyName = serverElement.GetAttributeValue("name", null);

				foreach (XmlNode proxyElement in serverElement.GetChildElementsByName("proxy"))
				{
					string location = proxyElement.GetAttributeValue("location");
					string proxy = proxyElement.InnerText;
					Location proxyLocation;

					if (!s_locations.TryGetValue(location, out proxyLocation))
					{
						throw new FormatException(String.Format("Server '{0}' references undefined location '{1}' in proxy map file {2}!.", centralServerName, location, fileName));
					}

					if (proxies.ContainsKey(proxyLocation))
					{
						throw new FormatException(String.Format("Duplicate proxy listing for location '{0}' for server '{1}' in proxy map file {2}!.", location, centralServerName, fileName));
					}
					proxies.Add(proxyLocation, proxy);
				}

				s_servers.Add(centralServerName, new Server(centralServerName, charSet, proxies, userFriendlyName));
			}
		}

		public static ProxyMap Load(string fileName)
		{
			XmlDocument doc = new XmlDocument();
			if (!File.Exists(fileName))
			{
				throw new IOException(String.Format("Unable to load proxy map file '{0}'. File could not be found.", fileName));
			}
			doc.Load(fileName);
			return Parse(doc, fileName);
		}

		public static ProxyMap Parse(XmlDocument doc, string fileName)
		{
			return new ProxyMap(doc, fileName);
		}

		public static ProxyMap Parse(string xml)
		{
			XmlDocument doc = new XmlDocument();
			doc.LoadXml(xml);
			return Parse(doc, string.Empty);
		}

		public Location GetIPLocation(IPAddress ipAddress)
		{
            Location subNetLocation = null;
			foreach (KeyValuePair<IPUtil.Subnet, Location> subnetToLocation in s_subnets)
			{
				if (subnetToLocation.Key.IsWithinNetwork(ipAddress))
				{
                    if (subNetLocation != null)
                    {
                        throw new FormatException(String.Format("IP address '{0}' maps to more than one location: {1}, {2}.", ipAddress.ToString(), subNetLocation.ToString(), subnetToLocation.Value.ToString()));
                    }
                    subNetLocation = subnetToLocation.Value;
				}
			}
			return subNetLocation;
		}

		public Server GetOptimalServer(string centralServer, Location location)
		{
			if (s_servers.TryGetValue(centralServer, out Server server))
			{
				if (server.Proxies.TryGetValue(location, out string optimalServerAddress))
				{
					return new Server(optimalServerAddress, server.CharSet, server.Name); // use charset from central server - proxy / edge has to use the same
				}
			}
			return null;
		}

		public Server GetCentralServerByName(string inputServerName)
		{
			if (s_servers.TryGetValue(inputServerName, out Server value))
			{
				return value;
			}
			return null;
		}

		public Server GetCentralServerByIpAddress(string inputServerName)
		{
			try
			{
				string inputServerNameWithoutPort = inputServerName.Split(':').First();
				IPAddress[] inputAddresses = Dns.GetHostAddresses(inputServerNameWithoutPort);
				foreach (string savedServerName in s_servers.Keys)
				{
					string savedServerNameWithoutPort = savedServerName.Split(':').First();
					IPAddress[] savedAddresses = Dns.GetHostAddresses(savedServerNameWithoutPort);
					if (inputAddresses.Intersect(savedAddresses).Count() > 0)
					{
						return s_servers[savedServerName];
					}
				}
			}
			catch 
			{
				// in the event that we fail to lookup one of the ip addresses, we don't want to throw an error just return null indicating that no match could be found
				return null;
			}
			// if no match was found but no error was thrown
			return null;
		}

		/// <summary>Get a list of all of the optimal servers not just a specific one</summary>
		public List<Server> ListOptimalServers()
		{
			// get user pi address, for resolving location with proxy maps
			IPAddress[] addresses = IPUtil.GetUserIPAddresses();

			return ListOptimalServers(addresses);
		}

		/// <summary>Get a list of all of the optimal servers not just a specific one</summary>
		/// <param name="addresses">User IP addresses, for resolving location with proxy maps</param>
		/// <remarks>This overload provided for unit testing.</remarks>
		internal List<Server> ListOptimalServers(IPAddress[] addresses)
		{
			List<Server> optimalServers = new List<Server>();
			foreach (Server server in this.s_servers.Values)
			{
				foreach (IPAddress address in addresses)
				{
					Location location = GetIPLocation(address);
					if (location == null)
					{
						continue;
					}

					Server optimalProxyServer = GetOptimalServer(server.Address, location);
					optimalServers.Add(optimalProxyServer);
				}
			}

			return optimalServers;
		}
	}
}
