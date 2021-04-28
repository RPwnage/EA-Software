/*************************************************************************************************/
/*!
    \file test_uuid.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <stdio.h>
#include <time.h>
#include "framework/blaze.h"
#include "framework/util/uuid.h"
#include "framework/test/blazeunittest.h"
#include "time.h"

class TestUUIDAndSecret
{
public:
    TestUUIDAndSecret() {}
    
    void testUUIDAndSecret()
    {
        Blaze::UUID uuid;
        Blaze::UUIDUtil::generateUUID( uuid );

        int32_t num = 100000;
        printf("Running uuid generate, encrypt and decrypt for %u times.\n", num);
        for (int32_t i = 0; i < num; ++i)
        {
            Blaze::UUID uuid;
            Blaze::UUIDUtil::generateUUID( uuid );
            printf("UUID: %s\n", uuid.c_str());

            Blaze::TdfBlob secretBlob;
            Blaze::UUIDUtil::generateSecretText(uuid, secretBlob);

            Blaze::UUID uuidDecrypt;
            Blaze::UUIDUtil::retrieveUUID(secretBlob, uuidDecrypt);
            printf("UUID: %s\n\n", uuidDecrypt.c_str());

            BUT_ASSERT_MSG("UUID does not match the secrete", (blaze_strcmp(uuid.c_str(), uuidDecrypt.c_str()) == 0));
        }
    }
};

int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: testUUIDAndSecret\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestUUIDAndSecret testUUID;
    testUUID.testUUIDAndSecret();
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}

