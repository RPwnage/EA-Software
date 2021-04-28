#pragma once
#ifndef BYTEVAULT_H
#define BYTEVAULT_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

#include "BlazeSDK/bytevaultmanager/bytevaultmanager.h"
#include "BlazeSDK/component/bytevaultcomponent.h"

namespace Ignition
{

class ByteVaultWindow;

class ByteVault :
    public LocalUserUiBuilder
{
public:
    ByteVault(uint32_t userIndex);
    ~ByteVault();

    ByteVaultWindow& getAdminWindow();
    ByteVaultWindow& getContextWindow();
    ByteVaultWindow& getCategoryWindow();
    ByteVaultWindow& getRecordWindow();
    ByteVaultWindow& getRecordsInfoWindow();

protected:
    virtual void onConnected();
    virtual void onDisconnected();

private:
    Pyro::UiNodeParameterStructPtr mGetAdminParams;
    Pyro::UiNodeParameterStructPtr mGetCategoryParams;
    Pyro::UiNodeParameterStructPtr mGetRecordsParams;
    Pyro::UiNodeParameterStructPtr mSharedAuthCredentials;

    PYRO_ACTION(ByteVault, Initialize);
    PYRO_ACTION(ByteVault, GetAdmin);
    PYRO_ACTION(ByteVault, GetContexts);
    PYRO_ACTION(ByteVault, GetCategories);
    PYRO_ACTION(ByteVault, GetRecord);
    PYRO_ACTION(ByteVault, GetRecords);
    PYRO_ACTION(ByteVault, UpsertRecord);
    PYRO_ACTION(ByteVault, DeleteRecord);
    PYRO_ACTION(ByteVault, SetAuthCredentials);

    void GetAdminCb(const Blaze::ByteVault::GetAdminResponse* response, Blaze::BlazeError blazeError, Blaze::JobId jobId);
    void GetContextCb(const Blaze::ByteVault::Context* response, Blaze::BlazeError blazeError, Blaze::JobId jobId);
    void GetContextsCb(const Blaze::ByteVault::GetContextsResponse* response, Blaze::BlazeError blazeError, Blaze::JobId jobId);
    void GetCategoryCb(const Blaze::ByteVault::Category* response, Blaze::BlazeError blazeError, Blaze::JobId jobId);
    void GetCategoriesCb(const Blaze::ByteVault::GetCategoriesResponse* response, Blaze::BlazeError blazeError, Blaze::JobId jobId);
    void GetRecordCb(const Blaze::ByteVault::GetRecordResponse* response, Blaze::BlazeError blazeError, Blaze::JobId jobId);
    void GetRecordsInfoCb(const Blaze::ByteVault::GetRecordInfoResponse* response, Blaze::BlazeError blazeError, Blaze::JobId jobId);
    void UpsertRecordCb(const Blaze::ByteVault::UpsertRecordResponse* response, Blaze::BlazeError blazeError, Blaze::JobId jobId);

    void ByteVaultWindowClosing(Pyro::UiNodeWindow *sender);

    void setSharedAuthCredentials(Pyro::UiNodeParameterStructPtr parameters);
    void getSharedAuthCredentials(Pyro::UiNodeParameterStruct& parameters);
    void setAuthCredentials(Blaze::ByteVault::AuthenticationCredentials& credentials);

    // Ignition UI event handlers
    void ContextAdminListRowSelected(Pyro::UiNodeTable* table, Pyro::UiNodeTableRow* row);
    void ContextRowSelected(Pyro::UiNodeTable* table, Pyro::UiNodeTableRow* row);
    void CategoryRowSelected(Pyro::UiNodeTable* table, Pyro::UiNodeTableRow* row);

    ByteVaultWindow& getByteVaultWindow(const char8_t* windowId);

    uint32_t mUserIndex;
};

class ByteVaultWindow :
    public Pyro::UiNodeWindow
{
    friend class ByteVault;

public:
    ByteVaultWindow(const char8_t* windowId);
    virtual ~ByteVaultWindow();

    ByteVault* getParent() { return (ByteVault*)Pyro::UiNodeWindow::getParent(); }
};

} // namespace Ignition

#endif // BYTEVAULT_H
