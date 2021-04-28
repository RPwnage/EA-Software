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

using System.IO;
using System.Threading;
using System.Threading.Tasks;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.Logging;

namespace NAnt.Authentication.Test
{
	[TestClass]
	public class SimpleTimeoutTest
	{
		[TestMethod]
		public void TimeoutTest()
		{
			string testCredStoreFile = Path.Combine(Path.GetDirectoryName(CredentialStore.GetDefaultCredStoreFilePath()), "test_credstore.dat");
			using (new CredStoreTest(testCredStoreFile))
			{
                // call retrieve or prompt
                const int timeout = 1;
				Task<Credential> credentialTask = Task.Run<Credential>
				(
					() => CredentialStore.Prompt(Log.Default, "noentryforthisserver", timeout, testCredStoreFile)
				);

				// suspend for more than long enough for dialogue to time out
				Thread.Sleep((timeout + 2) * 1000);

				// before assert checks, see if task faulted on exception and call wait to throw it instead of hitting assert errors
				if (credentialTask.IsCompleted && credentialTask.IsFaulted)
				{
					credentialTask.Wait();
				}

				Assert.IsTrue(credentialTask.IsCompleted); // check task completed succesfully
				Assert.IsTrue(credentialTask.Result == null); // if we timeout looking for a credential we're didn't have an entry for we should get null back
			}
		}
	}
}