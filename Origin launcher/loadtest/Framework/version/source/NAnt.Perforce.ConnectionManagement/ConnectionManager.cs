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
using System.Net.Sockets;
using System.Net;
using System.Runtime.InteropServices;

using NAnt.Authentication;
using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Shared.Properties;
using System.Diagnostics;

namespace NAnt.Perforce.ConnectionManagment
{
	// this static class is used to globally handle perforce connections in an efficient manner
	// - it handles authentication via a complex fallback through several authentication methods
	// - it caches successful connections for reuse
	// - uses "proxy map" file to remap server connection strings to optimal local proxies / edge servers
	public static class ConnectionManager
	{
		[DllImport("wininet.dll", SetLastError=true)]
		private static extern bool InternetGetConnectedState(out int flags, int reserved);

		// this class represents a cached attempt to connect to a previous server - it has two valid states:
		//		- m_connectServer == a valid connect p4server instance, m_connectionException == null	: we previously determined the correct server for given url and were able to connect to it
		//		- m_connectServer == null, m_connectionException == some form of connection excception	: we previously failed all possible connections to this valid servers for this uri - the cached
		//																								  exception should be rethrown on any thread that retrieves this state from the cache
		private class CachedConnectionStatus
		{
			internal readonly P4Server ConnectedServer;
			internal readonly P4Exception ConnectionException;

			internal CachedConnectionStatus(P4Server successfulServer)
			{
				ConnectedServer = successfulServer;
				ConnectionException = null;
			}

			internal CachedConnectionStatus(P4Exception connectionException)
			{
				ConnectedServer = null;
				ConnectionException = connectionException;
			}
		}

		public const string LogPrefix = "Perforce connection manager: ";

		public const string RemoteProxyMapLocation = "//fbstream/dev-na-cm/TnT/Build/Framework/data/P4ProtocolOptimalProxyMap.xml";
		public const string RemoteProxyMapServer = "dice-p4edge-fb.dice.ad.ea.com:2001"; // code assumes this is a central server
		public const P4Server.P4CharSet RemoteProxyMapCharSet = P4Server.P4CharSet.none;

		// ensure we only try to establish one connection at a time, we don't know if two separate attempts
		// actually resolve to the same server and we really want to avoid hitting the same server with the
		// same bad login attempt twice
		private static readonly object s_connectionLock = new object();

		// maps perforce connection string to previous connection attempts
		// missing entry means we haven't tried to connect to this server (or we're trying on another thread)
		// cached entry either have a valid server or an exception from a failed connection
		private static readonly Dictionary<string, CachedConnectionStatus> s_serverCache = new Dictionary<string, CachedConnectionStatus>();

		// proxy map is loaded on first use, it provides user specified mapping between central p4 server names 
		// and best (closest) p4 server (edge, central or proxy) to use for different subnets, if we can match
		// current machines subnet against the map we will use the specified optimal server
		private static ProxyMap s_p4ProxyServerMapping = null;

		// set when proxy map is loaded, used for reporting
		private static string s_proxyMapLocation = null;

		// name of users location assuming we have a match in proxy map
		private static ProxyMap.Location s_userLocation = null;

		// tracks if we've ever emitted to the log information about the p4 exe and version we are using
		// is only ever updated with in s_connectionLock scope
		private static bool s_donefirstTimePrintOfP4ExeInfo = false;

		// attempt to connect and authenticate with the exact server given, does not cached created connections
		public static P4Server AttemptP4ConnectExactServer(string serverAddress, Project project, out P4Exception connectionError, P4Server.P4CharSet charSet = P4Server.P4CharSet.none)
		{
			lock (s_connectionLock)
			{
				InitProxyMap(project);

				// TODO wire up cahcing
				// dvaliant 2017/05/26
				/* this function currently also forges a new connection rather than checking for cached connection */

				// still need central address to retrieve credentials since we store them by central usually
				string centralServerAddress = new P4Server(serverAddress, charset: charSet).ResolveToCentralAddress(resolveThroughEdgeServers: true);

				TryP4ServerConnect(serverAddress, centralServerAddress, P4TicketCache.ReadCache(project.Log, project.LogPrefix), project, out P4Server server, out connectionError, charSet);
				return server;
			}
		}

