#include "engine/social/ConversationHistoryManager.h"

#include <QUuid>
#include <QDesktopServices>
#include <QCryptographicHash>
#include <QDebug>

#include "chat/RemoteUser.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformService.h"

#include "engine/social/ConversationManager.h"
#include "engine/social/ConversationEvent.h"

#if defined(ORIGIN_PC)
#include <Windows.h>
#include <shlobj.h>
#endif

namespace Origin
{
namespace Engine
{
namespace Social
{

namespace 
{
    const QString DynamicSaltFilename("index.dat");
    const unsigned char StaticSalt[8] = {0xf0, 0xee, 0x50, 0x96, 0xd0, 0xf5, 0xf7, 0x9d};

    const quint32 LogMagicNumber = 0x929e945e; 
}

ConversationHistoryManager::ConversationHistoryManager(ConversationManager *convManager, const Chat::JabberID &currentUserJid) :
    mConversationManager(convManager),
    mCurrentUserJid(currentUserJid),
    mDiskRecordingEnabled(false)
{
    QDir topLevelLogDir;
#if defined(ORIGIN_PC)                                                                                                 
    wchar_t folderPath[MAX_PATH + 1];

    SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, folderPath);
    topLevelLogDir = QString::fromUtf16(folderPath) + "\\Origin\\Conversation Logs";
#else
    topLevelLogDir = Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DataLocation) + "/Conversation Logs";
#endif

    // Store the actual logs in a user-specific directory
    QCryptographicHash hasher(QCryptographicHash::Md5);
    hasher.addData(currentUserJid.asBare().toEncoded());
    hasher.addData(reinterpret_cast<const char *>(StaticSalt), sizeof(StaticSalt));

    mConversationLogRoot = QDir(topLevelLogDir.absoluteFilePath(hasher.result().toHex()));

