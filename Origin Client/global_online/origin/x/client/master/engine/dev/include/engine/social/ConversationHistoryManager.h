#ifndef _CONVERSATIONHISTORYMANAGER_H
#define _CONVERSATIONHISTORYMANAGER_H

#include <QMap>
#include <QDir>
#include <QByteArray>
#include <QHash>
#include <QSharedPointer>

#include "chat/JabberID.h"
#include "engine/social/Conversation.h"
#include "services/plugin/PluginAPI.h"

class QIODevice;

namespace Origin
{
namespace Chat
{
    class Conversable;
}

namespace Engine
{
namespace Social
{
    class ConversationManager;
    class ConversationRecord;
    class SocialController;

    /// \brief Tracks the conversation history for previous one-to-one conversations
    class ORIGIN_PLUGIN_API ConversationHistoryManager : public QObject
    {
        Q_OBJECT 

        friend class SocialController;
    public:
        ///
        /// \brief Returns the conversation record associated with the previous conversation with a converable
        ///
        /// \param conversable  Conversable to look up the conversation record for. Only RemoteUsers are currently
        ///                     supported, other conversables will return NULL
        ///
        QSharedPointer<ConversationRecord> previousRecordForConversable(Chat::Conversable *conversable) const;

    protected:
       ConversationHistoryManager(ConversationManager *convManager, const Chat::JabberID &currentUserJid);
       
       ///
       /// \brief Enables or disables recording conversation history to disk
       ///
       /// Disabling conversation history recording will also purge all existing conversation history
       ///
       void setDiskRecordingEnabled(bool);

    private slots:
        void conversationCreated(QSharedPointer<Origin::Engine::Social::Conversation>);
        void onConversationFinished(Origin::Engine::Social::Conversation::FinishReason reason);

    private:
        void initializeLogDirectory();
        void purgeLogDirectory();

        QString logFilenameForJabberId(const Chat::JabberID &) const;
    
        QSharedPointer<ConversationRecord> summarizeConversationRecord(ConversationRecord* full);

        QSharedPointer<ConversationRecord> previousRecordForJabberId(const Chat::JabberID &) const; 
        QSharedPointer<ConversationRecord> loadRecordFileForJabberId(const Chat::JabberID &) const; 

        void setPreviousRecordForJabberId(const Chat::JabberID &, ConversationRecord*);
        void saveRecordFileForJabberId(const Chat::JabberID &, QSharedPointer<ConversationRecord>);
       
        ConversationManager *mConversationManager;
        Chat::JabberID mCurrentUserJid;

        QDir mConversationLogRoot;
        QByteArray mDynamicSalt;

        QHash<Conversation*, Chat::JabberID> mOneToOneConversations;
        mutable QHash<Chat::JabberID, QSharedPointer<ConversationRecord> > mPreviousRecordCache;

        bool mDiskRecordingEnabled;
    };
}
}
}

#endif