		// backwards compatility trampoline, the Log and BuildMessage parameters are ignored
		[Obsolete("AttemptP4Connect no longer requires a Log or BuildMessage parameter. You can use the overload that doesn't require these parameters.")]
		public static P4Server AttemptP4Connect(string serverAddress, Project project, Log log, BuildMessage buildMessage, out P4Exception connectionError, bool warnOnUnrecognizedCentralServer = true, P4Server.P4CharSet defaultCharSet = P4Server.P4CharSet.none)
		{
			project.Log.ThrowDeprecation
			(
				Log.DeprecationId.OldP4ConnectAPI, Log.DeprecateLevel.Normal,
				DeprecationUtil.GetCallSiteKey(),
				"{0} AttemptP4Connect no longer requires a Log or BuildMessage parameter.You can use the overload that doesn't require these parameters.",
				DeprecationUtil.GetCallSiteLocation()
			);
			return AttemptP4Connect(serverAddress, project, out connectionError, warnOnUnrecognizedCentralServer, defaultCharSet);
		}

		// try really really hard to connect to a perforce server that serves the same content as
		// requested server that is both optimal for connection and that we can determine user 
		// credentials automatically for automatically
		public static P4Server AttemptP4Connect(string requestedAddress, Project project, out P4Exception connectionError, bool warnOnUnrecognizedCentralServer = true, P4Server.P4CharSet defaultCharSet = P4Server.P4CharSet.none)
		{
			lock (s_connectionLock)
			{
				#region Check for cached connection.
				string connectionServer = null;
				{
					// initialize the proxy map, in case it hasn't been initialized yet
					InitProxyMap(project);

					// check if this uri has been request before and if so return cached authenticated server
					if (s_serverCache.TryGetValue(requestedAddress, out CachedConnectionStatus connectionStatus))
					{
						// we've seen this server with this name before
						if (connectionStatus.ConnectedServer != null) // null value is valid, it signifies we failed to connect in a previous attempt and we shouldn't try again
						{
							project.Log.Debug.WriteLine(project.LogPrefix + LogPrefix + "Using cached server '{0}' as optimal connection for requested server '{1}'.", connectionStatus.ConnectedServer.Port, requestedAddress);
						}
						connectionError = connectionStatus.ConnectionException;
						return connectionStatus.ConnectedServer;
					}

					// check that the server in the URI matches the one listed in the proxy map by comparing names
					ProxyMap.Server mappedCentralServerObject = s_p4ProxyServerMapping.GetCentralServerByName(requestedAddress);
                    connectionServer = mappedCentralServerObject?.Address;
					if (connectionServer == null)
					{
                        // if the name in the URI is not found, compare the ip addresses of each of the central servers in the proxy map
						mappedCentralServerObject = s_p4ProxyServerMapping.GetCentralServerByIpAddress(requestedAddress);
                        connectionServer = mappedCentralServerObject?.Address;
						if (connectionServer == null)
						{
							string resolvedCentralServer = null;
							try
							{
								// if we still haven't found it in map try and get central server from p4 information
								resolvedCentralServer = new P4Server(requestedAddress).ResolveToCentralAddress(resolveThroughEdgeServers: true);
							}
							catch (P4Exception)
							{
								// catch this exception as it will be thrown if we can't resolve to central server, we want to swallow it in the following case:
								// 1. requested server IS a central server
								// 2. central server is operational but inaccessible from users machine
								// 3. optimal proxy server is accessible from users machine
								// in this situation we don't care if connection to requested failed as we can map from requested to optimal without talking to p4.
								// for simplicity we swallow all p4exceptions here and let optimal proxy code decide what to do in this case if it determines we NEED 
								// to resolve to central server, we only resolve here for cache look up and throwing warnings for user
							}

							// if we managed to resolve to a central server then try the same checks again with the resolved central
							if (resolvedCentralServer != null && resolvedCentralServer != requestedAddress)
							{
								mappedCentralServerObject = s_p4ProxyServerMapping.GetCentralServerByName(resolvedCentralServer);
								connectionServer = mappedCentralServerObject?.Address;

								// resolved name still not found - ip maybe?
								if (connectionServer == null)
								{
									mappedCentralServerObject = s_p4ProxyServerMapping.GetCentralServerByIpAddress(resolvedCentralServer);
									connectionServer = mappedCentralServerObject?.Address;

									if (connectionServer == null)
									{
										// none of the central servers in the proxy map match the URI
										if (warnOnUnrecognizedCentralServer)
										{
											project.Log.ThrowWarning(Log.WarningId.ProxyMapUnrecognizedCentralServer, Log.WarnLevel.Normal,
												"Requested server '{0}' is not listed in the proxy map file '{1}'. " +
												"Only central servers are listed - not proxy/edge servers. Please use a central server address. " +
												"If this warning is not thrown as error, this server will be tried but may not connect to the optimal proxy for your location. " +
												"If this is a new central server contact {2} so that we can update the remote proxy map with this new server name.",
												requestedAddress, s_proxyMapLocation, CMProperties.CMSupportChannel);
										}
										connectionServer = requestedAddress; // we will just use the server listed in the URI
									}
									else
									{
										// reslved has the same ip address as one of the ones in the proxy map but uses a different name
										project.Log.ThrowWarning(Log.WarningId.ProxyMapUnrecognizedCentralServer, Log.WarnLevel.Normal,
											"Requested server '{0}' is not the central server listed in the proxy map. " +
											"Please change the uri field in the masterconfig.xml file to use the server name listed in the proxy map: '{1}'.", requestedAddress, connectionServer);
									}
								}
								else
								{
									project.Log.ThrowWarning(Log.WarningId.ProxyMapUnrecognizedCentralServer, Log.WarnLevel.Normal,
										"Requested server '{0}' is not the central server listed in the proxy map. " +
										"Please change the uri field in the masterconfig.xml file to use the server name listed in the proxy map: '{1}'.", requestedAddress, connectionServer);
								}
							}
							else
							{
								connectionServer = requestedAddress;
							}
						}
						else
						{
							// URI has the same ip address as one of the ones in the proxy map but uses a different name
							project.Log.ThrowWarning(Log.WarningId.ProxyMapUnrecognizedCentralServer, Log.WarnLevel.Normal,
								"Requested server '{0}' uses an alternative name for the same server listed in the proxy map. " +
								"Please change the uri field in the masterconfig.xml file to use the server name listed in the proxy map: '{1}'.", requestedAddress, connectionServer);
						}
					}

					if (s_serverCache.TryGetValue(connectionServer, out connectionStatus))
					{
						// if not null we've seen something that proxies / edgifys this server
						if (connectionStatus.ConnectedServer != null) // null cache value is valid, it signifies we failed to connect in a previous attempt and we shouldn't try again
						{
							project.Log.Debug.WriteLine(project.LogPrefix + LogPrefix + "Using cached server '{0}' as optimal connection for requested server '{1}' as they share same central server '{2}'.", connectionStatus.ConnectedServer, requestedAddress, connectionServer);
						}
						connectionError = connectionStatus.ConnectionException;
						return connectionStatus.ConnectedServer;
					}
				}
				#endregion

				#region Create new connection.
				{
					// if we got to this point in the function we didn't have anything cached for this, try a new connection
					P4Server server = null;

					// get ticket cache, we can hopefully determine correct credentials to use from this
					P4TicketCache ticketCache = P4TicketCache.ReadCache(project.Log, project.LogPrefix);

					// try optimal server uri first
					ProxyMap.Server optimalConnection = DetermineOptimalServer(requestedAddress, project, ref connectionServer, defaultCharSet);
					if (optimalConnection.Address != requestedAddress)
					{
						project.Log.Debug.WriteLine(project.LogPrefix + LogPrefix + "Determined server '{0}' as optimal sync server for '{1}' for your location.", optimalConnection.Address, requestedAddress);
						if (TryP4ServerConnect(optimalConnection.Address, connectionServer, ticketCache, project, out server, out connectionError, optimalConnection.CharSet))
						{
							// cache all of these, as they all have the same central server and we want to redirect all of them to our cached server
							CachedConnectionStatus successfulConnection = new CachedConnectionStatus(server);
							s_serverCache[optimalConnection.Address] = successfulConnection;
							s_serverCache[requestedAddress] = successfulConnection;
							if (connectionServer != null)
							{
								s_serverCache[connectionServer] = successfulConnection;
							}
							return server;
						}
						else
						{
							bool connected = true;

							// InternetGetConnectedState returns true if there is a connection, false if there isn't.
							// Only emit a warning when an internet connection exists. This function is only available 
							// on Windows.
							if (!PlatformUtil.IsMonoRuntime)
							{
								connected = InternetGetConnectedState(out int connectionState, 0);
							}

							// If this computer is not connected to the network then a failure to connect to the central
							// server is to be expected. So, avoid emitting a warning (or an error, if warnings-as-errors
							// is on)
							if (connected)
							{
								// we only warn if no login for optimal was possible as reporting it for requested server further down makes more sense to user
								// and fixing issue for requested implicitly fixes it for optimal server
								project.Log.ThrowWarning
								(
									Log.WarningId.PackageServerErrors,
									Log.WarnLevel.Normal,
									"Failed to connect to server '{0}' - this was determined as your best server for retrieving content from requested server '{1}'. If this is incorrect please update the proxy map file at {2} and inform the CM Team via {3}. Connection exception:\n {4}",
									optimalConnection.Address, 
									requestedAddress, 
									s_proxyMapLocation, 
									CMProperties.CMSupportChannel,
									connectionError
								);
							}
						}
					}

					{
						// try explicitly given uri
						if (TryP4ServerConnect(requestedAddress, connectionServer, ticketCache, project, out server, out connectionError, optimalConnection.CharSet))
						{
							if (optimalConnection.Address == requestedAddress)
							{
								project.Log.Debug.WriteLine(project.LogPrefix + LogPrefix + "Determined requested server '{0}' is already optimal sync server for your location.", requestedAddress);
							}
							else
							{
								project.Log.ThrowWarning
								(
									Log.WarningId.PackageServerErrors,
									Log.WarnLevel.Normal,
									"NOTE: Successfully connected to {0}, however this server (or optimal proxy setting) is not specified for your location {1} in the Framework proxymap {2}. Please contact the CM Team via {3} to update the proxy list for this server to the proxymap.",
									connectionServer,
									string.Join("/", IPUtil.GetUserIPAddresses().Select(ip => ip.ToString())),
									s_proxyMapLocation,
									CMProperties.CMSupportChannel);
							}

							CachedConnectionStatus successfulConnection = new CachedConnectionStatus(server);
							s_serverCache[optimalConnection.Address] = successfulConnection;
							s_serverCache[requestedAddress] = successfulConnection;
							if (connectionServer != null)
							{
								s_serverCache[connectionServer] = successfulConnection;
							}
							return server;
						}
					}

					// we failed to connect to anything, cache null before we leave the lock,
					// to ensure noting else tries to connect to this server, since we already 
					// know it will fail
					CachedConnectionStatus failedConnection = new CachedConnectionStatus(connectionError);
					s_serverCache[optimalConnection.Address] = failedConnection;
					s_serverCache[requestedAddress] = failedConnection;
					if (connectionServer != null)
					{
						s_serverCache[connectionServer] = failedConnection;
					}
					return null;
				}
				#endregion
			}
		}

