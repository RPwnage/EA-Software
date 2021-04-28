/*************************************************************************************************/
/*!
\file packertest.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include <EABase/eabase.h>

#include "gamepacker/packer.h"
#include "packertest.h"


// Tests that you can create a packer
TEST_F(PackerTest, InitalizePacker)
{
    Packer::PackerSilo p;
    ASSERT_TRUE(p.getGames().empty());
}


// Tests that Foo does Xyz.
TEST_F(PackerTest, DoesXyz)
{
    // Exercises the Xyz feature of Packer.
}
