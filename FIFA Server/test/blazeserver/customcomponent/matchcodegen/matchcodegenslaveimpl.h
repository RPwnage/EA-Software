/*************************************************************************************************/
/*!
    \file   matchcodegenslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MATCHCODEGEN_SLAVEIMPL_H
#define BLAZE_MATCHCODEGEN_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "matchcodegen/rpc/matchcodegenslave_stub.h"
#include "matchcodegen/rpc/matchcodegenmaster.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace MatchCodeGen
{

class MatchCodeGenSlaveImpl : public MatchCodeGenSlaveStub
{
public:
    MatchCodeGenSlaveImpl();
    ~MatchCodeGenSlaveImpl() override;

protected:

private:
    bool onConfigure() override;
};

} // MatchCodeGen
} // Blaze

#endif // BLAZE_MATCHCODEGEN_SLAVEIMPL_H