		// try all the best credential combinations we can come up with for this exact server port, if we succeed return true and set out server,
		// if we fail return false and set connectionError and calling code can choose whether to throw it or not
		// note that this function should only ever be called from a context in which s_connectionLock is locked
		private static bool TryP4ServerConnect(string serverPort, string centralServer, P4TicketCache ticketCache, Project project, out P4Server server, out P4Exception connectionError, P4Server.P4CharSet charSet = P4Server.P4CharSet.none)
		{
			try
			{
				bool couldBeNoAuthenticationServer = true; // the first time we fail with invalid password on a no-password, no-ticket authentication
				// we can stop trying no-authentication methods
				connectionError = null;

				// create server object in this function then modify it in TryP4ServerConnectWithCredentials - this save recreating the object and resolving GetInfo()
				server = new P4Server(port: serverPort, charset: charSet);

				// first time we create a server instance print debugging information, concurrecny guard is s_connectionLock, but there is no compile
				// time constraint for this, we are relying on this function only being called from a callstack under AttemptP4Connect
				if (!s_donefirstTimePrintOfP4ExeInfo)
				{
					project.Log.Debug.WriteLine(project.LogPrefix + LogPrefix + "Using p4.exe at {0} (Version: {1}).", server.P4ExecutableLocation, server.ClientVersion);
					s_donefirstTimePrintOfP4ExeInfo = true;
				}

				// APPROACH 1 - use our own credential store for perforce server
				{
					Credential perforceCredential = CredentialStore.Retrieve(project.Log, serverPort, project.Properties[FrameworkProperties.CredStoreFilePropertyName]);

					// check in credential store for central server if provided and different from actual server, in most cases central and proxy/edge server will use same credentials
					if (perforceCredential == null && centralServer != null && centralServer != serverPort)
					{
						perforceCredential = CredentialStore.Retrieve(project.Log, centralServer, project.Properties[FrameworkProperties.CredStoreFilePropertyName]);
					}

					if (perforceCredential != null)
					{
						project.Log.Debug.WriteLine(project.LogPrefix + LogPrefix + "Attempting to connect to server '{0}' using Framework credential cache...", serverPort);
						var connectTimer = Stopwatch.StartNew();
						if (TryP4ServerConnectWithCredentials(server, perforceCredential.Name, project, ref connectionError, ref couldBeNoAuthenticationServer, password: perforceCredential.Password))
						{
							project.Log.Status.WriteLine(project.LogPrefix + LogPrefix + "Connected to server '{0}' using STORED CREDSTORE credential.", serverPort);
							var elapsed = connectTimer.ElapsedMilliseconds;
							if (elapsed > 4000)
								project.Log.Warning.WriteLine(project.LogPrefix + LogPrefix + "Connecting to p4 with STORED CREDSTORE took longer than expected ({0:F1} seconds)", (float)elapsed * 0.001f);
							return true;
						}
						else
						{
							// if credential store approach failed, check if it was an ldap error, if it was and we can reliably detect that the
							// account isn't locked out - invalidate the credential store for this server
							if (connectionError != null)
							{
								if (connectionError.Flatten().FirstOrDefault(ex => ex is InvalidPasswordException) is InvalidPasswordException pwdEx)
								{
									string credStoreFile = project.Properties[FrameworkProperties.CredStoreFilePropertyName];
									if (pwdEx.IsLDAPException)
									{
										bool accountLockedOut;
										if (ActiveDirectoryUtil.TryDetectADAccountLockedOut(perforceCredential.Name, out accountLockedOut))
										{
											if (!accountLockedOut)
											{
												project.Log.Status.WriteLine(project.LogPrefix + LogPrefix +
													"Server '{0}' rejected the credential store password with LDAP validation error. Invalidating credstore password to prevent bad login attempts.", serverPort);

												bool invalidatedLocalServer = CredentialStore.Invalidate(project.Log, serverPort, credStoreFile);
												bool invalidatedCentralServer = CredentialStore.Invalidate(project.Log, centralServer, credStoreFile);

												if (!invalidatedLocalServer && !invalidatedCentralServer)
												{
													string credStorePath = credStoreFile ?? CredentialStore.GetDefaultCredStoreFilePath();
													project.Log.Status.WriteLine(project.LogPrefix + LogPrefix +
														"Unable to invalidate credentials for local server '{0}' and central server '{1}'! Please report this issue via {2} and include both the names of the servers and attach the credential store file ({3})",
														serverPort,
														centralServer,
														CMProperties.CMSupportChannel,
														credStorePath);
												}
											}
											else
											{
												project.Log.Status.WriteLine(project.LogPrefix + LogPrefix +
													"Server '{0}' rejected the credential store password with LDAP validation error. Account is locked out however so credential store may be correct.", serverPort);
											}
										}
										else
										{
											project.Log.Status.WriteLine(project.LogPrefix + LogPrefix +
												"Server '{0}' rejected the credential store password with LDAP validation error. Account status could not be determined.", serverPort);
										}
									}
									else
									{
										bool invalidatedLocalServer = CredentialStore.Invalidate(project.Log, serverPort, credStoreFile);
										bool invalidatedCentralServer = CredentialStore.Invalidate(project.Log, centralServer, credStoreFile);
										if (!invalidatedLocalServer && !invalidatedCentralServer)
										{
											string credStorePath = credStoreFile ?? CredentialStore.GetDefaultCredStoreFilePath();
											project.Log.Status.WriteLine(project.LogPrefix + LogPrefix +
												"Unable to invalidate credentials for local server '{0}' and central server '{1}'! Please report this issue via {2} and include both the names of the servers and attach the credential store file ({3})",
												serverPort,
												centralServer,
												CMProperties.CMSupportChannel,
												credStorePath);
										}
									}
								}
							}
						}
					}
				}

				// APPROACH 2 - try to find working credentials from tickets file
				{
					List<KeyValuePair<string, string>> serverTickets = ticketCache.ServerTickets(server);
					if (serverTickets.Any())
					{
						project.Log.Debug.WriteLine(project.LogPrefix + LogPrefix + "Attempting to connect to server '{0}' using p4 ticket cache...", serverPort);
						foreach (KeyValuePair<string, string> userTicketPair in serverTickets)
						{
							if (TryP4ServerConnectWithCredentials(server, userTicketPair.Key, project, ref connectionError, ref couldBeNoAuthenticationServer, ticket: userTicketPair.Value))
							{
								project.Log.Status.WriteLine(project.LogPrefix + LogPrefix + "Connected to server '{0}' using TICKET FILE credential.", serverPort);
								return true;
							}
						}
					}
				}

				// APPROACH 3 - hail mary pass, we might be using zero security package server, just throw some user names at it
				{
					if (couldBeNoAuthenticationServer)
					{
						project.Log.Debug.WriteLine(project.LogPrefix + LogPrefix + "Attempting to connect to '{0}' without authentication...", serverPort);
						foreach (string userName in AuthenticationUtil.GetLocalUserNames(includeUndomainedVariants: true))
						{
							if (couldBeNoAuthenticationServer && TryP4ServerConnectWithCredentials(server, userName, project, ref connectionError, ref couldBeNoAuthenticationServer))
							{
								project.Log.Status.WriteLine(project.LogPrefix + LogPrefix + "Connected to server '{0}' using NO AUTHENTICATION.", serverPort);
								return true;
							}
						}
					}
				}

				// APPROACH 4 - all else attempts to find the credentials failed, try prompting the user to enter the credentials
				// (with timeout in case build is automated) 
				{
					Credential userInputCredential = CredentialStore.Prompt(project.Log, serverPort);
					if (userInputCredential != null) // will be null if we time out
					{
						int retries = 2;
						bool successfulConnection = TryP4ServerConnectWithCredentials(server, userInputCredential.Name, project, ref connectionError, ref couldBeNoAuthenticationServer, password: userInputCredential.Password);
						while (!successfulConnection && connectionError is PasswordException && retries > 0)
						{
							// get user to try again
							retries--;
							project.Log.Status.WriteLine(project.LogPrefix + LogPrefix + "Invalid P4 credentials!");
							userInputCredential = CredentialStore.Prompt(project.Log, serverPort);

							// if timeouts out on retry, fail
							if (userInputCredential == null)
							{
								return false;
							}

							// retry connection
							successfulConnection = TryP4ServerConnectWithCredentials(server, userInputCredential.Name, project, ref connectionError, ref couldBeNoAuthenticationServer, password: userInputCredential.Password);
						}

						// if user ever gave us input, don't fallback to other methods just return result of user credentials
						if (successfulConnection)
						{
							project.Log.Status.WriteLine(project.LogPrefix + LogPrefix + "Connected to server '{0}' using user input CREDSTORE credential.", serverPort);
							return true;
						}
						return false;
					}
				}

				// no successful connection
				if (connectionError == null)
				{
					// if we got here with null connection then we had no credentials
					connectionError = new P4Exception(String.Format("Could not determine credentials in p4 protocol package server for {0}.", serverPort));
				}
				server = null;
				return false;
			}
			catch (P4Exception p4Ex)
			{
				connectionError = p4Ex;
				server = null;
				return false;
			}
		}

