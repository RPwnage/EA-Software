#include "Swarm.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <list>
#include <SonarClient/win32/providers.h>
#include <TestUtils/TokenSigner.h>

static size_t getNumCPUCores()
{
#ifdef _WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	return (size_t) sysinfo.dwNumberOfProcessors;
#else
	return (size_t) sysconf( _SC_NPROCESSORS_ONLN );
#endif
}

static void usage(const char *reason = NULL)
{
	if (reason)
	{
		fprintf (stderr, "%s\n", reason);
	}

	fprintf (stderr, 
		"Arguments:\n"
		"<serverIP:port> <idRange>-<idRange>\n"
		"-ct <AverageConnectionLifeTime in seconds>\n"
		"-cr <connectRate per second>\n"
		"-loss <loss percent 0-100>\n"
		"-tf <frames of talk per user>\n"
        "-cpc <clients per channel>\n"
		"-et <execution time in miliseconds\n"
		"-sdr : Show disconnect reason\n"
		"-upk : Enables use of sonarmaster.prv in user home to sign tokens\n"
        "-block <number of users to block in channel>\n"
        "-sc : Show client connections\n"
		"\n");
}

int main (int argc, char **argv)
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);

    // Uncomment this to redirect stdio to files (Windows-only).
    //sonar::common::RedirectStdioToFile();
#endif

	sonar::TokenSigner *tokenSigner = NULL;

	Swarm::Config config;
	config.showDisconnectReason = false;
	config.talkFrames = 50 * 5;
    config.showClientConnections = false;

	argc --;
	argv ++;

	fprintf (stderr, "Sonar Voice Server Load Test Client\n\n");

	if (argc < 2)
	{
		usage();
		return -1;
	}

	char serverAddress[256];
	int serverPort;
	int userRangeStart;
	int userRangeEnd;
	
	char *colonPtr = strchr(argv[0], ':');
	
	if (colonPtr == NULL)
	{
		usage("Server IP and port invalid argument: Expected <IPv4Address>:<port>");
		return 1;
	}
	
	strncpy (serverAddress, argv[0], colonPtr - argv[0]);
	serverAddress[colonPtr - argv[0]] = '\0';
	colonPtr ++;
	
	if (sscanf (colonPtr, "%d", &serverPort) < 1)
	{
		usage("Server IP and port invalid argument: Expected <IPv4Address>:<port>");
		return -1;
	}

	char *dashPtr = strchr(argv[1], '-');

	
	if (dashPtr == NULL)
	{
		usage("User range start and end invalid argument: Expected <int>-<int>");
		return 1;
	}

	if (sscanf (argv[1], "%d", &userRangeStart) < 1 ||
		sscanf (dashPtr + 1, "%d", &userRangeEnd) < 1)
	{
		usage("User range start and end invalid argument: Expected <int>-<int>");
		return -1;
	}

	argc -= 2;
	argv += 2;

	bool usePrivateKey = false;
	int packetloss = 0;
	int connectRate = 50;
	int connectTime = 86400;
	long long executionTime = -1;
    config.clientsPerChannel = 4; // default
    config.blockedUserCount = 0; // default

	while (argc > 0)
	{
		if (strcmp(argv[0], "-cr") == 0)
		{
			connectRate = atoi (argv[1]);
			argc -= 2;
			argv += 2;
		} 
		else
		if (strcmp(argv[0], "-loss") == 0)
		{
			packetloss = atoi (argv[1]);
			argc -= 2;
			argv += 2;

		}
		else
		if (strcmp(argv[0], "-ct") == 0)
		{
			connectTime = atoi (argv[1]);
			argc -= 2;
			argv += 2;
		}
		else
		if (strcmp(argv[0], "-sdr") == 0)
		{
			config.showDisconnectReason = true;
			argc -= 1;
			argv += 1;
		}
		else
		if (strcmp(argv[0], "-tf") == 0)
		{
			config.talkFrames = atoi(argv[1]);
			argc -= 2;
			argv += 2;
		}
        else
        if (strcmp(argv[0], "-cpc") == 0)
        {
            config.clientsPerChannel = atoi(argv[1]);
            argc -= 2;
            argv += 2;
        }
		else
		if (strcmp(argv[0], "-et") == 0)
		{
			executionTime = atoi(argv[1]);
			argc -= 2;
			argv += 2;
		}
		else
		if (strcmp(argv[0], "-upk") == 0)
		{
			usePrivateKey = true;
			argc -= 1;
			argv += 1;
		}
		else
		if (strcmp(argv[0], "-h") == 0)
		{
			usage();
			return 0;
		}
        else
        if (strcmp(argv[0], "-block") == 0)
        {
            config.blockedUserCount = atoi(argv[1]);
            argc -= 2;
            argv += 2;
        }
        else
        if (strcmp(argv[0], "-sc") == 0)
        {
            config.showClientConnections = true;
            --argc;
            ++argv;
        }
    }

	sonar::String mbPrefix;

	if (getenv("COMPUTERNAME") == NULL)
	{
		if (getenv("HOSTNAME") == NULL)
		{
		    char hostname[256];
		    if (gethostname(hostname, 256) != 0)
		    {
			throw "HOSTNAME or COMPUTERNAME environment variables must be set";
		    }
		    
		    mbPrefix = hostname;
		}
		else
		{
		    mbPrefix = getenv("HOSTNAME");
		}
	}
	else
	{
		mbPrefix = getenv("COMPUTERNAME");
	}
	
	sonar::String prefix = mbPrefix;



	fprintf (stderr, "Talking to voice server on %s:%d\n", serverAddress, serverPort);
	fprintf (stderr, "Talk frames per user/min: %d\n", config.talkFrames);
	fprintf (stderr, "Unique prefix from hostname: %s\n", mbPrefix.c_str());
	fprintf (stderr, "Using user id range %d-%d\n", userRangeStart, userRangeEnd);
	fprintf (stderr, "Average Connection time: %d seconds\n", connectTime);
	fprintf (stderr, "Packet loss: %d%%\n", packetloss);
	fprintf (stderr, "Connect rate: %d/sec\n", connectRate);
	fprintf (stderr, "Clients per channel: %d\n", config.clientsPerChannel);
	fprintf (stderr, "Blocked user count per channel: %d\n", config.blockedUserCount);
    fprintf (stderr, "Using private key for tokens: %s\n", usePrivateKey ? "Yes": "No");

	if (executionTime > -1)
		fprintf (stderr, "Execution time: %lld msec\n", executionTime);
	
	if (usePrivateKey)
	{
		tokenSigner = new sonar::TokenSigner();
	}

	sonar::Udp4Transport::m_rxPacketLoss = packetloss;
	sonar::Udp4Transport::m_txPacketLoss = packetloss;

	config.connectionLengthSec = connectTime;
	config.connectsPerSec = connectRate;

	if (connectTime > 0)
    {
		config.disconnectBackoff = connectTime / 10;
        if (config.disconnectBackoff == 0)
            config.disconnectBackoff = 10;
    }
	else
		config.disconnectBackoff = 10;

	config.hostPrefix = prefix.c_str();

	Swarm *swarm = new Swarm(userRangeStart, userRangeEnd, userRangeStart, serverAddress, serverPort, config, usePrivateKey ? tokenSigner : NULL, executionTime);
	swarm->run();
	return 0;
}
