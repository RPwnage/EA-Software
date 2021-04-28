#ifndef _MULTIPLAYER_SESSIONINFO_H
#define _MULTIPLAYER_SESSIONINFO_H

#include <QString>
#include <QByteArray>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace MultiplayerInvite
{

/// \brief Represents information about a muliplayer session
class ORIGIN_PLUGIN_API SessionInfo
{
public:
    ///
    /// \brief Constructs a new SessionInfo instance
    ///
    /// \param  productId      Product ID of the game being played to. This is used to redirect to the store for
    ///                        unowned games.
    /// \param  multiplayerId  Multiplayer ID of the game being played. This is used to determine cross-product
    ///                        multiplayer compatibility
    /// \param  data           Opaque game-specific session data 
    ///
    SessionInfo(const QString &productId, const QString &multiplayerId, const QByteArray &data) :
        mNull(false),
        mProductId(productId),
        mMuliplayerId(multiplayerId),
        mData(data)
    {
    }

    ///
    /// Constructs a null SessionInfo instance
    ///
    SessionInfo() : mNull(true)
    {
    }
    
    /// \brief Returns the product ID of the game owning this session
    QString productId() const
    {
        return mProductId;
    }

    /// \brief Returns the muliplayer ID of the game owning this session
    QString multiplayerId() const
    {
        return mMuliplayerId;
    }

    /// \brief Returns the opaque game-specific session data
    QByteArray data() const
    {
        return mData;
    }

    /// \brief Returns if this instance is null
    bool isNull() const
    {
        return mNull;
    }

private:
    bool mNull;
    QString mProductId;
    QString mMuliplayerId;
    QByteArray mData;
};

}
}
}

#endif

