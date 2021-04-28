
#ifndef NISASSOCIATIONLIST_H
#define NISASSOCIATIONLIST_H

#include "BlazeSDK/associationlists/associationlistapi.h"

#include "NisSample.h"

namespace NonInteractiveSamples
{

class NisBase;

class NisAssociationList : 
    public NisSample,
    protected Blaze::Association::AssociationListAPIListener
{
    public:
        NisAssociationList(NisBase &nisBase);
        virtual ~NisAssociationList();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

        // From BaseAssociationListAPIListener:
        virtual void    onMembersRemoved(Blaze::Association::AssociationList* list);

    private:
        virtual void onMemberUpdated(Blaze::Association::AssociationList* list, const Blaze::Association::AssociationListMember *member)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "AssociationListAPIListener::onMemberUpdated\n");}

        virtual void onListRemoved(Blaze::Association::AssociationList* list)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "AssociationListAPIListener::onListRemoved\n");}

        void            GetLists(void);
        void            GetListsCb(Blaze::BlazeError error, Blaze::JobId jobId);
        void            DescribeAssociationList(const Blaze::Association::AssociationList * list);
        void            AddUsersToList(void);
        void            AddUsersToListCb(const Blaze::Association::AssociationList* list, const Blaze::Association::ListMemberInfoVector* listMemberVector, Blaze::BlazeError error, Blaze::JobId jobId);
        void            ClearList(void);
        void            ClearListCb(Blaze::BlazeError error, Blaze::JobId jobId);

        Blaze::Association::AssociationListAPI * mAssociationListAPI;
};

}

#endif // NISASSOCIATIONLIST_H
