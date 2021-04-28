// -- FILE ------------------------------------------------------------------
// name       : IRtfDocumentInfo.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.23
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;

#ifndef __RTFDOCUMENTINFO_H
#define __RTFDOCUMENTINFO_H

#include <qstring.h>
#include <qdatetime.h>
#include "ObjectBase.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfDocumentInfo: public ObjectBase
    {
        public:
            RtfDocumentInfo (QObject* parent = 0);
            virtual ~RtfDocumentInfo ();

            // ----------------------------------------------------------------------
            virtual QString& ToString();

            // ----------------------------------------------------------------------
            int getId ();
            void setId (int value);

            // ----------------------------------------------------------------------
            int getVersion ();
            void setVersion (int value);

            // ----------------------------------------------------------------------
            int getRevision ();
            void setRevision (int value);

            // ----------------------------------------------------------------------
            QString& getTitle ();
            void setTitle (QString& str);

            // ----------------------------------------------------------------------
            QString& getSubject ();
            void setSubject (QString& str);

            // ----------------------------------------------------------------------
            QString& getAuthor ();
            void setAuthor (QString& str);

            // ----------------------------------------------------------------------
            QString& getManager ();
            void setManager (QString& str);

            // ----------------------------------------------------------------------
            QString& getCompany ();
            void setCompany (QString& str);

            // ----------------------------------------------------------------------
            QString& getOperator ();
            void setOperator (QString& str);

            // ----------------------------------------------------------------------
            QString& getCategory ();
            void setCategory (QString& str);

            // ----------------------------------------------------------------------
            QString& getKeywords ();
            void setKeywords (QString& str);

            // ----------------------------------------------------------------------
            QString& getComment ();
            void setComment (QString& str);

            // ----------------------------------------------------------------------
            QString& getDocumentComment ();
            void setDocumentComment (QString& str);

            // ----------------------------------------------------------------------
            QString& getHyperLinkbase ();
            void setHyperLinkbase (QString& str);

            // ----------------------------------------------------------------------
            QDateTime& getCreationTime ();
            void setCreationTime (QDateTime& time);

            // ----------------------------------------------------------------------
            QDateTime& getRevisionTime ();
            void setRevisionTime (QDateTime& time);

            // ----------------------------------------------------------------------
            QDateTime& getPrintTime ();
            void setPrintTime (QDateTime& time);

            // ----------------------------------------------------------------------
            QDateTime& getBackupTime ();
            void setBackupTime (QDateTime& time);

            // ----------------------------------------------------------------------
            int getNumberOfPages ();
            void setNumberOfPages (int value);

            // ----------------------------------------------------------------------
            int getNumberOfWords ();
            void setNumberOfWords (int value);

            // ----------------------------------------------------------------------
            int getNumberOfCharacters ();
            void setNumberOfCharacters (int value);

            // ----------------------------------------------------------------------
            int getEditingTimeInMinutes ();
            void setEditingTimeInMinutes (int value);

            // ----------------------------------------------------------------------
            void Reset();


        private:
            int id;
            int version;
            int revision;
            QString title;
            QString subject;
            QString author;
            QString manager;
            QString company;
            QString operatorName;
            QString category;
            QString keywords;
            QString comment;
            QString documentComment;
            QString hyperLinkbase;
            QDateTime creationTime;
            QDateTime revisionTime;
            QDateTime printTime;
            QDateTime backupTime;
            int numberOfPages;
            int numberOfWords;
            int numberOfCharacters;
            int editingTimeInMinutes;

            QString toStr;
    }; // interface IRtfDocumentInfo

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
#endif //__RTFDOCUMENTINFO_H