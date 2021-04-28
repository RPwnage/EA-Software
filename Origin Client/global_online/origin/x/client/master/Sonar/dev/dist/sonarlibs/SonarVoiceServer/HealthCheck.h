#pragma once
#include <SonarCommon/Common.h>
#include "BackendClient.h"
#include <stdio.h>

#define ENABLE_HEALTH_CHECK 1

namespace sonar
{
#if ENABLE_HEALTH_CHECK
	class HealthCheck
	{
	public:
		struct Config
		{
			const char *publicip;
			const char *voipaddress;
			int voipport;
			int healthcheckinterval;
			int backendeventinterval;
			int backendhealthport;
			int metricsfileinterval;
			int clientstatsfileinterval;
			int badchannelpercent;
			int badclientpercent;
            int jitterbuffersize;
            int jitterscalefactor;
			int maxclients;
			
			Config()
			: publicip(NULL)
			, voipaddress(NULL)
			, voipport(0)
			, healthcheckinterval(0)
			, backendeventinterval(0)
			, backendhealthport(0)
			, metricsfileinterval(0)
			, clientstatsfileinterval(0)
			, badchannelpercent(0)
			, badclientpercent(0)
            , jitterbuffersize(0)
            , jitterscalefactor(0)
            , maxclients(0) {}
		};
		
		HealthCheck (Config &config, BackendClient* backendClient);
		~HealthCheck (void);
		void update();
        void updateBackendEventInterval(int interval);

	protected:
		enum State
		{
			HC_NONE = 0,
			HC_REGISTERED,
			HC_PING_START,
			HC_PING_PENDING,
			HC_PING_COMPLETE,
			HC_WRITE
		};
		
		enum HealthState
		{
			HC_PENDING,
			HC_OK,
			HC_FAIL
		};
		
	private:
		int pingVoiceEdge();
		void writeConfigJson();
		void writeHealthJson(HealthState state);

	private:
		State m_state;
		HealthState m_healthState;
		long long m_nextHealthCheck;
		BackendClient *m_backendClient;
		bool m_registered; // VoiceServer has registered with VoiceEdge
		FILE *m_file;
		int m_fileDescriptor;
		int m_pingTimeTaken;
		long long m_pingStartTime;
		char m_command[128];
		Config m_config;
	};
#endif // ENABLE_HEALTH_CHECK
}
