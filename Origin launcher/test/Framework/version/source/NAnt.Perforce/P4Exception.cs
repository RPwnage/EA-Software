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
using System.Linq;
using System.Text.RegularExpressions;
#if DEBUG
using System.Reflection;
#endif
using NAnt.Perforce.Internal;


namespace NAnt.Perforce
{
	public class P4AggregateException : P4Exception
	{
		public readonly ReadOnlyCollection<P4Exception> InnerExceptions;

		internal P4AggregateException(P4Exception[] innerExceptions)
			: base(JoinMessages(innerExceptions), GetOutputFirst(innerExceptions))
		{
			InnerExceptions = new ReadOnlyCollection<P4Exception>(innerExceptions.ToList());
		}

		private static string JoinMessages(P4Exception[] innerExceptions)
		{
			return String.Join("\n\n", innerExceptions.Select(ie => ie.Message));
		}

		private static P4Output GetOutputFirst(P4Exception[] innerExceptions)
		{
			if (innerExceptions.Length < 2)
			{
				throw new InvalidOperationException("P4AggregateException must aggregate at least two exceptions!"); // forces us to throw individual
				// exception singularly which is nicer for top level reporting
			}
			if (innerExceptions.Any(ex => ex is P4AggregateException))
			{
				throw new InvalidOperationException("P4AggregateException cannot aggregate P4AggregateExceptions!"); // unlike normal aggregate exceptions
				// we only want to be one level deep as these represent aggregates of errors from single p4 call
			}
			return innerExceptions.First().LinkedOutput;
		}
	}

	[Serializable]
	public class P4Exception : Exception
	{
#if DEBUG
		static P4Exception()
		{
			// exception types that can be created from exception regex need to expose a constructor that takes a string message and a
			// P4Output object, in debug builds ensure these constructor exist during static construction to catch these at runtime
			foreach (Type type in ExceptionRegexs.Values)
			{
				ConstructorInfo cInfo = type.GetConstructor
				(
					BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public,
					null,
					new Type[] { typeof(string), typeof(P4Output) },
					new ParameterModifier[] { new ParameterModifier(1), new ParameterModifier(1) }
				);
				if (cInfo == null)
				{
					throw new NotImplementedException("Dynamically created exception type missing correct constructor signature!");
				}
			}
		}
#endif

		internal P4Output LinkedOutput;

		public P4Exception(string message)
			: this(message, null)
		{
		}

		internal P4Exception(string message, P4Output output = null) 
			: base(FormatMessage(message, output)) 
		{
			LinkedOutput = output;
		}


		// it makes sense to allow these errors through sometimes, so we catch them and throw and special exceptions which can be ignored or not
		private readonly static Regex AllRevisionsIntegratedRegex = new Regex(@".*all revision\(s\) already integrated", RegexOptions.IgnoreCase);
		private readonly static Regex ChangelistDoesNotExistRegex = new Regex(@".*Change \d+ unknown", RegexOptions.IgnoreCase);
		private readonly static Regex FilesNotInClientRegex = new Regex(@".*file\(s\) not in client view", RegexOptions.IgnoreCase);
		private readonly static Regex InvalidOrUnsetPasswordRegex = new Regex(@".*Perforce password \(P4PASSWD\) invalid or unset", RegexOptions.IgnoreCase);
		private readonly static Regex InvalidPasswordRegex = new Regex(@".*Password invalid", RegexOptions.IgnoreCase);
		private readonly static Regex InvalidUserRegex = new Regex(@".*User .* doesn't exist", RegexOptions.IgnoreCase);
		private readonly static Regex MergesPendingRegex = new Regex(@".*merges still pending", RegexOptions.IgnoreCase);
		private readonly static Regex MustReferToClientRegex = new Regex(@".* - must refer to client '.*'", RegexOptions.IgnoreCase);
		private readonly static Regex NoFilesOpenedRegex = new Regex(@".*file\(s\) not opened on this client", RegexOptions.IgnoreCase);
		private readonly static Regex NoFilesRegex = new Regex(@".*no file\(s\) at", RegexOptions.IgnoreCase);
		private readonly static Regex NoFilesToReconcileRegex = new Regex(@".*no file\(s\) to reconcile", RegexOptions.IgnoreCase);
		private readonly static Regex NoFilesToResolve = new Regex(@".*no file\(s\) to resolve", RegexOptions.IgnoreCase);
		private readonly static Regex NoFilesToSubmitRegex = new Regex(@".*no files to submit", RegexOptions.IgnoreCase);
        private readonly static Regex NoFilesToUnshelveRegex = new Regex(@".*no file\(s\) to unshelve", RegexOptions.IgnoreCase);
		private readonly static Regex NoShelvedFilesToDelegeRegex = new Regex(@".*No shelved files in changelist to delete", RegexOptions.IgnoreCase);
		private readonly static Regex NoSuchFilesRegex = new Regex(@".*no such file\(s\)", RegexOptions.IgnoreCase);
		private readonly static Regex NoSuchStreamRegex = new Regex(@".*no such stream", RegexOptions.IgnoreCase);
		private readonly static Regex NoSuchUsersRegex = new Regex(@".*no such user\(s\)", RegexOptions.IgnoreCase);
		private readonly static Regex NotSyncErrorRegex = new Regex(@".*file\(s\) up-to-date", RegexOptions.IgnoreCase);
		private readonly static Regex UnicodeServerRegex = new Regex(@"Unicode clients require a unicode enabled server\.", RegexOptions.IgnoreCase);

