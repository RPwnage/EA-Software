/*************************************************************************************************/
/*!
    \file   uuid.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/random.h"
#include "framework/util/uuid.h"

#if defined(EA_PLATFORM_LINUX)
#include <uuid/uuid.h>
#else
#include <Rpc.h>
#endif
#include <openssl/des.h>

namespace Blaze
{
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

// NOTE: This DESKEY cannot be changed, or it will render all currently active secrets invalid.
static const int DESKEY_SIZE = 8;
static unsigned char DESKEY[DESKEY_SIZE] = { 0x0f,0xed,0x5f,0x65,0xa7,0x1b,0x6d,0x25};

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief generateUUID

    Generate a UUID, the UUID will be a 36 characters string 

    \param[out]  uuid - unique 36 chars string generated
    */
/*************************************************************************************************/
void UUIDUtil::generateUUID(UUID& uuid)
{
    char8_t uuidStr[MAX_UUID_LEN];
#ifdef EA_PLATFORM_LINUX
    uuid_t uuid_linux;
    uuid_generate(uuid_linux);
    uuid_unparse(uuid_linux, uuidStr);
    uuid.set(uuidStr);
#else
    GUID* uuid_win = BLAZE_NEW GUID;
    RPC_STATUS rc = UuidCreate(uuid_win);
    if (rc == RPC_S_OK || rc == RPC_S_UUID_LOCAL_ONLY)
    {
        unsigned char* uChar = nullptr;
        UuidToString(uuid_win, &uChar);
        blaze_snzprintf(uuidStr, MAX_UUID_LEN, "%s", uChar);

        // Copy to the outgoing UUID.
        uuid.set(uuidStr);

        RpcStringFree(&uChar);
    }
    else
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[UUIDUtil::generateUUID]- Unable create a unique uuid");
    }

    delete uuid_win;

#endif
}

/*************************************************************************************************/
/*!
    \brief generateSecretText

    Generate a secret string based on UUID passed in and encrypted the string, passed out through blob

    Note, this function can be used together with retrieveUUID
    The format of raw secret string is: <UUID>:<Random NUMBER>.

    \param[in]  uuid - the uuid used to generate raw secrete string 
    \param[in]  secretBlob - the encrypted secreted string
*/
/*************************************************************************************************/
void UUIDUtil::generateSecretText(const UUID& uuid, EA::TDF::TdfBlob& secretBlob)
{
    // prepare the raw secrete string
    char8_t rawSecretText[Blaze::MAX_UUID_LEN + 32];
    // 0 fill the random int value to ensure that our string is at least SECRET_MAX_LENGTH
    // or when it is encrypted we will end up with undefined values.
    blaze_snzprintf(rawSecretText, sizeof(rawSecretText), "%s:%020u", uuid.c_str(), gRandom->getStrongRandomNumber(Random::MAX));

    // prepare the des_key
    const_DES_cblock* des_key = &DESKEY;
   
    // get a key
    DES_key_schedule schedule;
    DES_set_key(des_key, &schedule);

    // ini outputBlock
    DES_cblock outputBlock;
    memset(outputBlock, 0, sizeof(outputBlock));

    // encrypt
    unsigned char secretOutput[SECRET_MAX_LENGTH];
    memset(secretOutput, 0, SECRET_MAX_LENGTH);
    DES_ncbc_encrypt(reinterpret_cast<unsigned char*>(rawSecretText), secretOutput, SECRET_MAX_LENGTH, &schedule, &outputBlock, DES_ENCRYPT);

    // set the encryption output to the blob
    secretBlob.setData((const uint8_t*) secretOutput, SECRET_MAX_LENGTH);
}

/*************************************************************************************************/
/*!
    \brief retrieveUUID

    Retrieve uuid from a passed in secret blob

    Note, this function should be used together with generateSecretText. Through these 2 functions,
    we can generate a unique id and a secret for the unique id, using the encrypted secret, we can retrieve
    the uuid and check if it matched the uuid we have.

    The format of raw secret string is: <UUID>:<Random NUMBER>

    \param[in]  secretBlob - the secret blob encrypted via generateSecretText
    \param[out]  uuid - the uuid used to generate the secret blob
    */
/*************************************************************************************************/
void UUIDUtil::retrieveUUID(const EA::TDF::TdfBlob& secretBlob, UUID& uuid)
{
    if (secretBlob.getCount() == 0)
        return;

    // prepare the des_key
    const_DES_cblock* des_key = &DESKEY;

    // get a key
    DES_key_schedule schedule;
    DES_set_key(des_key, &schedule);

    // init output block
    DES_cblock outputBlock;
    memset(outputBlock, 0, sizeof(outputBlock));

    // decrypt
    unsigned char secretOutput[SECRET_MAX_LENGTH];
    secretOutput[0] = '\0';
    DES_ncbc_encrypt(reinterpret_cast<unsigned char*>(secretBlob.getData()), secretOutput, SECRET_MAX_LENGTH, &schedule, &outputBlock, DES_DECRYPT);

    // retrieve uuid from output string
    char8_t* rawSecretText = reinterpret_cast<char8_t*>(secretOutput);
    char8_t uuidStr[MAX_UUID_LEN];
    blaze_strnzcpy(uuidStr, rawSecretText, MAX_UUID_LEN);
    uuid.set(uuidStr);
}

}   // Blaze
