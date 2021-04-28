// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
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

using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Reflection;
using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Perforce;
using NAnt.Perforce.ConnectionManagment;
using NAnt.Tests.Util;

namespace P4ProtocolTests
{
	// This is a test for the script init task
	[TestClass]
	public class ProxyMapTests
	{
		private static readonly IPAddress StockholmIP = IPAddress.Parse("10.46.161.7");
		private static readonly IPAddress EACIP = IPAddress.Parse("10.10.78.25");

		// This is a example proxy map used for the tests
		// We write this to a temp directory so the tests can find it
		private static string TempProxyMap = @"
<map>
	<locations>
		<location name=""EAC"">
			<subnet address=""10.9.0.0"" mask=""255.255.0.0""/>
			<subnet address=""10.10.0.0"" mask=""255.255.0.0""/>
		</location>
		<location name=""EARS"">
			<subnet address=""10.12.0.0"" mask=""255.255.0.0""/>
			<subnet address=""10.14.0.0"" mask=""255.255.0.0""/>
			<subnet address=""10.15.0.0"" mask=""255.255.0.0""/>
		</location>
		<location name=""DiceStockholm"">
			<subnet address=""10.20.124.0"" mask=""255.255.252.0""/>
			<subnet address=""10.20.196.0"" mask=""255.255.252.0""/>
			<subnet address=""10.20.200.0"" mask=""255.255.248.0""/>
			<subnet address=""10.20.208.0"" mask=""255.255.240.0""/>
			<subnet address=""10.20.224.0"" mask=""255.255.240.0""/>
			<subnet address=""10.20.96.0"" mask=""255.255.240.0""/>
			<subnet address=""10.22.0.0"" mask=""255.255.0.0""/>
			<subnet address=""10.46.0.0"" mask=""255.255.0.0""/>
		</location>
	</locations>
	<servers>
		<server name=""frostbite"" address=""dice-p4edge-fb.dice.ad.ea.com:2001"">
			<proxy location=""DiceStockholm"">dice-p4edge-fb.dice.ad.ea.com:2001</proxy>
			<proxy location=""EAC"">eac-p4edge-fb.eac.ad.ea.com:1999</proxy>
			<proxy location=""EARS"">ears-p4proxy2.rws.ad.ea.com:2001</proxy>
		</server>
		<server address=""go-perforce.rws.ad.ea.com:4487"" charset=""utf8"">
			<proxy location=""EAC"">eac-p4proxy.eac.ad.ea.com:4487</proxy>
			<proxy location=""EARS"">go-perforce.rws.ad.ea.com:4487</proxy>
		</server>
	</servers>
</map>
";

		private string TestDir;
		private string ProxyMapFilePath;
		private ProxyMap ProxyMap;

		[TestInitialize]
		public void TestInitialize()
		{
			TestDir = TestUtil.CreateTestDirectory("ProxyMapTests");
			ProxyMapFilePath = Path.Combine(TestDir, "P4ProtocolOptimalProxyMap.xml");
			File.WriteAllText(ProxyMapFilePath, TempProxyMap);
			ProxyMap = ProxyMap.Load(ProxyMapFilePath);
		}

		[TestCleanup]
		public void TestCleanup()
		{
			if (File.Exists(ProxyMapFilePath)) File.Delete(ProxyMapFilePath);
			if (Directory.Exists(TestDir)) Directory.Delete(TestDir);
			ProxyMap = null;
		}

		[TestMethod]
		public void StockholmLocationTest()
		{
			ProxyMap.Location location = ProxyMap.GetIPLocation(StockholmIP);

			Assert.IsTrue(location.Name == "DiceStockholm");
		}

		[TestMethod]
		public void StockholmOptimalTest()
		{
			ProxyMap.Location location = ProxyMap.GetIPLocation(StockholmIP);
			ProxyMap.Server server = ProxyMap.GetOptimalServer("dice-p4edge-fb.dice.ad.ea.com:2001", location);

			Assert.IsTrue(server.Address == "dice-p4edge-fb.dice.ad.ea.com:2001");
			Assert.IsTrue(server.CharSet == P4Server.P4CharSet.none);
			Assert.IsTrue(server.Name == "frostbite");
		}

		[TestMethod]
		public void OriginCharSetTest()
		{
			ProxyMap.Location location = ProxyMap.GetIPLocation(EACIP);
			ProxyMap.Server server = ProxyMap.GetOptimalServer("go-perforce.rws.ad.ea.com:4487", location);

			Assert.IsTrue(server.Address == "eac-p4proxy.eac.ad.ea.com:4487");
			Assert.IsTrue(server.CharSet == P4Server.P4CharSet.utf8);
		}

		[TestMethod]
		public void ListOptimalServers()
		{
			List<ProxyMap.Server> servers = ProxyMap.ListOptimalServers(new[] { EACIP });

			List<string> serverAddresses = new List<string>();
			foreach (ProxyMap.Server server in servers)
			{
				serverAddresses.Add(server.Address);
			}

			Assert.IsTrue(serverAddresses.Count == 2);
			Assert.IsTrue(serverAddresses.Contains("eac-p4edge-fb.eac.ad.ea.com:1999"));
			Assert.IsTrue(serverAddresses.Contains("eac-p4proxy.eac.ad.ea.com:4487"));
		}
	}
}