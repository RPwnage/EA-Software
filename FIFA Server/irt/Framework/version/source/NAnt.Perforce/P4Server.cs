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
using System.Linq;
using System.Net;
using System.Text.RegularExpressions;
using System.Threading;
using System.Security.Cryptography;

using NAnt.Perforce.Extensions.String;
using NAnt.Perforce.Internal;

namespace NAnt.Perforce
{
	public class P4Server : P4Context
	{
		const string EdgeServer = "edge-server";
		const string StandardServer = "standard";

		public enum P4CharSet
		{
			none,
			auto,
			cp1251,
			cp1253,
			cp850,
			cp858,
			cp936,
			cp949,
			cp950,
			eucjp,
			iso8859_1,
			iso8859_15,
			iso8859_5,
			iso8859_7,
			koi8_r,
			macosroman,
			shiftjis,
			utf16,
			utf16be,
			utf16be_bom,
			utf16le,
			utf16le_bom,
			utf16_nobom,
			utf32,
			utf32be,
			utf32be_bom,
			utf32le,
			utf32le_bom,
			utf32_nobom,
			utf8,
			utf8_bom,
			winansi,
			unrecognized // invalid option, used in testing only
		}

		public P4CharSet CharSet;

		public readonly string Port;

		public string Ticket
		{
			get
			{
				return _Ticket;
			}
			set
			{
				if (_User == null)
				{
					throw new P4Exception("Please set User before setting login ticket.");
				}

				_Ticket = value;

				// if user is setting ticket, we assume they want to use it, and if they were using explcit password we should now pass explicit ticket
				// if they weren't using password, retain current ticket explicity setting
				_UseExplicitTicket = _Ticket != null;
				_UseExplicitPassword = _Ticket == null && _UseExplicitPassword;
			}
		}

		public string Password 
		{ 
			get
			{
				return _Password;
			}
			set
			{
				if (_Password != value)
				{
					_Password = value;
					_Ticket = _Password != null ? null : _Ticket; // reset ticket, user wants to do a password login presumably
				}
			}
		}

		public string User 
		{ 
			get
			{
				return _User;
			}
			set
			{
				if (_User != value)
				{
					_User = value;
					_Ticket = null; // reset ticket, in case user wants to do password login
				}
			}
		}
		
		public P4Version ServerVersion 
		{ 
			get 
			{
				if (_ServerVersion == null)
				{
					_ServerVersion = new P4Version(GetInfo().GetField("serverversion"));
				}
				return _ServerVersion;
			} 
		}

		public P4Version ClientVersion
		{
			get
			{
				return new P4Version(P4Caller.GetClientVersionString(this));
			}
		}

		public string P4ExecutableLocation
		{
			get
			{
				return P4Caller.P4ExecutableLocation;
			}
		}

		public bool UseExplicitPassword // pass password with every command, most perforce security settings won't allow this and require explicit login
		{
			get { return _UseExplicitPassword; }
			set
			{
				if (value)
				{
					_UseExplicitPassword = true;
					_UseExplicitTicket = false;
				}
				else
				{
					_UseExplicitPassword = false;
				}
			}
		}

		public bool UseExplicitTicket // if true passes ticket with every command, can be set to false and p4 will default to tickets in ticket file
		{
			get { return _UseExplicitTicket; }
			set
			{
				if (value)
				{
					_UseExplicitTicket = true;
					_UseExplicitPassword = false;
				}
				else
				{
					_UseExplicitTicket = false;
				}
			}
		}

		public bool CaseSensitive { get { return (GetInfo().GetField("casehandling") == "insensitive") ? false : true; } }
		public bool IsProxy { get { return GetInfo().HasField("proxyaddress"); } }
		public bool IsEdgeServer { get { return ServerServices == EdgeServer; } }
		public bool IsStandardServer { get { return ServerServices == StandardServer; } }

		public string ServerAddress { get { return GetInfo().GetField("serveraddress"); } }
		public string ChangelistServer { get { return GetInfo().GetField("changeserver", null); } }
		public string ServerServices { get { return GetInfo().GetField("serverservices"); } }
		public string Cluster { get { return GetInfo().GetField("servercluster", null); } }

		protected override P4Location DefaultLocation { get { return P4Location.DefaultLocation; } }

