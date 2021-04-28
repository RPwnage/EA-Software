/*H********************************************************************************/
/*!
    \File netprint.c

    \Description
        Test ds_vsnprintf() for compliance with standard routines.

    \Copyright
        Copyright (c) 2009-2010 Electronic Arts Inc.

    \Version 10/28/2009 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "zlib.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

//$$TODO - sockaddr_in/sockaddr_in6 use should be cleaned up
#if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_PS4) || defined(DIRTYCODE_PC) || defined(DIRTYCODE_PS3) || defined(DIRTYCODE_XENON) || defined(DIRTYCODE_WINRT)
struct in6_addr {
    uint8_t         s6_addr[16];   /* IPv6 address */
};

struct sockaddr_in6 {
    uint16_t        sin6_family;   /* AF_INET6 */
    uint16_t        sin6_port;     /* port number */
    uint32_t        sin6_flowinfo; /* IPv6 flow information */
    struct in6_addr sin6_addr;     /* IPv6 address */
    uint32_t        sin6_scope_id; /* Scope ID (new in 2.4) */
};

#ifndef AF_INET6
#define AF_INET6 23
#endif

#endif

/*** Variables ********************************************************************/

// Variables

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _NetPrintStrCmp

    \Description
        Compare two strings; if they are the same, print one out, otherwise
        print both out.

    \Input *pStrCtrl    - pointer to "control" string (what we are expecting)
    \Input *pStrTest    - pointer to "test" string (what we actually produced)
    \Input *pStrType    - pointer to type field, to use in displaying the result

    \Output
        int32_t         - 0=same, 1=different

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintStrCmp(const char *pStrCtrl, const char *pStrTest, const char *pStrType)
{
    int32_t iResult;
    if (strcmp(pStrCtrl, pStrTest) != 0)
    {
        ZPrintf("netprint: [%s] ctrl(%s) != test(%s)\n", pStrType, pStrCtrl, pStrTest);
        iResult = 1;
    }
    else
    {
        ZPrintf("netprint: [%s] \"%s\"\n", pStrType, pStrTest);
        iResult = 0;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintStrIntCmp

    \Description
        Compare two strings AND two results; if they are the same, print one out,
        otherwise print both out.

    \Input *pStrCtrl    - pointer to "control" string (what we are expecting)
    \Input *_pStrTest   - pointer to "test" string (what we actually produced)
    \Input iCtrlRslt    - expected result
    \Input iTestRslt    - actual result
    \Input iBufferLimit - size of buffer we were writing into
    \Input *pStrType    - pointer to type field, to use in displaying the result

    \Output
        int32_t         - 0=same, 1=different

    \Version 02/15/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintStrIntCmp(const char *pStrCtrl, const char *_pStrTest, int32_t iCtrlRslt, int32_t iTestRslt, int32_t iBufferLimit, const char *pStrType)
{
    int32_t iResult, iStrCmp;
    const char *pStrTest;

    // if we have a zero-sized buffer, point to an empty string so we can compare/print it safely
    pStrTest = (iBufferLimit > 0) ? _pStrTest : "";

    // compare the strings
    iStrCmp = strcmp(pStrCtrl, pStrTest);

    if ((iStrCmp != 0) && (iCtrlRslt != iTestRslt))
    {
        ZPrintf("netprint: [%s] ctrl(%s) != test(%s) and ctrl(%d) != test(%d)\n", pStrType, pStrCtrl, pStrTest, iCtrlRslt, iTestRslt);
        iResult = 1;
    }
    else if (iStrCmp != 0)
    {
        ZPrintf("netprint: [%s] ctrl(%s) != test(%s)\n", pStrType, pStrCtrl, pStrTest);
        iResult = 1;
    }
    else if (iCtrlRslt != iTestRslt)
    {
        ZPrintf("netprint: [%s] ctrl(%d) != test(%d)\n", pStrType, iCtrlRslt, iTestRslt);
        iResult = 1;
    }
    else
    {
        if (iTestRslt > 0)
        {
            ZPrintf("netprint: [%s] \"%s\" (%d)\n", pStrType, pStrTest, iTestRslt);
        }
        else
        {
            ZPrintf("netprint: [%s] "" (%d)\n", pStrType, iTestRslt);
        }
        iResult = 0;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintFloat

    \Description
        Print a floating-point value with both ds_snzprintf and platform sprintf(),
        and compare the results.  Flag a warning if they are different.

    \Input *pFmt    - format string
    \Input fValue   - float to print

    \Output
        int32_t     - 0=same, 1=different

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintFloat(const char *pFmt, double fValue)
{
    char strCtrl[256], strTest[256];

    sprintf(strCtrl, pFmt, fValue);
    ds_snzprintf(strTest, sizeof(strTest), pFmt, fValue);

    return(_NetPrintStrCmp(strCtrl, strTest, "flt"));
}

/*F********************************************************************************/
/*!
    \Function _NetPrintInt

    \Description
        Print an integer value with both ds_snzprintf and platform sprintf(),
        and compare the results.  Flag a warning if they are different.

    \Input *pFmt    - format string
    \Input iValue   - integer to print

    \Output
        int32_t     - 0=same, 1=different

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintInt(const char *pFmt, int32_t iValue)
{
    char strCtrl[256], strTest[256];

    sprintf(strCtrl, pFmt, iValue);
    ds_snzprintf(strTest, sizeof(strTest), pFmt, iValue);

    return(_NetPrintStrCmp(strCtrl, strTest, "int"));
}

/*F********************************************************************************/
/*!
    \Function _NetPrintLongInt

    \Description
        Print a 64-bit integer value with both ds_snzprintf and platform sprintf(),
        and compare the results.  Flag a warning if they are different.

    \Input *pFmt    - format string
    \Input iValue   - 64-bit integer to print

    \Output
        int32_t     - 0=same, 1=different

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintLongInt(const char *pFmt, int64_t iValue)
{
    char strCtrl[256], strTest[256];

    sprintf(strCtrl, pFmt, iValue);
    ds_snzprintf(strTest, sizeof(strTest), pFmt, iValue);

    return(_NetPrintStrCmp(strCtrl, strTest, "lng"));
}

/*F********************************************************************************/
/*!
    \Function _NetPrintStr

    \Description
        Print a string with both ds_snzprintf() and platform sprintf(),
        and compare the results.  Flag a warning if they are different.

    \Input *pFmt    - format string
    \Input *pStr    - string to print

    \Output
        int32_t     - 0=same, 1=different

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintStr(const char *pFmt, const char *pStr)
{
    char strCtrl[256], strTest[256];

    sprintf(strCtrl, pFmt, pStr);
    ds_snzprintf(strTest, sizeof(strTest), pFmt, pStr);

    return(_NetPrintStrCmp(strCtrl, strTest, "str"));
}

/*F********************************************************************************/
/*!
    \Function _NetPrintPregen

    \Description
        Print formatted output with ds_vsnprintf() and compare to a pre-generated
        (static) string.  Flag a warning if they are different.

    \Input *pPreGen - pointer to pre-generated string (what we expect)
    \Input *pFmt    - format specifier
    \Input ...      - variable argument list

    \Output
        int32_t     - 0=same, 1=different

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintPregen(const char *pPreGen, const char *pFmt, ...)
{
    char strTest[1024];
    va_list Args;

    // format the output
    va_start(Args, pFmt);
    ds_vsnprintf(strTest, sizeof(strTest), pFmt, Args);
    va_end(Args);

    return(_NetPrintStrCmp(pPreGen, strTest, "pre"));
}

/*F********************************************************************************/
/*!
    \Function _NetPrintOverflow

    \Description
        Print formatted output with ds_vsnprintf() and compare to a pre-generated
        (static) string.  Flag a warning if they are different.

    \Input iBufferLimit     - size we want to limit buffer to
    \Input *pPreGen         - pre-generated string to compare formatted result to
    \Input iExpectedResult  - expected result code from ds_vsnzprintf()
    \Input *pFmt            - format specifier
    \Input ...              - variable argument list

    \Output
        int32_t             - 0=same, 1=different

    \Version 02/15/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintOverflow(int32_t iBufferLimit, const char *pPreGen, int32_t iExpectedResult, const char *pFmt, ...)
{
    char strTest[128];
    int32_t iResult, iStrCmp, iCheck, iMemStomp;
    char cMemChar = 0xcc;
    va_list Args;

    // pre-initialize array
    memset(strTest, cMemChar, sizeof(strTest));

    // format the output
    va_start(Args, pFmt);
    iResult = ds_vsnzprintf(strTest, iBufferLimit, pFmt, Args);
    va_end(Args);

    // make sure we didn't write outside our bounds
    for (iCheck = iBufferLimit, iMemStomp = 0; iCheck < (signed)sizeof(strTest); iCheck += 1)
    {
        if (strTest[iCheck] != cMemChar)
        {
            iMemStomp += 1;
        }
    }

    // did the test succeed or fail?
    iStrCmp = _NetPrintStrIntCmp(pPreGen, strTest, iExpectedResult, iResult, iBufferLimit, "ovr");
    if ((iStrCmp != 0) || (iMemStomp != 0))
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestFlts

    \Description
        Execute a series of floating-point printing tests.

    \Output
        int32_t     - number of test failures

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestFlts(void)
{
    int32_t iResult = 0;
    ZPrintf("netprint: Floating-point comparative tests\n");
    iResult += _NetPrintFloat("%2.0f", 10.0f);
    iResult += _NetPrintFloat("%f", pow(2,52));
    iResult += _NetPrintFloat("%f", pow(2,53));
    iResult += _NetPrintFloat("%.15f", 0.00000000000000000000099f); // we don't test .16 here; ms printf doesn't round at 16+ digits
    iResult += _NetPrintFloat("%.15f", 0.0099f);
    iResult += _NetPrintFloat("%.15f", 0.0000000099f);
    iResult += _NetPrintFloat("%f", 1.99f);
    iResult += _NetPrintFloat("%f", -1.99);
    iResult += _NetPrintFloat("%f", 1.0f);
    iResult += _NetPrintFloat("%f", 0.75f);
    iResult += _NetPrintFloat("%2.2f", 1.0);
    iResult += _NetPrintFloat("%+2.2f", 1.0);
    iResult += _NetPrintFloat("%f", -1.99);
    iResult += _NetPrintFloat("%.2f", 9.99);
    iResult += _NetPrintFloat("%.2f", 9.999);
    iResult += _NetPrintFloat("%.2f", -1.999);
    iResult += _NetPrintFloat("%.2f", 0.1);
    iResult += _NetPrintFloat("%.15f", 3.1415926535897932384626433832795);

    /* this section is for stuff that is not compatible with sprintf() or
       is not compatible with the single-param _NetPrintFloat().  For these
       tests, we compare against a pre-generated string to make sure our
       output is consistently what we expect across all platforms. */

    // make sure all fp selectors result in %f behavior
    iResult += _NetPrintPregen("%e 1.000000 %E 1.000000", "%%e %e %%E %E", 1.0f, 1.0f);
    iResult += _NetPrintPregen("%g 1.000000 %G 1.000000", "%%g %g %%G %G", 1.0f, 1.0f);
    iResult += _NetPrintPregen("%F 1.000000", "%%F %F", 1.0f);
    // test variable width with fp
    iResult += _NetPrintPregen(" 1", "%2.*f", 2, 1.0f);
    // test a really large number, but less than our maximum
    iResult += _NetPrintPregen("9223372036854775808.000000", "%f", pow(2,63));
    // test a really large number, greater than our max
    iResult += _NetPrintPregen("0.(BIG)", "%f", pow(2,64));

    // floating-point test summary
    ZPrintf("netprint: %d floating-point test discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestInts

    \Description
        Execute a series of integer printing tests.

    \Output
        int32_t     - number of test failures

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestInts(void)
{
    int32_t iResult = 0;
    const int32_t iBits = (signed)sizeof(int32_t)*8;
    const int32_t iMaxInt = (1 << (iBits - 1)) + 1;
    int32_t iInt0 = 8;

    ZPrintf("netprint: Integer tests\n");

    iResult += _NetPrintInt("%d = 8", iInt0);
    iResult += _NetPrintInt("%+d = +8", iInt0);
    iResult += _NetPrintInt("%d = -maxint", iMaxInt);
    iResult += _NetPrintInt("char %c = 'a'", 'a');
    iResult += _NetPrintInt("hex %x = ff", 0xff);
    iResult += _NetPrintInt("hex %02x = 00", 0);
    iResult += _NetPrintInt("oct %o = 10", 010);
    iResult += _NetPrintInt("oct %03o = 010", 8);
    iResult += _NetPrintLongInt("%llu", 0x900f123412341234ull);
    iResult += _NetPrintLongInt("0x%llx", 0x900f123412341234ull);
#if defined(DIRTYCODE_PC) || defined(DIRTYCODE_XENON)
    iResult += _NetPrintLongInt("%I64d", 0x900f123412341234ull);
#endif
#if defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_APPLEIOS)
    iResult += _NetPrintLongInt("%qd", 0x900f123412341234ull);
#endif
    iResult += _NetPrintPregen("signed -5 = unsigned 4294967291 = hex fffffffb", "signed %d = unsigned %u = hex %x", -5, -5, -5);
    iResult += _NetPrintPregen("4294967286,-10", "%u,%d", -10, -10);
    iResult += _NetPrintPregen("0,10,20,30,100,200,1000", "%d,%d,%d,%d,%d,%d,%d",  0, 10, 20, 30, 100, 200, 1000);
    iResult += _NetPrintPregen("0,-10,-20,-30,-100,-200,-1000", "%d,%d,%d,%d,%d,%d,%d",  0, -10, -20, -30, -100, -200, -1000);

    ZPrintf("netprint: %d integer test discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestPtrs

    \Description
        Execute a series of pointer printing tests.

    \Output
        int32_t     - number of test failures

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestPtrs(void)
{
    int32_t iResult = 0;
    char *pStr = "string test", *pNul = NULL;
    char strTemp[128];

    ZPrintf("netprint: Pointer tests\n");
#if DIRTYCODE_64BITPTR
    _NetPrintPregen("p=$123456789abcdef0", "p=%p", (void *)0x123456789abcdef0);
    sprintf(strTemp, "p=$%016lx, null p=(null)", (uintptr_t)pStr);
#else
    _NetPrintPregen("p=$12345678", "p=%p", (void *)0x12345678);
    sprintf(strTemp, "p=$%08x, null p=(null)", (uint32_t)pStr);
#endif
    iResult += _NetPrintPregen(strTemp, "p=%p, null p=%p", pStr, pNul);

    ZPrintf("netprint: %d pointer test discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestStrs

    \Description
        Execute a series of string printing tests.

    \Output
        int32_t     - number of test failures

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestStrs(void)
{
    int32_t iResult = 0;
    char *pStr = "string test";
    wchar_t *pWideStr = L"wide string test";

    ZPrintf("netprint: String tests\n");
    iResult += _NetPrintStr("string test=%s", pStr);
#if defined(DIRTYCODE_PC) || defined(DIRTYCODE_XENON)
    iResult += _NetPrintStr("wide string test=%S", (const char *)pWideStr);
#else
    iResult += _NetPrintStr("wide string test=%ls", (const char *)pWideStr);
#endif
    iResult += _NetPrintStr("string test=%s", NULL);

    ZPrintf("netprint: %d string test discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestAlgn

    \Description
        Execute a series of string alignment formatting tests.

    \Output
        int32_t     - number of test failures

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestAlgn(void)
{
    int32_t iResult = 0;

    ZPrintf("netprint: Alignment tests\n");

    iResult += _NetPrintStr(" left just: \"%-10s\"", "left");
    iResult += _NetPrintStr("right just: \"%10s\"", "right");
    iResult += _NetPrintInt(" 6: %04d zero padded", 6);
    iResult += _NetPrintInt(" 6: %-4d left just", 6);
    iResult += _NetPrintInt(" 6: %4d right just", 6);
    iResult += _NetPrintInt("-6: %04d zero padded", -6);
    iResult += _NetPrintInt("-6: %-4d left just", -6);
    iResult += _NetPrintInt("-6: %4d right just", -6);
    iResult += _NetPrintPregen(" 6: 0006 zero padded, variable-length", " 6: %0*d zero padded, variable-length", 4, 6);
    iResult += _NetPrintPregen(" 6: 6    left just, variable-length", " 6: %-*d left just, variable-length", 4, 6);

    iResult += _NetPrintPregen(" a: a        left just", " a: %-8c left just", 'a');
    iResult += _NetPrintPregen(" b:        b right just", " b: %8c right just", 'b');
    iResult += _NetPrintPregen(" c: c        left just, variable-length", " c: %-*c left just, variable-length", 8, 'c');
    iResult += _NetPrintPregen(" d:        d right just, variable-length", " d: %*c right just, variable-length", 8, 'd');

    ZPrintf("netprint: %d alignment test discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestMisc

    \Description
        Execute a series of miscelleneous printing tests.

    \Output
        int32_t     - number of test failures

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestMisc(void)
{
    int32_t iResult = 0;
    int32_t iInt1 = 0;
    char strTest[256], strTest2[128];

    ZPrintf("netprint: Misc tests\n");

    // test %n selector
    ds_snzprintf(strTest, sizeof(strTest), "%%n test %n", &iInt1);
    ds_snzprintf(strTest2, sizeof(strTest2), "(%d chars written)", iInt1);
    ds_strnzcat(strTest, strTest2, sizeof(strTest));
    iResult += _NetPrintStrCmp("%n test (8 chars written)", strTest, "msc");

    iResult += _NetPrintPregen("# test: 10", "# test: %#d", 10);
    iResult += _NetPrintPregen("1 1 1 1 1.000000 1 1 1", "%hd %hhd %ld %lld %f %zd %jd %td", 1, 1, 1l, 1ll, 1.0, 1, 1, 1);
    iResult += _NetPrintPregen("testing invalid trailing pct: ", "testing invalid trailing pct: %");
    iResult += _NetPrintPregen("testing valid trailing pct: %", "testing valid trailing pct: %%");

    ZPrintf("netprint: %d misc test discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestCust

    \Description
        Execute a series of custom printing tests.  These are for selectors that
        are specific to the DirtySock platform string formatting functions.

    \Output
        int32_t     - number of test failures

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestCust(void)
{
    int32_t iResult = 0;
    struct sockaddr SockAddr4;
    struct sockaddr_in6 SockAddr6, SockAddr6_2;
    struct in6_addr Sin6Addr = { { 0x5a, 0x23, 0x01, 0x32, 0xff, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0xde, 0x00, 0x02 } };
    uint32_t uAddr;

    SockaddrInit(&SockAddr4, AF_INET);
    SockaddrInSetAddr(&SockAddr4, 0xC0A80001);
    SockaddrInSetPort(&SockAddr4, 3658);

    // create an IPv4-mapped IPv6 address
    memset(&SockAddr6, 0, sizeof(SockAddr6));
    uAddr = SockaddrInGetAddr(&SockAddr4);
    SockAddr6.sin6_family = AF_INET6;
    SockAddr6.sin6_port = SocketNtohs(SockaddrInGetPort(&SockAddr4));
    SockAddr6.sin6_flowinfo = 0;
    SockAddr6.sin6_addr.s6_addr[10] = 0xff;
    SockAddr6.sin6_addr.s6_addr[11] = 0xff;
    SockAddr6.sin6_addr.s6_addr[12] = (uint8_t)(uAddr >> 24);
    SockAddr6.sin6_addr.s6_addr[13] = (uint8_t)(uAddr >> 16);
    SockAddr6.sin6_addr.s6_addr[14] = (uint8_t)(uAddr >> 8);
    SockAddr6.sin6_addr.s6_addr[15] = (uint8_t)(uAddr >> 0);

    // create a simulated full IPv6 address
    memset(&SockAddr6_2, 0, sizeof(SockAddr6_2));
    SockAddr6_2.sin6_family = AF_INET6;
    SockAddr6_2.sin6_port = SocketNtohs(3658);
    memcpy(&SockAddr6_2.sin6_addr, &Sin6Addr, sizeof(SockAddr6_2.sin6_addr));

    ZPrintf("netprint: Custom tests\n");

    iResult += _NetPrintPregen("addr=192.168.0.1", "addr=%a", SockaddrInGetAddr(&SockAddr4));
    iResult += _NetPrintPregen("addr=255.255.255.255", "addr=%a", 0xffffffff);
    iResult += _NetPrintPregen("addr=[192.168.0.1]:3658", "addr=%A", &SockAddr4);
    iResult += _NetPrintPregen("addr=[::ffff:192.168.0.1]:3658", "addr=%A", &SockAddr6);
    iResult += _NetPrintPregen("addr=[5a23:132:ff12:0:0:0:12de:2]:3658", "addr=%A", &SockAddr6_2);
    iResult += _NetPrintPregen("'dflt' = 'dflt'", "'dflt' = '%C'", 'dflt');
    iResult += _NetPrintPregen("'d*l*' = 'd*l*'", "'d*l*' = '%C'", ('d' << 24) | ('l' << 8));

    ZPrintf("netprint: %d custom test discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestOver

    \Description
        Execute a series of printing tests exercising the overflow functionality.

    \Output
        int32_t     - number of test failures

    \Version 02/15/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestOver(void)
{
    const char strOver[] = "<<<overflow>>>";
    int32_t iResult0 = 0, iResult1 = 0;

    // test some overflow scenarios with ds_snzprintf2
    ZPrintf("netprint: Overflow tests\n");
    iResult1 += _NetPrintOverflow( 0, "",                     14,    "%s", strOver);
    iResult1 += _NetPrintOverflow( 1, "",                     14,    "%s", strOver);
    iResult1 += _NetPrintOverflow(12, "<<<overflow",          14,    "%s", strOver);
    iResult1 += _NetPrintOverflow(14, "<<<overflow>>",        14,    "%s", strOver);
    iResult1 += _NetPrintOverflow(15, "<<<overflow>>>",       14,    "%s", strOver);
    iResult1 += _NetPrintOverflow(20, "      <<<overflow>>",  20,  "%20s", strOver);
    iResult1 += _NetPrintOverflow(20, "<<<overflow>>>     ",  20, "%-20s", strOver);
    iResult1 += _NetPrintOverflow(21, "      <<<overflow>>>", 20,  "%20s", strOver);
    iResult1 += _NetPrintOverflow(16, "-<<<overflow>>>",      16,  "-%s-", strOver);
    iResult1 += _NetPrintOverflow(17, "-<<<overflow>>>-",     16,  "-%s-", strOver);

    ZPrintf("netprint: %d overflow test discrepencies\n", iResult1);
    ZPrintf("netprint: ------------------------------------\n");
    return(iResult0+iResult1);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdNetPrint

    \Description
        Test the ds_vsnprintf function

    \Input *argz    - environment
    \Input argc     - standard number of arguments
    \Input *argv[]  - standard arg list

    \Output
        int32_t     - standard return value

    \Version 10/28/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdNetPrint(ZContext *argz, int32_t argc, char *argv[])
{
    int32_t iResult = 0;

    ZPrintf("netprint: ------------------------------------\n");
    ZPrintf("netprint: Testing ds_snzprintf() vs sprintf()\n");
    ZPrintf("netprint: ------------------------------------\n");

    iResult += _NetPrintTestStrs();
    iResult += _NetPrintTestFlts();
    iResult += _NetPrintTestInts();
    iResult += _NetPrintTestPtrs();
    iResult += _NetPrintTestAlgn();
    iResult += _NetPrintTestMisc();
    iResult += _NetPrintTestCust();
    iResult += _NetPrintTestOver();

    ZPrintf("netprint: Test results: %d total discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");
    return(0);
}


