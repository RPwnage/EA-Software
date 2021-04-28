#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDKimpl.h"

#include "OriginLSXRequest.h"
#include "OriginLSXEnumeration.h"

namespace Origin
{

    OriginErrorT OriginSDK::SendChatMessage(OriginUserT fromUser, OriginUserT toUser, const OriginCharT * thread, const OriginCharT* message, const OriginCharT* groupId)
    {
        if (fromUser == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if (message == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

		IObjectPtr< LSXRequest<lsx::SendChatMessageT, lsx::ErrorSuccessT> > req(
			new LSXRequest<lsx::SendChatMessageT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_CHAT), this));

        lsx::SendChatMessageT &r = req->GetRequest();

		r.FromId = fromUser;
		r.ToId = toUser;
        r.GroupId = groupId == NULL ? "" : groupId;
        r.Thread = thread == NULL ? "" : thread;
        r.Message = message;

        if(req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(req->GetResponse().Code);
        }
        return REPORTERROR(req->GetErrorCode());
    }

};
