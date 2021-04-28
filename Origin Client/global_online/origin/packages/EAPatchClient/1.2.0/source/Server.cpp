///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Server.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Server.h>
#include <EAPatchClient/Debug.h>


namespace EA
{
namespace Patch
{


Server::Server()
  : msURLBase()
  , msDirectoryURL()
{
}


Server::Server(const Server& other)
{
    operator=(other);
}


Server::~Server()
{
}


Server& Server::operator=(const Server& other)
{
    if(&other != this)
    {
        msDirectoryURL = other.msDirectoryURL;
        // others here as they are made.
    }

    return *this;
}


const char8_t* Server::GetURLBase() const
{
    return msURLBase.c_str();
}


void Server::SetURLBase(const char8_t* pURLBase)
{
    EAPATCH_ASSERT(pURLBase);
    msURLBase = pURLBase;
}


const char8_t* Server::GetPatchDirectoryURL() const
{
    return msDirectoryURL.c_str();
}


void Server::SetPatchDirectoryURL(const char8_t* pURL)
{
    EAPATCH_ASSERT(pURL);
    msDirectoryURL = pURL;
}




///////////////////////////////////////////////////////////////////////////////
// GetServer / SetServer
///////////////////////////////////////////////////////////////////////////////


Server* gpServer = NULL;


EAPATCHCLIENT_API Server* GetServer()
{
    return gpServer;
}


EAPATCHCLIENT_API void SetServer(Server* pServer)
{
   gpServer = pServer;
}




} // namespace Patch
} // namespace EA