		public static Dictionary<Regex, Type> ExceptionRegexs = new Dictionary<Regex, Type>()
		{
			{ AllRevisionsIntegratedRegex, typeof(RevisionsAlreadyIntegratedException) },
			{ ChangelistDoesNotExistRegex, typeof(ChangelistDoesNotExistException) },
			{ FilesNotInClientRegex, typeof(FilesNotInClientException) },
			{ InvalidOrUnsetPasswordRegex, typeof(InvalidOrUnsetPasswordException) },
			{ InvalidPasswordRegex, typeof(InvalidPasswordException) },
			{ InvalidUserRegex, typeof(InvalidUserException) },
			{ MergesPendingRegex, typeof(MergesPendingException) },
			{ MustReferToClientRegex, typeof(MustReferToClientException) },
			{ NoFilesOpenedRegex, typeof(NoFilesOpenException) },
			{ NoFilesRegex, typeof(NoSuchFilesException) },
			{ NoFilesToReconcileRegex, typeof(NoFilesToReconcileException) },
			{ NoFilesToResolve, typeof(NoFilesToResolveException) },
			{ NoFilesToSubmitRegex, typeof(NoFilesToSubmitException) },
            { NoFilesToUnshelveRegex, typeof(NoFilesToUnshelveException) },
			{ NoShelvedFilesToDelegeRegex, typeof(NoShelvedFilesToDeleteException) },
			{ NoSuchFilesRegex, typeof(NoSuchFilesException) },
			{ NoSuchStreamRegex, typeof(NoSuchStreamException) },
			{ NoSuchUsersRegex, typeof(NoSuchUsersException) },
			{ NotSyncErrorRegex, typeof(FilesUpToDateException) },
			{ UnicodeServerRegex, typeof(RequiresUnicodeServerException) }
		};

		// perforce does not treat encoding directives as errors but we want to, so these have special logic
		private readonly static Regex RequiresCommandCharsetRegex = new Regex(@"p4 can not support a wide charset", RegexOptions.IgnoreCase);
		private readonly static Regex UnrecognizedEncodingRegex = new Regex(@"Character set must be one of:", RegexOptions.IgnoreCase);

		public static Dictionary<Regex, Type> EncodingErrorRegexes = new Dictionary<Regex, Type>()
		{
			{ RequiresCommandCharsetRegex, typeof(CommandCharSetRequiredException) },
			{ UnrecognizedEncodingRegex, typeof(UnrecognizedEncodingException) }
		};

		// convenience function for condensing aggregate and non-aggregate p4 exception into an array
		public P4Exception[] Flatten()
		{
			if (this is P4AggregateException)
			{
				return ((P4AggregateException)this).InnerExceptions.ToArray();
			}
			return new P4Exception[] { this };
		}

		// will strip out instances of the given exception type from a flattened p4 exception
		// and if there any exception left will throw the the remainder, takes a condition also
		// as type stripping is usually conditional
		internal void ThrowStrippedIfNotOnly(bool condition, Type type)
		{
			if (condition)
			{
				throw this;
			}

			P4Exception[] stripped = Flatten().Where(inner => !type.IsInstanceOfType(inner)).ToArray();
			if (stripped.Length == 1)
			{
				throw stripped.First();
			}
			else if (stripped.Length > 1)
			{
				throw new P4AggregateException(stripped);
			}
		}

		// multi-type overload
		internal void ThrowStrippedIfNotOnly(IEnumerable<Type> types)
		{
			P4Exception[] stripped = Flatten().Where(inner => !ExceptionIsOneOfTypes(inner, types)).ToArray();
			if (stripped.Length == 1)
			{
				throw stripped.First();
			}
			else if (stripped.Length > 1)
			{
				throw new P4AggregateException(stripped);
			}
		}

