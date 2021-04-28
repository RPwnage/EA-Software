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
using System.Diagnostics;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;

namespace NAnt.Core.Util
{
	public static class IPUtil
	{
		public struct Subnet	
		{
			public readonly IPAddress Address;
			public readonly IPAddress Mask;

			public Subnet(IPAddress address, IPAddress mask)
			{
				Address = address;
				Mask = mask;

				Debug.Assert(MaskAddressToSubnet(Address, Mask).Equals(Address), $"Invalid subnet, address {Address} is too specific for mask {Mask}.");
			}

			public bool IsWithinNetwork(IPAddress ip)
			{
				IPAddress network = MaskAddressToSubnet(ip, Mask);
				return network.Equals(Address);
			}

			public override string ToString()
			{
				return Address.ToString() + "/" + Mask.ToString();
			}
		}

		static IPAddress[] s_cachedIpAddresses;
		static object s_cachedIpAddressesLock = new object();
		static Subnet[] s_cachedSubnets;
		static object s_cachedSubnetsLock = new object();

		public static IPAddress[] GetUserIPAddresses()
		{
			lock (s_cachedIpAddressesLock)
			{
				if (s_cachedIpAddresses != null)
					return s_cachedIpAddresses;

				s_cachedIpAddresses = NetworkInterface.GetAllNetworkInterfaces()
					// The OperationalStatus info is busted under WSL.  It always set to 'Unknown' status unfortunately.
					.Where(networkInterface => PlatformUtil.IsOnWindowsSubsystemForLinux
						? networkInterface.OperationalStatus == OperationalStatus.Up || networkInterface.OperationalStatus == OperationalStatus.Unknown
						: networkInterface.OperationalStatus == OperationalStatus.Up)
					.OrderBy(networkInterface => String.IsNullOrEmpty(networkInterface.GetIPProperties().DnsSuffix) ? 1 : 0)        // prefer adapters with DNS suffix as they are more likely to be EA network
					.SelectMany(networkInterface => networkInterface.GetIPProperties().UnicastAddresses)
					.Select(unicastAddress => unicastAddress.Address)
					.Where(ipAddress => ipAddress.AddressFamily == AddressFamily.InterNetwork && !IPAddress.IsLoopback(ipAddress))  // ignore loop back
					.ToArray();
				return s_cachedIpAddresses;
			}
		}

		public static Subnet[] GetUserSubnets()
		{
			lock (s_cachedSubnetsLock)
			{
				if (s_cachedSubnets != null)
					return s_cachedSubnets;
				List<Subnet> subnets = new List<Subnet>();
				foreach (NetworkInterface netInterface in NetworkInterface.GetAllNetworkInterfaces())
				{
					foreach (UnicastIPAddressInformation unicastIP in netInterface.GetIPProperties().UnicastAddresses)
					{
						IPAddress ipMask = null;
						try
						{
							// Some version of mono doesn't implement IPv4Mask and using it will get an exception.
							ipMask = unicastIP.IPv4Mask;
						}
						catch (Exception)
						{
							// Just return an empty array now if IPv4Mask property is not implemented.
							return subnets.ToArray();
						}

						if (unicastIP.Address.AddressFamily == AddressFamily.InterNetwork && !IPAddress.IsLoopback(unicastIP.Address))
						{
							subnets.Add
							(
								new Subnet
								(
									MaskAddressToSubnet(unicastIP.Address, ipMask),
									ipMask
								)
							);
						}
					}
				}
				s_cachedSubnets = subnets.ToArray();
				return s_cachedSubnets;
			}
		}

		public static IPAddress MaskAddressToSubnet(IPAddress address, IPAddress mask)
		{
			byte[] addressBytes = address.GetAddressBytes();
			byte[] maskBytes = mask.GetAddressBytes();
			byte[] subnetBytes = new byte[Math.Max(addressBytes.Length, maskBytes.Length)];
			for (int i = 0; i < Math.Min(addressBytes.Length, maskBytes.Length); ++i)
			{
				subnetBytes[i] = (byte)(addressBytes[i] & maskBytes[i]);
			}
			return new IPAddress(subnetBytes);
		}
	}
}
