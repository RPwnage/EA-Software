#include "ExecuteInstallerRequest.h"
#include "services/platform/EnvUtils.h"
#include <QFileInfo>
#include <QDir>

namespace Origin
{
namespace Downloader
{

ExecuteInstallerRequest::ExecuteInstallerRequest(const QString& installerAbsolutePath,
	const QString& commandLineArguments,
	const QString& currentDirectory,
	bool showUI, /* = true */
	bool useProxy, /* = true */
	bool waitForExit /* = true */) :

	mCommandLineArguments(commandLineArguments),
	mShowUI(showUI),
	mUseProxy(useProxy),
	mWaitForExit(waitForExit)
{
	QFileInfo currentFile( installerAbsolutePath );
	QString sTempDir = currentFile.dir().path();
	mInstallerPath = EnvUtils::ConvertToShortPath(sTempDir) + "/" + currentFile.fileName();

	mInstallerPath.replace("\\", "/");

	mCurrentDirectory = currentDirectory;
	mCurrentDirectory.replace("\\", "/");
	if (!mCurrentDirectory.endsWith('/'))
	{
		mCurrentDirectory.append('/');
	}
}

ExecuteInstallerRequest::ExecuteInstallerRequest(const ExecuteInstallerRequest& other)
{
	mInstallerPath = other.mInstallerPath;
	mCommandLineArguments = other.mCommandLineArguments;
	mCurrentDirectory = other.mCurrentDirectory;
	mShowUI = other.mShowUI;
	mUseProxy = other.mUseProxy;
	mWaitForExit = other.mWaitForExit;
}

ExecuteInstallerRequest& ExecuteInstallerRequest::operator=(const ExecuteInstallerRequest& other)
{
	if (this != &other)
	{
		mInstallerPath = other.mInstallerPath;
		mCommandLineArguments = other.mCommandLineArguments;
		mCurrentDirectory = other.mCurrentDirectory;
		mShowUI = other.mShowUI;
		mUseProxy = other.mUseProxy;
		mWaitForExit = other.mWaitForExit;
	}

	return *this;
}

} // namespace Downloader
} // namespace Origin

