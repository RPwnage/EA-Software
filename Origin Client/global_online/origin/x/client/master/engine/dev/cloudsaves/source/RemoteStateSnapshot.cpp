#include <limits>
#include <QDateTime>

#include "engine/cloudsaves/RemoteStateSnapshot.h"
#include "engine/cloudsaves/LocalStateSnapshot.h"
#include "engine/cloudsaves/PathSubstituter.h"
#include "services/debug/DebugService.h"

namespace
{
    const QString ManifestXmlNS("http://origin.com/cloudsaves/manifest");

    /**
     * Creates an XML Schema style date in UTC
     */
    QString datetimeToXmlSchema(const QDateTime &datetime)
    {
        return datetime.toUTC().toString(Qt::ISODate) + "Z";
    }

    QDateTime xmlSchemaToDatetime(const QString &xmlSchema)
    {
        return QDateTime::fromString(xmlSchema, Qt::ISODate);
    }
}

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    RemoteStateSnapshot::RemoteStateSnapshot(const LocalStateSnapshot &local, const RemoteStateSnapshot *remote)
    {
        m_lineage = local.lineage();

        const FileHashes &localHash = local.fileHash();

        for(FileHashes::const_iterator it = localHash.constBegin();
            it != localHash.constEnd();
            it++)
        {
            const FileFingerprint &fingerprint = it.key();
            const SyncableFile &localFile = it.value();

            if (remote)
            {
                FileHashes::const_iterator remoteFileIt = remote->fileHash().constFind(fingerprint);
    
                if (remoteFileIt != remote->fileHash().constEnd())
                {
                    // Use the existing remote path
                    QString existingRemotePath = (*remoteFileIt).remotePath();
                    m_fileHash.insert(fingerprint, SyncableFile(fingerprint, localFile.localInstances(), existingRemotePath));

                    continue;
                }
            }

            // Build a new remote path
            m_fileHash.insert(fingerprint, SyncableFile(fingerprint, localFile.localInstances()));
        }
    }

    RemoteStateSnapshot::RemoteStateSnapshot(const QDomDocument &doc, const PathSubstituter &substituter) 
    {
        QList<SyncableFile> remoteFiles;

        QDomElement docEl = doc.documentElement();

        // Make sure this looks like the document we want
        if (docEl.tagName() != "manifest")
        {
            // Not what we want
            return;
        }

        // Get our lineage UUID
        m_lineage = QUuid(docEl.attribute("lineage"));

        // Get all of the files
        QDomNodeList files = docEl.elementsByTagName("file");

        for(int i = 0; i < files.size(); i++) 
        {
            QDomElement fileEl = files.at(i).toElement();

            if (fileEl.isNull())
            {
                // Shouldn't happen but be strict
                continue;
            }

            // Parse the size
            bool parsedSize = false;
            qint64 size = fileEl.attribute("size").toLongLong(&parsedSize);

            if (!parsedSize) 
            {
                // Bad size
                continue;
            }

            // Parse the MD5
            QByteArray md5 = QByteArray::fromBase64(fileEl.attribute("md5").toLatin1());

            if (md5.isNull())
            {
                // Bad md5
                continue;
            }

            // Get the remote path
            QString remotePath = fileEl.attribute("href");

            // Get the local path
            QDomNodeList localNameNodes = fileEl.elementsByTagName("localName");

            if (localNameNodes.length() < 1)
            {
                // We need at least one local file
                continue;
            }

            QSet<LocalInstance> localInstances;
            for(int i = 0; i < localNameNodes.size(); i++)
            {
                QDomElement localPathEl = localNameNodes.item(i).toElement();

                QString localPath = localPathEl.text();
                QString substitutedPath = substituter.untemplatizePath(localPath);

                if (substitutedPath.isNull())
                {
                    ORIGIN_ASSERT(false);
                    continue;
                }

                // Try to get our timestamps
                // Note that the initial version of cloud saves doesn't track
                // timestamps and manifests handled by it won't have these 
                // elements
                QDateTime created;
                if (localPathEl.hasAttribute("created"))
                {
                    created = xmlSchemaToDatetime(localPathEl.attribute("created"));
                }

                QDateTime lastModified;
                if (localPathEl.hasAttribute("lastModified"))
                {
                    lastModified = xmlSchemaToDatetime(localPathEl.attribute("lastModified"));
                }

                localInstances << LocalInstance(substitutedPath, lastModified, created); 
            }

            FileFingerprint fingerprint(size, md5);
            m_fileHash.insert(fingerprint, SyncableFile(fingerprint, localInstances, remotePath));
        }

        m_valid = true;
    }

    RemoteStateSnapshot::RemoteStateSnapshot()
    {
        m_lineage = QUuid::createUuid();
    }
    
    QDomDocument RemoteStateSnapshot::toXml(const PathSubstituter &substituter)
    {
        QDomDocument doc;

        QDomElement rootEl = doc.createElementNS(ManifestXmlNS, "manifest");
        rootEl.setAttribute("lineage", lineage().toString());
        doc.appendChild(rootEl);

        // Get all the remote files in the state snapshot
        const QList<SyncableFile> remoteFiles = m_fileHash.values(); 
        for(QList<SyncableFile>::const_iterator it = remoteFiles.constBegin();
            it != remoteFiles.constEnd();
            it++)
        {
            const SyncableFile &remoteFile = *it; 

            // If we explicitly set the namespace Qt helpfully reincludes our namespace definition
            // Just let this fall in the default namespace which is semantically the same
            QDomElement fileEl = doc.createElement("file");
            rootEl.appendChild(fileEl);

            // Set the basic attributes
            fileEl.setAttribute("href", remoteFile.remotePath());
            fileEl.setAttribute("size", remoteFile.fingerprint().size());
            fileEl.setAttribute("md5", QString::fromLatin1(remoteFile.fingerprint().md5().toBase64()));

            QSet<LocalInstance> localInstances = remoteFile.localInstances();
            for(QSet<LocalInstance>::const_iterator it = localInstances.constBegin();
                it != localInstances.constEnd();
                it++)
            {
                const LocalInstance &localInstance = *it;
                QString substitutedPath = substituter.templatizePath(localInstance.path());

                if (substitutedPath.isNull())
                {
                    ORIGIN_ASSERT(false);
                    continue;
                }

                QDomElement localNameEl = doc.createElement("localName");
                
                // Add our dates if not null
                if (!localInstance.lastModified().isNull())
                {
                    localNameEl.setAttribute("lastModified", datetimeToXmlSchema(localInstance.lastModified()));
                }

                if (!localInstance.created().isNull())
                {
                    localNameEl.setAttribute("created", datetimeToXmlSchema(localInstance.created()));
                }

                fileEl.appendChild(localNameEl);
    
                localNameEl.appendChild(doc.createTextNode(substitutedPath));
            }
        }

        return doc; 
    }
}
}
}
