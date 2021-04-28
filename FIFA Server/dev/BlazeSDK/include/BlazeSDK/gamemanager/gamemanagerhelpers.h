/*! ************************************************************************************************/
/*!
    \file gamemanagerhelpers.h

    Contains internal GameManager helper classes.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_GAMEMANAGER_HELPERS_H
#define BLAZE_GAMEMANAGER_HELPERS_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/component/gamemanager/tdf/gamemanager.h"

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief returns true if the given SlotType is a participant slot.
    
        \param[in] slotType - the slot to check
        \return true if it is a participant slot
    ***************************************************************************************************/
    inline bool isParticipantSlot(SlotType slotType) { return ((slotType < MAX_PARTICIPANT_SLOT_TYPE) && (slotType >= SLOT_PUBLIC_PARTICIPANT)); }

    /*! ************************************************************************************************/
    /*! \brief returns true if the given SlotType is a spectator slot.
    
        \param[in] slotType - the slot to check
        \return true if it is a spectator slot
    ***************************************************************************************************/
    inline bool isSpectatorSlot(SlotType slotType) { return ((slotType >= SLOT_PUBLIC_SPECTATOR) && (slotType < MAX_SLOT_TYPE)); }


} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_HELPERS_H
