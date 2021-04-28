#ifndef _CHATMODEL_DISCOVERABLEINFORMATION_H
#define _CHATMODEL_DISCOVERABLEINFORMATION_H

#include <QByteArray>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

#include <string>
#include <vector>

#include "DiscoverableIdentity.h"

namespace Origin
{
namespace Chat
{
    ///
    /// Represents information discoverable over XEP-0030 Serivce discovery
    ///
    class DiscoverableInformation
    {
    public:
        ///
        /// Creates an empty DiscoverableInformation instance
        ///
        /// Note that empty instances are not valid until at least one feature and identity are added
        ///
        DiscoverableInformation()
        {
        }

        ///
        /// Returns true if this is a valid DiscoverableInformation instance
        /// 
        /// Valid instances must have at least one identity and at least one supported feature URI
        ///
        bool isValid() const
        {
            return !mIdentities.isEmpty() && !mSupportedFeatureUris.isEmpty();
        }

        ///
        /// Returns the set of identities for the entity
        ///
        QSet<DiscoverableIdentity> identities() const
        {
            return mIdentities;
        }

        ///
        /// Removes an identity from the entity
        /// 
        /// \param  identity  Identity to remove
        /// \return True if the identity was found, false otherwise
        ///
        bool removeIdentity(const DiscoverableIdentity &identity)
        {
            mCachedVerificationHash = QByteArray();
            return mIdentities.remove(identity);
        }

        ///
        /// Adds an identity to the entity
        /// 
        /// \param identity  Identity to add
        ///
        void addIdentity(const DiscoverableIdentity &identity)
        {
            mCachedVerificationHash = QByteArray();
            mIdentities.insert(identity);
        }
        
        ///
        /// Returns the set of feature URIs supported by the entity
        ///
        QSet<QString> supportedFeatureUris() const
        {
            return mSupportedFeatureUris;
        }

        ///
        /// Returns the set of feature URIs supported by the entity
        /// in a format suitable for libjingle (no Qt).
        ///
        std::string supportedFeatureUrisForJingle() const
        {
            QStringList uris;
            auto last(mSupportedFeatureUris.cend());
            for (auto i = mSupportedFeatureUris.cbegin(); i != last; ++i)
            {
                uris.push_back(*i);
            }
            return uris.join(',').toStdString();
        }

        ///
        /// Removes a supported feature URI from the entity
        /// 
        /// \param  identity  Identity to remove
        /// \return True if the identity was found, false otherwise
        ///
        bool removeSupporedFeatureUri(const QString &uri)
        {
            mCachedVerificationHash = QByteArray();
            return mSupportedFeatureUris.remove(uri);
        }

        ///
        /// Adds a supported feature URI to the entity
        /// 
        /// \param identity  Identity to add
        ///
        void addSupportedFeatureUri(const QString &uri)
        {
            mCachedVerificationHash = QByteArray();
            mSupportedFeatureUris.insert(uri);
        }

        ///
        /// Returns a base64 encoded XEP-0015 SHA-1 verification hash
        ///
        QByteArray sha1VerificationHash() const;

    private:
        bool mNull;

        QSet<DiscoverableIdentity> mIdentities;
        QSet<QString> mSupportedFeatureUris;

        mutable QByteArray mCachedVerificationHash;
    };
}
}

#endif

