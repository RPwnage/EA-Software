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
using System.IO;
using System.Linq;
using System.Net;

using NAnt.Core.Util;
using NAnt.Core.Logging;
using System.Net.Sockets;

namespace NAnt.Perforce
{
	public class P4TicketCache
	{
		private readonly Log m_log;
		private readonly string m_logPrefix;

		private Dictionary<string, Dictionary<string, string>> m_cacheDictionary; // dict[p4User][addressClusterOrIpOfP4Server] = "TICKET_STRING"
		private Dictionary<string, List<IPAddress>> m_serverIPAddresses = null;

		private P4TicketCache(Dictionary<string, Dictionary<string, string>> cacheDict, Log log, string logBasePrefix) // private, should only be constructed by our static functions
		{
			m_cacheDictionary = cacheDict;
			m_log = log;
			m_logPrefix = logBasePrefix + "Perforce Ticket Cache: ";
		}

		public void Refresh()
		{
			m_cacheDictionary = GetUserTicketsDictionary();
		}

		public IEnumerable<string> Servers()
		{
			return m_cacheDictionary.Values.SelectMany(serverDict => serverDict.Keys);
		}

		public List<KeyValuePair<string, string>> ServerTickets(P4Server server)
		{
			List<KeyValuePair<string, string>> serverTickets = new List<KeyValuePair<string, string>>();
			if (m_cacheDictionary.Any()) // skip address resolve if there are no tickets
			{
				string centralServerAddress = server.ResolveToCentralAddress();
				string clusterKey = server.Cluster != null ? clusterKey = String.Format("localhost:{0}", server.Cluster) : null;
				
				foreach (KeyValuePair<string, Dictionary<string, string>> userToServerDict in m_cacheDictionary)
				{
					foreach (KeyValuePair<string, string> serverToTicketPair in userToServerDict.Value)
					{
						string ticketP4Port = serverToTicketPair.Key;
						bool foundServerMatch = DoServersMatch(centralServerAddress, ticketP4Port, clusterKey);

						if (foundServerMatch)
						{
							if (!serverTickets.Any(kvp => kvp.Key == userToServerDict.Key && kvp.Value == serverToTicketPair.Value))
							{
								serverTickets.Add(new KeyValuePair<string, string>(userToServerDict.Key, serverToTicketPair.Value));
							}
						}
					}
				}

				if(!serverTickets.Any())
				{
					m_log.Debug.WriteLine(m_logPrefix + "No tickets were found on server {0}", centralServerAddress);
				}
			}
			return serverTickets;
		}

		public List<KeyValuePair<string, string>> ServerTickets(string server)
		{
			return ServerTickets(new P4Server(server)); // use new authenticated server if only name provided, its sufficiemt for central resolution check and cluster info
		}

		public Dictionary<string, string> UserTickets(string userName)
		{
			Dictionary<string, string> userTickets = new Dictionary<string, string>();
			foreach (KeyValuePair<string, Dictionary<string, string>> userToServerDict in m_cacheDictionary)
			{
				string p4user = userToServerDict.Key;

				if (userName.Equals(p4user, StringComparison.InvariantCultureIgnoreCase))
				{
					bool exactMatch = userName.Equals(p4user);

					foreach (KeyValuePair<string, string> serverToTicketPair in userToServerDict.Value)
					{
						string server = serverToTicketPair.Key;
						string ticket = serverToTicketPair.Value;

						//	We prioritize exact match, and only add a case-insensitive
						//	match if server has not been added.  This prevents an
						//	case-insensitive match to override an exact match.  On a
						//	case-sensitive server, a user can have multiple tickets
						//	for the same server differing only by case.
						if (exactMatch)
						{
							userTickets[server] = ticket;
						}
						else
						{
							if (!userTickets.ContainsKey(server))
							{
								userTickets.Add(server, ticket);
							}
							else
							{
								m_log.Warning.WriteLine(m_logPrefix + "The p4 server {0} is case-sensitive but we found multiple tickets for this server with usernames that match case-insentively.  The ticket for {1} will be skipped for we already have chosen a ticket for them.  If the chosen ticket is incorrect, be sure to pass the P4USER with the exact case.",  server, p4user);
							}
						}
					}
				}
			}
			return userTickets;
		}

