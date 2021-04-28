///////////////////////////////////////////////////////////////////////////////
// TrialServiceResponse.cpp
//
// Author: Hans van Veenendaal
// Copyright (c) 2015 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/trials/TrialServiceResponse.h"
#include "services/debug/DebugService.h"

#include <QDomDocument>
#include <NodeDocument.h>
#include <ReaderCommon.h>
#include "serverReader.h"

namespace Origin
{
    namespace Services
    {
        namespace Trials
        {

            TrialBurnTimeResponse::TrialBurnTimeResponse(AuthNetworkRequest request) :
                OriginAuthServiceResponse(request)
            {
                setDeleteOnFinish(true);
            }

            bool TrialBurnTimeResponse::parseSuccessBody(QIODevice *body)
            {
                QScopedPointer<INodeDocument> pDoc(CreateJsonDocument());

                QByteArray data = body->readAll();
                if(pDoc->Parse(data.data()))
                {
                    server::Read(pDoc.data(), m_burnTime);
                    m_timeStamp.fromString(reply()->rawHeader("Date"), Qt::RFC2822Date);

                    return true;
                }
                return false;
            }

            TrialCheckTimeResponse::TrialCheckTimeResponse(AuthNetworkRequest request) :
                OriginAuthServiceResponse(request)
            {
                setDeleteOnFinish(true);
            }

            bool TrialCheckTimeResponse::parseSuccessBody(QIODevice *body)
            {
                QScopedPointer<INodeDocument> pDoc(CreateJsonDocument());

                QByteArray data = body->readAll();
                if(pDoc->Parse(data.data()))
                {
                    server::Read(pDoc.data(), m_checkTime);
                    m_timeStamp.fromString(reply()->rawHeader("Date"), Qt::RFC2822Date);

                    return true;
                }
                return false;
            }

            TrialGrantTimeResponse::TrialGrantTimeResponse(AuthNetworkRequest request) :
                OriginAuthServiceResponse(request)
            {
                setDeleteOnFinish(true);
            }

            bool TrialGrantTimeResponse::parseSuccessBody(QIODevice *body)
            {
                QScopedPointer<INodeDocument> pDoc(CreateJsonDocument());

                QByteArray data = body->readAll();
                if (pDoc->Parse(data.data()))
                {
                    server::Read(pDoc.data(), m_grantTime);
                    m_timeStamp.fromString(reply()->rawHeader("Date"), Qt::RFC2822Date);
                    return true;
                }
                return false;
            }
        }
    }
}
