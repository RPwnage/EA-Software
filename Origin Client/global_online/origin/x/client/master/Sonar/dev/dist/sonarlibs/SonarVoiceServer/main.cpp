#include <sstream>
#include <sys/types.h>

#if __APPLE__
#include <signal.h>
#endif

#ifndef _WIN32
#include <sys/wait.h>
#endif

#include "Server.h"

#if defined(_DEBUG) && (_WIN32)
#include "Stackwalker.h"
#endif

using namespace sonar;

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

static void PrintUsage (const char *_pstrReason)
{
		fprintf (stderr, 
			"%s\n"
			"Usage:\n"
			"-voipaddress [address/ANY] -voipport [port]\n"
			"\n"
			"For Sonar BACKEND authentication mode, these arguments must be set:\n"
			"-backendaddress [address:port]\n"
			"\n"
			"Optional:\n"
			"-public / -publicip [address] <- Public IPv4 address or hostname of voice service\n"
			"-verbose <- Verbose output\n"
			"-nosleep <- No sleep in server main loop\n"
			"-maxclients [number] <- Maximum clients recommended for this server. 256 to 65535\n"
			"-location [string]   <- Geographical location of this server\n"
			"-instances [number]  <- Number of forked instances to start (using voipPort + X) (POSIX only)\n"
			"-shutdowndelay [number sec] <- Causes server to shutdown after certain number of seconds, used for testing\n"
			"-chanstats [1/0] <- Makes server log channel decomission stats to file and console\n"
			"-gnuPlot [1/0] <- Prints out metrics to stdout so that they can be plotted in real-time\n"
			"-audioloss [number] <- audio loss percent [0-100]\n"
			"-loadtest [1/0] <- outputs combined stats for load testing\n"
			"-healthcheckinterval [ms] <- number of milliseconds between writing out health check JSON file\n"
			"-backendeventinterval [ms] <- number of milliseconds between sending metrics to the Backend Server (i.e., Voice Edge)\n"
			"-backendhealthport [port] <- port for pinging the Backend Server (i.e., Voice Edge)\n"
			"-metricsfileinterval [ms] <- number of milliseconds between writing out metrics JSON file\n"
			"-clientstatsfileinterval [ms] <- number of milliseconds between writing out channel and client JSON files\n"
			"-printoutinterval [ms] <- number of milliseconds between writing out metrics to stderr\n"
			"-badchannelpercent [0-100] <- percentage of bad clients in a channel before considering channel to be bad\n"
            "-badclientpercent [0-100] <- percentage of badness (loss%% + oos%% + jitter%%) before considering client to be bad (default to 3)\n"
            "-jitterbuffersize [size] <- size of server side jitter buffer. 0 - disabled\n"
            "-jitterscalefactor [number] <- number, S, to be used as a divisor in the jitter%% calculation, e.g., jitterMean / S (default to 2000)\n",
			_pstrReason);
}

