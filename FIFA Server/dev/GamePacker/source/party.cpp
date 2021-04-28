#include "gamepacker/internal/packerinternal.h"
#include "gamepacker/party.h"

namespace Packer
{

Party::Party()
{
    // intentionally blank for now
}

Party::~Party()
{
}

void Party::setPlayerPropertyValue(const FieldDescriptor * fieldDescriptor, PlayerIndex playerIndex, const PropertyValue * value)
{
    EA_ASSERT(playerIndex < mPartyPlayers.size());
    return mFieldValueTable.setFieldValue(fieldDescriptor, playerIndex, value);
}

PropertyValue Party::getPlayerPropertyValue(const FieldDescriptor * fieldDescriptor, PlayerIndex playerIndex, bool * hasValue) const
{
    EA_ASSERT(playerIndex < mPartyPlayers.size());
    return mFieldValueTable.getFieldValue(fieldDescriptor, playerIndex, hasValue);
}

bool Party::hasPlayerPropertyField(const FieldDescriptor * fieldDescriptor) const
{
    return mFieldValueTable.hasField(fieldDescriptor);
}

} // GamePacker
