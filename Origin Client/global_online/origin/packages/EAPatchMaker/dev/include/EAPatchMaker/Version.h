///////////////////////////////////////////////////////////////////////////////
// Version.h
// 
// Copyright (c) Electronic Arts. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHMAKER_VERSION_H
#define EAPATCHMAKER_VERSION_H


// EAPATCHMAKER_VERSION
//
// This is a base 10 integral value of the form MMMmmmppp, though leading 
// zeros are not represented.
//
#define EAPATCHMAKER_VERSION_MAJOR   1
#define EAPATCHMAKER_VERSION_MINOR   5
#define EAPATCHMAKER_VERSION_PATCH   2
#define EAPATCHMAKER_VERSION         ((EAPATCHMAKER_VERSION_MAJOR * 1000000) + (EAPATCHMAKER_VERSION_MINOR * 1000) + EAPATCHMAKER_VERSION_PATCH)
#define EAPATCHMAKER_VERSION_STRING  "1.05.02"


// EAPATCHMAKER_CREATE_VERSION_NUMBER
//
// Example usage:
//     #if EAPATCHMAKER_VERSION > EAPATCHMAKER_CREATE_VERSION_NUMBER(1, 1, 0)
//         printf("EAPatchMaker version is greater than 1.1.0.\n");
//     #endif
//
#define EAPATCHMAKER_CREATE_VERSION_NUMBER(major, minor, patch) \
    (((major) * 1000000) + ((minor) * 1000) + (patch))


#endif // Header include guard
