

namespace EA.PackageSystem.ConsolePackageManager
{
	public struct CredStoreOptions
	{

		public bool UsePackageServer { get; }
		public string PathToCredstoreFile { get; }
		public string OutputFilepath { get; }
		public bool OutputTokenToConsole { get; }
		public string Server { get; }
		public string Username { get; }
		public string Password { get; }

		public CredStoreOptions(bool usePackageServer, string pathToCredstore, string outputFilepath, bool outputTokenToConsole, string server, string username, string password)
		{
			UsePackageServer = usePackageServer;
			PathToCredstoreFile = pathToCredstore;
			OutputFilepath = outputFilepath;
			OutputTokenToConsole = outputTokenToConsole;
			Server = server;
			Username = username;
			Password = password;
		}
	}
}
