#include "DiscoverableInformation.h"

#include <QCryptographicHash>
#include <QStringList>

namespace Origin
{
namespace Chat
{
    QByteArray DiscoverableInformation::sha1VerificationHash() const
    {
        // Cache this because we want to send this with every presence update
        if (mCachedVerificationHash.isNull())
        {
            QStringList identityTags;

            for(auto it = mIdentities.constBegin();
                it != mIdentities.constEnd();
                it++)
            {
                // Build the tag
                identityTags += it->category() + "/" + it->type() + "/" + it->lang() + "/" + it->name();
            }

            // Sort lexographically
            identityTags.sort();

            QStringList featureTags = mSupportedFeatureUris.toList();

            // Sort the features lexographically
            featureTags.sort();

            // Identities must come first
            const QStringList allTags = identityTags + featureTags;

            const QString stringToHash = allTags.join("<") + "<";
            const QByteArray bytesToHash = stringToHash.toUtf8();

            QCryptographicHash hasher(QCryptographicHash::Sha1);
            hasher.addData(bytesToHash);

            mCachedVerificationHash = hasher.result().toBase64();
        }

        return mCachedVerificationHash;
    }
}
}
