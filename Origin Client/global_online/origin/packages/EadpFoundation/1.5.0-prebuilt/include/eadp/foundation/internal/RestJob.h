// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>
#include <eadp/foundation/Hub.h>
#include <eadp/foundation/internal/JobBase.h>
#include <eadp/foundation/internal/HttpClient.h>
#include <eadp/foundation/internal/HttpResponse.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

/*
 * Job to send an Http request and deliver the response to a specified callback
 */
class EADPSDK_API RestJob : public JobBase
{
public:
    explicit RestJob(IHub* hub);
    virtual ~RestJob();

    ErrorPtr initialize(const SharedPtr<HttpRequest>& request, const HttpResponse::Callback& callback);

    void execute() override;
    void cleanup() override;

protected:
    IHub* getHub() const { return m_hub; }
    Allocator& getAllocator() const { return m_hub->getAllocator(); }

    EA_NON_COPYABLE(RestJob);

    IHub* m_hub;
    SharedPtr<IHttpClient> m_httpClient;
};

}
}
}

