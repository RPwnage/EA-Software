/******************************************************************************/
/*!
Utils

Copyright 2010 Electronic Arts Inc.

Version       1.0     02/24/2009 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/utils.h"

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
            \return      true if successful, false if not.
        */
        /******************************************************************************/
        bool Utils::GetCallstack(void* exceptionPointers, unsigned int* addresses, int addressesMax, int* addressesFilledCount)
        {
            bool callstackRetrieved = false;

            if (addresses && addressesFilledCount && (addressesMax > 0))
            {
                size_t addressCount = ExtractPs3Callstack(exceptionPointers, reinterpret_cast<void**>(addresses), addressesMax);

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
        /*! Utils::ExtractPs3Callstack

            \brief      Extracts the callstack for PS3.

            \param       topStackFrame - Stack frame pointer at which to start the stack trace, or NULL to start at the top of current stack.
            \param       returnAddressArray - Pointer to an array to place the callstack addresses in.
            \param       returnAddressArrayCapacity - Capacity of array to limit callstack addresses returned.
            \return      number of callstack addresses found.
        */
        /******************************************************************************/
        size_t Utils::ExtractPs3Callstack(void *topStackFrame, void* returnAddressArray[], size_t returnAddressArrayCapacity)
        {
            size_t    entryIndex(0);
            uint64_t* frame;           // The PS3 with 32 bit pointers still uses 64 bit pointers in its stack frames.
            void*     instruction;

            if (returnAddressArrayCapacity != 0)
            {

                if((topStackFrame != NULL) /*&& ((*((uint64_t**)topStackFrame)) != NULL)*/)
                {
                    frame = (uint64_t*)(uintptr_t)(*((uint64_t*)topStackFrame));

                }
                else
                {
                    // General PowerPC asm for reading the instruction pointer (program counter).
                    // mflr r4                  // Save link register
                    // bl NextLine              // Jump to the next line. Instruction pointer will be in the link register (LR)
                    // NextLine:                //
                    // mflr r3                  // Copy LR to some other register.
                    // mtlr r4                  // Restore the original LR.
                    // std r4, pInstruction     // std for 64 bit pointers, stw for 32 bit pointers
#if defined(__SNC__)
                    // We make a bogus function call to ourselves that we know will have no effect. We do this because we 
                    // need to force the generation of a stack frame for this function. Otherwise the optimizer might 
                    // realize this function calls no others and eliminate the stack frame.
                    //				CallstackContext context;
                    //				context.mGPR1 = 0;
                    //				context.mIAR  = 0;
                    ExtractPs3Callstack(NULL,returnAddressArray, 0);
    
                    frame = (uint64_t*)__builtin_frame_address();
#else
                    // The following statement causes the compiler to generate a stack frame
                    // for this function, as if this function were going to need to call 
                    // another function. We could call some arbitrary function here, but the
                    // statement below has the same effect under GCC.
                    __asm__ __volatile__("" : "=l" (frame) : );
    
                    // Load into pFrame (%0) the contents of what is pointed to by register 1 with 0 offset.
                    #if (EA_PLATFORM_PTR_SIZE == 8)
                        __asm__ __volatile__("ld %0, 0(1)" : "=r" (frame) : );
                    #else
                        uint64_t  temp;
                        __asm__ __volatile__("ld %0, 0(1)" : "=r" (temp) : );
                        frame = (uint64_t*)(uintptr_t)temp;
                    #endif
#endif
                }


                instruction = (void*)(uintptr_t)frame[2];

                while(frame && (entryIndex < (returnAddressArrayCapacity - 1)))
                {
                    // The LR save area of a stack frame holds the return address, and what
                    // we really want is the address of the calling instruction, which is in
                    // many cases the instruction located just before the return address.
                    // On PPC, this is simple to calculate due to every instruction being 
                    // four bytes in length. To do: Only do the -4 adjustment below if the 
                    // instruction before pInstruction is a function-calling instruction.
                    instruction = (void*)((uintptr_t)instruction - 4);

                    returnAddressArray[entryIndex++] = instruction;

                    frame = (uint64_t*)(uintptr_t)*frame;

                    // If pFrame or *pFrame is 0 we quit.
                    if(!frame || !(uintptr_t)*frame )
                    {
                        break;
                    }

                    instruction = (void*)(uintptr_t)frame[2];
                }

                returnAddressArray[entryIndex] = 0;
            }

            return entryIndex;
        }

    }
}