		private static bool TryP4ServerConnectWithCredentials(P4Server server, string user, Project project, ref P4Exception connectionError, ref bool couldBeNoAuthenticationServer, string ticket = null, string password = null)
		{
			server.User = user;
			if (ticket != null)
			{
				try
				{
					project.Log.Debug.WriteLine(project.LogPrefix + LogPrefix + "Authenticating with '{0}' as user '{1}' using ticket login...", server.Port, server.User);
					server.Ticket = ticket;
					server.GetClients(1); // arbitrary test command that requires authentication
					return true;
				}
				catch (P4Exception p4Ex)
				{
					server.Ticket = null; // reset ticket 
					project.Log.Info.WriteLine(project.LogPrefix + LogPrefix + "Authenticating with '{0}' as user '{1}' using ticket login failed: {2}", server.Port, server.User, p4Ex.Message);
					connectionError = p4Ex;
					return false;
				}
			}
			else if (password != null)
			{
				try
				{
					project.Log.Debug.WriteLine(project.LogPrefix + LogPrefix + "Authenticating with '{0}' as user '{1}' using password login...", server.Port, server.User);
					server.Password = password;
					server.Login();
					return true;
				}
				catch (P4Exception p4Ex)
				{
					server.Password = null; // reset password
					project.Log.Info.WriteLine(project.LogPrefix + LogPrefix + "Authenticating with '{0}' as user '{1}' using password login failed: {2}", server.Port, server.User, p4Ex.Message);
					connectionError = p4Ex;
					return false;
				}
			}
			else if (couldBeNoAuthenticationServer) // no authentication, rare case, see if we don't need to login to this server with this user name
			{
				try
				{
					project.Log.Debug.WriteLine(LogPrefix + "Authenticating with '{0}' as user '{1}' with no authentication...", server.Port, server.User);
					server.GetClients(1); // arbitrary test command that requires authentication
					return true;
				}
				catch (P4Exception p4Ex)
				{
					project.Log.Info.WriteLine(LogPrefix + "Authentication with '{0}' as user '{1}' with no authentication failed: {2}", server.Port, server.User, p4Ex.Message);

					// if this was a password exception, then this server must required a password
					if (p4Ex is PasswordException)
					{
						couldBeNoAuthenticationServer = false;
					}

					connectionError = p4Ex;
					return false;
				}
			}
			else
			{
				return false;
			}
		}

