
#include "NisAssociationList.h"

#include "NisBase.h"

#include <EAAssert/eaassert.h>

using namespace Blaze::Association;

namespace NonInteractiveSamples
{

ListType SAMPLE_LIST_TYPE               = LIST_TYPE_FRIEND;
const char * SAMPLE_USERNAME_TO_ADD     = "tomss120"; // please change this locally as needed

NisAssociationList::NisAssociationList(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisAssociationList::~NisAssociationList()
{
}

void NisAssociationList::onCreateApis()
{
    mAssociationListAPI = getBlazeHub()->getAssociationListAPI(getBlazeHub()->getPrimaryLocalUserIndex());
    getUiDriver().addDiagnosticCallFunc(mAssociationListAPI->addListener(this));
}

void NisAssociationList::onCleanup()
{
    getUiDriver().addDiagnosticCallFunc(mAssociationListAPI->removeListener(this));
}

//------------------------------------------------------------------------------
// Base class calls this.
void NisAssociationList::onRun()
{
    getUiDriver().addDiagnosticFunction();

    // First we need to fetch the lists we are interested in
    GetLists();
}

//------------------------------------------------------------------------------
// Fetch the association lists we are interested in
void NisAssociationList::GetLists(void)
{
    getUiDriver().addDiagnosticFunction();

    ListInfoVector listInfoVector;

    ListInfo* listInfo1 = listInfoVector.allocate_element();
    listInfo1->getId().setListType(SAMPLE_LIST_TYPE);
    listInfoVector.push_back(listInfo1);

    // We're only interested in a single list, in this case

    getUiDriver().addDiagnosticCallFunc(mAssociationListAPI->getLists(listInfoVector, MakeFunctor(this, &NisAssociationList::GetListsCb)));
}

//------------------------------------------------------------------------------
// The result of getLists
void NisAssociationList::GetListsCb(Blaze::BlazeError error, Blaze::JobId jobId)
{
    getUiDriver().addDiagnosticCallback();

    if( error == Blaze::ERR_OK )
    {
        // Describe the lists
        AssociationListAPI::AssociationListList::const_iterator it = mAssociationListAPI->getAssociationLists().begin();
        AssociationListAPI::AssociationListList::const_iterator itend = mAssociationListAPI->getAssociationLists().end();
        for (;it != itend; ++it)
        {
            AssociationList* list = *it;
            DescribeAssociationList(list);
        }
        
        // Now lets add a user to the same list
        AddUsersToList();
    }
    else
    {
        reportBlazeError(error);
    }
}

//------------------------------------------------------------------------------
// Describe an AssociationList and it's members
void NisAssociationList::DescribeAssociationList(const AssociationList * list)
{
    getUiDriver().addDiagnosticFunction();

    EA_ASSERT(list != nullptr);

    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  getListType()       = %d", list->getListType());
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  getListName()       = %s", list->getListName());
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  getBlazeObjectId()  = %s", list->getBlazeObjectId().toString().c_str());
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  getMaxSize()        = %u", list->getMaxSize());
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  isRollover()        = %d", list->isRollover());
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  isSubscribed()      = %d", list->isSubscribed());
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  getMemberCount()    = %u", list->getMemberCount());
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  getMemberVector():");

    AssociationList::MemberVector::const_iterator itm = list->getMemberVector().begin();
    AssociationList::MemberVector::const_iterator itmend = list->getMemberVector().end();
    for (; itm != itmend; ++itm)
    {
        const AssociationListMember* member = *itm;
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "    AssociationListMember:");
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "      getBlazeId()      = %I64d", member->getBlazeId());
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "      getExternalId()    = %I64d", member->getExternalId());
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "      getPersonaName()  = %s", member->getPersonaName());
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "      mTimeAdded        = %I64d", member->getAddedTime());
    }
}

//------------------------------------------------------------------------------
// Add user(s) to a list
void NisAssociationList::AddUsersToList(void)
{
    getUiDriver().addDiagnosticFunction();

    // Let's add a single user to a particular list
    ListMemberIdVector listMemberIdVector;
    ListMemberId * listMemberId = listMemberIdVector.allocate_element();
    listMemberId->getUser().setName(SAMPLE_USERNAME_TO_ADD);
    listMemberIdVector.push_back(listMemberId);

    listMemberId = listMemberIdVector.allocate_element();
    // client need to find out the external id of the through the external system
    listMemberId->getUser().getPlatformInfo().getExternalIds().setXblAccountId(123);
    listMemberIdVector.push_back(listMemberId);

    // client need to find out the external id of the through the external system
    listMemberId = listMemberIdVector.allocate_element();
    listMemberId->getUser().getPlatformInfo().getExternalIds().setXblAccountId(154645);
    listMemberIdVector.push_back(listMemberId);


    AssociationList* list = mAssociationListAPI->getListByType(SAMPLE_LIST_TYPE);
    EA_ASSERT(list != nullptr);
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "addUsersToList %s", SAMPLE_USERNAME_TO_ADD);
    list->addUsersToList(listMemberIdVector, MakeFunctor(this, &NisAssociationList::AddUsersToListCb));
}

//------------------------------------------------------------------------------
// The result of addUsersToList
void NisAssociationList::AddUsersToListCb(const AssociationList* list, const ListMemberInfoVector* listMemberVector, Blaze::BlazeError error, Blaze::JobId jobId)
{
    getUiDriver().addDiagnosticCallback();    

    if (error == Blaze::ERR_OK)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "Succeeded, list is now:");
        DescribeAssociationList(list);

        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "New members added:"); 
        ListMemberInfoVector::const_iterator itl = listMemberVector->begin();
        ListMemberInfoVector::const_iterator itlend = listMemberVector->end();
        for (;itl != itlend; ++itl)
        {
            const ListMemberInfo* member = *itl;
            char buf[2048] = "";
            member->print(buf, sizeof(buf), 1);
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, buf);
        }

        // Now clear the list
        ClearList();
    }
    else if (error == Blaze::ASSOCIATIONLIST_ERR_USER_NOT_FOUND)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "Please edit this sample locally:");
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "Set SAMPLE_USERNAME_TO_ADD to a username of your own choice.");       
    }
    else
    { 
        reportBlazeError(error);
    }
}

//------------------------------------------------------------------------------
// Clear an association list
void NisAssociationList::ClearList(void)
{
    getUiDriver().addDiagnosticFunction();

    AssociationList* list = mAssociationListAPI->getListByType(SAMPLE_LIST_TYPE);
    EA_ASSERT(list != nullptr);
    getUiDriver().addDiagnosticCallFunc(list->clearList(MakeFunctor(this, &NisAssociationList::ClearListCb)));
}

//------------------------------------------------------------------------------
// The result of clearList
void NisAssociationList::ClearListCb(Blaze::BlazeError error, Blaze::JobId jobId)
{
    getUiDriver().addDiagnosticCallback();

    if (error == Blaze::ERR_OK)
    {
        done();
    }
    else
    {
        reportBlazeError(error);
    }
}

//------------------------------------------------------------------------------
// An AssociationListAPIListener notification that members have been removed from the given list
void NisAssociationList::onMembersRemoved(AssociationList* list)
{
    getUiDriver().addDiagnosticFunction();

    if (list != nullptr)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "onMembersRemoved list is:");
        DescribeAssociationList(list);
    }
}

}

