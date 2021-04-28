/*************************************************************************************************/
/*!
    \file   matchcodegenmasterimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class MatchCodeGenMasterImpl

    MatchCodeGen Master implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "matchcodegenmasterimpl.h"

// global includes
#include "framework/controller/controller.h"

#include "framework/replication/replicatedmap.h"
#include "framework/replication/replicator.h"

// matchcodegen includes
#include "matchcodegen/tdf/matchcodegentypes.h"

namespace Blaze
{
namespace MatchCodeGen
{

// static
MatchCodeGenMaster* MatchCodeGenMaster::createImpl()
{
    return BLAZE_NEW_NAMED("MatchCodeGenMasterImpl") MatchCodeGenMasterImpl();
}

/*** Public Methods ******************************************************************************/
MatchCodeGenMasterImpl::MatchCodeGenMasterImpl()    
{
	mNextCodeId = 0;

	// initialize buffer with INVALID_GAME_ID
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		mGameIdBuffer[i] = GameManager::INVALID_GAME_ID;
	}
}

MatchCodeGenMasterImpl::~MatchCodeGenMasterImpl()
{
}

bool MatchCodeGenMasterImpl::onConfigure()
{
    TRACE_LOG("[MatchCodeGenMasterImpl].configure: configuring component.");

    return true;
}

GenerateCodeMasterError::Error MatchCodeGenMasterImpl::processGenerateCodeMaster(const GenerateCodeRequest &request, GenerateCodeResponse &response, const Message *message)
{
	if (request.getGameId() == GameManager::INVALID_GAME_ID)
	{
		return GenerateCodeMasterError::MATCHCODEGEN_ERR_INVALIDGAMEID;
	}

	uint64_t codeId = generateCodeFromGameId(request.getGameId());
	eastl::string code = translateCodeIdToString(codeId);

	response.setCode(code.c_str());

	return GenerateCodeMasterError::commandErrorFromBlazeError(ERR_OK);
}

LookupGameIdMasterError::Error MatchCodeGenMasterImpl::processLookupGameIdMaster(const LookupGameIdRequest &request, LookupGameIdResponse &response, const Message *message)
{
	uint64_t codeId = translateStringToCodeId(request.getCode());
	if (codeId == INVALID_CODE)
	{
		return LookupGameIdMasterError::MATCHCODEGEN_ERR_INVALIDCODE;
	}

	GameManager::GameId gameId = lookupGameIdFromCode(codeId);
	if (gameId == GameManager::INVALID_GAME_ID)
	{
		return LookupGameIdMasterError::MATCHCODEGEN_ERR_INVALIDCODE;
	}

	response.setGameId(gameId);
	return LookupGameIdMasterError::commandErrorFromBlazeError(ERR_OK);
}

uint64_t MatchCodeGenMasterImpl::generateCodeFromGameId(GameManager::GameId gameId)
{
	uint64_t codeId = mNextCodeId;
	uint64_t index = codeId % BUFFER_SIZE;

	mGameIdBuffer[index] = gameId;

	if (mNextCodeId == (CODE_RANGE - 1))
	{
		mNextCodeId = 0;
	}
	else
	{
		mNextCodeId++;
	}

	return codeId;
}

GameManager::GameId MatchCodeGenMasterImpl::lookupGameIdFromCode(uint64_t codeId)
{
	if (!validateCodeId(codeId))
		return GameManager::INVALID_GAME_ID;

	uint64_t index = codeId % BUFFER_SIZE;
	GameManager::GameId gameId = mGameIdBuffer[index];

	return gameId;
}

bool MatchCodeGenMasterImpl::validateCodeId(uint64_t codeId)
{
	if (codeId >= CODE_RANGE)
		return false;

	// we only store BUFFER_SIZE codeIds
	// therefore valid codes are in the range 		 [mNextCodeId - BUFFER_SIZE, mNextCodeId - 1]
	// or, if this range wraps from CODE_RANGE to 0: [0, mNextCodeId-1] union [CODE_RANGE + mNextCodeId - BUFFER_SIZE, CODE_RANGE-1]

	uint64_t minValidCodeId;
	if (mNextCodeId >= BUFFER_SIZE)
	{
		minValidCodeId = mNextCodeId - BUFFER_SIZE;
	}
	else
	{
		minValidCodeId = CODE_RANGE + mNextCodeId - BUFFER_SIZE;
	}

	uint64_t maxValidCodeId;
	if (mNextCodeId > 0)
	{
		maxValidCodeId = mNextCodeId - 1;
	}
	else
	{
		maxValidCodeId = CODE_RANGE - 1;
	}

	if (minValidCodeId < maxValidCodeId)
	{
		// no wrapping, normal range
		return (codeId >= minValidCodeId && codeId <= maxValidCodeId);
	}
	else
	{
		// valid range wraps from CODE_RANGE to 0
		// valid if in [0, maxValidCodeId] OR [minValidCodeId, CODE_RANGE-1] (already validated that it's within [0, CODE_RANGE-1])
		return (codeId <= maxValidCodeId || codeId >= minValidCodeId);
	}
}

eastl::string MatchCodeGenMasterImpl::translateCodeIdToString(uint64_t codeId)
{
	if (codeId >= CODE_RANGE)
		return "";

	uint64_t dividend = codeId;

	eastl::string result = "";
	for (int i = 0; i < CODE_LENGTH; i++)
	{
		uint64_t quotient = dividend / CODE_BASE;
		uint64_t remainder = dividend % CODE_BASE;

		if (i == (CODE_LENGTH - 1) && quotient > 0)
		{
			// codeId was bigger than can be represented in 6 digits of base36
			// should never happen, as that implies it's also >= CODE_RANGE
			return "";
		}

		dividend = quotient;

		eastl::string newCharacter = "";
		newCharacter += VALID_CHARACTERS[remainder];
		result = newCharacter + result;
	}

	return result;
}

uint64_t MatchCodeGenMasterImpl::translateStringToCodeId(eastl::string string)
{
	// codes must be exactly CODE_LENGTH long
	if (string.length() != CODE_LENGTH)
	{
		return INVALID_CODE;
	}

	// lower the string before comparing with VALID_CHARACTERS
	string.make_lower();

	// codes must only contain alphanumeric characters (enumerated in VALID_CHARACTERS)
	for (char character : string)
	{
		if (EA::StdC::Strchr(VALID_CHARACTERS, character) == NULL)
		{
			return INVALID_CODE;
		}
	}

	char8_t* pEnd = nullptr;
	uint64_t codeId = EA::StdC::StrtoU64(string.c_str(), &pEnd, CODE_BASE);

	if (codeId >= CODE_RANGE)
		return INVALID_CODE;

	return codeId;
}

} // MatchCodeGen
} // Blaze
