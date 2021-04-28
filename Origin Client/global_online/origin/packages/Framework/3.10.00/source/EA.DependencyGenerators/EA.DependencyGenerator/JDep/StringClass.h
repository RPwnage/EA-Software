/*

	StringClass.h - String support class

*/

#ifndef __STRINGCLASS_H__
#define __STRINGCLASS_H__

namespace JDepString
{
    const int STRING_Prealloc = 32;

    class String
    {
    private:
	    char *pBuf;
	    char szString[STRING_Prealloc];
	    bool bAllocd;

    public:
	    String() : pBuf(0), bAllocd(false) {szString[0] = 0;}

	    String(const char *pSrc);
	    ~String() {if(bAllocd) delete [] pBuf;}

	    void Alloc(int Len) {Release(); if(Len > STRING_Prealloc) {pBuf = new char[Len]; bAllocd=true;} }
	    void Release(void) {if(bAllocd) delete [] pBuf; pBuf = 0; szString[0] = 0; bAllocd = false;}

	    operator char *(void) const {return pBuf;}
	    char &operator[](int Index) const {return pBuf[Index];}

	    void Set(const char *pSrc, int Len);

	    String &operator=(const char *pSrc);
	    String &operator=(const String &Src);

	    String &operator+=(const char *pSrc);

	    bool operator==(char *pStr) const;
	    bool operator==(String &Str) const;

	    int Length(void) const;
    };
}

// this is super bad practice, but unfortunately the String Class defined above 
// conflicts with the .Net framework version in 1.1 so we need to wrap it in a ns
using namespace JDepString;

#endif
