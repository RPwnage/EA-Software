///////////////////////////////////////////////////////////////////////////////
// DownloadSession.h
//
// Implements a class that holds telemetry-related session information for a download session
//
// Created by Thomas Bruckschlegel
// Copyright (c) 2010 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef DOWNLOADSESSION_H
#define DOWNLOADSESSION_H

#include "EASTL/string.h"
#include "PersistentSession.h"

class DownloadSession : public PersistentSession
{
public:	
	DownloadSession(const char8_t* productId, const char8_t *startStatus) : 
      PersistentSession(productId), 
      m_startStatus(startStatus),
      m_contentType(""),
      m_packageType(""),
      m_url(""),
      m_bytesDownloaded(0),
      m_bitrateKbps(0),
      m_autoPatch(""),
      m_symlink(false),
      m_nonDipUpgrade(false),
      m_environment("")
	{
	}

	~DownloadSession() {};

    void setEnvironment(const char8_t *environment)     { m_environment = environment; }
    void setStartStatus(const char8_t *startStatus)     { m_startStatus = startStatus; }
    void setContentType(const char8_t *contentType)     { m_contentType = contentType; }
    void setPackageType(const char8_t *packageType)     { m_packageType = packageType; }
    void setUrl(const char8_t *url)                     { m_url = url; }
    void setBytesDownloaded(uint64_t bytesDownloaded)   { m_bytesDownloaded = bytesDownloaded; }
    void setBitrateKbps(uint64_t bitrateKbps)           { m_bitrateKbps = bitrateKbps; }
    void setAutoPatch(const char8_t *autoPatch)         { m_autoPatch = autoPatch; }
    void setSymlink(bool symlink)                       { m_symlink = symlink; }
    void setNonDipUpgrade(bool nonDipUpgrade)           { m_nonDipUpgrade = nonDipUpgrade; }

    const char8_t *environment() const  { return m_environment.c_str(); }
	const char8_t *startStatus() const  { return m_startStatus.c_str(); }
	const char8_t *contentType() const  { return m_contentType.c_str(); }
	const char8_t *packageType() const  { return m_packageType.c_str(); }
	const char8_t *url() const          { return m_url.c_str(); }
	uint64_t bytesDownloaded() const    { return m_bytesDownloaded; }
	uint64_t bitrateKbps() const        { return m_bitrateKbps; }
    const char8_t *autoPatch() const    { return m_autoPatch.c_str(); }
    bool symlink() const                { return m_symlink; }
    bool nonDipUpgrade() const          { return m_nonDipUpgrade; }

private:
	eastl::string8 m_startStatus;
	eastl::string8 m_contentType;
	eastl::string8 m_packageType;
	eastl::string8 m_url;
    uint64_t       m_bytesDownloaded;
    uint64_t       m_bitrateKbps;
    eastl::string8 m_autoPatch;
    bool           m_symlink;
    bool           m_nonDipUpgrade;
    eastl::string8 m_environment;
};

#endif //DOWNLOADSESSION_H