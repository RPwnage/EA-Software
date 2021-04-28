//    Copyright © 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Hans van Veenendaal

#include "services/downloader/Common.h"
#include "Install.h"
#include "engine/content/Entitlement.h"
#include "services/debug/DebugService.h"

#include <QCoreApplication>
#include <QThreadPool>

using namespace Origin::Downloader;

Install::Install(Origin::Engine::Content::EntitlementRef entitlement) :
    QObject(entitlement.data())
{
    mEntitlement = entitlement;
}

bool Install::start(const Origin::Downloader::ExecuteInstallerRequest& req)
{
    mInstalling = Origin::Services::IProcess::createNewProcess(req.getInstallerPath(), req.getCommandLineArguments(), req.getCurrentDirectory(), "", true, false, true, false, req.useProxy(), false, this);
    ORIGIN_VERIFY_CONNECT_EX(mInstalling, SIGNAL(finished(uint32_t, int, Origin::Services::IProcess::ExitStatus)), this, SLOT(onProcessFinished(uint32_t, int, Origin::Services::IProcess::ExitStatus)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mInstalling, SIGNAL(error(Origin::Services::IProcess::ProcessError, quint32)), this, SLOT(onProcessError(Origin::Services::IProcess::ProcessError, quint32)), Qt::QueuedConnection);
    mInstalling->start();

	return true;
}

void Install::onProcessError(Origin::Services::IProcess::ProcessError error, quint32 systemErrorValue)
{
    emit installFailed(-1);
}

void Install::onProcessFinished(uint32_t pid, int exitCode, Origin::Services::IProcess::ExitStatus status)
{
	if (exitCode != 0)
	{
		emit installFailed(exitCode);
	}
	else
	{
		emit installFinished();
	}
}
