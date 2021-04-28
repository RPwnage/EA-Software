///////////////////////////////////////////////////////////////////////////////
// Version.h
// 
// Copyright (c) Electronic Arts. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_VERSION_H
#define EAPATCHCLIENT_VERSION_H


// EAPATCHCLIENT_VERSION
//
// This is a base 10 integral value of the form MMMmmmppp, though leading 
// zeros are not represented. The "patch" below has no relation to the
// "patch" in the name EAPatch. In string form, the standard EA format for
// package version string is used: "M.mm.pp". However, comparisons should
// never be done with the string version, as it is only informative.
//
#define EAPATCHCLIENT_VERSION_MAJOR   1
#define EAPATCHCLIENT_VERSION_MINOR   5
#define EAPATCHCLIENT_VERSION_PATCH   1
#define EAPATCHCLIENT_VERSION         ((EAPATCHCLIENT_VERSION_MAJOR * 1000000) + (EAPATCHCLIENT_VERSION_MINOR * 1000) + EAPATCHCLIENT_VERSION_PATCH)
#define EAPATCHCLIENT_VERSION_STRING  "1.05.01"


// EAPATCHCLIENT_CREATE_VERSION_NUMBER
//
// Example usage:
//     #if EAPATCHCLIENT_VERSION > EAPATCHCLIENT_CREATE_VERSION_NUMBER(1, 1, 0)
//         printf("EAPatchClient version is greater than 1.1.0.\n");
//     #endif
//
#define EAPATCHCLIENT_CREATE_VERSION_NUMBER(major, minor, patch) \
    (((major) * 1000000) + ((minor) * 1000) + (patch))


#endif // Header include guard
