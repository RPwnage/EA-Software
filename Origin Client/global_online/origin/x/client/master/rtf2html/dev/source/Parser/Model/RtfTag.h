// -- FILE ------------------------------------------------------------------
// name       : IRtfTag.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------

#ifndef __RTFTAG_H
#define __RTFTAG_H

#include "RtfElement.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    class RtfTag : public RtfElement
    {
        public:
            RtfTag( QString name, QObject* parent = 0 );
            RtfTag( QString name, QString value, QObject* parent = 0 );

            virtual ~RtfTag();

            // ----------------------------------------------------------------------
            /// <summary>
            /// Returns the name together with the concatenated value as it stands in the rtf.
            /// </summary>
            virtual QString& FullName ();

            // ----------------------------------------------------------------------
            virtual QString& Name ();

            // ----------------------------------------------------------------------
            virtual bool HasValue ();

            // ----------------------------------------------------------------------
            virtual QString& ValueAsText ();

            // ----------------------------------------------------------------------
            virtual int ValueAsNumber ();

            virtual QString& ToString();

            // ----------------------------------------------------------------------
            //protected abstract void DoVisit( IRtfElementVisitor visitor );
            virtual void DoVisit( RtfElementVisitor* visitor );

            virtual bool IsEqual( ObjectBase* obj );

        private:
            // ----------------------------------------------------------------------
            // members
            QString fullName;
            QString name;
            QString valueAsText;
            int valueAsNumber;

            QString tagStr;
    }; // interface IRtfTag

} // namespace Itenso.Rtf
#endif //__RTFTAG_H
// -- EOF -------------------------------------------------------------------
