#if PLATFORM == "common"
    #define PLATFORM_UPPERCASE "COMMON"
#elif PLATFORM == "pc"
    #define PLATFORM_UPPERCASE "PC"
#elif PLATFORM == "xone"
    #define PLATFORM_UPPERCASE "XONE"
#elif PLATFORM == "ps4"
    #define PLATFORM_UPPERCASE "PS4"
#elif PLATFORM == "nx"
    #define PLATFORM_UPPERCASE "NX"
#elif PLATFORM == "ios"
    #define PLATFORM_UPPERCASE "IOS"
#elif PLATFORM == "android"
    #define PLATFORM_UPPERCASE "ANDROID"
#elif PLATFORM == "ps5"
    #define PLATFORM_UPPERCASE "PS5"
#elif PLATFORM == "xbsx"
    #define PLATFORM_UPPERCASE "XBSX"
#elif PLATFORM == "steam"
    #define PLATFORM_UPPERCASE "STEAM"
#elif PLATFORM == "stadia"
    #define PLATFORM_UPPERCASE "STADIA"
#endif

#include "commondefines.cfg"
#include "titledefines.cfg"

// hostedPlatforms controls what platforms are running on this server.
// For shared platform deployments, you can add/remove platforms by setting the XPLAY_HOST_ flags to 1 or 0 in titledefines.cfg.
// (For single cluster deployments, the XPLAY_HOST_ flags are not used to set hostedPlatforms; however, they are still used to
// exclude unused databaseConnections in databases.cfg, and are therefore set to 0 in titledefines.cfg when PLATFORM != "common".)
// 
// If you want to restrict the platforms that can interact with each other, set platformRestrictions in framework/usersessions.cfg
#if PLATFORM == "common"
    #define PLATFORM_SEPARATOR ""
    hostedPlatforms = [
        #if XPLAY_HOST_PC == "1"
          "pc"
           #define PLATFORM_SEPARATOR ","
        #endif
        #if XPLAY_HOST_XONE == "1"
          #PLATFORM_SEPARATOR#"xone"
          #define PLATFORM_SEPARATOR ","
        #endif
        #if XPLAY_HOST_XBSX == "1"
          #PLATFORM_SEPARATOR#"xbsx"
          #define PLATFORM_SEPARATOR ","
        #endif
        #if XPLAY_HOST_PS4 == "1"
          #PLATFORM_SEPARATOR#"ps4"
          #define PLATFORM_SEPARATOR ","
        #endif
        #if XPLAY_HOST_PS5 == "1"
          #PLATFORM_SEPARATOR#"ps5"
          #define PLATFORM_SEPARATOR ","
        #endif
        #if XPLAY_HOST_NX == "1"
          #PLATFORM_SEPARATOR#"nx"
          #define PLATFORM_SEPARATOR ","
        #endif
        #if XPLAY_HOST_IOS == "1"
          #PLATFORM_SEPARATOR#"ios"
          #define PLATFORM_SEPARATOR ","
        #endif
        #if XPLAY_HOST_ANDROID == "1"
          #PLATFORM_SEPARATOR#"android"
        #endif
        #if XPLAY_HOST_STEAM == "1"
          #PLATFORM_SEPARATOR#"steam"
        #endif
        #if XPLAY_HOST_STADIA == "1"
          #PLATFORM_SEPARATOR#"stadia"
        #endif
    ]
#else
    hostedPlatforms = [ "#PLATFORM#" ]
#endif

configuredPlatform = "#PLATFORM#"

#include "components.cfg"
#include "logging.cfg"

#ifndef SSL_KEY
#define SSL_KEY "/home/gos-ops/cert/gos2015/key.pem"
#endif

#ifndef SSL_CERT
#define SSL_CERT "/home/gos-ops/cert/gos2015/cert.pem"
#endif

#ifndef INTERNAL_SSL_KEY
#define INTERNAL_SSL_KEY "/home/gos-ops/cert/gos2015/key.pem"
#endif

#ifndef INTERNAL_SSL_CERT
#define INTERNAL_SSL_CERT "/home/gos-ops/cert/gos2015/cert.pem"
#endif