		private static string FormatMessage(string message, P4Output output)
		{
			if (output != null)
			{
				string responseFileDetails = output.ResponseFileArgs.Any() ? "\nResponseFile: " + String.Join("\n", output.ResponseFileArgs) : String.Empty;
				string inputDetails = !String.IsNullOrEmpty(output.Input) ? "\nInput: " + String.Join("\n", output.Input) : String.Empty;
				string outputDetails = output.Messages.Any() ? "\nOutput: " + String.Join("\n", output.Messages) : String.Empty;
				string previousCommandDetails = output.CommandHistory.Any() ? "\nPrevious:\n\t" + String.Join("\n\t", output.CommandHistory) : String.Empty;

				return String.Format
				(
					"{0}\n\nCommand: {1}{2}{3}{4}{5}", 
					message, 
					output.Command,
					responseFileDetails,
					inputDetails,
					outputDetails,
					previousCommandDetails
				);
			}
			return message;
		}

		private static bool ExceptionIsOneOfTypes(P4Exception p4Ex, IEnumerable<Type> types)
		{
			return types.Any(t => t.IsInstanceOfType(p4Ex));
		}
	}

	// run exception is a catch-all for errors we don't recognise from p4 call exceptions
	public class RunException : P4Exception
	{
		internal RunException(string error, P4Output output)
			: base(error, output)
		{
		}
	}

	// response from login command came back in format we didn't recognise
	public class LoginOutputFormatException : P4Exception
	{
		internal LoginOutputFormatException(P4Output output)
			: base("Error running login. No ticket in output!", output)
		{
		}
	}

