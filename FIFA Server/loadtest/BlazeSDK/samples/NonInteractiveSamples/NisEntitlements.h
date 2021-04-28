
#ifndef NISENTITLEMENTS_H
#define NISENTITLEMENTS_H

#include "NisSample.h"

namespace NonInteractiveSamples
{

class NisEntitlements
    : public NisSample
{
    public:
        NisEntitlements(NisBase &nisBase);
        virtual ~NisEntitlements();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

    private:
        void            AppGrantEntitlement(void);
        void            GrantEntitlementCb(Blaze::BlazeError aError, Blaze::JobId jobId);
        void            AppListEntitlements(void);
        void            ListEntitlementsCb(const Blaze::Authentication::Entitlements *entitlements, Blaze::BlazeError aError, Blaze::JobId jobId);

        Blaze::Authentication::AuthenticationComponent* mAuthComponent;
};

}

#endif // NISENTITLEMENTS_H
