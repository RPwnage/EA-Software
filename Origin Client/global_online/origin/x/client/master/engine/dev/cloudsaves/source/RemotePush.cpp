#include "services/network/NetworkAccessManager.h"

#include "engine/cloudsaves/RemotePush.h"
#include "engine/cloudsaves/ChangeSet.h"
#include "engine/cloudsaves/AuthorizedS3Transfer.h"
#include "engine/cloudsaves/AuthorizedS3TransferBatch.h"
#include "engine/cloudsaves/LastSyncStore.h"
#include "engine/cloudsaves/RemoteUsageMonitor.h"
#include "engine/cloudsaves/PathSubstituter.h"

#include "engine/content/Entitlement.h"
#include "engine/content/CloudContent.h"
#include "engine/content/LocalContent.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"
#include "engine/cloudsaves/RemoteSync.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
	RemotePush::RemotePush(RemoteSync *sync) : 
		AbstractStateTransfer(sync), 
		m_targetState(NULL),
		m_changeSet(NULL)
	{
	}

	RemotePush::~RemotePush()
	{
		delete m_changeSet;
	}

	void RemotePush::run()
	{
		// Calculate our target remote state
		m_targetState = new RemoteStateSnapshot(*remoteSync()->currentLocalState(), remoteSync()->remoteState());

		qint64 usageSize = m_targetState->localSize();
		if (usageSize > remoteSync()->entitlement()->localContent()->cloudContent()->maximumCloudSaveLocalSize())
		{
			emit failed(SyncFailure(QuotaExceededFailure, usageSize, remoteSync()->entitlement()->localContent()->cloudContent()->maximumCloudSaveLocalSize()));

			// TELEMETRY
			GetTelemetryInterface()->Metric_CLOUD_WARNING_SPACE_CLOUD_LOW(remoteSync()->offerId().toUtf8().data(), usageSize);
            
			return;
		}

		// Calculate our changeset
		if (remoteSync()->hasRemoteState())
		{
			m_changeSet = new ChangeSet(*remoteSync()->remoteState(), *m_targetState);
		}
		else
		{
			m_changeSet = new ChangeSet(*m_targetState);
		}

		if (m_changeSet->isEmpty())
		{
			if (remoteSync()->hasRemoteState())
			{
				// Update with our current remote usage
                RemoteUsageMonitor::instance()->setUsageForEntitlement(remoteSync()->entitlement(), 
                                                                       remoteSync()->remoteState()->localSize(),
                                                                       remoteSync()->session()->manifestEtag());

				// Even if we have no files to sync we might need to push the new lineage 
				// UUID to the server so we remember that we've chosen the local lineage. 
				// This suppresses duplicate conflict dialogs.
				if (remoteSync()->currentLocalState()->lineage() == remoteSync()->remoteState()->lineage())
				{
					// Nothing to do
					emit succeeded();
					return;
				}
			}

		}

		// Note: we don't update our remote usage here because it's about to
		// change

		// Upload any changed files 
		if (!m_changeSet->added().isEmpty())
		{
			const QList<SyncableFile> &toUpload = m_changeSet->added();
			AuthorizedS3TransferBatch* batch = new AuthorizedS3TransferBatch(remoteSync()->session(), this);

			// For telemetry
			qint64 totalUploadSize = 0;

			for(QList<SyncableFile>::const_iterator it = toUpload.constBegin();
				it != toUpload.constEnd();
				it++)
			{
				const SyncableFile &syncable = *it;
				
                // OFM-344 - cloud storage cap is increaesed to 1 GB.  We now need to write out temporary 
                // gzipped files, to avoid running out of memory for very large cloud pushes.  The temp zip
                // files will be cleaned up with batch.
                QTemporaryFile *zipFile = new QTemporaryFile(batch);
                zipFile->open();
                
                // Doesn't matter which local path we use
				QString localSource = (*syncable.localInstances().constBegin()).path();
				QUrl remoteUrl = remoteSync()->session()->manifestRelativeUrl(syncable.remotePath());

				AuthorizedS3Upload* uploadTransfer = new AuthorizedS3Upload(localSource, zipFile, remoteUrl, syncable.fingerprint().size());
				batch->addTransfer(uploadTransfer);
				ORIGIN_LOG_EVENT << "Cloud Save: File " << qPrintable(localSource) << " added to be pushed to the cloud";

				totalUploadSize += syncable.fingerprint().size();
			}

			batch->start();
			ORIGIN_VERIFY_CONNECT(batch, SIGNAL(succeeded()), this, SLOT(commitPush()));
			ORIGIN_VERIFY_CONNECT(batch, SIGNAL(failed()), this, SLOT(rollbackPush()));
			ORIGIN_VERIFY_CONNECT(batch, SIGNAL(failed()), batch, SLOT(deleteLater())); 
			ORIGIN_VERIFY_CONNECT(batch, SIGNAL(progress(qint64, qint64)), this, SIGNAL(progress(qint64, qint64)));

			// TELEMETRY to monitor the number of files uploaded, and the total size of the upload.
			GetTelemetryInterface()->Metric_CLOUD_PUSH_MONITORING(remoteSync()->offerId().toUtf8().data(), toUpload.size(), totalUploadSize);
		}
		else
		{
            //even if there is no save files will commit an empty manifest as a save file to optimize the fall back content id case for
            //remote pulls

			// Nothing to do; just commit us
			commitPush();
		}

		exec();
	}
	
	void RemotePush::commitPush()
	{
		// Clean up the upload batch
		AuthorizedS3TransferBatch* uploadBatch = dynamic_cast<AuthorizedS3TransferBatch*>(sender());

		// We can be called directly if there's nothing to upload
		if (uploadBatch)
		{
			uploadBatch->deleteLater();
		}

		// Build the manifest XML
        PathSubstituter substituter(remoteSync()->entitlement());
		QByteArray manifestData = m_targetState->toXml(substituter).toByteArray();

		// Upload the manifest
		AuthorizedS3TransferBatch *manifestBatch = new AuthorizedS3TransferBatch(remoteSync()->session(), this);
		manifestBatch->addTransfer(new AuthorizedS3Put(manifestData, remoteSync()->session()->saveManifestUrl(), "text/xml"));

		ORIGIN_VERIFY_CONNECT(manifestBatch, SIGNAL(transferSucceeded(AuthorizedS3Transfer*, S3Operation*)), this, SLOT(finalizePush(AuthorizedS3Transfer*, S3Operation*)));
		ORIGIN_VERIFY_CONNECT(manifestBatch, SIGNAL(failed()), this, SLOT(rollbackPush()));
		ORIGIN_VERIFY_CONNECT(manifestBatch, SIGNAL(failed()), manifestBatch, SLOT(deleteLater()));
		ORIGIN_VERIFY_CONNECT(manifestBatch, SIGNAL(succeeded()), manifestBatch, SLOT(deleteLater()));

		manifestBatch->start();
	}
		
	void RemotePush::rollbackPush()
	{
		emit failed(SyncFailure(NetworkFailure));
		ORIGIN_LOG_EVENT << "Cloud Save: Remote push failed due to network issues, rolling back";

		// TELEMETRY
		GetTelemetryInterface()->Metric_CLOUD_ERROR_PUSH_FAILED(remoteSync()->offerId().toUtf8().data());
	}

	void RemotePush::finalizePush(AuthorizedS3Transfer *, S3Operation *operation)
	{
		sender()->deleteLater();
		
		// Sync was successful! Update our last sync snapshot
		LastSyncStore::setLastSyncForCloudSavesId(remoteSync()->cloudSavesId(), *remoteSync()->currentLocalState());

		// Find out the ETag Amazon assigned us
		// We can calculate this from the MD5 of the gzipped data but this is less error prone
		QByteArray etag = operation->reply()->rawHeader("ETag");

		// We know the exact remote usage now
		RemoteUsageMonitor::instance()->setUsageForEntitlement(remoteSync()->entitlement(),remoteSync()->currentLocalState()->localSize(), etag);

		if (!m_changeSet->deleted().isEmpty())
		{
			// Clean orphans
			AuthorizedS3TransferBatch* batch = new AuthorizedS3TransferBatch(remoteSync()->session(), this);

			const QList<SyncableFile> &toDelete = m_changeSet->deleted();
			for(QList<SyncableFile>::const_iterator it = toDelete.constBegin();
				it != toDelete.constEnd();
				it++)
			{
				const SyncableFile &syncable = *it;
				QUrl remoteUrl = remoteSync()->session()->manifestRelativeUrl(syncable.remotePath());
				AuthorizedS3Delete* deleteTransfer = new AuthorizedS3Delete(remoteUrl);

				batch->addTransfer(deleteTransfer);
			}
			
			// We don't care about the result
			ORIGIN_VERIFY_CONNECT(batch, SIGNAL(succeeded()), batch, SLOT(deleteLater()));
			ORIGIN_VERIFY_CONNECT(batch, SIGNAL(failed()), batch, SLOT(deleteLater()));            
            
            // We need to wait until the deleting is finished before we emit success and this RemotePush is destroyed, 
            // because RemotePush is a QThread and its destruction causes the destruction of any objects on this thread,
            // including any AuthorizedS3TransferBatch objects created by this RemotePush.
            ORIGIN_VERIFY_CONNECT(batch, SIGNAL(succeeded()), this, SIGNAL(succeeded()));
            ORIGIN_VERIFY_CONNECT(batch, SIGNAL(failed()), this, SIGNAL(succeeded()));

			batch->start();
		}
        else
        {
		    emit succeeded();
        }
        
        ORIGIN_LOG_EVENT << "Cloud Save: Push finalized";

	    // TELEMETRY
		GetTelemetryInterface()->Metric_CLOUD_AUTOMATIC_SAVE_SUCCESS(remoteSync()->offerId().toUtf8().data());
	}
}
}
}
