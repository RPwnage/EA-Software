#ifndef STEAM_HELPER_H
#define STEAM_HELPER_H

#include "Ignition/Ignition.h"
namespace Ignition
{
    class SteamHelper {
        // Predefined mapping of personas for steam accounts 
        // https://eadpjira.ea.com/browse/EADPCOMMERCEBUGS-100299
        inline static eastl::string steamNexusPersonas[] = {
            "nexus001#6Qw4pNC4RVzRW6W",
            "nexus002#xZf8tnWTWVWV2Yp",
            "nexus003#tdMYZJMV9JtyvfB",
            "nexus004#R2sgXQqhHKCXZWy",
            "nexus005#KBKKsBGNnRnQnBG",
            "nexus006#ztwtVnR95fW7BJP",
            "nexus007#K8pZ87GVCVVz2Kd",
            "nexus008#xysJdqHXBtwV78s",
            "nexus009#Nc5rZPQPw5DtKMR",
            "nexus010#R67WMp7yvH7WvWD",
            "nexus011#65g9GPKpmXPttFB",
            "nexus012#GKmj5cHwDFpHGKf",
            "nexus013#S6dscfGpKSzfhwp",
            "nexus014#FQNc6NwVDxdvZJf"
        };

    public:

        inline static const char8_t* getPersonaFromNexusPersona(const char8_t* nexusPersona)
        {
            for (auto& persona : steamNexusPersonas)
            {
                if (persona.find(nexusPersona) != eastl::string::npos)
                {
                    return persona.c_str();
                }
            }
            return nexusPersona;
        }
    };
}
#endif