int main (int argc, char **argv)
{
#if defined(_DEBUG) && (_WIN32)
	InitAllocCheck(ACOutput_XML);
#endif

	fprintf (stderr, 
		"Sonar Voice Server %d.%d.%d-%s\n"
		"Copyright 2006-2013 ESN Social Software AB. ALL RIGHTS RESERVED.\n"
		"Protocol version %ld\n\n",
		Server::VERSION_MAJOR,
		Server::VERSION_MINOR,
		Server::VERSION_PATCH,
		Server::VERSION_NAME,
		sonar::protocol::PROTOCOL_VERSION);

	fprintf (stderr, "This product includes software developed by the OpenSSL Project\n for use in the OpenSSL Toolkit (http://www.openssl.org/)\nCopyright (c) 1998-2011 The OpenSSL Project.  All rights reserved.\n\n");

	argv ++;
	argc --;

	const char *pstrVoipAddress = "ANY";
	const char *pstrVoipPort = "22990";
	const char *pstrMaxClients = "16384";
	const char *pstrPublicHostname = "";
	const char *pstrBackendAddress = "127.0.0.1:22991";
	const char *pstrLocation = "";
	const char *pstrShutdownDelay = "0";
	const char *pstrChanStats = "0";
	const char *pstrGnuPlot = "0";
	const char *pstrAudioLoss = "0";
	const char *pstrLoadTest = "0";
#if ENABLE_HEALTH_CHECK
	const char *pstrHealthCheckInterval = "0";
#endif
	const char *pstrBackendEventInterval = "0";
	const char *pstrBackendHealthPort = "0";
	const char *pstrMetricsFileInterval = "0";
	const char *pstrClientStatsFileInterval = "0";
	const char *pstrPrintoutInterval = "0";
	const char *pstrBadChannelPercent = "0";
	const char *pstrBadClientPercent = "0";
    const char *pstrJitterBufferSize = "0";
    const char *pstrJitterScaleFactor = "0";

	int cInstances = 1;
	size_t numCores = getNumCPUCores();

	if (numCores > 0)
	{
		if (numCores > 8)
		{
			fprintf (stderr, "More than 8 CPU cores detected, clamping at 8\n");
			numCores = 8;
		}

		cInstances = (int) numCores;
	}
	
	if (argc > 0)
	{
		if (strcmp (argv[0], "--help") == 0 ||
			strcmp (argv[0], "-h") == 0)
		{
			PrintUsage("Help requested");
			return 0;
		}
	}

	for (int index = 0; index < argc - 1; index ++)
	{
		if (strcmp (argv[index], "-instances") == 0)
		{
			cInstances = atoi (argv[index + 1]);
		}
		else
		if (strcmp (argv[index], "-voipaddress") == 0)
		{
			pstrVoipAddress = argv[index + 1];
		}
		else
		if (strcmp (argv[index], "-voipport") == 0)
		{
			pstrVoipPort = argv[index + 1];
		}
		else
		if (strcmp (argv[index], "-backendaddress") == 0)
		{
			pstrBackendAddress = argv[index + 1];
		}
		else
		if (strcmp (argv[index], "-maxclients") == 0)
		{
			pstrMaxClients = argv[index + 1];
		}
		else
		if (strcmp (argv[index], "-location") == 0)
		{
			pstrLocation = argv[index + 1];
		}
		else
		if (strcmp (argv[index], "-public") == 0)
		{
			pstrPublicHostname = argv[index + 1];
		}
		else
		if (strcmp (argv[index], "-publicip") == 0)
		{
			pstrPublicHostname = argv[index + 1];
		}
		else
		if (strcmp (argv[index], "-shutdowndelay") == 0)
		{
			pstrShutdownDelay = argv[index + 1];
		}
		else
		if (strcmp (argv[index], "-chanstats") == 0)
		{
			pstrChanStats = argv[index + 1];
		}
		else
		if (strcmp (argv[index], "-gnuPlot") == 0)
		{
			pstrGnuPlot = argv[index + 1];
		}
		else
		if (strcmp(argv[index], "-audioloss") == 0)
		{
			pstrAudioLoss = argv[index + 1];
		}
		else
		if (strcmp(argv[index], "-loadtest") == 0)
		{
			pstrLoadTest = argv[index + 1];
		}
#if ENABLE_HEALTH_CHECK
		else
		if (strcmp(argv[index], "-healthcheckinterval") == 0)
		{
			pstrHealthCheckInterval = argv[index + 1];
		}
#endif
		else
		if (strcmp(argv[index], "-backendeventinterval") == 0)
		{
			pstrBackendEventInterval = argv[index + 1];
		}
		else
		if (strcmp(argv[index], "-backendhealthport") == 0)
		{
			pstrBackendHealthPort = argv[index + 1];
		}
		else
		if (strcmp(argv[index], "-metricsfileinterval") == 0)
		{
			pstrMetricsFileInterval = argv[index + 1];
		}
		else
		if (strcmp(argv[index], "-clientstatsfileinterval") == 0)
		{
			pstrClientStatsFileInterval = argv[index + 1];
		}
		else
		if (strcmp(argv[index], "-printoutinterval") == 0)
		{
			pstrPrintoutInterval = argv[index + 1];
		}
		else
		if (strcmp(argv[index], "-badchannelpercent") == 0)
		{
			pstrBadChannelPercent = argv[index + 1];
		}
		else
		if (strcmp(argv[index], "-badclientpercent") == 0)
		{
			pstrBadClientPercent = argv[index + 1];
		}
        else
        if (strcmp(argv[index], "-jitterbuffersize") == 0)
        {
            pstrJitterBufferSize = argv[index + 1];
        }
        else
        if (strcmp(argv[index], "-jitterscalefactor") == 0)
        {
            pstrJitterScaleFactor = argv[index + 1];
        }
    }

	Server::Config config;
	setvbuf(stderr, NULL, _IONBF, 0);

	config.channelStatsLogging = atoi(pstrChanStats) > 0;
	config.shutdownDelay = atoi (pstrShutdownDelay);
	config.gnuPlot = atoi(pstrGnuPlot) > 0;
	config.audioLoss = atoi(pstrAudioLoss);
	config.loadTest = atoi(pstrLoadTest) > 0;
    config.jitterBufferSize = atoi(pstrJitterBufferSize);
    config.jitterScaleFactor = atoi(pstrJitterScaleFactor);

#if ENABLE_HEALTH_CHECK
	config.healthCheckInterval = atoi(pstrHealthCheckInterval);
	if( config.healthCheckInterval == 0 )
		config.healthCheckInterval = Server::HEALTHCHECK_INTERVAL_MSEC;
#endif
	config.backendEventInterval = atoi(pstrBackendEventInterval);
	if( config.backendEventInterval == 0 )
		config.backendEventInterval = Server::BACKEND_EVENT_INTERVAL_MSEC;
	
	config.backendHealthPort = atoi(pstrBackendHealthPort);
	if( config.backendHealthPort == 0 )
		config.backendHealthPort = Server::BACKEND_HEALTH_PORT;

	config.metricsFileInterval = atoi(pstrMetricsFileInterval);
	if( config.metricsFileInterval == 0 )
		config.metricsFileInterval = Server::METRICS_FILE_INTERVAL_MSEC;

	config.clientStatsFileInterval = atoi(pstrClientStatsFileInterval);
	if( config.clientStatsFileInterval == 0 )
		config.clientStatsFileInterval = Server::CLIENT_STATS_FILE_INTERVAL_MSEC;

	config.printoutInterval = atoi(pstrPrintoutInterval);
	if( config.printoutInterval == 0 )
		config.printoutInterval = Server::PRINTOUT_INTERVAL_MSEC;

	config.badChannelPercent = atoi(pstrBadChannelPercent);
	if( config.badChannelPercent == 0 )
		config.badChannelPercent = Server::BAD_CHANNEL_PERCENT;

    config.badClientPercent = atoi(pstrBadClientPercent);
    if( config.badClientPercent == 0 )
        config.badClientPercent = Server::BAD_CLIENT_PERCENT;

    config.jitterScaleFactor = atoi(pstrJitterScaleFactor);
    if( config.jitterScaleFactor == 0 )
        config.jitterScaleFactor = Server::JITTER_SCALE_FACTOR;

#ifdef _WIN32
	WSADATA wsaData;

	if (WSAStartup (MAKEWORD (2, 0), &wsaData) != 0)
	{
		throw -1;
	} 

	Server *server;

	try
	{
		server = new Server(
			pstrPublicHostname, 
			pstrVoipAddress,
			pstrBackendAddress, 
			pstrLocation,
			atoi (pstrMaxClients),
			atoi (pstrVoipPort),
			config);

		server->run();

	} catch (const Server::StartupException &exp)
	{
		fprintf (stderr, "Startup error: %s\n", exp.what());
		return -1;
	}
	delete server;

#elif __APPLE__
    {
        // attempt to disable SIGPIPE exception on Mac - fwiw, it doesn't seem to work
        //signal(SIGPIPE, SIG_IGN);
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));

        sa.sa_handler = SIG_IGN;

        if (-1 == sigaction(SIGPIPE, &sa, NULL))
        {
            fprintf(stderr, "Unable to install SIGPIPE handler");
        }

        Server *server;
        try
        {
            int basePort = atoi(pstrVoipPort);
            server = new Server(
                pstrPublicHostname, 
                pstrVoipAddress,
                pstrBackendAddress, 
                pstrLocation,
                atoi (pstrMaxClients),
                basePort,
                config);

            server->run();

        } catch (const Server::StartupException &exp)
        {
            fprintf (stderr, "Startup error: %s\n", exp.what());
            return -1;
        }
        delete server;
        exit(0);
    }