		public string UserTicket(string userName, string server)
		{
			P4Server p4server = new P4Server(server);
			string result = UserTicket(userName, p4server);

			return(result);
		}

		public string UserTicket(string userName, P4Server server)
		{
			string result = null;

			List<KeyValuePair<string, string>> serverTickets = ServerTickets(server);

			foreach (KeyValuePair<string, string> userTicketPair in serverTickets)
			{
				string p4user = userTicketPair.Key;
				string ticket = userTicketPair.Value;

				//	The logic we _should_ be doing here is determine whether the server
				//	is case-insensitive or case-sensitive.  If case-sensitive, the only
				//	valid username is an exact match.
				//
				//	However, because this was not how the original implementation was
				//	handled, I will instead support a case-insensitive username.  On
				//	case-sensitive servers, perforce supports multiple users that differ
				//	only by case.
				if (userName.Equals(p4user))
				{
					result = ticket;
					break;
				}

				if (userName.Equals(p4user, StringComparison.InvariantCultureIgnoreCase))
				{
					if (server.CaseSensitive)
					{
						if (result == null)
						{
							result = ticket;
						}
						else
						{
							m_log.Warning.WriteLine(m_logPrefix + "The p4 server is case-sensitive but we found multiple tickets for this server with usernames that match case-insentively.  The ticket for {0} will be skipped for we already have chosen a ticket for them.  If the chosen ticket is incorrect, be sure to pass the P4USER with the exact case.",  p4user);
						}
					}
					else
					{
						result = ticket;
						break;
					}
				}
			}
			return(result);
		}

		public static P4TicketCache ReadCache(Log log, string logBasePrefix = "")
		{
			return new P4TicketCache(GetUserTicketsDictionary(), log, logBasePrefix);
		}

		private static Dictionary<string, Dictionary<string, string>> GetUserTicketsDictionary()
		{
			Dictionary<string, Dictionary<string, string>> tickets = new Dictionary<string, Dictionary<string, string>>();
			string ticketFileLocation = Environment.GetEnvironmentVariable("P4TICKETS");
			if (String.IsNullOrWhiteSpace(ticketFileLocation))
			{
				string ticketFileName = PlatformUtil.IsWindows ? "p4tickets.txt" : ".p4tickets";
				ticketFileLocation = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ticketFileName);
			}

			if (File.Exists(ticketFileLocation))
			{
				// example file layout:
				/*
					my.perforce.server:1337=mydomain\myuser:TICKETTICKETTICKETTICKETTICKETTI
					12.34.56.78:1337=mydomain\myuser:TICKETTICKETTICKETTICKETTICKETTI
					localhost:cluster=mydomain\myuser:TICKETTICKETTICKETTICKETTICKETTI
				*/

				string[] lines = File.ReadAllLines(ticketFileLocation);
				foreach (string line in lines)
				{
					string[] lineSplit = line.Trim().Split(new char[] { '=' }, 2);
					if (lineSplit.Length == 2)
					{
						string addressClusterOrIP = lineSplit[0];
						string nameAndTicket = lineSplit[1];
						string[] nameAndTicketSplit = nameAndTicket.Split(new char[] { ':' }, 2);
						if (nameAndTicketSplit.Length == 2)
						{
							string ticketUser = nameAndTicketSplit[0];
							string ticket = nameAndTicketSplit[1];
							Dictionary<string, string> userTickets = null;
							if (!tickets.TryGetValue(ticketUser, out userTickets))
							{
								userTickets = new Dictionary<string, string>();
								tickets[ticketUser] = userTickets;
							}
							userTickets[addressClusterOrIP] = ticket;
						}
					}
				}
			}

			return tickets;
		}

