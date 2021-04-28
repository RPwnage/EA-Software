#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDKimpl.h"
#include "assert.h"

#include "OriginLSXRequest.h"
#include "OriginLSXEnumeration.h"

namespace Origin
{

    OriginErrorT OriginSDK::SelectStore(uint64_t storeId, uint64_t catalogId, uint64_t eWalletCategoryId, const OriginCharT *pVirtualCurrency, const OriginCharT *pLockboxUrl, const OriginCharT *pSuccessUrl, const OriginCharT * pFailedUrl)
    {
        if(storeId == 0 && catalogId == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(pLockboxUrl == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);


        IObjectPtr< LSXRequest<lsx::SelectStoreT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::SelectStoreT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_COMMERCE), this));

        lsx::SelectStoreT &r = req->GetRequest();

        r.StoreId = storeId;
        r.CatalogId = catalogId;
        r.EWalletCategoryId = eWalletCategoryId;
        r.VirtualCurrency = pVirtualCurrency == NULL ? "" : pVirtualCurrency;
        r.LockboxUrl = pLockboxUrl;
        r.SuccessUrl = pSuccessUrl == NULL ? "" : pSuccessUrl;
        r.FailedUrl = pFailedUrl == NULL ? "" : pFailedUrl;

        if(req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(req->GetResponse().Code);
        }
        return REPORTERROR(req->GetErrorCode());
    }


    OriginErrorT OriginSDK::GetStore(OriginUserT user, uint64_t storeId, OriginStoreCallback callback, void *pContext, uint32_t timeout)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if(callback == NULL || timeout == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetStoreT, lsx::GetStoreResponseT, OriginStoreT, OriginStoreCallback> > req(
            new LSXRequest<lsx::GetStoreT, lsx::GetStoreResponseT, OriginStoreT, OriginStoreCallback>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::GetStoreConvertData, callback, pContext));

        lsx::GetStoreT &r = req->GetRequest();

        r.UserId = user;
        r.StoreId = storeId;

        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::GetStoreSync(OriginUserT user, uint64_t storeId, OriginStoreT ** ppStore, size_t *pNumberOfStores, uint32_t timeout, OriginHandleT *pHandle)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if(ppStore == NULL || pNumberOfStores == NULL || timeout == 0 || pHandle == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetStoreT, lsx::GetStoreResponseT> > req(
            new LSXRequest<lsx::GetStoreT, lsx::GetStoreResponseT>
                (GetService(lsx::FACILITY_COMMERCE), this));

        lsx::GetStoreT &r = req->GetRequest();

        r.UserId = user;
        r.StoreId = storeId;

        *pHandle = NULL;
        *ppStore = NULL;
        *pNumberOfStores = 0;

        if(req->Execute(timeout))
        {
            OriginErrorT ret = req->GetErrorCode();

            if(ret == ORIGIN_SUCCESS)
            {
                OriginStoreT *pStore = TYPE_NEW(OriginStoreT, 1);

                req->StoreMemory(pStore);

                ConvertData(req, *pStore, req->GetResponse().Store, true);

                *pHandle = req->CopyDataStoreToHandle();
                *ppStore = pStore;
                *pNumberOfStores = 1;
            }
            return ret;
        }
        else
        {
            return REPORTERROR(req->GetErrorCode());
        }
    }

    OriginErrorT OriginSDK::GetStoreConvertData(IXmlMessageHandler *pHandler, OriginStoreT &pStore, size_t &size, lsx::GetStoreResponseT &response)
    {
        ConvertData(pHandler, pStore, response.Store, false);
        size = 1;

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::GetCatalog(OriginUserT user, OriginCatalogCallback callback, void *pContext, uint32_t timeout)
    {
        if (user == 0 || callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetCatalogT, lsx::GetCatalogResponseT, OriginCategoryT *, OriginCatalogCallback> > req(
            new LSXRequest<lsx::GetCatalogT, lsx::GetCatalogResponseT, OriginCategoryT *, OriginCatalogCallback>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::GetCatalogConvertData, callback, pContext));

        req->GetRequest().UserId = user;

        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    const OriginCharT * OriginSDK::ConvertData(IXmlMessageHandler *pHandler, const AllocString &src, bool bCopyStrings)
    {
        if(bCopyStrings)
        {
            // Allocate a new string as the container will go out of scope before the game can get to the data.
            size_t len = src.length();
            char *buffer = TYPE_NEW(char, len + 1);
            strncpy(buffer, src.c_str(), len + 1);
            pHandler->StoreMemory(buffer);

            return buffer;
        }
        else
        {
            // Give a pointer to the string contents.
            return src.c_str();
        }
    }

    void OriginSDK::ConvertData(IXmlMessageHandler *pHandler, OriginItemT &Dest, lsx::EntitlementT &Src, bool bCopyStrings)
    {
        Dest.Type			= ConvertData(pHandler, Src.Type, bCopyStrings);
        Dest.ItemId			= ConvertData(pHandler, Src.ItemId, bCopyStrings);
        Dest.EntitlementId	= ConvertData(pHandler, Src.EntitlementId, bCopyStrings);
        Dest.EntitlementTag	= ConvertData(pHandler, Src.EntitlementTag, bCopyStrings);
        Dest.Group			= ConvertData(pHandler, Src.Group, bCopyStrings);
        Dest.ResourceId		= ConvertData(pHandler, Src.ResourceId, bCopyStrings);
        Dest.GrantDate      = Src.GrantDate;
        Dest.TerminateDate	= Src.Expiration;
        Dest.QuantityLeft	= Src.UseCount;
        Dest.LastModifiedDate = Src.LastModifiedDate;
        Dest.Version        = Src.Version;
    }

    void OriginSDK::ConvertData(OriginItemT &Dest, const lsx::EntitlementT &Src)
    {
        Dest.Type			= Src.Type.c_str();
        Dest.ItemId			= Src.ItemId.c_str();
        Dest.EntitlementId	= Src.EntitlementId.c_str();
        Dest.EntitlementTag	= Src.EntitlementTag.c_str();
        Dest.Group			= Src.Group.c_str();
        Dest.ResourceId		= Src.ResourceId.c_str();
        Dest.TerminateDate	= Src.Expiration;
        Dest.GrantDate      = Src.GrantDate;
        Dest.QuantityLeft	= (uint32_t)Src.UseCount;
        Dest.LastModifiedDate = Src.LastModifiedDate;
        Dest.Version        = Src.Version;
    }

    void OriginSDK::ConvertData(lsx::EntitlementT &Dest, OriginItemT &Src)
    {
        Dest.Type			= Src.Type;
        Dest.ItemId			= Src.ItemId;
        Dest.EntitlementId	= Src.EntitlementId;
        Dest.EntitlementTag	= Src.EntitlementTag;
        Dest.Group			= Src.Group;
        Dest.ResourceId		= Src.ResourceId;
        Dest.Expiration		= Src.TerminateDate;
        Dest.GrantDate      = Src.GrantDate;
        Dest.UseCount		= (uint32_t)Src.QuantityLeft;
        Dest.LastModifiedDate = Src.LastModifiedDate;
        Dest.Version        = Src.Version;
    }

    void OriginSDK::ConvertData(IXmlMessageHandler *pHandler, OriginStoreT &dest, lsx::StoreT &src, bool bCopyStrings)
    {
        dest.Name = ConvertData(pHandler, src.Name, bCopyStrings);
        dest.Title = ConvertData(pHandler, src.Title, bCopyStrings);
        dest.Group = ConvertData(pHandler, src.Group, bCopyStrings);
        dest.Status = ConvertData(pHandler, src.Status, bCopyStrings);
        dest.DefaultCurrency = ConvertData(pHandler, src.DefaultCurrency, bCopyStrings);
        dest.StoreId = src.StoreId;
        dest.bIsDemoStore = src.IsDemoStore ? 1 : 0;

        dest.CatalogCount = src.Catalog.size();
        dest.pCatalogs = NULL;
        if(dest.CatalogCount > 0)
        {
            OriginCatalogT *pCatalogs = TYPE_NEW(OriginCatalogT, dest.CatalogCount);
            dest.pCatalogs = pCatalogs;

            pHandler->StoreMemory(pCatalogs);

            ConvertData(pHandler, pCatalogs, src.Catalog, 0, dest.CatalogCount, bCopyStrings);
        }
    }

    void OriginSDK::ConvertData(IXmlMessageHandler *pHandler, OriginCatalogT &dest, lsx::CatalogT &src, bool bCopyStrings)
    {
        dest.Name = ConvertData(pHandler, src.Name, bCopyStrings);
        dest.Status = ConvertData(pHandler, src.Status, bCopyStrings);
        dest.CurrencyType = ConvertData(pHandler, src.CurrencyType, bCopyStrings);
        dest.Group = ConvertData(pHandler, src.Group, bCopyStrings);
        dest.CatalogId = src.CatalogId;

        // TODO: We may have to add categories here.
    }

    void OriginSDK::ConvertData(IXmlMessageHandler *pHandler, OriginCatalogT *dest, std::vector<lsx::CatalogT, Origin::Allocator<lsx::CatalogT> > &src, size_t index, size_t count, bool bCopyStrings)
    {
        assert(index + count <= src.size());

        for(size_t i = index; i < index + count; i++)
        {
            OriginCatalogT &Dest = dest[i - index];
            lsx::CatalogT &Src = src[i];

            ConvertData(pHandler, Dest, Src, bCopyStrings);
        }
    }

    void OriginSDK::ConvertData(IXmlMessageHandler *pHandler, OriginItemT *dest, std::vector<lsx::EntitlementT, Origin::Allocator<lsx::EntitlementT> > &src, size_t index, size_t count, bool bCopyStrings)
    {
        assert(index + count <= src.size());

        for(size_t i = index; i < index + count; i++)
        {
            OriginItemT &Dest = dest[i - index];
            lsx::EntitlementT &Src = src[i];

            if(bCopyStrings)
                ConvertData(pHandler, Dest, Src, bCopyStrings);
            else
                ConvertData(Dest, Src);
        }
    }

    void OriginSDK::ConvertData(IXmlMessageHandler *pHandler, OriginOfferT *dest, std::vector<lsx::OfferT, Origin::Allocator<lsx::OfferT> > &src, size_t index, size_t count, bool bCopyStrings)
    {
        assert(index + count <= src.size());

        for(size_t i = index; i < index + count; i++)
        {
            OriginOfferT &Dest = dest[i - index];
            lsx::OfferT &Src = src[i];

            Dest.Entitlements = NULL;

            Dest.Type			= ConvertData(pHandler, Src.Type, bCopyStrings);
            Dest.OfferId		= ConvertData(pHandler, Src.OfferId, bCopyStrings);
            Dest.Name			= ConvertData(pHandler, Src.Name, bCopyStrings);
            Dest.Description	= ConvertData(pHandler, Src.Description, bCopyStrings);
            Dest.ImageId		= ConvertData(pHandler, Src.ImageId, bCopyStrings);
            Dest.GameDistributionSubType = ConvertData(pHandler, Src.GameDistributionSubType, bCopyStrings);
            Dest.bIsOwned		= Src.bIsOwned;
            Dest.bIsHidden		= Src.bHidden;
            Dest.bCanPurchase	= Src.bCanPurchase;
            Dest.PurchaseDate	= Src.PurchaseDate;
            Dest.DownloadDate	= Src.DownloadDate;
            Dest.PlayableDate	= Src.PlayableDate;
            Dest.UseEndDate		= Src.UseEndDate;
            Dest.DownloadSize	= Src.DownloadSize;

            Dest.Currency		= ConvertData(pHandler, Src.Currency, bCopyStrings);
            Dest.bIsDiscounted  = Src.bIsDiscounted;
            Dest.Price			= Src.Price;
            Dest.RegularPrice	= ConvertData(pHandler, Src.LocalizedPrice, bCopyStrings);
            Dest.OriginalPrice  = Src.OriginalPrice;
            Dest.LocalizedOriginalPrice = ConvertData(pHandler, Src.LocalizedOriginalPrice, bCopyStrings);

            Dest.QuantityAllow	= Src.InventoryCap;
            Dest.QuantitySold	= Src.InventorySold;
            Dest.QuantityAvail	= Src.InventoryAvailable;

            Dest.EntitlementCount = Src.Entitlements.size();

            if(Dest.EntitlementCount > 0)
            {
                OriginItemT *pItems = TYPE_NEW(OriginItemT, Dest.EntitlementCount);
                Dest.Entitlements = pItems;
                pHandler->StoreMemory(pItems);

                ConvertData(pHandler, pItems, Src.Entitlements, 0, Dest.EntitlementCount, bCopyStrings);
            }
        }
    }

    void OriginSDK::ConvertData(IXmlMessageHandler *pHandler, OriginCategoryT *dest, std::vector<lsx::CategoryT, Origin::Allocator<lsx::CategoryT> > &src, size_t index, size_t count, bool bCopyStrings)
    {
        assert(index + count <= src.size());

        for(size_t i = index; i < index + count; i++)
        {
            OriginCategoryT &Dest = dest[i - index];
            lsx::CategoryT &Src = src[i];

            Dest.pCategories = NULL;
            Dest.pOffers = NULL;

            Dest.Type			= ConvertData(pHandler, Src.Type, bCopyStrings);
            Dest.CategoryId		= ConvertData(pHandler, Src.CategoryId, bCopyStrings);
            Dest.ParentId		= ConvertData(pHandler, Src.ParentId, bCopyStrings);
            Dest.Name			= ConvertData(pHandler, Src.Name, bCopyStrings);
            Dest.Description	= ConvertData(pHandler, Src.Description, bCopyStrings);
            Dest.bMostPopular	= (char)Src.MostPopular;
            Dest.ImageID		= ConvertData(pHandler, Src.ImageId, bCopyStrings);

            Dest.CategoryCount = Src.Categories.size();
            if(Dest.CategoryCount > 0)
            {
                OriginCategoryT *pCategories = TYPE_NEW(OriginCategoryT, Dest.CategoryCount);
                Dest.pCategories = pCategories;
                pHandler->StoreMemory(pCategories);

                ConvertData(pHandler, pCategories, Src.Categories, 0, Dest.CategoryCount, bCopyStrings);
            }

            Dest.OfferCount = Src.Offers.size();
            if(Dest.OfferCount > 0)
            {
                OriginOfferT *pOffers = TYPE_NEW(OriginOfferT, Dest.OfferCount);
                Dest.pOffers = pOffers;
                pHandler->StoreMemory(pOffers);

                ConvertData(pHandler, pOffers, Src.Offers, 0, Dest.OfferCount, bCopyStrings);
            }
        }
    }

    void OriginSDK::ConvertData(IXmlMessageHandler *pHandler, OriginCategoryT *&categories, size_t &count, lsx::CatalogT &catalog, bool bCopyStrings)
    {
        count = catalog.Categories.size();

        if(count)
        {
            categories = TYPE_NEW(OriginCategoryT, catalog.Categories.size());
            pHandler->StoreMemory(categories);

            ConvertData(pHandler, categories, catalog.Categories, 0, catalog.Categories.size(), bCopyStrings);
        }
        else
        {
            categories = NULL;
        }
    }


    OriginErrorT OriginSDK::GetCatalogSync(OriginUserT user, OriginCategoryT **pCategories, size_t *pCategoryCount, uint32_t timeout, OriginHandleT *handle)
    {
        if(pCategories == NULL || pCategoryCount == NULL || handle == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        *handle = NULL;

        IObjectPtr< LSXRequest<lsx::GetCatalogT, lsx::GetCatalogResponseT> > req(
            new LSXRequest<lsx::GetCatalogT, lsx::GetCatalogResponseT>
                (GetService(lsx::FACILITY_COMMERCE), this));

        req->GetRequest().UserId = user;

        if(req->Execute(timeout))
        {
            ConvertData(req, *pCategories, *pCategoryCount, req->GetResponse().Catalog, true);
            *handle = req->CopyDataStoreToHandle();
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::GetCatalogConvertData(IXmlMessageHandler * handler, OriginCategoryT *&pCategories, size_t &size, lsx::GetCatalogResponseT &response)
    {
        ConvertData(handler, pCategories, size, response.Catalog, false);
        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::GetWalletBalance(OriginUserT user, const OriginCharT *currency, OriginWalletCallback callback, void *pContext, uint32_t timeout)
    {
        if(currency == NULL || callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetWalletBalanceT, lsx::GetWalletBalanceResponseT, int64_t, OriginWalletCallback> > req(
            new LSXRequest<lsx::GetWalletBalanceT, lsx::GetWalletBalanceResponseT, int64_t, OriginWalletCallback>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::GetWalletBalanceConvertData, callback, pContext));

        lsx::GetWalletBalanceT &r = req->GetRequest();

        r.UserId = user;
        r.Currency = currency;

        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::GetWalletBalanceConvertData(IXmlMessageHandler * /*pHandler*/, int64_t &balance, size_t &Size, lsx::GetWalletBalanceResponseT &response)
    {
        balance = response.Balance;
        Size = sizeof(int);

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::GetWalletBalanceSync(OriginUserT user, const OriginCharT *currency, int64_t *pBalance, uint32_t timeout)
    {
        if(currency == NULL || pBalance == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetWalletBalanceT, lsx::GetWalletBalanceResponseT> > req(
            new LSXRequest<lsx::GetWalletBalanceT, lsx::GetWalletBalanceResponseT>
                (GetService(lsx::FACILITY_COMMERCE), this));

        lsx::GetWalletBalanceT &r = req->GetRequest();

        r.UserId = user;
        r.Currency = currency;

        if(req->Execute(timeout))
        {
            *pBalance = req->GetResponse().Balance;
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::ShowStoreUI(OriginUserT user, const OriginCharT *pCategoryList[], size_t categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], size_t offerCount, OriginErrorSuccessCallback callback, void* pContext)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
			new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
			    (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::ShowIGOConvertData, callback, pContext));

		FILL_REQUEST_CHECK(ShowStoreUIBuildRequest(req->GetRequest(), user, pCategoryList, categoryCount, pMasterTitleIdList, MasterTitleIdCount, pOfferList, offerCount));

        if(req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::ShowStoreUISync(OriginUserT user, const OriginCharT *pCategoryList[], size_t categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], size_t offerCount)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_COMMERCE), this));

        FILL_REQUEST_CHECK(ShowStoreUIBuildRequest(req->GetRequest(), user, pCategoryList, categoryCount, pMasterTitleIdList, MasterTitleIdCount, pOfferList, offerCount));

        if(req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(req->GetResponse().Code);
        }

        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::ShowStoreUIBuildRequest(lsx::ShowIGOWindowT& data, OriginUserT user, const OriginCharT *pCategoryList[], size_t categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], size_t offerCount)
    {
        if(user == ((OriginUserT)NULL))
            return ORIGIN_ERROR_INVALID_USER;
        if(pCategoryList == NULL && categoryCount != 0)
            return ORIGIN_ERROR_INVALID_ARGUMENT;
        if(pMasterTitleIdList == NULL && MasterTitleIdCount != 0)
            return ORIGIN_ERROR_INVALID_ARGUMENT;
        if(pOfferList == NULL && offerCount != 0)
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        data.UserId = user;
        data.Show = true;
        data.WindowId = lsx::IGOWINDOW_STORE;

        data.Categories.reserve(categoryCount);
        for(unsigned int i = 0; i < categoryCount; i++)
        {
            data.Categories.push_back(pCategoryList[i]);
        }

        data.MasterTitleIds.reserve(MasterTitleIdCount);
        for(unsigned int i = 0; i < MasterTitleIdCount; i++)
        {
            data.MasterTitleIds.push_back(pMasterTitleIdList[i]);
        }

        data.Offers.reserve(offerCount);
        for(unsigned int i = 0; i < offerCount; i++)
        {
            data.Offers.push_back(pOfferList[i]);
        }

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::ShowCodeRedemptionUI(OriginUserT user, OriginErrorSuccessCallback callback, void* pContext)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
			new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
			    (GetService(lsx::FACILITY_IGO), this, &OriginSDK::ShowIGOConvertData, callback, pContext));

		FILL_REQUEST_CHECK(ShowCodeRedemptionUIBuildRequest(req->GetRequest(), user));

        if(req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::ShowCodeRedemptionUISync(OriginUserT user)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_IGO), this));

        FILL_REQUEST_CHECK(ShowCodeRedemptionUIBuildRequest(req->GetRequest(), user));

        if(req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(req->GetResponse().Code);
        }

        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::ShowCodeRedemptionUIBuildRequest(lsx::ShowIGOWindowT& data, OriginUserT user)
    {
        data.UserId = user;
        data.Show = true;
        data.WindowId = lsx::IGOWINDOW_CODE_REDEMPTION;

        return REPORTERROR(ORIGIN_SUCCESS);
    }


    OriginErrorT OriginSDK::QueryCategories(OriginUserT user, const OriginCharT *pParentCategoryList[], size_t parentCategoryCount, OriginEnumerationCallbackFunc callback, void * pContext, uint32_t timeout, OriginHandleT *pHandle)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if((pParentCategoryList == NULL && parentCategoryCount != 0))
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(callback == NULL && pHandle == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        LSXEnumeration<lsx::QueryCategoriesT, lsx::QueryCategoriesResponseT, OriginCategoryT> * req =
            new LSXEnumeration<lsx::QueryCategoriesT, lsx::QueryCategoriesResponseT, OriginCategoryT>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::QueryCategoriesConvertData, callback, pContext);

        lsx::QueryCategoriesT &r = req->GetRequest();

        r.UserId = user;

        r.FilterCategories.reserve(parentCategoryCount);
        for(unsigned int i = 0; i < parentCategoryCount; i++)
        {
            r.FilterCategories.push_back(pParentCategoryList[i]);
        }

        OriginHandleT h = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if(pHandle != NULL)
        {
            *pHandle = h;
        }

        // Dispatch the command.
        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            DestroyHandle(h);
            if (pHandle != NULL) *pHandle = NULL;
            return REPORTERROR(err);
        }
    }

    void OriginSDK::ConvertData(IXmlMessageHandler *pHandler, OriginCategoryT &dest, lsx::CategoryT &src, bool bCopyStrings)
    {
        dest.pCategories = NULL;
        dest.pOffers = NULL;

        dest.Type			= ConvertData(pHandler, src.Type, bCopyStrings);
        dest.CategoryId		= ConvertData(pHandler, src.CategoryId, bCopyStrings);
        dest.ParentId		= ConvertData(pHandler, src.ParentId, bCopyStrings);
        dest.Name			= ConvertData(pHandler, src.Name, bCopyStrings);
        dest.Description	= ConvertData(pHandler, src.Description, bCopyStrings);
        dest.bMostPopular	= (char)src.MostPopular;
        dest.ImageID		= ConvertData(pHandler, src.ImageId, bCopyStrings);

        dest.CategoryCount = src.Categories.size();
        if(dest.CategoryCount > 0)
        {
            OriginCategoryT *pCategories = TYPE_NEW(OriginCategoryT, dest.CategoryCount);
            dest.pCategories = pCategories;
            pHandler->StoreMemory(pCategories);

            ConvertData(pHandler, pCategories, src.Categories, 0, dest.CategoryCount, bCopyStrings);
        }

        dest.OfferCount = src.Offers.size();
        if(dest.OfferCount > 0)
        {
            OriginOfferT *pOffers = TYPE_NEW(OriginOfferT, dest.OfferCount);
            dest.pOffers = pOffers;
            pHandler->StoreMemory(pOffers);

            ConvertData(pHandler, pOffers, src.Offers, 0, dest.OfferCount, bCopyStrings);
        }
    }

    OriginErrorT OriginSDK::QueryCategoriesConvertData(IEnumerator *pEnumerator, OriginCategoryT *pCategories, size_t index, size_t count, lsx::QueryCategoriesResponseT &response)
    {
        for(size_t i = 0; i < count; i++)
        {
            OriginCategoryT &dest = pCategories[i];
            lsx::CategoryT &src = response.Categories[i + index];

            ConvertData(pEnumerator, dest, src, false);
        }
        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::QueryCategoriesSync(OriginUserT user, const OriginCharT *pParentCategoryList[], size_t parentCategoryCount, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
    {
        if(pTotalCount == NULL || (pParentCategoryList == NULL && parentCategoryCount != 0) || pHandle == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        LSXEnumeration<lsx::QueryCategoriesT, lsx::QueryCategoriesResponseT, OriginCategoryT> * req =
            new LSXEnumeration<lsx::QueryCategoriesT, lsx::QueryCategoriesResponseT, OriginCategoryT>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::QueryCategoriesConvertData);

        lsx::QueryCategoriesT &r = req->GetRequest();

        r.UserId = user;
        r.FilterCategories.reserve(parentCategoryCount);
        for(unsigned int i = 0; i < parentCategoryCount; i++)
        {
            r.FilterCategories.push_back(pParentCategoryList[i]);
        }

        *pHandle = RegisterResource(OriginResourceContainerT::Enumerator, req);

        // Dispatch the command.
        if(req->Execute(timeout, pTotalCount))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::QueryOffers(OriginUserT user, const OriginCharT *pCategoryList[], size_t categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], size_t offerCount, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
    {
        if(pCategoryList == NULL && categoryCount != 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(pMasterTitleIdList == NULL && MasterTitleIdCount != 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(pOfferList == NULL && offerCount != 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(pHandle == NULL && callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        LSXEnumeration<lsx::QueryOffersT, lsx::QueryOffersResponseT, OriginOfferT> * req =
            new LSXEnumeration<lsx::QueryOffersT, lsx::QueryOffersResponseT, OriginOfferT>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::QueryOffersConvertData, callback, pContext);

        lsx::QueryOffersT &r = req->GetRequest();

        r.UserId = user;

        r.FilterCategories.reserve(categoryCount);
        for(unsigned int i = 0; i < categoryCount; i++)
        {
            r.FilterCategories.push_back(pCategoryList[i]);
        }

        r.FilterMasterTitleIds.reserve(MasterTitleIdCount);
        for(unsigned int i = 0; i < MasterTitleIdCount; i++)
        {
            r.FilterMasterTitleIds.push_back(pMasterTitleIdList[i]);
        }

        r.FilterOffers.reserve(offerCount);
        for(unsigned int i = 0; i < offerCount; i++)
        {
            r.FilterOffers.push_back(pOfferList[i]);
        }

        OriginHandleT h = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if(pHandle != NULL)
        {
            *pHandle = h;
        }

        // Dispatch the command.
        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            DestroyHandle(h);
            if (pHandle != NULL) *pHandle = NULL;
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::QueryOffersConvertData(IEnumerator *pEnumerator, OriginOfferT *pOffers, size_t index, size_t count, lsx::QueryOffersResponseT &response)
    {
        ConvertData(pEnumerator, pOffers, response.Offers, index, count, false);

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::QueryOffersSync(OriginUserT user, const OriginCharT *pCategoryList[], size_t categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], size_t offerCount, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
    {
        if(pCategoryList == NULL && categoryCount != 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(pMasterTitleIdList == NULL && MasterTitleIdCount != 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(pOfferList == NULL && offerCount != 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        LSXEnumeration<lsx::QueryOffersT, lsx::QueryOffersResponseT, OriginOfferT> * req =
            new LSXEnumeration<lsx::QueryOffersT, lsx::QueryOffersResponseT, OriginOfferT>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::QueryOffersConvertData);

        lsx::QueryOffersT &r = req->GetRequest();

        r.UserId = user;

        r.FilterCategories.reserve(categoryCount);
        for(unsigned int i = 0; i < categoryCount; i++)
        {
            r.FilterCategories.push_back(pCategoryList[i]);
        }

        r.FilterMasterTitleIds.reserve(MasterTitleIdCount);
        for(unsigned int i = 0; i < MasterTitleIdCount; i++)
        {
            r.FilterMasterTitleIds.push_back(pMasterTitleIdList[i]);
        }

        r.FilterOffers.reserve(offerCount);
        for(unsigned int i = 0; i < offerCount; i++)
        {
            r.FilterOffers.push_back(pOfferList[i]);
        }

        *pHandle = RegisterResource(OriginResourceContainerT::Enumerator, req);

        // Dispatch the command.
        if(req->Execute(timeout, pTotalCount))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::Checkout(OriginUserT user, const OriginCharT * Currency, const OriginCharT *pOfferList[], size_t offerCount, OriginResourceCallback callback, void *pContext, uint32_t timeout)
    {
        if(pOfferList == NULL && offerCount != 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(pOfferList != NULL && offerCount == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::CheckoutT, lsx::ErrorSuccessT, int> > req(
            new LSXRequest<lsx::CheckoutT, lsx::ErrorSuccessT, int>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::CheckoutConvertData, callback, pContext));

        lsx::CheckoutT &r = req->GetRequest();

        r.UserId = user;
        r.Currency = Currency;
        for(unsigned int i = 0; i < offerCount; i++)
        {
            r.Offers.push_back(pOfferList[i]);
        }

        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::CheckoutSync(OriginUserT user, const OriginCharT * Currency, const OriginCharT *pOfferList[], size_t offerCount, uint32_t timeout)
    {
        if(pOfferList == NULL && offerCount != 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(pOfferList != NULL && offerCount == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::CheckoutT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::CheckoutT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_COMMERCE), this));

        lsx::CheckoutT &r = req->GetRequest();

        r.UserId = user;
        r.Currency = Currency;
        for(unsigned int i = 0; i < offerCount; i++)
        {
            r.Offers.push_back(pOfferList[i]);
        }

        if(req->Execute(timeout))
        {
            return REPORTERROR(req->GetResponse().Code);
        }

        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::CheckoutConvertData(IXmlMessageHandler * /*pHandler*/, int &/*dummy*/, size_t &size, lsx::ErrorSuccessT &response)
    {
        size = 0;

        return response.Code;
    }

    OriginErrorT OriginSDK::QueryManifest(OriginUserT user, const OriginCharT * manifest, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if(manifest == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(pHandle == NULL && callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        LSXEnumeration<lsx::QueryManifestT, lsx::QueryManifestResponseT, OriginItemT> * req =
            new LSXEnumeration<lsx::QueryManifestT, lsx::QueryManifestResponseT, OriginItemT>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::QueryManifestConvertData, callback, pContext);

        lsx::QueryManifestT &r = req->GetRequest();

        r.UserId = user;
        r.Manifest = manifest;

        OriginHandleT h = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if(pHandle != NULL)
        {
            *pHandle = h;
        }

        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            DestroyHandle(h);
            if (pHandle != NULL) *pHandle = NULL;
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::QueryManifestConvertData(IEnumerator *pEnumerator, OriginItemT *pItems, size_t index, size_t count, lsx::QueryManifestResponseT &response)
    {
        ConvertData(pEnumerator, pItems, response.Entitlements, index, count, false);

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::QueryManifestSync(OriginUserT user, const OriginCharT* manifest, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if(manifest == NULL ||  pHandle == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        LSXEnumeration<lsx::QueryManifestT, lsx::QueryManifestResponseT, OriginItemT> * req =
            new LSXEnumeration<lsx::QueryManifestT, lsx::QueryManifestResponseT, OriginItemT>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::QueryManifestConvertData);

        lsx::QueryManifestT &r = req->GetRequest();

        r.UserId = user;
        r.Manifest = manifest;

        *pHandle = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if(req->Execute(timeout, pTotalCount))
        {
            return REPORTERROR(req->GetErrorCode());
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::ShowCheckoutUI(OriginUserT user, const OriginCharT *pOfferList[], size_t offerCount, OriginErrorSuccessCallback callback, void* pContext)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
			new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
    			(GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::ShowIGOConvertData, callback, pContext));

		FILL_REQUEST_CHECK(ShowCheckoutUIBuildRequest(req->GetRequest(), user, pOfferList, offerCount));

        if(req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }

    }
    OriginErrorT OriginSDK::ShowCheckoutUISync(OriginUserT user, const OriginCharT *pOfferList[], size_t offerCount)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_COMMERCE), this));

        FILL_REQUEST_CHECK(ShowCheckoutUIBuildRequest(req->GetRequest(), user, pOfferList, offerCount));

        if(req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(req->GetResponse().Code);
        }

        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::ShowCheckoutUIBuildRequest(lsx::ShowIGOWindowT& data, OriginUserT user, const OriginCharT *pOfferList[], size_t offerCount)
    {
        if (user == 0 || pOfferList == NULL || offerCount == 0)
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        data.Show = true;
        data.UserId = user;
        data.WindowId = lsx::IGOWINDOW_CHECKOUT;

        data.Offers.reserve(offerCount);

        for(unsigned int i = 0; i < offerCount; i++)
        {
            data.Offers.push_back(pOfferList[i]);
        }
        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::QueryEntitlements(OriginUserT user, const OriginCharT *pCategoryList[], size_t categoryCount, const OriginCharT *pOfferList[], size_t offerCount, const OriginCharT *pItemList[], size_t itemCount, const OriginCharT * pGroup, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
    {
        if(pCategoryList != NULL || categoryCount != 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
        if(offerCount != 0 && pOfferList == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
        if(itemCount != 0 && pItemList == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
        if(callback == NULL && pHandle == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(categoryCount > 0)
            return REPORTERROR(ORIGIN_ERROR_TOO_MANY_VALUES_IN_LIST);
        if(itemCount > 1)
            return REPORTERROR(ORIGIN_ERROR_TOO_MANY_VALUES_IN_LIST);

        LSXEnumeration<lsx::QueryEntitlementsT, lsx::QueryEntitlementsResponseT, OriginItemT> * req =
            new LSXEnumeration<lsx::QueryEntitlementsT, lsx::QueryEntitlementsResponseT, OriginItemT>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::QueryEntitlementsConvertData, callback, pContext);

        lsx::QueryEntitlementsT &r = req->GetRequest();

        r.UserId = user;
        r.Group = pGroup != NULL ? pGroup : "";

        r.FilterOffers.reserve(offerCount);
        for(unsigned int i = 0; i < offerCount; i++)
        {
            r.FilterOffers.push_back(pOfferList[i]);
        }

        r.FilterItems.reserve(itemCount);
        for(unsigned int i = 0; i < itemCount; i++)
        {
            r.FilterItems.push_back(pItemList[i]);
        }

        OriginHandleT h = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if(pHandle != NULL)
        {
            *pHandle = h;
        }

        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            DestroyHandle(h);
            if (pHandle != NULL) *pHandle = NULL;
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::QueryEntitlementsSync(OriginUserT user, const OriginCharT *pCategoryList[], size_t categoryCount, const OriginCharT *pOfferList[], size_t offerCount, const OriginCharT *pItemList[], size_t itemCount, const OriginCharT * pGroup, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
    {
        if(pCategoryList != NULL || categoryCount != 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
        if(offerCount != 0 && pOfferList == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
        if(itemCount != 0 && pItemList == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(categoryCount > 0)
            return REPORTERROR(ORIGIN_ERROR_TOO_MANY_VALUES_IN_LIST);
        if(itemCount > 1)
            return REPORTERROR(ORIGIN_ERROR_TOO_MANY_VALUES_IN_LIST);

        LSXEnumeration<lsx::QueryEntitlementsT, lsx::QueryEntitlementsResponseT, OriginItemT> * req =
            new LSXEnumeration<lsx::QueryEntitlementsT, lsx::QueryEntitlementsResponseT, OriginItemT>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::QueryEntitlementsConvertData );

        lsx::QueryEntitlementsT &r = req->GetRequest();

        r.UserId = user;
        r.Group = pGroup != NULL ? pGroup : "";

        r.FilterOffers.reserve(offerCount);
        for(unsigned int i = 0; i < offerCount; i++)
        {
            r.FilterOffers.push_back(pOfferList[i]);
        }
        r.FilterItems.reserve(itemCount);
        for(unsigned int i = 0; i < itemCount; i++)
        {
            r.FilterItems.push_back(pItemList[i]);
        }

        *pHandle = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if(req->Execute(timeout, pTotalCount))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::QueryEntitlements(OriginUserT user, const OriginCharT *pOfferList[], OriginSizeT offerCount, const OriginCharT *pItemList[], OriginSizeT itemCount, const OriginCharT *pGroupList[], OriginSizeT groupCount, bool includeChildGroups, bool includeExpiredTrialDLC, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
    {
        if(offerCount != 0 && pOfferList == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
        if(itemCount != 0 && pItemList == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
        if(groupCount != 0 && pGroupList == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
        if(callback == NULL && pHandle == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(itemCount > 1)
            return REPORTERROR(ORIGIN_ERROR_TOO_MANY_VALUES_IN_LIST);

        LSXEnumeration<lsx::QueryEntitlementsT, lsx::QueryEntitlementsResponseT, OriginItemT> * req =
            new LSXEnumeration<lsx::QueryEntitlementsT, lsx::QueryEntitlementsResponseT, OriginItemT>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::QueryEntitlementsConvertData, callback, pContext);

        lsx::QueryEntitlementsT &r = req->GetRequest();

        r.UserId = user;

        if(groupCount > 0)
        {
            // This is for backwards compatibility, so things will still work in Origin 9.3
            if(groupCount == 1)
            {
                r.Group = pGroupList[0] != NULL ? pGroupList[0] : "";
            }
            else
            {
                r.FilterGroups.reserve(groupCount);
                for(unsigned int i = 0; i < groupCount; i++)
                {
                    r.FilterGroups.push_back(pGroupList[i] != NULL ? pGroupList[i] : "");
                }
            }
        }

        r.FilterOffers.reserve(offerCount);
        for(unsigned int i = 0; i < offerCount; i++)
        {
            r.FilterOffers.push_back(pOfferList[i] != NULL ? pOfferList[i] : "");
        }

        r.FilterItems.reserve(itemCount);
        for(unsigned int i = 0; i < itemCount; i++)
        {
            r.FilterItems.push_back(pItemList[i] != NULL ? pItemList[i] : "");
        }

        r.includeChildGroups = includeChildGroups;
        r.includeExpiredTrialDLC = includeExpiredTrialDLC;

        OriginHandleT h = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if(pHandle != NULL)
        {
            *pHandle = h;
        }

        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            DestroyHandle(h);
            if (pHandle != NULL) *pHandle = NULL;
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::QueryEntitlementsSync(OriginUserT user, const OriginCharT *pOfferList[], OriginSizeT offerCount, const OriginCharT *pItemList[], OriginSizeT itemCount, const OriginCharT *pGroupList[], OriginSizeT groupCount, bool includeChildGroups, bool includeExpiredTrialDLC, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
    {
        if(offerCount != 0 && pOfferList == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
        if(itemCount != 0 && pItemList == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
        if(groupCount != 0 && pGroupList == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(itemCount > 1)
            return REPORTERROR(ORIGIN_ERROR_TOO_MANY_VALUES_IN_LIST);

        LSXEnumeration<lsx::QueryEntitlementsT, lsx::QueryEntitlementsResponseT, OriginItemT> * req =
            new LSXEnumeration<lsx::QueryEntitlementsT, lsx::QueryEntitlementsResponseT, OriginItemT>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::QueryEntitlementsConvertData );

        lsx::QueryEntitlementsT &r = req->GetRequest();

        r.UserId = user;

        if(groupCount > 0)
        {
            // This is for backwards compatibility, so things will still work in Origin 9.3
            if(groupCount == 1)
            {
                r.Group = pGroupList[0] != NULL ? pGroupList[0] : "";
            }
            else
            {
                r.FilterGroups.reserve(groupCount);
                for(unsigned int i = 0; i < groupCount; i++)
                {
                    r.FilterGroups.push_back(pGroupList[i] != NULL ? pGroupList[i] : "");
                }
            }
        }

        r.FilterOffers.reserve(offerCount);
        for(unsigned int i = 0; i < offerCount; i++)
        {
            r.FilterOffers.push_back(pOfferList[i] != NULL ? pOfferList[i] : "");
        }

        r.FilterItems.reserve(itemCount);
        for(unsigned int i = 0; i < itemCount; i++)
        {
            r.FilterItems.push_back(pItemList[i] != NULL ? pItemList[i] : "");
        }

        r.includeChildGroups = includeChildGroups;
        r.includeExpiredTrialDLC = includeExpiredTrialDLC;

        *pHandle = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if(req->Execute(timeout, pTotalCount))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::QueryEntitlementsConvertData(IEnumerator *pEnumerator, OriginItemT *pItems, size_t index, size_t count, lsx::QueryEntitlementsResponseT &response)
    {
        ConvertData(pEnumerator, pItems, response.Entitlements, index, count, false);

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::ConsumeEntitlement(OriginUserT user, uint32_t uses, bool bOveruse, OriginItemT *pItem, OriginEntitlementCallback callback, void *pContext, uint32_t timeout)
    {
        if(pItem == NULL || uses == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::ConsumeEntitlementT, lsx::ConsumeEntitlementResponseT, OriginItemT, OriginEntitlementCallback> > req(
            new  LSXRequest<lsx::ConsumeEntitlementT, lsx::ConsumeEntitlementResponseT, OriginItemT, OriginEntitlementCallback>
                (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::ConsumeEntitlementConvertData, callback, pContext));

        lsx::ConsumeEntitlementT &r = req->GetRequest();

        r.UserId = user;
        ConvertData(r.Entitlement, *pItem);
        r.Uses = uses;
        r.bOveruse = bOveruse;

        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::ConsumeEntitlementConvertData(IXmlMessageHandler *pHandler, OriginItemT &item, size_t &size, lsx::ConsumeEntitlementResponseT &response)
    {
        size = sizeof(OriginItemT);
        ConvertData(pHandler, item, response.Entitlement, false);
        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::ConsumeEntitlementSync(OriginUserT user, uint32_t uses, bool bOveruse, OriginItemT *pItem, uint32_t timeout, OriginHandleT *pHandle)
    {
        if(uses == 0 || pItem == NULL || pHandle == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::ConsumeEntitlementT, lsx::ConsumeEntitlementResponseT> > req(
            new LSXRequest<lsx::ConsumeEntitlementT, lsx::ConsumeEntitlementResponseT>
                (GetService(lsx::FACILITY_COMMERCE), this));

        lsx::ConsumeEntitlementT &r = req->GetRequest();

        r.UserId = user;
        ConvertData(r.Entitlement, *pItem);
        r.Uses = uses;
        r.bOveruse = bOveruse;

        if(req->Execute(timeout))
        {
            lsx::ConsumeEntitlementResponseT &res = req->GetResponse();

            ConvertData(req, *pItem, res.Entitlement, true);

            *pHandle = req->CopyDataStoreToHandle();

            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }


}
