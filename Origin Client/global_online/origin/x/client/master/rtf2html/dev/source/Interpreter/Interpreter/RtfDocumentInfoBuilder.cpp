// -- FILE ------------------------------------------------------------------
// name       : RtfDocumentInfoBuilder.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.23
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Rtf.Model;
//using Itenso.Rtf.Support;

#include <assert.h>
#include "RtfDocumentInfoBuilder.h"
#include "RtfGroup.h"
#include "RtfTag.h"
#include "RtfSpec.h"

namespace RTF2HTML
{


    // ----------------------------------------------------------------------
    RtfDocumentInfoBuilder::RtfDocumentInfoBuilder( RtfDocumentInfo* info, QObject* parent )
        : RtfElementVisitorBase (RtfElementVisitorOrder::NonRecursive, parent )
    {
        // we iterate over our children ourselves -> hence non-recursive
        if ( info == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "info" );
        }
        this->info = info;

        textBuilder = new RtfTextBuilder(this);
        timestampBuilder = new RtfTimestampBuilder(this);
    } // RtfDocumentInfoBuilder

    RtfDocumentInfoBuilder::~RtfDocumentInfoBuilder()
    {
    }

    // ----------------------------------------------------------------------
    void RtfDocumentInfoBuilder::Reset()
    {
        info->Reset();
    } // Reset

    // ----------------------------------------------------------------------
    void RtfDocumentInfoBuilder::DoVisitGroup( RtfGroup* group )
    {
        QString dest = group->Destination();

        if (dest == RtfSpec::TagInfo)
        {
            VisitGroupChildren( group );
        }
        else if (dest == RtfSpec::TagInfoTitle)
        {
            info->setTitle (ExtractGroupText( group ));
        }
        else if (dest == RtfSpec::TagInfoSubject)
        {
            info->setSubject (ExtractGroupText( group ));
        }
        else if (dest == RtfSpec::TagInfoAuthor)
        {
            info->setAuthor (ExtractGroupText( group ));
        }
        else if (dest == RtfSpec::TagInfoManager)
        {
            info->setManager (ExtractGroupText( group ));
        }
        else if (dest == RtfSpec::TagInfoCompany)
        {
            info->setCompany ( ExtractGroupText( group ));
        }
        else if (dest == RtfSpec::TagInfoOperator)
        {
            info->setOperator (ExtractGroupText( group ));
        }
        else if (dest == RtfSpec::TagInfoCategory)
        {
            info->setCategory (ExtractGroupText( group ));
        }
        else if (dest == RtfSpec::TagInfoKeywords)
        {
            info->setKeywords (ExtractGroupText( group ));
        }
        else if (dest == RtfSpec::TagInfoComment)
        {
            info->setComment ( ExtractGroupText( group ) );
        }
        else if (dest == RtfSpec::TagInfoDocumentComment)
        {
            info->setDocumentComment (ExtractGroupText( group ));
        }
        else if (dest == RtfSpec::TagInfoHyperLinkBase)
        {
            info->setHyperLinkbase ( ExtractGroupText( group ));
        }
        else if (dest == RtfSpec::TagInfoCreationTime)
        {
            //since ExtractTimeStamp "new"s QDateTime, delete it after it's been copied
            QDateTime* ts = ExtractTimestamp( group );
            info->setCreationTime (*ts);
            delete ts;
        }
        else if (dest == RtfSpec::TagInfoRevisionTime)
        {
            //since ExtractTimeStamp "new"s QDateTime, delete it after it's been copied
            QDateTime* ts = ExtractTimestamp( group );
            info->setRevisionTime ( *ts );
            delete ts;
        }
        else if (dest == RtfSpec::TagInfoPrintTime)
        {
            //since ExtractTimeStamp "new"s QDateTime, delete it after it's been copied
            QDateTime* ts = ExtractTimestamp( group );
            info->setPrintTime ( *ts );
            delete ts;
        }
        else if (dest == RtfSpec::TagInfoBackupTime)
        {
            //since ExtractTimeStamp "new"s QDateTime, delete it after it's been copied
            QDateTime* ts = ExtractTimestamp( group );
            info->setBackupTime ( *ts );
            delete ts;
        }
    } // DoVisitGroup

    // ----------------------------------------------------------------------
    void RtfDocumentInfoBuilder::DoVisitTag( RtfTag* tag )
    {
        QString tagName = tag->Name();
        int value = tag->ValueAsNumber();

        if (tagName == RtfSpec::TagInfoVersion)
        {
            info->setVersion (value);
        }
        else if (tagName == RtfSpec::TagInfoRevision)
        {
            info->setRevision (value);
        }
        else if (tagName == RtfSpec::TagInfoNumberOfPages)
        {
            info->setNumberOfPages (value);
        }
        else if (tagName == RtfSpec::TagInfoNumberOfWords)
        {
            info->setNumberOfWords (value);
        }
        else if (tagName == RtfSpec::TagInfoNumberOfChars)
        {
            info->setNumberOfCharacters (value);
        }
        else if (tagName == RtfSpec::TagInfoId)
        {
            info->setId (value);
        }
        else if (tagName == RtfSpec::TagInfoEditingTimeMinutes)
        {
            info->setEditingTimeInMinutes (value);
        }
    } // DoVisitTag

    // ----------------------------------------------------------------------
    QString& RtfDocumentInfoBuilder::ExtractGroupText( RtfGroup* group )
    {
        textBuilder->Reset();
        textBuilder->VisitGroup( group );
        return textBuilder->CombinedText();
    } // ExtractGroupText

    // ----------------------------------------------------------------------
    QDateTime* RtfDocumentInfoBuilder::ExtractTimestamp( RtfGroup* group )
    {
        timestampBuilder->Reset();
        timestampBuilder->VisitGroup( group );
        return timestampBuilder->CreateTimestamp();
    } // ExtractTimestamp

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