#else
	int basePort = atoi(pstrVoipPort);

	for (int index = 0; index < cInstances; index ++)
	{
		fprintf (stderr, "Forking instance %d of %d\n", index, cInstances);

		pid_t pidChild = fork();

		if (pidChild == -1)
		{
			fprintf (stderr, "ERROR: fork() failed (%d)\n", errno);
			continue;
		}

		if (pidChild == 0)
		{
			Server *server;
			try
			{
				server = new Server(
					pstrPublicHostname, 
					pstrVoipAddress,
					pstrBackendAddress, 
					pstrLocation,
					atoi (pstrMaxClients),
					basePort + index,
					config);

				server->run();

			} catch (const Server::StartupException &exp)
			{
				fprintf (stderr, "Startup error: %s\n", exp.what());
				return -1;
			}
			delete server;
			exit(0);
		}
	}

	for (;;)
	{
		int status;
		pid_t done = wait(&status);
		if (done == -1) 
		{
			if (errno == ECHILD) break; // no more child processes
		} 
		else 
		{
			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) 
			{
				fprintf (stderr, "Pid %d failed\n", done);
				exit(1);
			}
		}
	}


#endif

	ERR_free_strings();
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();

#if defined(_DEBUG) && (_WIN32)
	ULONG leakedBytes = DeInitAllocCheck();

	if (leakedBytes > 0)
	{
		fprintf (stderr, "%s: %d leaked bytes detected!\n", __FUNCTION__, leakedBytes);
		assert (leakedBytes == 0);
		getchar();
	}
#endif
	
	return 0;
}