	#region Credential Exceptions
	// base for exceptions that deal with bad passwords, for users to catch this class of exception when logging in
	public abstract class CredentialException : P4Exception
	{
		internal CredentialException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	// base for exceptions that deal with bad passwords, for users to catch this class of exception when logging in
	public abstract class PasswordException : CredentialException
	{
		internal PasswordException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class InvalidUserException : CredentialException
	{
		internal InvalidUserException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	// only return from login attempt with bad password
	public class InvalidPasswordException : PasswordException
	{
		// returns true if auth is backed by LDAP and LDAP validation failed - this could be due to
		// incorrect password *OR* due to account lockout in AD
		public bool IsLDAPException { get { return LinkedOutput.Errors.Contains("'ldap' validation failed: "); } }

		internal InvalidPasswordException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	// returned from commands other than login when provided credentials are incorrect
	// or no credentials and there are implicit credentials (ie env vars, login tickets)
	public class InvalidOrUnsetPasswordException : PasswordException
	{
		internal InvalidOrUnsetPasswordException(string message, P4Output output)
			: base(message, output)
		{
		}
	}
	#endregion

	#region Encoding Exceptions
	// base for exceptions that deal with encoding issues
	public abstract class EncodingException : P4Exception
	{
		internal EncodingException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class CommandCharSetRequiredException : EncodingException
	{
		internal CommandCharSetRequiredException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class UnrecognizedEncodingException : EncodingException
	{
		internal UnrecognizedEncodingException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class RequiresUnicodeServerException : EncodingException
	{
		internal RequiresUnicodeServerException(string message, P4Output output)
			: base(message, output)
		{
		}
	}
	#endregion

	public class ClientOptionsParsingException : P4Exception
	{
		internal ClientOptionsParsingException(string clientOptions)
			: base(String.Format("Unable to parse client options '{0}'!", clientOptions), null)
		{
		}
	}

	public class ClientAlreadyExistException : P4Exception
	{
		internal ClientAlreadyExistException(string clientName)
			: base(String.Format("Client '{0}' already exists!", clientName), null)
		{
		}
	}

	public class ClientDoesNotExistException : P4Exception
	{
		internal ClientDoesNotExistException(string clientName)
			: base(String.Format("Client '{0}' does not exist!", clientName), null)
		{
		}
	}

	public class DepotAlreadyExistException : P4Exception
	{
		internal DepotAlreadyExistException(string depotName)
			: base(String.Format("Depot '{0}' already exists!", depotName), null)
		{
		}
	}

	public class DepotDoesNotExistException : P4Exception
	{
		internal DepotDoesNotExistException(string depotName)
			: base(String.Format("Depot '{0}' does not exist!", depotName), null)
		{
		}
	}

	public class StreamPathParsingException : P4Exception
	{
		internal StreamPathParsingException(string streamPath)
			: base(String.Format("Unable to parse stream path '{0}'!", streamPath), null)
		{
		}
	}

	public class StreamAlreadyExistException : P4Exception
	{
		internal StreamAlreadyExistException(string streamName)
			: base(String.Format("Stream '{0}' already exists!", streamName), null)
		{
		}
	}

	public class NoSuchStreamException : P4Exception
	{
		internal NoSuchStreamException(string streamName)
			: base(String.Format("Stream '{0}' does not exist!", streamName), null)
		{
		}

		internal NoSuchStreamException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class NoSuchFilesException : P4Exception
	{
		internal NoSuchFilesException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class RevisionsAlreadyIntegratedException : P4Exception
	{
		internal RevisionsAlreadyIntegratedException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class NoFilesToResolveException : P4Exception
	{
		internal NoFilesToResolveException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class MergesPendingException : P4Exception
	{
		internal MergesPendingException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class MustReferToClientException : P4Exception
	{
		internal MustReferToClientException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class NoFilesToReconcileException : P4Exception
	{
		internal NoFilesToReconcileException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class NoFilesToSubmitException : P4Exception
	{
		internal NoFilesToSubmitException(string message, P4Output output)
			: base(message, output)
		{
		}
	}
    public class NoFilesToUnshelveException : P4Exception
    {
        internal NoFilesToUnshelveException(string message, P4Output output)
            : base(message, output)
        {
        }
    }
    
    public class NoShelvedFilesToDeleteException : P4Exception
	{
		internal NoShelvedFilesToDeleteException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class FilesUpToDateException : P4Exception
	{
		internal FilesUpToDateException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class NoFilesOpenException : P4Exception
	{
		internal NoFilesOpenException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class FilesNotInClientException : P4Exception
	{
		internal FilesNotInClientException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class ChangelistDoesNotExistException : P4Exception
	{
		internal ChangelistDoesNotExistException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class UserDoesNotExistException : P4Exception
	{
		internal UserDoesNotExistException(string userName)
			: base(String.Format("User '{0}' does not exist!", userName), null)
		{
		}
	}

	public class UserAlreadyExistException : P4Exception
	{
		internal UserAlreadyExistException(string userName)
			: base(String.Format("User '{0}' already exists!", userName), null)
		{
		}
	}

	public class NoSuchUsersException : P4Exception
	{
		internal NoSuchUsersException(string message, P4Output output)
			: base(message, output)
		{
		}
	}

	public class CannotEditSubmittedChangelistException : P4Exception
	{
		internal CannotEditSubmittedChangelistException(uint changelist)
			: base(String.Format("Cannot edit submitted changelist '{0}'.", changelist), null)
		{
		}
	}

	public class InvalidChangelistSpecifiedException : P4Exception
	{
		internal InvalidChangelistSpecifiedException(string badCLSpecifier)
			: base(String.Format("Invalid CL specification '{0}'. Must be a valid changelist ID or 'default'.", badCLSpecifier), null)
		{
		}
	}
	
	public abstract class UnsupportedFeatureException : P4Exception
	{
		internal UnsupportedFeatureException(string supportError)
			: base(supportError, null)
		{
		}

		protected static string BuildExceptionMessage(string supportType, P4Version version, string feature, uint requiredMajorVersion, uint requiredMinorVersion, uint? requiredRevision = null)
		{
			string currentVersionString = String.Format("{0}.{1} - {2}", version.MajorVersion, version.MajorVersion, version.Revision);
			string requiredVersionString = String.Format("{0}.{1}{2}", requiredMajorVersion, requiredMinorVersion, requiredRevision.HasValue ? String.Format(" - {0}", requiredRevision) : String.Empty);
			return String.Format("{0} (Version {1}) does not support {2}. Please use at least version {3}.", supportType, currentVersionString, feature, requiredVersionString);
		}
	}

	public class ClientDoesNotSupportFeatureException : UnsupportedFeatureException
	{
		internal ClientDoesNotSupportFeatureException(P4Server server, string feature, uint requiredMajorVersion, uint requiredMinorVersion, uint? requiredRevision = null)
			: base(BuildExceptionMessage("Client", server.ClientVersion, feature, requiredMajorVersion, requiredMinorVersion, requiredRevision))
		{
		}
	}

	public class ServerDoesNotSupportFeatureException : UnsupportedFeatureException
	{
		internal ServerDoesNotSupportFeatureException(P4Server server, string feature, uint requiredMajorVersion, uint requiredMinorVersion, uint? requiredRevision = null)
			: base(BuildExceptionMessage("Server", server.ServerVersion, feature, requiredMajorVersion, requiredMinorVersion, requiredRevision))
		{
		}
	}

	public class UnknownVersionException : P4Exception
	{
		internal UnknownVersionException(string versionString)
			: base(String.Format("Unsupported p4 version. Could not parse version string '{0}'.", versionString), null)
		{
		}
	}

	public class TimeOutException : P4Exception
	{
		internal TimeOutException(string p4Command, int timeoutValue)
			: base(String.Format("Perforce operation timed out in {0} ms.\nCommand:{1}\n\n", timeoutValue, p4Command))
		{
		}
	}
}