#ifndef USE_LOCALHOST_NETWORKING
interfaces = {
    internal = "10.0.0.0"
    external = "159.153.0.0"
}
#else
interfaces = {
    internal = "127.0.0.1"
    external = "127.0.0.1"
}
#endif

// Define the list of networks that are trusted by the blaze server.  Only clients from trusted
// networks can make connections to things like the master server endpoints.
// Information on IP Addresses and Subnet: https://developer.ea.com/display/blaze/IP+addresses+explained
#define TRUSTED_NETWORKS "\"127.0.0.1\", \"10.0.0.0/8\", \"192.168.0.0/16\", \"172.16.0.0/12\", \"159.153.0.0/16\", \"35.172.92.48\", \"54.145.3.191\", \"54.162.109.254\", \"3.94.38.50\", \"100.24.108.204\", \"3.94.225.164\",\"35.169.85.61\", \"52.20.163.155\", \"54.158.107.61\"" 
#define IPV4_INTERNAL_ADDRESSES "\"10.0.0.0/8\", \"172.16.0.0/12\", \"192.168.0.0/16\", \"127.0.0.1\""

// WARNING:
// Adding a new instance in the ServerConfigs will require full restart of the cluster (Not a ZDT/rolling restart deploy)
//
serverConfigs = {
    configMaster = {
        instanceType = "CONFIG_MASTER"
        components = ["framework"]
        endpoints = "masterEndpoints"
        serviceImpact = "TRANSIENT"
    }
    coreNSMaster = {
        instanceType = "AUX_MASTER"
        components = ["framework", "coreNonSharded"]
        endpoints = "auxMasterEndpoints"       
        serviceImpact = "TRANSIENT"
    }    
    coreSlave = {
        instanceType = "SLAVE"
        components = ["usersessions_master", "framework_slave", "framework", "core", "coreNonSharded", "osdk_custom"]
        endpoints = "slaveEndpoints"
    }
    coreMaster = {
        instanceType = "AUX_MASTER"
        components = ["framework", "core", "osdk_custom"]
        endpoints = "auxMasterEndpoints"
        serviceImpact = "TRANSIENT"
    }
    grSlave = {
        instanceType = "AUX_SLAVE"
        components = ["framework_slave", "framework", "gamereporting"]
        endpoints = "auxSlaveEndpoints"
    }
    searchSlave = {
        instanceType = "AUX_SLAVE"
        components = ["framework_slave", "framework", "search"]
        endpoints = "auxSlaveEndpoints"
    }
    mmSlave = {
        instanceType = "AUX_SLAVE"
        components = ["framework_slave", "framework", "matchmaker"]
        endpoints = "auxSlaveEndpoints"
    }
    osdkAuxMaster = {
        instanceType = "AUX_MASTER"
        components = ["framework_slave", "framework", "osdk_custom_aux", "fifa_custom"]
        endpoints = "auxMasterEndpoints"
        serviceImpact = "TRANSIENT"
    }
    osdkAuxSlave = {
        instanceType = "AUX_SLAVE"
        components = ["framework_slave", "framework", "osdk_custom_aux", "fifa_custom_with_stats"]
        endpoints = "auxSlaveEndpoints"
        immediatePartialLocalUserReplication = true
    }
    gdprAuxSlave = {
        instanceType = "AUX_SLAVE"
        components = ["framework", "gdpr"]
        endpoints = "auxSlaveEndpoints"
    }
    pkrMaster = {
        instanceType = "AUX_MASTER"
        components = ["framework", "gamepacker"]
        endpoints = "auxMasterEndpoints"
        serviceImpact = "TRANSIENT"
    }
    pkrSlave = {
        instanceType = "AUX_SLAVE"
        components = ["framework", "gamepacker"]
        endpoints = "auxSlaveEndpoints"
    }
}

redisConfig = {
#if SERVER_PLATFORM == "Windows"
  maxActivationRetries = 10
#else
  maxActivationRetries = 0
#endif

  clusters = {
#ifndef ARSON
    #include "../init/redis.cfg"
#else
    #include "../init/redis#ARSON_INSTANCE_ID#.cfg"
#endif
  }
  aliases = {
    framework = "main"
  }
}

instanceIdAllocatorConfig = {
    redisRefreshInterval = "10s"
    redisTTL = "2m"
    maxRedisRefreshFailures = 0
}