		private P4Record _Info = null;
		private bool _UseExplicitPassword = false; // only if user sets this to true do we want to do this, doesn't work on some security levels
		private bool _UseExplicitTicket = true; // this is the best default for automated code, be explicit about authentication ticket to use once logged in
		private P4Version _ServerVersion = null;
		private string _User = null;
		private string _Password = null;
		private string _Ticket = null;

		private static Regex _TicketRegex = new Regex(@"^([A-Z0-9]{32})$");

		public P4Server(string port, string user = null, string password = null, P4CharSet charset = P4Server.P4CharSet.none, bool skipServerVersionVerify=false)
		{
			if (port == null)
			{
				throw new ArgumentNullException("port", "A port must be specified when creating P4Server object.");
			}

			Port = port;
			CharSet = charset;

			_User = user;
			_Password = password;

			if (!skipServerVersionVerify)
			{
				VerifyServerVersion();
			}
		}

		public void VerifyServerVersion()
		{
			if (!ServerVersion.AtLeast(2015, 2))
			{
				throw new Exception(String.Format("ERROR: The p4 server version is currently '{0}'. Please make sure the server is at least version '2015.2' .", ServerVersion.ToString()));
			}
		}

		public string ResolveToCentralAddress(bool resolveThroughEdgeServers = false) // normally you don't want to resolve through edge server as they are treated as separate central servers for purposes of tickets / clients / etc
		{
			if (resolveThroughEdgeServers && IsEdgeServer)
			{
				return GetInfo().GetField("replica");
			}
			else
			{
				return ServerAddress;
			}
		}

