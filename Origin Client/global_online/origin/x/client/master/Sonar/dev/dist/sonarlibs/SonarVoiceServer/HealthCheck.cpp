#include "HealthCheck.h"
#include <stdio.h>
#include <fcntl.h>

#if !defined(_WIN32)
#include <unistd.h>
#else
#include <io.h>
#endif

#if defined(_WIN32)

inline FILE *popen(const char *command, const char *mode)
{
    return _popen(command, mode);
}

inline int pclose(FILE* stream)
{
    return _pclose(stream);
}

//inline char *read(char *str, int n, FILE *stream)
//{
//    return fgets(str, n, stream);
//}

#endif

namespace sonar
{

HealthCheck::HealthCheck (Config &config, BackendClient* backendClient)
: m_state(HC_NONE)//(REGISTERED)
, m_healthState(HC_PENDING)
, m_nextHealthCheck(common::getTimeAsMSEC())
, m_backendClient(backendClient)
, m_registered(false)
, m_file(NULL)
, m_fileDescriptor(0)
, m_pingTimeTaken(0)
, m_pingStartTime(0)
, m_config(config)
{
#ifndef _WIN32
	char hostname[32];
	snprintf(hostname, 32, "%s", m_backendClient->getAddress());
	char *domain = strtok(hostname, ":");
	
	snprintf(m_command, 128, "curl -s -w \"%%{http_code} %%{time_total}\\n\" -o /dev/null http://%s:%d/voiceedge/health_check", domain, m_config.backendhealthport);
	//snprintf(m_command, 128, "curl -s -w \"%%{http_code} %%{time_total}\\n\" -o /dev/null %s", "https://www.google.com");
	SYSLOGX(LOG_DEBUG, "health check command = %s\n", m_command);
#endif

	writeConfigJson();
	writeHealthJson(m_healthState); // PENDING
#if 0 // DEBUG
	m_nextHealthCheck += m_config.healthcheckinterval;
#endif
}

HealthCheck::~HealthCheck (void)
{

}

void HealthCheck::update ()
{
#if 1 // disable if you want to debug
	// do not update until the VoiceServer has been registered with the VoiceEdge
	if( !m_registered )
	{
		//fprintf (stderr, "HealthCheck::update(), !m_registered\n");
		AbstractOutboundClient::CLIENTSTATE backendState = m_backendClient->getState();
		if( backendState == AbstractOutboundClient::CS_REGISTERED )
		{
			m_registered = true;
			m_state = HC_REGISTERED;
			m_nextHealthCheck = common::getTimeAsMSEC(); // do initial check right away
		}
		
		if( !m_registered )
			return;
	}
#endif

	// check if we are due for a health check
	long long int now = common::getTimeAsMSEC();
	if( now < m_nextHealthCheck )
		return;

	//SYSLOGX(LOG_DEBUG, "now = %lld\n", now);
	
	switch( m_state )
	{
	case HC_REGISTERED:
	{
		//SYSLOGX(LOG_DEBUG, "HC_REGISTERED\n");
		m_state = HC_PING_START;
	} // deliberately fall through

	case HC_PING_START:
	{
		//SYSLOGX(LOG_DEBUG, "HC_PING_START\n");
		int err = pingVoiceEdge();
		if( err == 0 )
		{
			m_state = HC_PING_PENDING;
			m_healthState = HC_OK;
			// deliberately fall through
		}
		else
		{
			m_state = HC_WRITE;
			m_healthState = HC_FAIL;
			break;
		}
	}

	case HC_PING_PENDING:
	{
		//SYSLOGX(LOG_DEBUG, "HC_PING_PENDING\n");
		char buf[16];
		memset(buf, 0, 16);

#ifdef _WIN32
        int r = read(m_fileDescriptor, buf, 16);
#else
		ssize_t r = read(m_fileDescriptor, buf, 16);
#endif
		if( r == -1 && errno == EAGAIN )
		{
			break; // stay in PING_PENDING
		}

		// received data
		int http_code = 0;
		if( r > 0 )
		{
			//SYSLOGX(LOG_DEBUG, "HC_PING_PENDING, r = %ld\n", r);
			buf[15] = '\0'; // make sure that strtok() will not overrun 'buf'
			char *ptr = strtok(buf, " ");
			http_code = (ptr == NULL) ? 0 : atoi(ptr);	// Will return zero if no integral number is found after leading whitespace.
									// Anything after the integral number, if any, will be discarded
			if( http_code != 200 )
			{
				m_healthState = HC_FAIL;
				SYSLOGX(LOG_DEBUG, "voiceEdge response code = [%d]\n", http_code);
			}

			ptr = strtok(NULL, " ");
			m_pingTimeTaken = (ptr == NULL) ? 0 : int(atof(ptr) * 1000.f);
			m_state = HC_PING_COMPLETE;
		}
		else // error
		{
			SYSLOGX(LOG_ERR, "HC_PING_PENDING, r = %ld, errno = %d\n", r, errno);
			m_healthState = HC_FAIL;
			m_pingTimeTaken = 0;
			m_state = HC_PING_COMPLETE;
		}
		// deliberately fall through
	}

	case HC_PING_COMPLETE:
	{
		//SYSLOGX(LOG_DEBUG, "HC_PING_COMPLETE\n");
		pclose(m_file);
		m_state = HC_WRITE;
	} // deliberately fall through

	case HC_WRITE:
	{
		//SYSLOGX(LOG_DEBUG, "HC_WRITE\n");
		if( m_backendClient->getState() == AbstractOutboundClient::CS_DISCONNECTED )
		{
			SYSLOGX(LOG_DEBUG, "HC_WRITE, CS_DISCONNECTED\n");
			m_healthState = HC_FAIL;
			m_registered = false;
		}

		writeHealthJson(m_healthState);
		m_nextHealthCheck = common::getTimeAsMSEC() + m_config.healthcheckinterval;
		//SYSLOGX(LOG_DEBUG, "HC_WRITE, m_nextHealthCheck = %lld\n", m_nextHealthCheck);
		m_state = HC_REGISTERED; // NONE
	}
	
	default:
	  break;
	}
}

void HealthCheck::updateBackendEventInterval(int interval)
{
    if( (interval > 0) && (m_config.backendeventinterval != interval) )
    {
        m_config.backendeventinterval = interval;
        writeConfigJson(); // update with new interval
    }
}

int HealthCheck::pingVoiceEdge()
{
    int err = 0;

#if !defined(_WIN32)
	struct timeval tv;
	err = gettimeofday(&tv, NULL);
	if( err == 0 )
	{
		//SYSLOGX(LOG_DEBUG, "m_command = %s\n", m_command);
		m_pingStartTime = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
		m_file = popen(m_command, "r");
		m_fileDescriptor = fileno(m_file);
		fcntl(m_fileDescriptor, F_SETFL, O_NONBLOCK);
	}
#endif
	
	return err;
}

void HealthCheck::writeConfigJson()
{
#ifndef _WIN32
	const char *healthCheckLocation = "Node/sonar-voiceserver/";
	char filename[256];
	snprintf(filename, 256, "%sconfig.json", healthCheckLocation);
	
	FILE *f = fopen(filename, "w");
	if( f != NULL )
	{
		fprintf(f,
			"{\"config\": {\n"
			"  \"public\": \"%s\",\n"
			"  \"voipaddress\": \"%s\",\n"
			"  \"voipport\": \"%d\",\n"
			"  \"backendaddress\": \"%s\",\n"
			"  \"maxclients\": \"%d\",\n"
			"  \"healthcheckinterval\": \"%d\",\n"
			"  \"backendeventinterval\": \"%d\",\n"
			"  \"metricsfileinterval\": \"%d\",\n"
			"  \"clientstatsfileinterval\": \"%d\",\n"
            "  \"badchannelpercent\": \"%d\",\n"
            "  \"badclientpercent\": \"%d\",\n"
            "  \"jitterbuffersize\": \"%d\",\n"
            "  \"jitterscalefactor\": \"%d\"\n"
			"}}\n",
			m_config.publicip,			/* public */
			m_config.voipaddress,			/* voipaddress */
			m_config.voipport,			/* voipport */
			m_backendClient->getAddress(),		/* backendaddress */
			m_config.maxclients,			/* maxclients */
			m_config.healthcheckinterval,		/* healthcheckinterval */
			m_config.backendeventinterval,		/* backendeventinterval */
			m_config.metricsfileinterval,		/* metricsfileinterval */
			m_config.clientstatsfileinterval,	/* clientstatsfileinterval */
			m_config.badchannelpercent,		/* badchannelpercent */
			m_config.badclientpercent,		/* badclientpercent */
			m_config.jitterbuffersize,		/* jitterbuffersize */
			m_config.jitterscalefactor		/* jitterscalefactor */
		);

		fclose(f);
	}
#endif
}

void HealthCheck::writeHealthJson(HealthState state)
{
#ifndef _WIN32
	const char *healthCheckLocation = "Node/sonar-voiceserver/";
	char filename[256];
	char status[8];
	snprintf(filename, 256, "%shealth_check.json", healthCheckLocation);
	
	switch(state)
	{
	case HC_PENDING:
		snprintf(status, 8, "PENDING");
		break;
	case HC_OK:
		snprintf(status, 8, "OK");
		break;
	default:
		snprintf(status, 8, "FAIL");
	};

	FILE *f = fopen(filename, "w");
	if( f != NULL )
	{
		fprintf(f,
			"{\"health\": {\n"
			"  \"name\": \"Sonar Voice Server\",\n"
			"  \"version\": \"%s\",\n"
			"  \"overallServicesStatus\": \"%s\",\n"
			"  \"services\": [\n"
			"     { \"service\": {\n"
			"         \"name\": \"voiceEdge\",\n"
			"         \"status\": \"%s\",\n"
			"         \"soft\": \"false\",\n"
			"         \"statusDetail\": {\n"
			"           \"node\": {\n"
			"             \"url\": \"%s\",\n"
			"             \"status\": \"%s\",\n"
			"             \"lastCheck\": %lld,\n"
			"             \"timeTaken\": %d,\n"
			"             \"info\": \"\"\n"
			"           }\n"
			"         }\n"
			"       }\n"
			"     }\n"
			"   ]\n"
			"}}\n",
			"1.0.0",			/* version */
			status,				/* overallServicesStatus */
			status,				/* service.status */
			m_backendClient->getAddress(),	/* service.statusDetail.node.url */
			status,				/* service.statusDetail.node.status */
			m_pingStartTime,		/* service.statusDetail.node.lastCheck */
			m_pingTimeTaken			/* service.statusDetail.node.timeTaken */
		);

		fclose(f);
	}
#endif
}

}
