/*! *********************************************************************************************/
/*!
    \file 


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include <test.h>
#include <test/tdf/alltypes.h>
#include <EATDF/tdfbitfield.h>


TEST(tdfbitfield, testBitfield)
{
    // Some simple test to see that the bit fields all work as expected.
    // 1-bit tests:
    {
        EA::TDF::Test::ABitField bitField;

        bitField.setBIT_0();
        EXPECT_TRUE(bitField.getBIT_0() == true);

        // Get Values
        uint32_t value = 0;
        EXPECT_TRUE(bitField.getValueByName("BIT_0", value) == true);     // Case-correct lookup 
        EXPECT_TRUE(value == 1);
        EXPECT_TRUE(bitField.getValueByName("bit_0", value) == true);     // Case insensitive lookup 
        EXPECT_TRUE(value == 1);
        EXPECT_TRUE(bitField.getValueByName("biker_0", value) == false);  // Invalid name

        // Set Values
        value = 1;
        EXPECT_TRUE(bitField.setValueByName("BIT_0", value) == true);     
        EXPECT_TRUE(bitField.getBIT_0() == true);
        value = 0xFFFF;
        EXPECT_TRUE(bitField.setValueByName("Bit_0", value) == true);     // Changes should not 'leak' over bit fields
        EXPECT_TRUE(bitField.getBIT_0() == true);
        EXPECT_TRUE(bitField.getBIT_1_2() == 0);
        EXPECT_TRUE(bitField.getBIT_3() == false);
        value = 0;
        EXPECT_TRUE(bitField.setValueByName("bit_0", value) == true);     
        EXPECT_TRUE(bitField.getBIT_0() == false);
        EXPECT_TRUE(bitField.setValueByName("BIZIT_0", value) == false);     

        // Generic value sets: 
        EA::TDF::TdfGenericValue genValue;
        value = 1;
        genValue.set(value);
        EXPECT_TRUE(bitField.setValueByName("BIT_0", genValue) == true);     
        EXPECT_TRUE(bitField.getBIT_0() == true);

        EA::TDF::TdfGenericValue genValueFloat;
        genValue.set(17.0f);
        EXPECT_TRUE(bitField.setValueByName("BIT_0", genValueFloat) == false);     
    }

    // 
    {
        EA::TDF::Test::AnotherBitField bitField;

        uint32_t value = 0xFFFF;
        bitField.setBits(0xFFFFFFFF);
        EXPECT_TRUE(bitField.getBIT_0() == true);
        EXPECT_TRUE(bitField.getBIT_1_16() == 0xFFFF);
        EXPECT_TRUE(bitField.getBIT_17_20() == 0xF);
        EXPECT_TRUE(bitField.getBIT_21_30() == 0x3FF);
        EXPECT_TRUE(bitField.getBIT_31() == true);

        // Get Values
        EXPECT_TRUE(bitField.getValueByName("BIT_0", value) == true);
        EXPECT_TRUE(value == 1);
        EXPECT_TRUE(bitField.getValueByName("BIT_1_16", value) == true);
        EXPECT_TRUE(value == 0xFFFF);
        EXPECT_TRUE(bitField.getValueByName("BIT_17_20", value) == true);
        EXPECT_TRUE(value == 0xF);
        EXPECT_TRUE(bitField.getValueByName("BIT_21_30", value) == true);
        EXPECT_TRUE(value == 0x3FF);
        EXPECT_TRUE(bitField.getValueByName("BIT_31", value) == true);  // Invalid name
        EXPECT_TRUE(value == 1);

        bitField.setBits(0);

        // Set Values
        value = 0x40F;
        EXPECT_TRUE(bitField.setValueByName("BIT_0", value) == true);     
        EXPECT_TRUE(bitField.getBIT_0() == true);
        EXPECT_TRUE(bitField.getBIT_1_16()  == 0);
        EXPECT_TRUE(bitField.setValueByName("BIT_1_16", value) == true);     
        EXPECT_TRUE(bitField.getBIT_1_16()  == 0x40F);
        EXPECT_TRUE(bitField.getBIT_17_20() == 0);
        EXPECT_TRUE(bitField.setValueByName("BIT_17_20", value) == true);     
        EXPECT_TRUE(bitField.getBIT_17_20() == 0xF);
        EXPECT_TRUE(bitField.getBIT_21_30() == 0);
        EXPECT_TRUE(bitField.setValueByName("BIT_21_30", value) == true);     
        EXPECT_TRUE(bitField.getBIT_21_30() == 0x00F);
        EXPECT_TRUE(bitField.getBIT_31() == 0);

        bitField.setBits(0);

        // Generic value sets: 
        EA::TDF::TdfGenericValue genValue;
        value = 0x40F;
        genValue.set(value);
        EXPECT_TRUE(bitField.setValueByName("BIT_1_16", genValue) == true);     
        EXPECT_TRUE(bitField.getBIT_1_16() == value);
    }
}
