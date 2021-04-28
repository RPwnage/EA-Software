// -- FILE ------------------------------------------------------------------
// name       : RtfSpec.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Text;

#include "RtfSpec.h"

namespace RTF2HTML
{
    namespace RtfSpec
    {
        int GetCodePage( int charSet )
        {
            switch ( charSet )
            {
                case 0:
                    return 1252; // ANSI
                case 1:
                    return 0; // Default
                case 2:
                    //return 42; // Symbol
                    return 0;   //ICU doesn't support
                case 77:
                    return 10000; // Mac Roman
                case 78:
                    //return 10001; // Mac Shift Jis
                    return 932; //since ICU doesn't have Mac version, use the pc one
                case 79:
                    //return 10003; // Mac Hangul
                    return 949; //since ICU doesn't have Mac version, use the pc one
                case 80:
                    //return 10008; // Mac GB2312
                    return 936; //since ICU doesn't have Mac version, use the pc one
                case 81:
                    //return 10002; // Mac Big5
                    return 950; // Big5 -since ICU doesn't have Mac version, use the pc one
                case 82:
                    return 0; // Mac Johab (old)
                case 83:
                    //return 10005; // Mac Hebrew
                    return 1255; // Hebrew - since ICU doesn't have Mac version, use the pc one
                case 84:
                    //return 10004; // Mac Arabic
                    return 1256; // Arabic
                case 85:
                    return 10006; // Mac Greek
                case 86:
                    return 10081; // Mac Turkish
                case 87:
//                  return 10021; // Mac Thai
                    return 874; // Thai
                case 88:
//                  return 10029; // Mac East Europe
                    return 1250; // Eastern European
                case 89:
                    return 10007; // Mac Russian
                case 128:
                    return 932; // Shift JIS
                case 129:
                    return 949; // Hangul
                case 130:
//                  return 1361; // Johab
                    return 0;       //ICU doesn't support
                case 134:
                    return 936; // GB2312
                case 136:
                    return 950; // Big5
                case 161:
                    return 1253; // Greek
                case 162:
                    return 1254; // Turkish
                case 163:
                    return 1258; // Vietnamese
                case 177:
                    return 1255; // Hebrew
                case 178:
                    return 1256; // Arabic
                case 179:
                    return 0; // Arabic Traditional (old)
                case 180:
                    return 0; // Arabic user (old)
                case 181:
                    return 0; // Hebrew user (old)
                case 186:
                    return 1257; // Baltic
                case 204:
                    return 1251; // Russian
                case 222:
                    return 874; // Thai
                case 238:
                    return 1250; // Eastern European
                case 254:
                    return 437; // PC 437
                case 255:
                    return 850; // OEM
            }

            return 0;
        } // GetCodePage

        char* GetCodePageName (int codePage)
        {
            char* codePageName = "windows-1252";
            switch(codePage)
            {
                case 437:
                    codePageName = "cp437";
                    break;

                case 850: // OEM
                    codePageName = "cp850";
                    break;

                case 874: // Thai
                    codePageName = "windows-874";
                    break;

                case 932: // Shift JIS
                    codePageName = "cp932";
                    break;

                case 936: // GB2312
                    codePageName = "cp936";
                    break;

                case 949: // Hangul
                    codePageName = "windows-949";
                    break;

                case 950: // Big5
                    codePageName = "Big5";//"windows-950";
                    break;

                case 1250: // Eastern European
                    codePageName = "cp1250";
                    break;

                case 1251: // Russian
                    codePageName = "cp1251";
                    break;


                case 1252:  // ANSI
                    codePageName = "cp1252";
                    break;

                case 1253: // Greek
                    codePageName = "cp1253";
                    break;

                case 1254: // Turkish
                    codePageName = "cp1254";
                    break;

                case 1255: // Hebrew
                    codePageName = "cp1255";
                    break;

                case 1256: // Arabic
                    codePageName = "cp1256";
                    break;

                case 1257: // Baltic
                    codePageName = "cp1257";
                    break;

                case 1258: // Vietnamese
                    codePageName = "cp1258";
                    break;

                case 10000: // Mac Roman
                    codePageName = "macroman";
                    break;

                case 10006: // Mac Greek
                    codePageName = "macrgr";
                    break;

                case 10007: // Mac Russian
                    codePageName = "mac-cyrillic";
                    break;

                case 10081: // Mac Turkish
                    codePageName = "mactr";
                    break;

                case 0:
                default:
                    codePageName = "windows-1252";
                    break;
            }
            return codePageName;
        }
    } // class RtfSpec

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