		private static ProxyMap.Server DetermineOptimalServer(string requestedServerAddress, Project project, ref string centralServerForRequested, P4Server.P4CharSet defaultCharSet)
		{
			// make sure proxy map is setup	
			InitProxyMap(project);

			// does the resolved central server have an entry in map?
			if (s_userLocation != null)
			{
				// ideally user is requesting central server location, in which case we can skip the resolve and potential connection errors
				ProxyMap.Server optimalServer = s_p4ProxyServerMapping.GetOptimalServer(requestedServerAddress, s_userLocation);
				if (optimalServer != null)
				{
					centralServerForRequested = centralServerForRequested ?? requestedServerAddress; // set central server to requested if we don't need to map,
																									 // assumes proxy map only contains central servers
					return optimalServer;
				}

				// we didn't have an entry for requested server, try mapping to central - it could be a proxy / edge was requested or it might just be an alias for central server
				try
				{
					if (centralServerForRequested == null)
					{
						P4Server requested = new P4Server(requestedServerAddress);
						centralServerForRequested = requested.ResolveToCentralAddress(resolveThroughEdgeServers: true);
					}
					ProxyMap.Server optimalServerFromCentral = s_p4ProxyServerMapping.GetOptimalServer(centralServerForRequested, s_userLocation);
					if (optimalServerFromCentral != null)
					{
						return optimalServerFromCentral;
					}

					// we don't have a mapping for server - but the central server does list a charset, take this over uri
					// charset - user may not have listed a charset when one is required and we trust proxymap more
					ProxyMap.Server centralProxyMapEntryForRequested = s_p4ProxyServerMapping.GetCentralServerByName(centralServerForRequested) ?? s_p4ProxyServerMapping.GetCentralServerByIpAddress(centralServerForRequested);
					if (centralProxyMapEntryForRequested != null)
					{
						defaultCharSet = centralProxyMapEntryForRequested.CharSet;
					}
				}
				catch (P4Exception)
				{
					// couldn't connect to request address in order to resolve central
				}
			}

			// just return given uri
			return new ProxyMap.Server(requestedServerAddress, defaultCharSet);
		}

