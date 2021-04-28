#ifndef __ORIGINMAP_H__
#define __ORIGINMAP_H__

//This is a workaround to warnings when including map.
//See: http://stackoverflow.com/questions/3232669/is-there-an-alternative-to-suppressing-warnings-for-unreachable-code-in-the-xtre
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4702)
#include <xtree>
#include <map>
#pragma warning(pop)
#endif //WIN32

#ifndef WIN32
#include <map>
#endif
#endif  //__ORIGINMAP_H__