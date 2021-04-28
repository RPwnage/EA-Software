#include <QDir>
#include <QStringList>
#include <QtConcurrentRun>

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "engine/cloudsaves/LocalStateCalculator.h"
#include "engine/cloudsaves/FileFingerprint.h"
#include "engine/cloudsaves/SaveFileCrawler.h"
#include "engine/cloudsaves/PathSubstituter.h"

#include "engine/content/ContentController.h"
#include "engine/content/CloudContent.h"
#include "engine/content/LocalContent.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves 
{
#if !defined(ORIGIN_MAC)

    LocalAutoreleasePool::LocalAutoreleasePool() {};
    LocalAutoreleasePool::~LocalAutoreleasePool() {};

#endif

    void LocalStateCalculator::run()
    {
        LocalAutoreleasePool localPool;
        QList<QFuture<bool>> backgroundFingerprints;

        mCalculatedState = new LocalStateSnapshot(mPreviousSync.lineage());

        // We need these to help crawl save files
        PathSubstituter substituter(mEntitlement);

        QList<EligibleFile> eligibleFiles = SaveFileCrawler::findEligibleFiles(mEntitlement, substituter);

        for(auto it = eligibleFiles.constBegin();
            it != eligibleFiles.constEnd();
            it++) 
        {
            const QFileInfo &info = (*it).info();
            const QString path(info.absoluteFilePath());
            LocalInstance::FileTrust trust = (*it).isTrusted() ? LocalInstance::Trusted : LocalInstance::Untrusted; 

            if (!info.isFile())
            {
                // Don't sync this
                continue;
            }

            // See if we can import the file from our cache
            if (!mCalculatedState->importLocalFile(info, trust, mPreviousSync))
            {
                // Calculate the fingerprint in the background
                QFuture<bool> fingerprintFuture = QtConcurrent::run(this, &LocalStateCalculator::fingerprintAndAddPath, path, trust);
                backgroundFingerprints << fingerprintFuture;
            }
        }

        // Wait for all background fingerprints to finish
        // This is important even if we encounter an error so they still refer to a valid mCalculatedState
        bool fingerprintingSucceeded = true;

        for(auto it = backgroundFingerprints.constBegin();
            it != backgroundFingerprints.constEnd();
            it++)
        {
            // Make sure we always evaluate it->result()
            // Otherwise we won't wait for the future to finish
            fingerprintingSucceeded = it->result() && fingerprintingSucceeded;
        }

        mCalculatedState->setValid(fingerprintingSucceeded);

        emit finished(mCalculatedState);
    }

    bool LocalStateCalculator::fingerprintAndAddPath(const QString &path, LocalInstance::FileTrust trust)
    {
        QFile toFingerprint(path);
        if (toFingerprint.open(QIODevice::ReadWrite))
        {
            // Generate the fingerprint
            mCalculatedState->addLocalFile(toFingerprint, trust, FileFingerprint::fromFile(toFingerprint));
            return true;
        }
        else
        {
            ORIGIN_LOG_WARNING << "Unable to open " << path << " for cloud save fingerprinting. Aborting local state calculation";
            return false;
        }
    }
}
}
}
