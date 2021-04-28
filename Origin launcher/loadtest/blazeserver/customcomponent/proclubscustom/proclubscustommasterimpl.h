/*************************************************************************************************/
/*!
    \file   proclubscustommasterimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_proclubscustom_MASTERIMPL_H
#define BLAZE_proclubscustom_MASTERIMPL_H

/*** Include files *******************************************************************************/
#include "proclubscustom/rpc/proclubscustommaster_stub.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace proclubscustom
{

class ProclubsCustomMasterImpl : public proclubscustomMasterStub
{
public:
    ProclubsCustomMasterImpl();
    ~ProclubsCustomMasterImpl();

private:
	virtual uint16_t  		getDbSchemaVersion() const	{ return 1; }
    virtual const char8_t*  getDbName() const { return "main"; }
	virtual uint32_t        getDbId() const				{ return mDbId; }

    bool onConfigure();

	uint32_t mDbId;
};

} // ProClubsCustom
} // Blaze

#endif  // BLAZE_proclubscustom_MASTERIMPL_H
