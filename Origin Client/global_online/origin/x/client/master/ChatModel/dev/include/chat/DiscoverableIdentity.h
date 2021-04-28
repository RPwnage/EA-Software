#ifndef _CHATMODEL_DISCOVERABLEIDENTITY_H
#define _CHATMODEL_DISCOVERABLEIDENTITY_H

#include <QString>
#include <QHash>

namespace Origin
{
namespace Chat
{
    ///
    /// Represents the identity of entity discoverable over XEP-0030
    ///
    /// \sa DiscoverableInformation
    ///
    class DiscoverableIdentity
    {
    public:
        ///
        /// Creates a null DiscoverableIdentity instance
        ///
        DiscoverableIdentity() :
            mNull(true)
        {
        }

        ///
        /// Creates a non-null DiscoverableIdentity instance
        ///
        /// \param category  Category of the entity
        /// \param type      Type of the entity within its category
        /// \param name      Optional natural language name of the entity
        ///
        DiscoverableIdentity(const QString &category, const QString &type, const QString &name = QString(), const QString &lang = QString()) :
            mNull(false),
            mCategory(category),
            mType(type),
            mName(name),
            mLang(lang)
        {
        }

        ///
        /// Returns if this is a null DiscoverableIdentity instance
        ///
        /// \sa DiscoverableIdentity
        ///
        bool isNull() const
        {
            return mNull;
        }

        ///
        /// Returns the category of the entity
        ///
        /// Registered categories can be found at http://xmpp.org/registrar/disco-categories.html 
        ///
        QString category() const
        {
            return mCategory;
        }

        ///
        /// Returns the type of the entity within its category
        ///
        /// Registered categories can be found at http://xmpp.org/registrar/disco-categories.html
        ///
        QString type() const
        {
            return mType;
        }

        ///
        /// Returns the optional natural language name of the entity
        ///
        QString name() const
        {
            return mName;
        }

        ///
        /// Returns the optional BCP-47 language code of the name
        ///
        QString lang() const
        {
            return mLang;
        }

        ///
        /// Compares this instance for equality
        ///
        bool operator==(const DiscoverableIdentity &other) const
        {
            return (isNull() == other.isNull()) &&
                   (category() == other.category()) &&
                   (type() == other.type()) &&
                   (name() == other.name()) &&
                   (lang() == other.lang());
        }

    private:
        bool mNull;

        QString mCategory;
        QString mType;
        QString mName;
        QString mLang;
    };
    
    inline uint qHash(const Origin::Chat::DiscoverableIdentity &identity)
    {
        return qHash(identity.category()) ^ 
               qHash(identity.type()) ^ 
               qHash(identity.name()) ^
               qHash(identity.lang());
    }
}
}

#endif
