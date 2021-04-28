// -- FILE ------------------------------------------------------------------
// name       : RtfDocumentInfo.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.23
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;

#include <assert.h>
#include "RtfDocumentInfo.h"

namespace RTF2HTML
{
    RtfDocumentInfo::RtfDocumentInfo (QObject* parent)
        : ObjectBase (parent)
    {
    }

    RtfDocumentInfo::~RtfDocumentInfo()
    {
    }

    // ------------------------------------------------------------------------
    // ----------------------------------------------------------------------
    int RtfDocumentInfo::getId ()
    {
        return id;
    } // Id

    void RtfDocumentInfo::setId (int value)
    {
        id = value;
    }

    // ----------------------------------------------------------------------
    int RtfDocumentInfo::getVersion ()
    {
        return version;
    } // Version

    void RtfDocumentInfo::setVersion (int value)
    {
        version = value;
    }

    // ----------------------------------------------------------------------
    int RtfDocumentInfo::getRevision ()
    {
        return revision;
    } // Revision

    void RtfDocumentInfo::setRevision (int value)
    {
        revision = value;
    } // Revision
    // ----------------------------------------------------------------------
    QString& RtfDocumentInfo::getTitle ()
    {
        return title;
    } // Title

    void RtfDocumentInfo::setTitle (QString& str)
    {
        title = str;
    } // Title

    // ----------------------------------------------------------------------
    QString& RtfDocumentInfo::getSubject ()
    {
        return subject;
    } // Title

    void RtfDocumentInfo::setSubject (QString& str)
    {
        subject = str;
    } // Title

    // ----------------------------------------------------------------------
    QString& RtfDocumentInfo::getAuthor ()
    {
        return author;
    } // Title

    void RtfDocumentInfo::setAuthor (QString& str)
    {
        author = str;
    } // Title

    // ----------------------------------------------------------------------
    QString& RtfDocumentInfo::getManager ()
    {
        return manager;
    } // Title

    void RtfDocumentInfo::setManager (QString& str)
    {
        manager = str;
    } // Title

    // ----------------------------------------------------------------------
    QString& RtfDocumentInfo::getCompany ()
    {
        return company;
    } // Title

    void RtfDocumentInfo::setCompany (QString& str)
    {
        company = str;
    } // Title

    // ----------------------------------------------------------------------
    QString& RtfDocumentInfo::getOperator ()
    {
        return operatorName;
    } // Title

    void RtfDocumentInfo::setOperator (QString& str)
    {
        operatorName = str;
    } // Title

    // ----------------------------------------------------------------------
    QString& RtfDocumentInfo::getCategory ()
    {
        return category;
    } // Title

    void RtfDocumentInfo::setCategory (QString& str)
    {
        category = str;
    } // Title

    // ----------------------------------------------------------------------
    QString& RtfDocumentInfo::getKeywords ()
    {
        return keywords;
    } // Title

    void RtfDocumentInfo::setKeywords (QString& str)
    {
        keywords = str;
    } // Title

    // ----------------------------------------------------------------------
    QString& RtfDocumentInfo::getComment ()
    {
        return comment;
    } // Title

    void RtfDocumentInfo::setComment (QString& str)
    {
        comment = str;
    } // Title

    // ----------------------------------------------------------------------
    QString& RtfDocumentInfo::getDocumentComment ()
    {
        return documentComment;
    } // Title

    void RtfDocumentInfo::setDocumentComment (QString& str)
    {
        documentComment = str;
    } // Title

    // ----------------------------------------------------------------------
    QString& RtfDocumentInfo::getHyperLinkbase()
    {
        return hyperLinkbase;
    } // HyperLinkbase

    void RtfDocumentInfo::setHyperLinkbase(QString& str)
    {
        hyperLinkbase = str;
    } // HyperLinkbase

    // ----------------------------------------------------------------------
    QDateTime& RtfDocumentInfo::getCreationTime ()
    {
        return creationTime;
    } // CreationTime

    void RtfDocumentInfo::setCreationTime (QDateTime& time)
    {
        creationTime = time;
    } // CreationTime

    // ----------------------------------------------------------------------
    QDateTime& RtfDocumentInfo::getRevisionTime ()
    {
        return revisionTime;
    } // CreationTime

    void RtfDocumentInfo::setRevisionTime (QDateTime& time)
    {
        revisionTime = time;
    } // CreationTime

    // ----------------------------------------------------------------------
    QDateTime& RtfDocumentInfo::getPrintTime ()
    {
        return printTime;
    } // CreationTime

    void RtfDocumentInfo::setPrintTime (QDateTime& time)
    {
        printTime = time;
    } // CreationTime

    // ----------------------------------------------------------------------
    QDateTime& RtfDocumentInfo::getBackupTime ()
    {
        return backupTime;
    } // CreationTime

    void RtfDocumentInfo::setBackupTime (QDateTime& time)
    {
        backupTime = time;
    } // CreationTime

    // ----------------------------------------------------------------------
    int RtfDocumentInfo::getNumberOfPages ()
    {
        return numberOfPages;
    }

    void RtfDocumentInfo::setNumberOfPages (int value)
    {
        numberOfPages = value;
    }

    // ----------------------------------------------------------------------
    int RtfDocumentInfo::getNumberOfWords ()
    {
        return numberOfWords;
    }

    void RtfDocumentInfo::setNumberOfWords (int value)
    {
        numberOfWords = value;
    }

    // ----------------------------------------------------------------------
    int RtfDocumentInfo::getNumberOfCharacters ()
    {
        return numberOfCharacters;
    }

    void RtfDocumentInfo::setNumberOfCharacters (int value)
    {
        numberOfCharacters = value;
    }

    // ----------------------------------------------------------------------
    int RtfDocumentInfo::getEditingTimeInMinutes ()
    {
        return editingTimeInMinutes;
    }

    void RtfDocumentInfo::setEditingTimeInMinutes (int value)
    {
        editingTimeInMinutes = value;
    }

    // ----------------------------------------------------------------------
    void RtfDocumentInfo::Reset()
    {
        const int USE_AS_UNDEFINED = -1;

        id = USE_AS_UNDEFINED;
        version = USE_AS_UNDEFINED;
        revision = USE_AS_UNDEFINED;

        title = "";
        subject = "";
        author = "";
        manager = "";
        company = "";
        operatorName = "";
        category = "";
        keywords = "";
        comment = "";
        documentComment = "";
        hyperLinkbase = "";

        creationTime = QDateTime();
        revisionTime = QDateTime();
        printTime = QDateTime();
        backupTime = QDateTime();

        numberOfPages = USE_AS_UNDEFINED;
        numberOfWords = USE_AS_UNDEFINED;
        numberOfCharacters = USE_AS_UNDEFINED;
        editingTimeInMinutes = USE_AS_UNDEFINED;
    } // Reset

    // ----------------------------------------------------------------------
    QString& RtfDocumentInfo::ToString()
    {
        if (toStr.isEmpty())
        {
            toStr = "RTFDocInfo";
        }
        return toStr;
    } // ToString



} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
