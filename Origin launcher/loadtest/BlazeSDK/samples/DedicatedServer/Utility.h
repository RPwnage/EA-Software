/*! ****************************************************************************
    \file Utility.h
    \brief
    Various small utility functions


    \attention
        Copyright (c) Electronic Arts 2011.  ALL RIGHTS RESERVED.

*******************************************************************************/

#ifndef __UTILITY_H__
#define __UTILITY_H__

//------------------------------------------------------------------------------
const char * GetDirtySockVersionString(void);
void Log(const char *format, ...);
void CheckForMemoryLeaks(void);

//------------------------------------------------------------------------------
#endif // __UTILITY_H__

