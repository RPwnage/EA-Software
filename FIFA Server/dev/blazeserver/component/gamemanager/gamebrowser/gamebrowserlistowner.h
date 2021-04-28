/*! ************************************************************************************************/
/*!
    \file gamebrowserlistowner.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAME_BROWSER_LIST_OWNER_H
#define BLAZE_GAMEMANAGER_GAME_BROWSER_LIST_OWNER_H

#include "EASTL/intrusive_ptr.h"
#include "EAAssert/eaassert.h"
#include "component/gamemanager/tdf/gamebrowser.h"

namespace Blaze
{
namespace GameManager
{

class NotifyGameListUpdate;

enum GameBrowserListOwnerType
{
    GAMEBROWSERLIST_OWNERTYPE_INVALID,
    GAMEBROWSERLIST_OWNERTYPE_USERSESSION,
    GAMEBROWSERLIST_OWNERTYPE_GBSERVICE
};

// Interface for GameBrowserList owners
class IGameBrowserListOwner
{
public:
    IGameBrowserListOwner(GameBrowserListOwnerType ownerType, GameBrowserListId listId)
        : mOwnerType(ownerType)
        , mListId(listId)
        , mRefCount(0)
    {

    }

    virtual bool canUpdate() const = 0;
    virtual void onUpdate(const NotifyGameListUpdate& listUpdate, uint32_t msgNum) = 0; //leaky abstraction
    virtual void onDestroy() = 0;

    GameBrowserListOwnerType getOwnerType() const
    {
        return mOwnerType;
    }

    GameBrowserListId getListId() const
    {
        return mListId;
    }

    inline void AddRef()
    {
        ++mRefCount;
    }

    inline void Release()
    {
        EA_ASSERT(mRefCount > 0);
        if (--mRefCount == 0)
            delete this;
    }

protected:
    // Make destructor non-public to ensure that only mechanism to delete the instance is via ref-counting.
    virtual ~IGameBrowserListOwner()
    {
        EA_ASSERT(mRefCount == 0);
    }
    
    friend void intrusive_ptr_add_ref(IGameBrowserListOwner*);
    friend void intrusive_ptr_release(IGameBrowserListOwner*);
    
    GameBrowserListOwnerType mOwnerType;
    GameBrowserListId mListId;
    uint32_t mRefCount;
};

inline void intrusive_ptr_add_ref(IGameBrowserListOwner* ptr) { ptr->AddRef(); }
inline void intrusive_ptr_release(IGameBrowserListOwner* ptr) { ptr->Release(); }

using IGameBrowserListOwnerPtr = eastl::intrusive_ptr<IGameBrowserListOwner>;

} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAME_BROWSER_LIST_OWNER_H