		private static void InitProxyMap(Project project)
		{
			if (s_p4ProxyServerMapping == null)
			{
				/// four logic steps:
				/// 1, try to load local proxy map
				/// 2, if 1 successful, try load remote proxy map using best proxy from local proxy map, else load remote proxy map from central server
				/// 3, if 2 succeed use remote proxy map, if 2 failed but 1 suceeded, use local proxy map, else throw exception
				/// 4, use proxy map to determine user location

				// get user pi address, for resolving location with proxy maps
				IPAddress[] addresses = IPUtil.GetUserIPAddresses();

				// preload p4 tickets for authentication
				P4TicketCache ticketCache = P4TicketCache.ReadCache(project.Log, project.LogPrefix);

				// step 1 - try load local proxy maps				
				Exception localLoadException = null;
				ProxyMap localProxyMap = null;
				ProxyMap.Location localUserLocation = null;

				// P4-REFACTOR-TODO - relying on Project.NantLocation
				bool proxyMapWasOverridden = false;
				string overrideProxyMapLocation = project.Properties["ondemandp4proxymapfile"];
				string localProxyMapLocation;
				if (overrideProxyMapLocation != null)
				{
					localProxyMapLocation = overrideProxyMapLocation;
					proxyMapWasOverridden = true;
				}
				else
				{
					if (File.Exists(Path.Combine(Project.NantLocation, "P4ProtocolOptimalProxyMap.xml")))
					{
						// Look for the proxy map in the same folder as the nant location, for cases where the assembly and the proxy map have been copied locally
						localProxyMapLocation = Path.Combine(Project.NantLocation, "P4ProtocolOptimalProxyMap.xml");
					}
					else
					{
						// Look for the proxy map in the default location within the framework package relative to the assembly
						localProxyMapLocation = Path.Combine(Project.NantLocation, "../data/P4ProtocolOptimalProxyMap.xml");
					}
				}
				localProxyMapLocation = PathNormalizer.Normalize(localProxyMapLocation);

				try
				{
					localProxyMap = ProxyMap.Load(localProxyMapLocation);
				}
				catch (Exception ex)
				{
					localLoadException = ex;
				}

				// step 2 - try load remote proxy map
				ProxyMap remoteProxyMap = null;
				string remoteProxyMapLocation = null;
				if (!proxyMapWasOverridden) // if user specified a custom local proxy map do not use remote
				{
					// first determine, if local proxy map informs us of the best proxy to get remote map from
					if (localProxyMap != null)
					{
						foreach (IPAddress address in addresses)
						{
							localUserLocation = localProxyMap.GetIPLocation(address);
							if (localUserLocation != null)
							{
								break;
							}
						}
						if (localUserLocation != null)
						{
							ProxyMap.Server optimalRemoteServer = localProxyMap.GetOptimalServer(RemoteProxyMapServer, localUserLocation);
							if (optimalRemoteServer != null && optimalRemoteServer.Address != RemoteProxyMapServer) // if our best server is central server, skip ahead
							{
								try
								{
									P4Server server = null;
									P4Exception exception = null;
									if (TryP4ServerConnect(optimalRemoteServer.Address, RemoteProxyMapServer, ticketCache, project, out server, out exception, optimalRemoteServer.CharSet))
									{
										// print proxy map from location
										string mapContents = server.Print(fileSpecs: new P4Location(RemoteProxyMapLocation)).First().Contents;
										remoteProxyMap = ProxyMap.Parse(mapContents);
										remoteProxyMapLocation = String.Format("({0}) {1}", optimalRemoteServer.Address, RemoteProxyMapLocation);

										// if we successfully connected cache this for future use, BUT only cache actual address,
										// we don't want to cache mapping from local proxy map in case it is different from up to date
										// proxy map
										CachedConnectionStatus successfulConnection = new CachedConnectionStatus(server);
										s_serverCache[optimalRemoteServer.Address] = successfulConnection;
									}
								}
								catch { }
							}
						}
					}

					// if we got here we failed to get remote map from best proxy according to local map OR local map had no proxy info OR central server is best server
					// try to get direct from central server
					if (remoteProxyMap == null)
					{
						try
						{
							P4Server server = null;
							P4Exception exception = null;
							if (TryP4ServerConnect(RemoteProxyMapServer, RemoteProxyMapServer, ticketCache, project, out server, out exception, RemoteProxyMapCharSet))
							{
								// print proxy map from location
								string mapContents = server.Print(fileSpecs: new P4Location(RemoteProxyMapLocation)).First().Contents;
								remoteProxyMap = ProxyMap.Parse(mapContents);
								remoteProxyMapLocation = String.Format("({0}) {1}", RemoteProxyMapServer, RemoteProxyMapLocation);

                                // DO NOT CACHE HERE - this connection was based on local proxy map, which we do not trust
							}
						}
						catch { }
					}
				}

				// step 3, look at what was successful and pick one to be our final proxy map
				{
					if (remoteProxyMap != null)
					{
						// we found a valid remote proxy map
						s_p4ProxyServerMapping = remoteProxyMap;
						s_proxyMapLocation = remoteProxyMapLocation;
					}
					else if (localProxyMap != null)
					{
						// we didn't find remote, fall back to local
						s_p4ProxyServerMapping = localProxyMap;
						s_proxyMapLocation = localProxyMapLocation;
						s_userLocation = localUserLocation; // we may have already calculated this
					}
					else
					{
						// both attempts failed, throw the exception we got trying to load local
						throw localLoadException;
					}
					project.Log.Status.WriteLine(project.LogPrefix + "Retrieving proxy map from {0}.", s_proxyMapLocation);
				}

				// step 4, determine local user location
				{
					// loop through local ip addresses and see if it matches 
					if (s_userLocation == null)
					{
						foreach (IPAddress address in addresses)
						{
							s_userLocation = s_p4ProxyServerMapping.GetIPLocation(address);
							if (s_userLocation != null)
							{
								break;
							}
						}
					}

					if (s_userLocation != null)
					{
						project.Log.Info.WriteLine(LogPrefix + "Determined user studio location as '{0}'.", s_userLocation.Name);
					}
					else
					{
						// try to give user enough information to figure out what went wrong
						project.Log.Minimal.WriteLine("Unable to determine location for p4 package server protocol! You may find downloads extremely slow.");

						if (addresses.Any())
						{
							IPUtil.Subnet[] subnets = IPUtil.GetUserSubnets();
							if (subnets.Any())
							{
								project.Log.Minimal.WriteLine("(Your subnet is probably {0})", subnets.First());
							}

							project.Log.Minimal.WriteLine("(Your IP address is probably {0})", addresses.First());

							project.Log.ThrowWarning
							(
								Log.WarningId.PackageServerErrors,
								Log.WarnLevel.Normal,
								"Please add your subnet to proxy map file at '{0}' or, if unsure, seek via {1} with this error message as well as your IP address and studio location. " +
								"If you are not part of an EA studio please set the <ondemand> element in your masterconfig to false and talk to your EA partner about an alternate way to acquire these packages.",
								s_proxyMapLocation,
								CMProperties.CMSupportChannel
							);
						}
						else
						{
							// If we weren't able to detect user's IP address, then the above warning message would be confusing.  Throw a different warning.
							project.Log.ThrowWarning
							(
								Log.WarningId.PackageServerErrors,
								Log.WarnLevel.Normal,
								"There seems to be problem detecting your IP address from the available network interfaces.  Unable to detect optimal proxy server!  You may want to check if your network adapter is setup properly.",
								s_proxyMapLocation,
								CMProperties.CMSupportChannel
							);
						}
					}
				}
			}
		}

		// try to get fully qualified name using DNS but don't fail if we can't, we only compare qualified names for warnings
		private static bool TryGetFullyQualifiedName(string dnsSafeHost, out string fullyQualifiedName)
		{
			try
			{
				fullyQualifiedName = Dns.GetHostEntry(dnsSafeHost).HostName;
				return true;
			}
			catch (SocketException) // if this was due to a connection error, other code will throw the p4 version of the connection failure
			{
				fullyQualifiedName = null;
				return false;
			}
		}
	}
}
