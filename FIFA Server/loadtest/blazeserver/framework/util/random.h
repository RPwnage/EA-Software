/*************************************************************************************************/
/*!
    \file random.h

    Contains a namespace with random number generation functions.

    
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_RANDOM_H
#define BLAZE_RANDOM_H
#include <stdlib.h>

#if defined(EA_PLATFORM_WINDOWS)
#include <wincrypt.h>
#endif

#include "framework/tdf/usersessiontypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{


/*! ************************************************************************************************/
/*!
    \brief A random number helper class. Provides two ways to generate random numbers:
     1) by using static functions initializeSeed() and getRandomNumber() with better
     performance but low quality of generated random number
     2) by using getStrongRandomNumber() on Random object where generated number has cryptography
     quality however there is some performance cost
*************************************************************************************************/
class Random
{
public:
    Random(); 
    ~Random(); 

#if defined(EA_PLATFORM_LINUX)
    static const int MAX = RAND_MAX;
#else
    static const int MAX = INT_MAX;
#endif

    /*! ************************************************************************************************/
    /*!
        \brief seeds  the random number generator
    *************************************************************************************************/
    static void initializeSeed();


    /*! ************************************************************************************************/
    /*!
        \brief return a uniformly distributed random float from the range [minVal..maxVal).

            NOTE: Unless srand is initialized before this function is called it will return
            the same sequence of random numbers from Blaze Startup.
    
        \param[in] minVal the lower bound of the random number range.  Must be a valid float.
        \param[in] maxVal the upper bound of the random number range.  Must be a valid float.
        \return an float between 0 and range-1 (inclusive).
    *************************************************************************************************/
    static float getRandomFloat(float minVal = 0.0f, float maxVal = 1.0f);    

    /*! ************************************************************************************************/
    /*!
        \brief return a uniformly distributed random integer from the range [0..range).

            NOTE: Unless srand is initialized before this function is called it will return
            the same sequence of random numbers from Blaze Startup.
    
        \param[in] range the upper bound of the random number range.  Must be between 0 and Random::MAX.
        \return an int between 0 and range-1 (inclusive).
    *************************************************************************************************/
    static int getRandomNumber(int range);

    /*! ************************************************************************************************/
    /*!
        \brief return a high quality uniformly distributed random integer from the range [0..range).

            NOTE: The generated number has best possible quality on given system and 
            it can be used in cryptography or to generate keys such as session key, 
            however there is performance cost involved.
    
        \param[in] range the upper bound of the random number range.  Must be between 0 and Random::MAX.
        \return an int between 0 and range-1 (inclusive). Returns -1 on failure.
    *************************************************************************************************/
    int getStrongRandomNumber(int range);

    bool getStrongRandomNumber256(Blaze::Int256Buffer& result);
    bool getStrongRandomNumber256(int64_t (&result)[4]);

private:
    bool getStrongRandomNumber(void *buf, size_t bufLen);
    
#if defined(EA_PLATFORM_LINUX)
    static uint32_t mRandomSeed;
    FILE *mRndFile;
#elif defined(EA_PLATFORM_WINDOWS)
    HCRYPTPROV mProviderHandle;
#endif

 };

extern EA_THREAD_LOCAL Random* gRandom;

} // Blaze
#endif // BLAZE_RANDOM_H
