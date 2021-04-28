/*************************************************************************************************/
/*!
    \file   matchcodegenmasterimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MATCHCODEGEN_MASTERIMPL_H
#define BLAZE_MATCHCODEGEN_MASTERIMPL_H

/*** Include files *******************************************************************************/
#include "matchcodegen/rpc/matchcodegenmaster_stub.h"
#include "gamemanager/tdf/gamemanager.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace MatchCodeGen
{

class MatchCodeGenMasterImpl : public MatchCodeGenMasterStub
{
public:
    MatchCodeGenMasterImpl();
    ~MatchCodeGenMasterImpl() override;

private:

	static const uint64_t			  CODE_BASE = 36;
	static const uint64_t			  CODE_LENGTH = 6;
	static const uint64_t			  CODE_RANGE = 2176782336; // 36 ^ 6 = 2176782336
	static const uint64_t			  INVALID_CODE = CODE_RANGE;
	static const uint64_t			  BUFFER_SIZE = 1119744; // 2176782336 / 1944 = 1119744. 1944 is an arbitrary divisor, the requirement is only that CODE_RANGE is divisible by BUFFER_SIZE.
	static const inline char		  VALID_CHARACTERS[] = "0123456789abcdefghijklmnopqrstuvwxyz";

	static_assert(CODE_RANGE % BUFFER_SIZE == 0, "CODE_RANGE must be evenly divisible by BUFFER_SIZE.");
	static_assert(sizeof(VALID_CHARACTERS) - 1 == CODE_BASE, "VALID_CHARACTERS must have exactly CODE_BASE elements (ignoring null terminator).");

    bool onConfigure() override;

	GenerateCodeMasterError::Error processGenerateCodeMaster(const GenerateCodeRequest &request, GenerateCodeResponse &response, const Message *message) override;
	LookupGameIdMasterError::Error processLookupGameIdMaster(const LookupGameIdRequest &request, LookupGameIdResponse &response, const Message *message) override;

	// stores gameId in mGameIdBuffer and returns a CodeId index for lookup, increments mNextCodeId
	uint64_t generateCodeFromGameId(GameManager::GameId gameId);

	// looks up codeId in mGameIdBuffer, returns GameId if one exists, INVALID_GAME_ID otherwise
	GameManager::GameId lookupGameIdFromCode(uint64_t codeId);

	// validates that codeId is a valid index into mGameIdBuffer (in range, within BUFFER_SIZE of mNextCodeId)
	bool validateCodeId(uint64_t codeId);

	// translate CodeId to 6-digit alpha-numeric string, and vice versa.
	eastl::string translateCodeIdToString(uint64_t codeId);
	uint64_t translateStringToCodeId(eastl::string string);

	GameManager::GameId mGameIdBuffer[BUFFER_SIZE];
	uint64_t mNextCodeId;
};

} // MatchCodeGen
} // Blaze

#endif  // BLAZE_MATCHCODEGEN_MASTERIMPL_H
