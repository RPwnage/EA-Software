#ifndef _CHATMODEL_NUCLEUSID_H
#define _CHATMODEL_NUCLEUSID_H

#include <QtGlobal>

namespace Origin
{
namespace Chat
{
    ///
    /// Nucleus ID type
    ///
    /// Nucleus IDs are opaque unique numeric identifiers for Nucleus users
    /// 
    typedef quint64 NucleusID;

    ///
    /// Invalid Nucleus ID
    ///
    static const quint64 InvalidNucleusID = 0;
}
}

#endif