		//	Perforce tickets appear to be stored in one of three ways:
		//
		//		1.	dns.name:port
		//		2.	IP.Address:port
		//		3.	localhost:clusterName
		//
		//	For (1), it is expected that the server name has been resolved to the
		//	central address.  Because this function is private, this is safe.  Though
		//	the functionality of this could be moved out (to say P4Server) where
		//	it might not be able to make that assumption.
		//
		//	There are two ways to handle (2) when the IP address is stored.  We can
		//	either get the IP of the server we are trying to match; or, we can call
		//	perforce (e.g. `p4 -p ip.address:port info`).
		//
		//	The result from calling perforce should be more reliable, but it is
		//	considerably slower in best case (where the address is still reachable),
		//	and marginably worse for worst case.
		//
		//	Using the IP address, assuming that server is not an IP, then it uses
		//	however .net* internally caches the result which potentially may be wrong.
		//	*Which is most likely how the OS internally caches the IP addresses.
		//
		//	(3) We pass in the clusterName.  As previously mentioned, if this was moved
		//	to P4Server or another function, it would be better to internally get this
		//	information.
		//
		//	TODO(bk):  Move DoServersMatch and GetIPAddresses out of P4TicketCache to
		//	a more appropriate location that other classes could use.
		private bool DoServersMatch(string server, string ticketServer, string clusterName)
		{
			bool result = false;

			string[] split = ticketServer.Split(':');
			//  The server address part of the p4port (most often format address:port) is
			//  the second last entry.  Though rare, perforce does support other formats
			//  but similar to the normal format, the address is the second last entry.
			int addressIndex = Math.Max(split.Length - 2, 0);
			string ticketAddress = split[addressIndex];

			//  Perforce sometimes stores an IP for the ticket address.
			IPAddress ticketIP = null;
			if (IPAddress.TryParse(ticketAddress, out ticketIP))
			{
				m_serverIPAddresses = m_serverIPAddresses ?? new Dictionary<string, List<IPAddress>>();

				if (!m_serverIPAddresses.TryGetValue(server, out List<IPAddress> serverIPAddresses))
				{
					serverIPAddresses = GetIPAddresses(m_log, m_logPrefix, server);
					m_serverIPAddresses.Add(server, serverIPAddresses);
				}

				foreach (IPAddress serverIP in serverIPAddresses)
				{
					if (ticketIP.Equals(serverIP))
					{
						result = true;
						break;
					}
				}
			}
			else
			{
				if (ticketServer.Equals(server) || ticketServer.Equals(clusterName))
				{
					result = true;
				}
			}
			return(result);
		}

		//	Returns a list of IPv4 and IPv6 addresses for the given p4port.  Under most
		//	circumstances there should only be a single result but GetHostAddresses can
		//	return multiple results.
		private static List<IPAddress> GetIPAddresses(Log log, string logPrefix, string p4port)
		{
			List<IPAddress> result = new List<IPAddress>();

			if (String.IsNullOrWhiteSpace(p4port))
			{
				return (result);
			}

			string[] split = p4port.Split(':');

			string address = split[0];

			IPAddress[] addresses = null;
			try
			{
				//  I use GetHostAddresses over GetHostEntry for the latter does a DNS
				//  reverse lookup which can fail if the address is an IP.  Also, if
				//  address is an IP, GetHostAddresses will simply return the address;
				//  which, for our needs is enough.
				addresses = Dns.GetHostAddresses(address);
			}
			catch (SocketException exception)
			{
				log.Warning.WriteLine(logPrefix + "Getting the ip address of {0} failed with exception: {1}",
					p4port, exception.Message);
			}

			if (addresses != null)
			{
				foreach (IPAddress ip in addresses)
				{
					// TODO(bk):  Perforce supports IPv4 or IPv6 addresses.  Does it support other?
					if (ip.AddressFamily == AddressFamily.InterNetwork ||
						ip.AddressFamily == AddressFamily.InterNetworkV6)
					{
						result.Add(ip);
					}
				}

				if(!result.Any())
				{
					log.Warning.WriteLine(logPrefix + "{0} found IP addresses, but none were IPv4 or IPv6"
								+ " and thus result is empty.", p4port);
				}
			}
			return(result);
		}
	}
}