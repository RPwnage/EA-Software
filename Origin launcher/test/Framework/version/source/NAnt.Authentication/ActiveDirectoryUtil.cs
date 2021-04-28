using System;
using System.IO;
using System.Reflection;

namespace NAnt.Authentication
{		
	public static class ActiveDirectoryUtil
	{
		// try to detect if an active directory account is locked out, assuming user name contains domain
		// and user name and we have access to the domain in order to make the query
		public static bool TryDetectADAccountLockedOut(string fullUserName, out bool locked)
		{
			Assembly assem = null;
			try
			{
				// this isn't supported in most versions of mono, so check if assembly can be resolved before trying to use it
				assem = Assembly.Load("System.DirectoryServices.AccountManagement, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089");
			}
			catch (FileNotFoundException)
			{
				locked = false;
				return false;
			}
			return TryDetectedADAccountLockedOutAssemblyResolved(fullUserName, assem, out locked);
		}

		// On OSX machine this function won't get executed.  But in order to compile this code on an OSX machine, we cannot
		// reference System.DirectoryServices directly or the code won't compile.  So we have to use reflection to execute 
		// this code if we want to allow Framework to build and debug on OSX host using "Visual Studio 2019 for Mac".
		private static bool TryDetectedADAccountLockedOutAssemblyResolved(string fullUserName, Assembly accountManagementAssm, out bool locked)
		{
			string user = null;
			string domain = null;
			if (AuthenticationUtil.SplitDomainAndUser(fullUserName, out user, out domain))
			{
				try
				{
					Type principalContextType = accountManagementAssm.GetType("System.DirectoryServices.AccountManagement.PrincipalContext", false);
					Type enumContextType = accountManagementAssm.GetType("System.DirectoryServices.AccountManagement.ContextType", false);
					if (principalContextType != null && enumContextType != null)
					{
						FieldInfo contextTypeDomainEnumField = enumContextType.GetField("Domain");
						object contextTypeDomainEnum = contextTypeDomainEnumField == null ? null : contextTypeDomainEnumField.GetValue(enumContextType);

						ConstructorInfo principalContextTypeCtor = principalContextType.GetConstructor(new[] { enumContextType, typeof(string) });
						object domainPrincipalContext = (principalContextTypeCtor==null || contextTypeDomainEnum==null) ? null : principalContextTypeCtor.Invoke(new object[] { contextTypeDomainEnum, domain });
						if (domainPrincipalContext != null)
						{
							using ((IDisposable)domainPrincipalContext)
							{
								Type userPrincipalType = accountManagementAssm.GetType("System.DirectoryServices.AccountManagement.UserPrincipal", false);
								Type enumIdentityType = accountManagementAssm.GetType("System.DirectoryServices.AccountManagement.IdentityType", false);
								if (userPrincipalType != null && enumIdentityType != null)
								{
									FieldInfo indentityTypeSamAccountNameEnumField = enumIdentityType.GetField("SamAccountName");
									object identityTypeSamAccountNameEnum = indentityTypeSamAccountNameEnumField==null ? null : indentityTypeSamAccountNameEnumField.GetValue(enumIdentityType);

									MethodInfo findByIdentityMethod = userPrincipalType.GetMethod("FindByIdentity", new[] { principalContextType, enumIdentityType, typeof(string) });
									object userPrincipal = (findByIdentityMethod==null || identityTypeSamAccountNameEnum==null) ? null : findByIdentityMethod.Invoke(null, new object[] { domainPrincipalContext, identityTypeSamAccountNameEnum, user });

									if (userPrincipal != null)
									{
										using ((IDisposable) userPrincipal)
										{
											MethodInfo isAccountLockedOutMethod = userPrincipalType.GetMethod("IsAccountLockedOut");
											object isAcctLockedOut = isAccountLockedOutMethod==null ? null : isAccountLockedOutMethod.Invoke(userPrincipal, null);
											if (isAcctLockedOut != null && isAcctLockedOut is bool)
											{
												locked = (bool)isAcctLockedOut;
												return true;
											}
										}
									}
								}
							}
						}
					}
				}
				catch (Exception ex)
				{
					Type principalServerDownExceptionType = accountManagementAssm.GetType("System.DirectoryServices.AccountManagement.PrincipalServerDownException", false);
					if (principalServerDownExceptionType != null && ex.GetType() == principalServerDownExceptionType)
					{
						// don't throw exception if domain cannot be reached
					}
					else
					{
						throw;
					}
				}
			}
			locked = false;
			return false;
		}
	}
}