    ORIGIN_VERIFY_CONNECT(
            convManager, SIGNAL(conversationCreated(QSharedPointer<Origin::Engine::Social::Conversation>)),
            this, SLOT(conversationCreated(QSharedPointer<Origin::Engine::Social::Conversation>)));
}
       
QSharedPointer<ConversationRecord> ConversationHistoryManager::previousRecordForConversable(Chat::Conversable *conversable) const
{
    Chat::RemoteUser *remoteUser = dynamic_cast<Chat::RemoteUser*>(conversable);

    if (remoteUser)
    {
        return previousRecordForJabberId(remoteUser->jabberId().asBare());
    }

    return QSharedPointer<ConversationRecord>();
}

void ConversationHistoryManager::conversationCreated(QSharedPointer<Origin::Engine::Social::Conversation> convRef)
{
    // Strip the smart pointer
    // Note we never dereference this unless we know its alive still. We basically use this as a single bit meaning
    // "this was a one-to-one conversation"
    Conversation *conv = convRef.data();

    if (conv->type() != Conversation::OneToOneType)
    {
        // Don't care
        return;
    }

    ORIGIN_VERIFY_CONNECT(
            conv, SIGNAL(conversationFinished(Origin::Engine::Social::Conversation::FinishReason)),
            this, SLOT(onConversationFinished(Origin::Engine::Social::Conversation::FinishReason)));

    mOneToOneConversations.insert(conv, conv->conversable()->jabberId());
}

void ConversationHistoryManager::onConversationFinished(Origin::Engine::Social::Conversation::FinishReason reason)
{
    if (mDiskRecordingEnabled)
    {
        Conversation *conv = dynamic_cast<Conversation*>(sender());

        if (!conv)
        {
            ORIGIN_ASSERT(0);
            return;
        }

        if (!mOneToOneConversations.contains(conv))
        {
            // This wasn't a one-to-one conversation
            return;
        }

        // Save the conversation to disk
        Chat::JabberID previousJid = mOneToOneConversations.take(conv);
        setPreviousRecordForJabberId(previousJid, conv);
    }
}

QString ConversationHistoryManager::logFilenameForJabberId(const Chat::JabberID &jid) const
{
    if (mDynamicSalt.isNull())
    {
        return QString();
    }

    QCryptographicHash hasher(QCryptographicHash::Md5);
    
    hasher.addData(jid.asBare().toEncoded());
    hasher.addData(mDynamicSalt);
    hasher.addData(reinterpret_cast<const char *>(StaticSalt), sizeof(StaticSalt));

    return mConversationLogRoot.filePath(hasher.result().toHex() + ".chatlog");
}

// This removes events we don't care about
QSharedPointer<ConversationRecord> ConversationHistoryManager::summarizeConversationRecord(ConversationRecord* full)
{
    QSharedPointer<ConversationRecord> summary(new ConversationRecord(full->startTime()));
    summary->setEndTime(full->endTime());

    QList<const ConversationEvent *> events = full->events();

    for(auto it = events.constBegin();
        it != events.constEnd();
        it++)
    {
        const ConversationEvent *event = *it;

        if (auto messageEvent = dynamic_cast<const MessageEvent*>(event))
        {
            // We have to copy this because the old conversation has ownership of the original event
            summary->addEvent(new MessageEvent(*messageEvent));
        }
    }

    return summary;
}
        
QSharedPointer<ConversationRecord> ConversationHistoryManager::previousRecordForJabberId(const Chat::JabberID &jid) const
{
    if (!mPreviousRecordCache.contains(jid) && mDiskRecordingEnabled)
    {
        mPreviousRecordCache.insert(jid, loadRecordFileForJabberId(jid));
    }
        
    return mPreviousRecordCache.value(jid);
}
        
QSharedPointer<ConversationRecord> ConversationHistoryManager::loadRecordFileForJabberId(const Chat::JabberID &jid) const
{
    QString logFilename = logFilenameForJabberId(jid);

    if (logFilename.isNull())
    {
        // Don't have a salt
        return QSharedPointer<ConversationRecord>();
    }

    QFile logFile(logFilename);

    if (!logFile.open(QIODevice::ReadOnly))
    {
        // Probably doesn't exist yet
        return QSharedPointer<ConversationRecord>();
    }

    QDataStream loadStream(&logFile);

    quint32 magicNumber = 0x0;
    QDateTime startTime, endTime;

    loadStream >> magicNumber;

    if ((loadStream.status() != QDataStream::Ok) || (magicNumber != LogMagicNumber))
    {
        return QSharedPointer<ConversationRecord>();
    }

    loadStream >> startTime;
    loadStream >> endTime;

    if (loadStream.status() != QDataStream::Ok)
    {
        return QSharedPointer<ConversationRecord>();
    }

    ConversationRecord *record = new ConversationRecord(startTime);
    record->setEndTime(endTime);

    while((loadStream.status() == QDataStream::Ok) && !loadStream.atEnd())
    {
        QDateTime eventTime;
        bool outgoing;
        QString threadId;
        QString body;

        loadStream >> eventTime;
        loadStream >> outgoing;
        loadStream >> threadId;
        loadStream >> body;

        Chat::Message message;
        
        if (outgoing)
        {
            message = Chat::Message("chat", mCurrentUserJid, jid, Origin::Chat::ChatStateActive, threadId); 
        }
        else
        {
            message = Chat::Message("chat", jid, mCurrentUserJid, Origin::Chat::ChatStateActive, threadId); 
        }

        message.setBody(body);

        MessageEvent *messageEvent = new MessageEvent(eventTime, message);
        record->addEvent(messageEvent);
    }


    if (loadStream.status() != QDataStream::Ok)
    {
        delete record;
        return QSharedPointer<ConversationRecord>();
    }

    return QSharedPointer<ConversationRecord>(record);
}

void ConversationHistoryManager::setPreviousRecordForJabberId(const Chat::JabberID &jid, ConversationRecord* record)
{
    QSharedPointer<ConversationRecord> summary = summarizeConversationRecord(record);
    
    if (summary->events().isEmpty())
    {
        // Nothing to record; keep our existing record
        return;
    }

    mPreviousRecordCache.insert(jid, summary);

    if (mDiskRecordingEnabled)
    {
        // Save to disk
        saveRecordFileForJabberId(jid, summary);
    }
}

void ConversationHistoryManager::saveRecordFileForJabberId(const Chat::JabberID &jid, QSharedPointer<ConversationRecord> record)
{
    ORIGIN_ASSERT(mDiskRecordingEnabled);

    QString logFilename = logFilenameForJabberId(jid);

    if (logFilename.isNull())
    {
        // Don't have a salt
        return;
    }

    QFile logFile(logFilename);

    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        ORIGIN_LOG_WARNING << "Unable to open chat log file " << logFilename << " for writing";
        return;
    }