		public void Login(string user = null, string password = null)
		{
			_User = user ?? _User;
			_Password = password ?? _Password;

			if (_User == null)
			{
				throw new P4Exception("Must set User in order to login!");
			}

			if (_Password == null)
			{
				throw new P4Exception("Must set Password in order to login! If you would like to use ticket login set Ticket property.");
			}

			int retries = 0;
			while (true)
			{
				try
				{
					// p4's ldap connector appears to be written in perl, in rare cases
					// you'll get perl output from warnings / errors before you get the 
					// ticket back, for this reason we use a regex to detect the first 
					// ticket line
					// it is possble to get two ticket lines when connecting to an edge 
					// server as it lists its ticket first and then the central server 
					// ticket, we are only interested in the ticket for the edge server
					P4Output loginOutput = P4Caller.Run(this, "login", new string[] { "-a", "-p" }, input: Password, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
					IEnumerable<string> trimmedMessages = loginOutput.Messages.Select(m => m.Trim());
					IEnumerable<string> tickets = trimmedMessages.Where(tm => _TicketRegex.IsMatch(tm));
					if (!tickets.Any())
					{
						throw new LoginOutputFormatException(loginOutput);
					}
					_Ticket = tickets.First();

					P4Caller.Run(this, "login", new string[] { "-a" }, input: Password, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
					_Info = P4Caller.Run(this, "info", timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs).Records.First();
					return;
				}
				catch (P4Exception ex)
				{
					P4Exception[] flattened = ex.Flatten();
					InvalidPasswordException pwdEx = (InvalidPasswordException)flattened.FirstOrDefault(x => x is InvalidPasswordException);
					if (pwdEx != null) // "password invalid" actully means a variety of things could have occured do with ldap errors, we process those here
					{
						RunException runEx = (RunException)flattened.FirstOrDefault(x => x is RunException);
						if (runEx != null)
						{
							if (Regex.IsMatch(runEx.Message, "Unable to get FQDN from FlatName"))
							{
								// gn dn, let this fall through to retry case
							}
							else
							{
								throw; // in all other cases, assume password is actually invalid
							}
						}
						else
						{
							throw; // every password invalid exception should have a run exeception alongside it with the actual details, if it doesn't just throw since something has gone wrong
						}
					}
					else
					{
						throw;
					}
					if (retries < 3)
					{
						Thread.Sleep(500);
						++retries;
					}
					else
					{
						throw;
					}
				}
			}
		}

		#region Clients
		public bool ClientExists(string name)
		{
			EnsureLoggedIn();
			return GetClients(1, name, true).Any();
		}

		public P4Client GetClient(string name)
		{
			if (ClientExists(name))
			{
				return new P4Client(this, P4Caller.Run(this, "client", new string[] { "-o", name }, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs).Records.First());
			}
			throw new ClientDoesNotExistException(name);
		}

		/* gets all clients based on certain criteria */
		#region GetClients
		public P4ClientStub[] GetClients(string nameFilter = null, bool caseSenstiveFilter = false, string user = null)
		{
			EnsureLoggedIn();
			List<string> args = new List<string>();
			if (nameFilter != null)
			{
				if (caseSenstiveFilter) 
				{
					args.AddRange(new string[] { "-e", nameFilter });
				}
				else
				{
					args.AddRange(new string[] { "-E", nameFilter });
				}
			}
			if (user != null)
			{
				args.AddRange(new string[] {"-u", user });
			}
			P4Output output = P4Caller.Run(this, "clients", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			return (from record in output.Records select new P4ClientStub(this, record)).ToArray();
		}

		public P4ClientStub[] GetClients(uint max, string nameFilter = null, bool caseSenstiveFilter = false, string user = null)
		{
			EnsureLoggedIn();
			List<string> args = CommonMaxArg(max).ToList();
			if (nameFilter != null)
			{
				if (caseSenstiveFilter)
				{
					args.AddRange(new string[] { "-e", nameFilter });
				}
				else
				{
					args.AddRange(new string[] { "-E", nameFilter });
				}
			}
			if (user != null)
			{
				args.AddRange(new string[] { "-u", user });
			}
			P4Output output = P4Caller.Run(this, "clients", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			return (from record in output.Records select new P4ClientStub(this, record)).ToArray();
		}
		#endregion

		#region CreateClient
		public P4Client CreateClient(string name, string root, Dictionary<string, string> viewMap, P4Client.ClientOptions options = null, P4Client.LineEndOption lineEnd = P4Client.DefaultLineEnd, P4Client.SubmitOptionSet submitOptions = P4Client.DefaultSubmitOptions, P4Client.ClientTypes clientType = P4Client.DefaultClientType)
		{
			if (ClientExists(name))
			{
				throw new ClientAlreadyExistException(name);
			}
			return CreateOrOverwriteClient(name, root, viewMap, options, lineEnd, submitOptions, clientType);
		}

		public P4Client CreateClient(string name, string root, string stream, P4Client.ClientOptions options = null, P4Client.LineEndOption lineEnd = P4Client.DefaultLineEnd, P4Client.SubmitOptionSet submitOptions = P4Client.DefaultSubmitOptions, P4Client.ClientTypes clientType = P4Client.DefaultClientType)
		{
			if (ClientExists(name))
			{
				throw new ClientAlreadyExistException(name);
			}
			return CreateOrOverwriteClient(name, root, stream, options, lineEnd, submitOptions, clientType);
		}
		#endregion

		#region CreateOrOverwriteClient
		public P4Client CreateOrOverwriteClient(string name, string root, Dictionary<string, string> viewMap, P4Client.ClientOptions options = null, P4Client.LineEndOption lineEnd = P4Client.DefaultLineEnd, P4Client.SubmitOptionSet submitOptions = P4Client.DefaultSubmitOptions, P4Client.ClientTypes clientType = P4Client.DefaultClientType)
		{
			EnsureLoggedIn();
			P4Client newClient = new P4Client(this, name, root, viewMap, options, lineEnd, submitOptions, clientType);
			newClient.Save();
			return newClient;
		}

		public P4Client CreateOrOverwriteClient(string name, string root, string stream, P4Client.ClientOptions options = null, P4Client.LineEndOption lineEnd = P4Client.DefaultLineEnd, P4Client.SubmitOptionSet submitOptions = P4Client.DefaultSubmitOptions, P4Client.ClientTypes clientType = P4Client.DefaultClientType)
		{
			EnsureLoggedIn();
			P4Client newClient = new P4Client(this, name, root, stream, options, lineEnd, submitOptions, clientType);
			newClient.Save();
			return newClient;
		}
		#endregion

		/* create a client if it does not exist otherwise update it with the new viewMap, note we have to explicitly check 
		if the old client exists otherwise we will create a default with the default mappings that we may not want */
		#region CreateOrUpdateClient
		public P4Client CreateOrUpdateClient(string name, string root, Dictionary<string, string> viewMap, P4Client.ClientOptions options = null, P4Client.LineEndOption lineEnd = P4Client.DefaultLineEnd, P4Client.SubmitOptionSet submitOptions = P4Client.DefaultSubmitOptions, P4Client.ClientTypes clientType = P4Client.DefaultClientType)
		{
			EnsureLoggedIn();
			if (ClientExists(name))
			{
				P4Client existing = GetClient(name);
				existing.Root = root;
				existing.UpdateViewMap(viewMap);
				existing.Options = options ?? P4Client.DefaultOptions;
				existing.LineEnd = lineEnd;
				existing.SubmitOptions = submitOptions;
				existing.ClientType = clientType;
				existing.Save();
				return existing;
			}
			else
			{
				return CreateOrOverwriteClient(name, root, viewMap, options, lineEnd, submitOptions, clientType);
			}
		}

		public P4Client CreateOrUpdateClient(string name, string root, string stream, P4Client.ClientOptions options = null, P4Client.LineEndOption lineEnd = P4Client.DefaultLineEnd, P4Client.SubmitOptionSet submitOptions = P4Client.DefaultSubmitOptions, P4Client.ClientTypes clientType = P4Client.DefaultClientType)
		{
			EnsureLoggedIn();
			if (ClientExists(name))
			{
				P4Client existing = GetClient(name);
				existing.Root = root;
				existing.Stream = stream;
				existing.ClientType = clientType;
				existing.Options = options ?? P4Client.DefaultOptions;
				existing.LineEnd = lineEnd;
				existing.SubmitOptions = submitOptions;
				existing.Save();
				return existing;
			}
			else
			{
				return CreateOrOverwriteClient(name, root, stream, options, lineEnd, submitOptions, clientType);
			}
		}
		#endregion

		static private int ThreadSafeClientNameCounter = 0;
		static private int ThreadSafeClientUniqueNum
		{
			get
			{
				// The Interlocked.Increment handles overflow by wrapping without throwing exception.  This is really
				// not a concern to us as we won't really do that many sync in a run and the number will never really 
				// overflow.  Even if number got overflow, it is not really a concern either, as this number is combined
				// with UtcNow.Ticks to generate client name.  We just wanted a lock-free way of generating a unique
				// client name for each thread.  As long as following code doesn't throw exception, we're good.
				return Interlocked.Increment(ref ThreadSafeClientNameCounter);
			}
		}

		public string CreateTempClientName(string prefix)
		{
			int clientNum = ThreadSafeClientUniqueNum;
			// Use HEX for the numbers to make the string shorter in case the client name gets too long for perforce.
			string name = String.Format("{0}_{1}_{2:X}_{3:X}", prefix, Dns.GetHostName(), DateTime.UtcNow.Ticks, clientNum);
			while (ClientExists(name))
			{
				name = String.Format("{0}_{1}_{2:X}_{3:X}", prefix, Dns.GetHostName(), DateTime.UtcNow.Ticks, clientNum);
			}
			return name;
		}

		public P4TempClient CreateTempClient(string prefix, string root, Dictionary<string, string> viewMap, P4Client.ClientTypes clientType = P4Client.DefaultClientType)
		{
			EnsureLoggedIn();
			string name = CreateTempClientName(prefix);
			P4TempClient newClient = new P4TempClient(this, name, root, viewMap, clientType);
			newClient.Save();
			return newClient;
		}

		public P4TempClient CreateTempClient(string prefix, string root, string stream, P4Client.ClientTypes clientType = P4Client.DefaultClientType)
		{
			EnsureLoggedIn();
			string name = CreateTempClientName(prefix);
			P4TempClient newClient = new P4TempClient(this, name, root, stream, clientType);
			newClient.Save();
			return newClient;
		}
		#endregion

		#region Depots
		public bool DepotExists(string name)
		{
			EnsureLoggedIn();
			return GetDepots().Any(d => d.Name == name);
		}

		public P4Depot GetDepot(string name)
		{
			P4Depot depot = GetDepots().FirstOrDefault(d => d.Name == name);
			if (depot == null)
			{
				throw new DepotDoesNotExistException(name);
			}
			return depot;
		}

		public P4Depot[] GetDepots()
		{
			EnsureLoggedIn();
			P4Output output = P4Caller.Run(this, "depots", timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			return (from record in output.Records select new P4Depot(this, record)).ToArray();
		}

		public P4Depot CreateDepot(string name, string descriptiom, string map, P4Depot.DepotType type = P4Depot.DepotType.local, uint streamDepth = 1)
		{
			if (DepotExists(name))
			{
				throw new DepotAlreadyExistException(name);
			}
			return CreateOrOverwriteDepot(name, descriptiom, map, type, streamDepth);
		}

		public P4Depot CreateOrOverwriteDepot(string name, string descriptiom, string map, P4Depot.DepotType type = P4Depot.DepotType.local, uint streamDepth = 1)
		{
			EnsureLoggedIn();
			P4Depot newDepot = new P4Depot(this, name, descriptiom, map, type, streamDepth);
			newDepot.Save();
			return newDepot;
		}
		#endregion

		#region Streams
		public bool StreamExists(string stream)
		{
			EnsureLoggedIn();
			return GetStreams(1, false, false, new Dictionary<string, string>() { { "Stream", stream } }).Any();
		}

		public P4Stream GetStream(string stream)
		{
			if (StreamExists(stream))
			{
				return new P4Stream(this, P4Caller.Run(this, "stream", new string[] { "-o", stream }, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs).Records.First());
			}
			throw new ClientDoesNotExistException(stream);
		}

		public P4Stream[] GetStreams(bool includeUnloaded = false, bool throwOnNoMatches = false, Dictionary<string, string> searchFilter = null)
		{
			EnsureLoggedIn();
			List<string> args = new List<string>();
			if (includeUnloaded)
			{
				args.Add("-U");
			}
			if (searchFilter != null && searchFilter.Any())
			{
				args.AddRange(new string[] { "-F", String.Join(" ", searchFilter.Select(kvp => String.Format("{0}={1}", kvp.Key, kvp.Value))).Quoted() });
			}

			try
			{
				P4Output output = P4Caller.Run(this, "streams", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
				return (from record in output.Records select new P4Stream(this, record)).ToArray();
			}
			catch (P4Exception p4Ex)
			{
				p4Ex.ThrowStrippedIfNotOnly(throwOnNoMatches, typeof(NoSuchStreamException));
				return (from record in p4Ex.LinkedOutput.Records select new P4Stream(this, record)).ToArray();
			}
		}

		public P4Stream[] GetStreams(uint max, bool includeUnloaded = false, bool throwOnNoMatches = false, Dictionary<string, string> searchFilter = null)
		{
			EnsureLoggedIn();
			List<string> args = CommonMaxArg(max).ToList();
			if (includeUnloaded)
			{
				args.Add("-U");
			}
			if (searchFilter != null && searchFilter.Any())
			{
				args.AddRange(new string[] { "-F", String.Join(" ", searchFilter.Select(kvp => String.Format("{0}={1}", kvp.Key, kvp.Value))).Quoted() });
			}

			try
			{
				P4Output output = P4Caller.Run(this, "streams", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
				return (from record in output.Records select new P4Stream(this, record)).ToArray();
			}
			catch (P4Exception p4Ex)
			{
				p4Ex.ThrowStrippedIfNotOnly(throwOnNoMatches, typeof(NoSuchStreamException));
				return (from record in p4Ex.LinkedOutput.Records select new P4Stream(this, record)).ToArray();
			}
		}

		public P4Stream CreateStream(string stream, string descriptiom, string displayName = null, string parent = P4Stream.NoParent, P4Stream.StreamType type = P4Stream.StreamType.mainlinetype, params P4Stream.Path[] paths)
		{
			if (StreamExists(stream))
			{
				throw new StreamAlreadyExistException(stream);
			}
			return CreateOrOverwriteStream(stream, descriptiom, displayName, parent, type, paths);
		}

		public P4Stream CreateOrOverwriteStream(string stream, string descriptiom, string displayName = null, string parent = P4Stream.NoParent, P4Stream.StreamType type = P4Stream.StreamType.mainlinetype, params P4Stream.Path[] paths)
		{
			EnsureLoggedIn();
			P4Stream newStream = new P4Stream(this, stream, descriptiom, displayName, parent, type, paths);
			newStream.Save();
			return newStream;
		}
		#endregion

		#region Users
		/* return a list of users known to the perforce server */
		#region GetUsers
		public P4User[] GetUsers(bool includeServiceUsers = false, bool proxyUsersOnly = false, bool centralUsersOnly = false, bool throwOnNoMatchingUsers = false, string user = null)
		{
			EnsureLoggedIn();
			List<string> args = new List<string>() { };
			if (includeServiceUsers)
			{
				args.Add("-a");
			}
			if (proxyUsersOnly)
			{
				args.Add("-r");
			}
			if (centralUsersOnly)
			{
				args.Add("-c");
			}
			if (user != null)
			{
				args.Add(user.Quoted()); // quoted to prevent / vs // issues on unix
			}
			try
			{
				P4Output output = P4Caller.Run(this, "users", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
				return (from record in output.Records select new P4User(this, record)).ToArray();
			}
			catch (P4Exception p4Ex)
			{
				p4Ex.ThrowStrippedIfNotOnly(throwOnNoMatchingUsers, typeof(NoSuchUsersException));
				return (from record in p4Ex.LinkedOutput.Records select new P4User(this, record)).ToArray();
			}
		}

		public P4User[] GetUsers(uint max, bool includeServiceUsers = false, bool proxyUsersOnly = false, bool centralUsersOnly = false, bool throwOnNoMatchingUsers = false, string user = null)
		{
			EnsureLoggedIn();
			List<string> args = new List<string>() { };
			args.AddRange(CommonMaxArg(max));
			if (includeServiceUsers)
			{
				args.Add("-a");
			}
			if (proxyUsersOnly)
			{
				args.Add("-r");
			}
			if (centralUsersOnly)
			{
				args.Add("-c");
			}
			if (user != null)
			{
				args.Add(user.Quoted()); // quoted to prevent / vs // issues on unix
			}
			try
			{
				P4Output output = P4Caller.Run(this, "users", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
				return (from record in output.Records select new P4User(this, record)).ToArray();
			}
			catch (P4Exception p4Ex)
			{
				p4Ex.ThrowStrippedIfNotOnly(throwOnNoMatchingUsers, typeof(NoSuchUsersException));
				return (from record in p4Ex.LinkedOutput.Records select new P4User(this, record)).ToArray();
			}
		}
		#endregion

		/* check if a user exists */
		public bool UserExists(string userName)
		{
			return GetUsers(1, true, false, false, false, userName).Any();
		}

		/* get a details about a single user */
		public P4User GetUser(string userName)
		{
			P4Record[] records = P4Caller.Run(this, "user", new string[] { "-o", userName }, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs).Records;
			if (records.Any())
			{
				return new P4User(this, records.First());
			}
			throw new UserDoesNotExistException(userName);
		}

		public P4User CreateCurrentUser(P4User.UserType type, string email, string fullName)
		{
			if (User == null)
			{
				throw new P4Exception("This operation requires a user. Please set User property.");
			}
			if (UserExists(User))
			{
				throw new UserAlreadyExistException(User);
			}
			return CreateOrOverwriteCurrentUser(type, email, fullName);
		}

		public P4User CreateOrOverwriteCurrentUser(P4User.UserType type, string email, string fullName)
		{
			if (User == null)
			{
				throw new P4Exception("This operation requires a user. Please set User property.");
			}
			P4User newUser = new P4User(this, User, type, email, fullName);
			newUser.Save();
			return newUser;
		}

		public void ChangeCurrentUserPassword(string newPassword, string oldPassword = null)
		{
			oldPassword = oldPassword ?? Password;
			string[] args = new string[] { "-P", newPassword };
			if (oldPassword != null)
			{
				args = new string[] { "-O", oldPassword }.Concat(args).ToArray();
			}
			P4Caller.Run(this, "passwd", args, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			Password = newPassword;
		}
		#endregion

		// TODO: see if these Changes functions can be moved to P4Context
		// Changes overloads
		#region Changes
		public P4Changelist[] Changes(bool includeDescriptions = false, bool includeIntegrations = false, string clientName = null, string userName = null, params P4FileSpec[] fileSpecs)
		{
			return DoChanges(includeDescriptions, includeIntegrations, clientName, userName, fileSpecs);
		}

		public P4Changelist[] Changes(uint max, bool includeDescriptions = false, bool includeIntegrations = false, string clientName = null, string userName = null, params P4FileSpec[] fileSpecs)
		{
			return DoChanges(includeDescriptions, includeIntegrations, clientName, userName, fileSpecs, CommonMaxArg(max));
		}

		public P4Changelist[] Changes(P4Changelist.CLStatus status, bool includeDescriptions = false, bool includeIntegrations = false, string clientName = null, string userName = null, params P4FileSpec[] fileSpecs)
		{
			return DoChanges(includeDescriptions, includeIntegrations, clientName, userName, fileSpecs, CommonStatusArg(status));
		}

		public P4Changelist[] Changes(uint max, P4Changelist.CLStatus status, bool includeDescriptions = false, bool includeIntegrations = false, string clientName = null, string userName = null, params P4FileSpec[] fileSpecs)
		{
			return DoChanges(includeDescriptions, includeIntegrations, clientName, userName, fileSpecs, CommonMaxArg(max).Concat(CommonStatusArg(status)).ToArray());
		}
		#endregion

		#region OpenedFiles
		public P4OpenedFile[] OpenedFiles(string changelist = null, string client = null, string user = null, params P4Location[] locations)
		{
			return DoGetOpenedFiles(changelist, client, user, locations);
		}

		public P4OpenedFile[] OpenedFiles(uint max, string changelist = null, string client = null, string user = null, params P4Location[] locations)
		{
			return DoGetOpenedFiles(changelist, client, user, locations, CommonMaxArg(max).ToList());
		}
		#endregion

		// only input we ever need to sanitize is password being passed on its own to login command
		internal string SanitizeInput(string input)
		{
			if (_Password != null && input == _Password)
			{
				return "*****";
			}
			return input;
		}

		internal string SanitizeCommand(string command)
		{
			if (_Ticket != null)
			{
				string ticketMatch = String.Format("-P {0}", _Ticket);
				command = command.Replace(ticketMatch, "-P *****");
			}
			if (_Password != null)
			{
				string passwordMatch = String.Format("-P {0}", _Password);
				command = command.Replace(passwordMatch, "-P *****");
			}
			return command;
		}

		internal List<string> ServerGlobalArgs()
		{
			List<string> globalArgs = new List<string> { String.Format("-p {0}", Port) };
			if (_User != null)
			{
				globalArgs.Add(String.Format("-u \"{0}\"", User)); // quote user string to avoid unix \ vs \\ issues
			}
			if (_UseExplicitPassword && Password != null)
			{
				globalArgs.Add(String.Format("-P {0}", Password));
			}
			else if (_UseExplicitTicket && Ticket != null)
			{
				globalArgs.Add(String.Format("-P {0}", Ticket));
			}
			globalArgs.Add(String.Format("-C {0}", CharSet.ToString("g").Replace('_', '-')));
			return globalArgs;
		}

		internal override void EnsureLoggedIn()
		{
			if (_User == null)
			{
				throw new P4Exception("This operation requires a user. Please set User property.");
			}
			if (_Password != null && !_UseExplicitPassword && Ticket == null) // allow null password, passwordless users are allowed
			{
				Login(_User, _Password);
			}
			else if (_Info == null)
			{
				_Info = P4Caller.Run(this, "info", timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs).Records.First();
			}
		}

		static string s_noneClientName; 
		static string NoneClientName 
		{
			get 
			{
				if (s_noneClientName == null) 
				{
					using (RandomNumberGenerator rng = new RNGCryptoServiceProvider())
					{
						byte[] data = new byte[16]; 
						rng.GetBytes(data);
						s_noneClientName = "none_nant_" + BitConverter.ToString(data).Replace("-", string.Empty);
					}
				}
				return s_noneClientName;
			}
		}


		internal override List<string> BuildGlobalArgs(uint retries, string responseFilePath = null)
		{
			List<string> args = CommonGlobalArgs(retries, responseFilePath);
			args.AddRange(ServerGlobalArgs());
			args.AddRange(new string[] { "-c", NoneClientName }); // force no client, otherwise p4.exe will pick up env vars / reg keys
			return args;
		}

		internal override P4Server GetServer()
		{
			return this;
		}

		private P4Record GetInfo()
		{
			if (_Info == null)
			{
				_Info = P4Caller.Run(this, "info", timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs).Records.First();
			}
			return _Info;
		}
	}
}
