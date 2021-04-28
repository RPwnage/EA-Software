// -- FILE ------------------------------------------------------------------
// name       : RtfGroup.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.Text;
//using Itenso.Sys;

#include "RtfTag.h"
#include "rtfGroup.h"
#include "RtfSpec.h"
#include <assert.h>
#include <sstream>

using namespace RTF2HTML;

namespace RTF2HTML
{

    RtfGroup::RtfGroup(QObject* parent)
        :RtfElement( RtfElementKind::Group, parent )
    {
    } // RtfGroup

    RtfGroup::~RtfGroup()
    {
    }

    // ----------------------------------------------------------------------
    RtfElementCollection& RtfGroup::Contents()
    {
        return contents;
    } // Contents

    // ----------------------------------------------------------------------
    RtfElementCollection& RtfGroup::WritableContents ()
    {
        return contents;
    } // WritableContents

    // ----------------------------------------------------------------------
    QString& RtfGroup::Destination ()
    {
        if ( contents.Count() > 0 )
        {
            RtfElement* firstChild = contents.Get( 0 );
            if ( firstChild->Kind() == RtfElementKind::Tag )
            {
                RtfTag* firstTag = (RtfTag*)(firstChild);
                if ( RtfSpec::TagExtensionDestination == firstTag->Name()  )
                {
                    if ( contents.Count() > 1 )
                    {
                        RtfElement* secondChild = contents.Get( 1 );
                        if ( secondChild->Kind() == RtfElementKind::Tag )
                        {
                            RtfTag* secondTag = (RtfTag*)secondChild;
                            return secondTag->Name();
                        }
                    }
                }
                return firstTag->Name();
            }
        }
        return emptyStr;
    } // Destination

    // ----------------------------------------------------------------------
    bool RtfGroup::IsExtensionDestination ()
    {
        if ( contents.Count() > 0 )
        {
            RtfElement* firstChild = contents.Get( 0 );
            if ( firstChild->Kind() == RtfElementKind::Tag )
            {
                RtfTag* firstTag = (RtfTag*)firstChild;
                if ( RtfSpec::TagExtensionDestination == firstTag->Name() )
                {
                    return true;
                }
            }
        }
        return false;
    } // IsExtensionDestination

    // ----------------------------------------------------------------------
    RtfGroup* RtfGroup::SelectChildGroupWithDestination( QString& destination )
    {
        if ( destination == NULL )
        {
            assert (0);
        }

        std::vector<RTF2HTML::ObjectBase*> contentList = contents.InnerList();

        std::vector<RTF2HTML::ObjectBase*>::iterator it = contentList.begin();
        for (it = contentList.begin(); it != contentList.end(); it++)
        {
            RTF2HTML::RtfElement* child = (RtfElement*)(*it);

            if ( child->Kind() == RtfElementKind::Group )
            {
                RtfGroup* group = (RtfGroup*)child;
                if ( destination == group->Destination())
                {
                    return group;
                }
            }
        }
        return NULL;
    } // SelectChildGroupWithDestination

    // ----------------------------------------------------------------------
    QString& RtfGroup::ToString()
    {
        int count = contents.Count();
        grpStr = QString("{%1 items").arg(count);
        if ( count > 0 )
        {
            // visualize the first two child elements for convenience during debugging
            grpStr += ": [";
            grpStr += contents.Get(0)->ToString();

            if ( count > 1 )
            {
                grpStr += ", ";
                grpStr += contents.Get(1)->ToString();
                if ( count > 2 )
                {
                    grpStr += ", ";
                    if ( count > 3 )
                    {
                        grpStr += "..., ";
                    }
                    grpStr += contents.Get( count - 1)->ToString();
                }
            }
            grpStr += "]";
        }
        grpStr += "}";
        return grpStr;
    } // ToString

    // ----------------------------------------------------------------------
    void RtfGroup::DoVisit( RtfElementVisitor* visitor )
    {
        visitor->VisitGroup( this );
    } // DoVisit

    // ----------------------------------------------------------------------
    bool RtfGroup::IsEqual( ObjectBase* obj )
    {
        Q_UNUSED (obj);
        //doesn't seem to be called so for now, ignore
        assert (0);
        return true;
        /*
                RtfGroup* compare = (RtfGroup*)obj;
                return compare != NULL && IsEqual( obj ) &&
                    contents.Equals( compare->contents );
        */
    } // IsEqual

    /*
        // ----------------------------------------------------------------------
        protected override int ComputeHashCode()
        {
            return HashTool.AddHashCode( base.ComputeHashCode(), contents );
        } // ComputeHashCode

        // ----------------------------------------------------------------------
        // members
        private readonly RtfElementCollection contents = new RtfElementCollection();
    */

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