    QDataStream saveStream(&logFile);

    saveStream << LogMagicNumber;
    saveStream << record->startTime();
    saveStream << record->endTime();

    QList<const ConversationEvent *> events = record->events();

    for(QList<const ConversationEvent *>::ConstIterator it = events.constBegin();
        it != events.constEnd();
        it++)
    {
        const ConversationEvent *event = *it;
        const MessageEvent *messageEvent = dynamic_cast<const MessageEvent*>(event);

        if (!messageEvent)
        {
            continue;
        }

        const Chat::Message message = messageEvent->message();

        if (message.isNull() || (message.type() != "chat"))
        {
            // Don't care
            continue;
        }

        const bool outgoing = message.from().asBare() == mCurrentUserJid.asBare();

        // We can't construct message instances with arbitrary send times
        // Instead just fake the event time
        if (!message.sendTime().isNull())
        {
            saveStream << message.sendTime();
        }
        else
        {
            saveStream << messageEvent->time();
        }

        saveStream << outgoing;
        saveStream << message.threadId();
        saveStream << message.body();
    }
}
       
void ConversationHistoryManager::setDiskRecordingEnabled(bool enabled)
{
    if (enabled == mDiskRecordingEnabled)
    {
        // Nothing to do
        return;
    }

    if (enabled)
    {
        initializeLogDirectory();
    }
    else
    {
        purgeLogDirectory();
    }
    
    mDiskRecordingEnabled = enabled;
}
        
void ConversationHistoryManager::initializeLogDirectory()
{
    if (!mConversationLogRoot.exists())
    {
        QDir().mkpath(mConversationLogRoot.absolutePath());
    }

    QFile saltFile(mConversationLogRoot.filePath(DynamicSaltFilename));

    if (saltFile.open(QIODevice::ReadOnly))
    {
        // Read our salt data from disk
        mDynamicSalt = saltFile.readAll();
    }
    else if (saltFile.open(QIODevice::WriteOnly))
    {
        // Generate a new salt
        mDynamicSalt = QUuid::createUuid().toByteArray();

        // Write it to disk
        saltFile.write(mDynamicSalt);
    }
    else
    {
        ORIGIN_LOG_WARNING << "Unable to save salt file for conversation history. Conversation history will not be persisted";
    }
}

void ConversationHistoryManager::purgeLogDirectory()
{
    QStringList entries = mConversationLogRoot.entryList(QStringList("*.chatlog"), QDir::Files | QDir::NoSymLinks | QDir::Writable);

    for(QStringList::ConstIterator it = entries.constBegin();
        it != entries.constEnd();
        it++)
    {
        mConversationLogRoot.remove(*it);
    }

    mConversationLogRoot.remove(DynamicSaltFilename);

    // This will leave the directory around if there are any unexpected files inside of it. That should be safe and
    // still do the right thing from our perspective
    mConversationLogRoot.rmdir(mConversationLogRoot.absolutePath());
}

}
}
}
