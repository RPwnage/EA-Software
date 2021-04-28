/******************************************************************************/
/*!
    Utils

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     02/24/2009 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/utils.h"
#include <revolution/os/OSContext.h>

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! Utils::GetCallstack

            \brief      Get the callstack information (the addresses).

            \param       exceptionPointers - The platform-specific exception information.
            \param       addresses - An array to store the addresses into.
            \param       addressesMax - The max number of entries available in the addresses array.
            \param       addressesFilledCount - The number of entries used when collecting the callstack (optional).
            \param       baseRsoAddress - The base address of the RSO module (if any, otherwise pass in a value of zero)
            \return      true if successful, false if not.
        */
        /******************************************************************************/
        bool Utils::GetCallstack(void* exceptionPointers, unsigned int* addresses, int addressesMax, int* addressesFilledCount, unsigned int baseRsoAddress)
        {
            bool callstackRetrieved = false;

            if (exceptionPointers && addresses && addressesFilledCount && (addressesMax > 0))
            {
                OSContext* osContext = static_cast<OSContext*>(exceptionPointers);
                unsigned int* frame = (unsigned int*)*((unsigned int*)osContext->gpr[1]);
                unsigned int instruction = frame[1];
                int addressCount = 0;

                // NOTE: The instruction pointer that caused the exception is stored at the exception handling register srr0
                // Save that off first, then walk the frame stack to capture the instructions of the callstack.
                addresses[addressCount++] = GetAdjustedAddress(osContext->srr0, baseRsoAddress);

                // lets walk the stack as much as possible
                for (int stackIndex = 0; (addressCount < (addressesMax - 1)) && instruction; ++stackIndex)
                {
                    addresses[addressCount++] = GetAdjustedAddress((instruction - 4), baseRsoAddress);

                    // Save off this frame pointer to detect if we should stop (below)
                    const void* const prevFrame = frame;

                    // Crawl to next frame in the stack
                    frame = reinterpret_cast<unsigned int*>(*frame);

                    // We must check for valid frame data before continuing.
                    // - If frame is NULL
                    // - If the frame is invalid (marked as 0xffffffff)
                    // - If the frame is a pointer to an address thats before prevFrame
                    if(!frame || (reinterpret_cast<unsigned int>(frame) == 0xffffffff) || (frame < prevFrame))
                    {
                        break;
                    }

                    // lets capture the backchain
                    instruction = frame[1];
                }

                addresses[addressCount] = 0;
                callstackRetrieved = (addressCount > 0); // we found a callstack if we filled anything

                // If requested, return the number of addresses filled
                if (addressesFilledCount)
                {
                    *addressesFilledCount = addressCount;
                }
            }

            return callstackRetrieved;
        }

        /******************************************************************************/
        /*! Utils::GetAdjustedFrameAddress

            \brief      Adjusts address for loaded rso offset if necessary

            \param      address - original address
            \param      baseRsoAddress - The base address of the RSO module (if any, otherwise pass in a value of zero)
            \return     adjusted address (same as address for non-rso builds)

        */
        /******************************************************************************/
        unsigned int Utils::GetAdjustedAddress(unsigned int address, unsigned int baseRsoAddress)
        {
            unsigned int adjustedAddress = address;

            // Only adjust the address if it is located in the RSO itself, leave the addresses
            // located before the RSO alone (i.e. for bootloaders).
            if (adjustedAddress >= baseRsoAddress)
            {
                adjustedAddress -= baseRsoAddress;
            }

            return adjustedAddress;
        }
    }
}