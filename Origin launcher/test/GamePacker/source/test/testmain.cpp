/*************************************************************************************************/
/*!
\file testmain.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include <EABase/eabase.h>

#include "gtest/gtest.h"



int main(int argc, char **argv) 
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


// need to define these operators, they are needed for eastl
void * operator new[](unsigned __int64 size, char const *, int, unsigned int, char const *, int)
{
    return new char8_t[size];
}

void * operator new[](unsigned __int64 size, unsigned __int64, unsigned __int64, char const *, int, unsigned int, char const *, int)
{
    return new char8_t[size];
}
