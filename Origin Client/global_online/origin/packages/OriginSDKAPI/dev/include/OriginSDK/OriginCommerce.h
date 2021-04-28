#ifndef __ORIGIN_COMMERCE_H__
#define __ORIGIN_COMMERCE_H__

#include "OriginTypes.h"
#include "OriginEnumeration.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// \defgroup ecommerce Commerce
/// \brief This module contains the functions that provide Origin's Commerce features.
///
/// There are sychronous and asynchronous versions of several of these functions. For an explanation of why, see
/// \ref syncvsasync in the <em>Integrator's Guide</em>.
///
/// For more information on the integration of this feature, see \ref na-intguide-commerce in the <em>Integrator's
/// Guide</em>.

/// \ingroup ecommerce
/// \brief Selects the store to use for the In-Fiction Store. [deprecated]
///
/// Allows the game to select a different store as the in-fiction store. The IGO will still be using the Origin store.
/// \param [in] StoreId The OFB id of the store you want to connect to. Specifying NULL here will select the Origin Store.
/// \param [in] CatalogId The OFB catalogId you want to get data from. In general a game has two catalogs, one for paid currency/points and one for achievement points. Can be NULL if there is only one catalog in the store.
/// \param [in] pLockboxUrl The lockbox url needed for in-fiction checkout. (Cannot be NULL)
/// \param [in] pSuccessUrl When the lockbox operation succeeds the browser will be redirected to this URL. (Cannot be NULL)
/// \param [in] pFailedUrl When the lockbox operation fails the browser will be redirected to this URL. (Cannot be NULL)
OriginErrorT ORIGIN_SDK_API OriginSelectStore(uint64_t StoreId, uint64_t CatalogId, const OriginCharT *pLockboxUrl, const OriginCharT *pSuccessUrl, const OriginCharT * pFailedUrl);

/// \ingroup ecommerce
/// \brief Selects the store to use for the In-Fiction Store. [deprecated]
///
/// Allows the game to select a different store as the in-fiction store. The IGO will still be using the Origin store.
/// \param [in] StoreId The OFB id of the store you want to connect to. Specifying NULL here will select the Origin Store.
/// \param [in] CatalogId The OFB catalogId you want to get data from. In general a game has two catalogs, one for paid currency/points and one for achievement points. Can be NULL if there is only one catalog in the store.
/// \param [in] EWalletCategoryId The Category of the EWallet Products.
/// \param [in] pVirtualCurrency The Virtual Currency to use in Virtual Paid Transactions.
/// \param [in] pLockboxUrl The lockbox url needed for in-fiction checkout. (Cannot be NULL)
/// \param [in] pSuccessUrl When the lockbox operation succeeds the browser will be redirected to this URL. (Cannot be NULL)
/// \param [in] pFailedUrl When the lockbox operation fails the browser will be redirected to this URL. (Cannot be NULL)
OriginErrorT ORIGIN_SDK_API OriginSelectStoreEX(uint64_t StoreId, uint64_t CatalogId, uint64_t EWalletCategoryId, const OriginCharT *pVirtualCurrency, const OriginCharT *pLockboxUrl, const OriginCharT *pSuccessUrl, const OriginCharT * pFailedUrl);

/// \ingroup ecommerce
/// \brief Get all the catalogs in a store. [deprecated]
/// The OriginGetStore function uses the StoreId set by the \ref OriginSelectStore function.
/// \param [in] user The local user
/// \param [in] storeId The store you want information for. Specify 0 if you want to use the store specified in \ref OriginSelectStore. The specified value will be the newly selected store.
/// \param [in] callback The user callback function that gets called when the store information is received.
/// \param [in] pContext A user provided pointer to a context
/// \param [in] timeout A timeout in milliseconds for how long to wait for a response.
/// \return indication whether the call succeeded
OriginErrorT ORIGIN_SDK_API OriginGetStore(OriginUserT user, uint64_t storeId, OriginStoreCallback callback, void *pContext, uint32_t timeout);

/// \ingroup ecommerce
/// \brief Get all the catalogs in a store. [deprecated]
/// The OriginGetStoreSync function uses the StoreId set by the \ref OriginSelectStore function.
/// \param [in] user The local user
/// \param [in] storeId The store you want information for. Specify 0 if you want to use the store specified in \ref OriginSelectStore. The specified value will be the newly selected store.
/// \param[out] ppStore An address of a pointer that receives the array of stores
/// \param[out] pNumberOfStores The address of an integer that receive the number of stores [either 1, or 0]
/// \param [in] timeout A timeout in milliseconds for how long to wait for a response.
/// \param[out] pHandle	A handle that can be used to release the resources associated with the store.
/// \return indication whether the call succeeded
OriginErrorT ORIGIN_SDK_API OriginGetStoreSync(OriginUserT user, uint64_t storeId, OriginStoreT ** ppStore, OriginSizeT *pNumberOfStores, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup ecommerce
/// \brief Gets the catalog from the store. [deprecated]
///
/// \param [in] user The local user.
/// \param [in] callback The user function that will get called when the catalog is received.
/// \param [in] pContext A user-provided pointer to a context.
/// \param [in] timeout A timeout in milliseconds for how long to wait for a response.
OriginErrorT ORIGIN_SDK_API OriginGetCatalog(OriginUserT user, OriginCatalogCallback callback, void *pContext, uint32_t timeout);

/// \ingroup ecommerce
/// \brief Gets the catalog from the store. [deprecated]
///
/// \param [in] user The local user.
/// \param[out] ppCategories A pointer to an SDK allocated version of the catalog.
/// \param[out] pCategoryCount The number of categories in the root catalog.
/// \param [in] timeout A timeout in milliseconds for how long to wait for a response.
/// \param[out] handle A handle to the allocated resources. Use \ref OriginDestroyHandle to free the associated resources.
OriginErrorT ORIGIN_SDK_API OriginGetCatalogSync(OriginUserT user, OriginCategoryT **ppCategories, OriginSizeT *pCategoryCount, uint32_t timeout, OriginHandleT *handle);


/// \ingroup ecommerce
/// \brief Gets the users virtual paid currency wallet balance.
///
/// \param [in] user The local user.
/// \param [in] currency The three character virtual currency identifier (e.g., _NR).
/// \param [in] callback The callback to be called when the balance is received or when an error occurs.
/// \param [in] pContext A user-provided context for the callback function.
/// \param [in] timeout A timeout in milliseconds for how long to wait for a response.
OriginErrorT ORIGIN_SDK_API OriginGetWalletBalance(OriginUserT user, const OriginCharT *currency, OriginWalletCallback callback, void *pContext, uint32_t timeout);

/// \ingroup ecommerce
/// \brief Gets the current balance of the users virtual paid currency wallet.
///
/// \param [in] user The local user.
/// \param [in] currency The three character virtual currency identifier (e.g., _NR).
/// \param[out] pBalance The amount of points in the virtual currency wallet.
/// \param [in] timeout A timeout in milliseconds for how long to wait for a response.
OriginErrorT ORIGIN_SDK_API OriginGetWalletBalanceSync(OriginUserT user, const OriginCharT *currency, int64_t *pBalance, uint32_t timeout);

/// \ingroup ecommerce
/// \brief Shows the Store UI.
///
/// Displays the in-game store via the overlay system, and allows the customer to purchase items and/or re-download already
/// purchased items.
///
/// The store contents can be limited to certain categories and/or items by passing in filters.
///	\param [in] user The local user.
///	\param [in] pCategoryList[] An array of category name pointers.
///	\param [in] categoryCount The count of category name pointers (0=dont filter).
///	\param [in] pOfferList[] An array of offer identifiers.
///	\param [in] offerCount A count of offer identifiers (0=dont filter).
/// \param [in] callback The callback to be called when the IGO is shown. Can be NULL.
/// \param [in] pContext a user-defined context to be used in the callback
OriginErrorT ORIGIN_SDK_API OriginShowStoreUI(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginErrorSuccessCallback callback, void* pContext);

/// \ingroup ecommerce
/// \brief Shows the Store UI.
///
/// Displays the in-game store via the overlay system, and allows the customer to purchase items and/or re-download already
/// purchased items.
///
/// The store contents can be limited to certain categories and/or items by passing in filters.
///	\param [in] user The local user.
///	\param [in] pCategoryList[] An array of category name pointers.
///	\param [in] categoryCount The count of category name pointers (0=dont filter).
///	\param [in] pOfferList[] An array of offer identifiers.
///	\param [in] offerCount A count of offer identifiers (0=dont filter).
OriginErrorT ORIGIN_SDK_API OriginShowStoreUISync(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pOfferList[], OriginSizeT offerCount);

/// \ingroup ecommerce
/// \brief Shows the Store UI Asynchronously.
///
/// Displays the in-game store via the overlay system, and allows the customer to purchase items and/or re-download already
/// purchased items.
///
/// The store contents can be limited to certain categories and/or items by passing in filters.
///	\param [in] user The local user.
///	\param [in] pCategoryList[] An array of category name pointers.
///	\param [in] categoryCount The count of category name pointers (0=dont filter).
/// \param [in] pMasterTitleIdList[] A list of Master Title Ids to include the products.
/// \param [in] MasterTitleIdCount The number of Master Title Ids in the list.
///	\param [in] pOfferList[] An array of offer identifiers.
///	\param [in] offerCount A count of offer identifiers (0=dont filter).
/// \param [in] callback The callback to be called when the IGO is shown. Can be NULL.
/// \param [in] pContext a user-defined context to be used in the callback
OriginErrorT ORIGIN_SDK_API OriginShowStoreUIEX(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginErrorSuccessCallback callback, void* pContext);

/// \ingroup ecommerce
/// \brief Shows the Store UI Synchronously.
///
/// Displays the in-game store via the overlay system, and allows the customer to purchase items and/or re-download already
/// purchased items.
///
/// The store contents can be limited to certain categories and/or items by passing in filters.
///	\param [in] user The local user.
///	\param [in] pCategoryList[] An array of category name pointers.
///	\param [in] categoryCount The count of category name pointers (0=dont filter).
/// \param [in] pMasterTitleIdList[] A list of Master Title Ids to include the products.
/// \param [in] MasterTitleIdCount The number of Master Title Ids in the list.
///	\param [in] pOfferList[] An array of offer identifiers.
///	\param [in] offerCount A count of offer identifiers (0=dont filter).
OriginErrorT ORIGIN_SDK_API OriginShowStoreUIEXSync(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], OriginSizeT offerCount);

/// \ingroup ecommerce
/// \brief Shows the Code Redemption UI Asynchronously.
///
/// Displays the in-game code redemption UI via the overlay system, and allows the customer to enter a code.
/// Redeeming the code will grant the customer an entitlement to the offer configured for that code.
/// After redemption, the corresponding purchase event will be sent to the application via the notification interface.
///	\param [in] user The local user.
/// \param [in] callback The callback to be called when the IGO is shown. Can be NULL.
/// \param [in] pContext a user-defined context to be used in the callback
OriginErrorT ORIGIN_SDK_API OriginShowCodeRedemptionUI(OriginUserT user, OriginErrorSuccessCallback callback, void* pContext);

/// \ingroup ecommerce
/// \brief Shows the Code Redemption UI Synchronously.
///
/// Displays the in-game code redemption UI via the overlay system, and allows the customer to enter a code.
/// Redeeming the code will grant the customer an entitlement to the offer configured for that code.
/// After redemption, the corresponding purchase event will be sent to the application via the notification interface.
///	\param [in] user The local user.
OriginErrorT ORIGIN_SDK_API OriginShowCodeRedemptionUISync(OriginUserT user);


/// \ingroup ecommerce
/// \brief Gets the categories in the current selected catalog.
///
/// Queries the offer categories available for this title. Typically used by in-fiction store implementations. See
/// \ref enumeratorpattern in the <i>Origin SDK Integrator's Guide</i>. The returned list of items can be filtered by
/// passing a list of parent category identifiers or by passing the empty string to return only top-level categories.
/// Alternatively, the caller can pass an empty list and all categories will be returned (and the game can construct
/// the store hierarchy by traversing the \ref OriginCategoryT records and using strParentID to understand the
/// relationship between category levels).
/// \param [in] user The local user.
/// \param [in] pParentCategoryList[] A list of parent categories from which you want the child categories.
/// \param [in] parentCategoryCount The number of parent categories.
/// \param [in] callback The callback function that will be called when data arrives.
/// \param [in] pContext The user context pointer for the callback function. Not used except to pass back to the callback.
/// \param [in] timeout The amount of time the SDK will wait for a response.
/// \param[out] pHandle A pointer to the enumeration handle. Destroy this handle in the callback when all data is received.
/// \sa XMarketplaceCreateOfferEnumerator(DWORD dwUserIndex, DWORD dwOfferType, DWORD dwContentCategories, DWORD cItem, PDWORD *pcbBuffer, PHANDLE *phEnum)
OriginErrorT ORIGIN_SDK_API OriginQueryCategories(OriginUserT user, const OriginCharT *pParentCategoryList[], OriginSizeT parentCategoryCount, OriginEnumerationCallbackFunc callback, void * pContext, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup ecommerce
/// \brief Gets the categories from the currently selected catalog synchronically.
///
/// Queries the offer categories available for this title. Typically used by in-fiction store implementations. See
/// \ref enumeratorpattern in the <i>Origin SDK Integrator's Guide</i>. The returned list of items can be filtered by
/// passing a list of parent category identifiers or by passing the empty string to return only top-level categories.
/// Alternatively, the caller can pass an empty list and all categories will be returned (and the game can construct
/// the store hierarchy by traversing the \ref OriginCategoryT records and using strParentID to understand the
/// relationship between category levels).
/// \param [in] user The local user.
/// \param [in] pParentCategoryList[] A list of parent categories from which you want the child categories.
/// \param [in] parentCategoryCount The number of parent categories.
/// \param [in] pTotalCount The total number of categories returned. Use \ref OriginReadEnumeration to read the data.
/// \param [in] timeout The amount of time the SDK will wait for a response.
/// \param[out] pHandle A pointer to the enumeration handle. Call \ref OriginDestroyHandle when you are done with this enumeration.
/// \sa XMarketplaceCreateOfferEnumerator(DWORD dwUserIndex, DWORD dwOfferType, DWORD dwContentCategories, DWORD cItem, PDWORD *pcbBuffer, PHANDLE *phEnum)
OriginErrorT ORIGIN_SDK_API OriginQueryCategoriesSync(OriginUserT user, const OriginCharT *pParentCategoryList[], OriginSizeT parentCategoryCount, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup ecommerce
/// \brief Receives the offers from the specified categories.
///
/// Queries the individual offers available for this title. Typically used by in-fiction store implementations.
/// If all filter parameters are not specified the products will be requested on the titles Master TitleId.
/// \param [in] user The local user.
/// \param [in] pCategoryList[] A list of categories to filter the products.
/// \param [in] categoryCount The number of categories in the list.
/// \param [in] pOfferList[] The list of offers to filter on.
/// \param [in] offerCount The number of offer id's in the list.
/// \param [in] callback The callback function that will be called when data arrives.
/// \param [in] pContext The user context pointer for the callback function.  Not used except to pass back to the callback.
/// \param [in] timeout The amount of time the SDK will wait for a response.
/// \param[out] pHandle A pointer to the enumeration handle. Destroy this handle in the callback once all data is received.
/// \sa XMarketplaceCreateOfferEnumerator(DWORD dwUserIndex, DWORD dwOfferType, DWORD dwContentCategories, DWORD cItem, PDWORD *pcbBuffer, PHANDLE *phEnum)
OriginErrorT ORIGIN_SDK_API OriginQueryOffers(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup ecommerce
/// \brief Receives the offers from the specified categories synchronically.
///
/// Queries the individual offers available for this title. Typically used by in-fiction store implementations.
/// If all filter parameters are not specified the products will be requested on the titles Master TitleId.
/// \param [in] user The local user.
/// \param [in] pCategoryList[] A list of categories to filter the products.
/// \param [in] categoryCount The number of categories in the list.
/// \param [in] pOfferList[] The list of offers to filter on.
/// \param [in] offerCount The number of offer ID's in the list.
/// \param [in] pTotalCount The total number of items returned. Use \ref OriginReadEnumeration to read the data.
/// \param [in] timeout The amount of time the SDK will wait for a response.
/// \param[out] pHandle A pointer to the enumeration handle. Destroy this handle in the callback when all data is received.
/// \sa XMarketplaceCreateOfferEnumerator(DWORD dwUserIndex, DWORD dwOfferType, DWORD dwContentCategories, DWORD cItem, PDWORD *pcbBuffer, PHANDLE *phEnum)
OriginErrorT ORIGIN_SDK_API OriginQueryOffersSync(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup ecommerce
/// \brief Receives the offers from the specified categories.
///
/// Queries the individual offers available for this title. Typically used by in-fiction store implementations.
/// If all filter parameters are not specified the products will be requested on the titles Master TitleId.
/// \param [in] user The local user.
/// \param [in] pCategoryList[] A list of categories to filter the products.
/// \param [in] categoryCount The number of categories in the list.
/// \param [in] pMasterTitleIdList[] A list of Master Title Ids to include the products.
/// \param [in] MasterTitleIdCount The number of Master Title Ids in the list.
/// \param [in] pOfferList[] The list of offers to filter on.
/// \param [in] offerCount The number of offer id's in the list.
/// \param [in] callback The callback function that will be called when data arrives.
/// \param [in] pContext The user context pointer for the callback function.  Not used except to pass back to the callback.
/// \param [in] timeout The amount of time the SDK will wait for a response.
/// \param[out] pHandle A pointer to the enumeration handle. Destroy this handle in the callback once all data is received.
/// \sa XMarketplaceCreateOfferEnumerator(DWORD dwUserIndex, DWORD dwOfferType, DWORD dwContentCategories, DWORD cItem, PDWORD *pcbBuffer, PHANDLE *phEnum)
OriginErrorT ORIGIN_SDK_API OriginQueryOffersEX(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup ecommerce
/// \brief Receives the offers from the specified categories synchronically.
///
/// Queries the individual offers available for this title. Typically used by in-fiction store implementations.
/// If all filter parameters are not specified the products will be requested on the titles Master TitleId.
/// \param [in] user The local user.
/// \param [in] pCategoryList[] A list of categories to filter the products.
/// \param [in] categoryCount The number of categories in the list.
/// \param [in] pMasterTitleIdList[] A list of Master Title Ids to include the products.
/// \param [in] MasterTitleIdCount The number of Master Title Ids in the list.
/// \param [in] pOfferList[] The list of offers to filter on.
/// \param [in] offerCount The number of offer ID's in the list.
/// \param [in] pTotalCount The total number of items returned. Use \ref OriginReadEnumeration to read the data.
/// \param [in] timeout The amount of time the SDK will wait for a response.
/// \param[out] pHandle A pointer to the enumeration handle. Destroy this handle in the callback when all data is received.
/// \sa XMarketplaceCreateOfferEnumerator(DWORD dwUserIndex, DWORD dwOfferType, DWORD dwContentCategories, DWORD cItem, PDWORD *pcbBuffer, PHANDLE *phEnum)
OriginErrorT ORIGIN_SDK_API OriginQueryOffersSyncEX(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup ecommerce
/// \brief Queries the result of a purchase transaction.
///
/// Queries what offers were purchased for a particular manifest. The manifest ID is returned by the \ref ORIGIN_EVENT_PURCHASE event
/// \param [in] user The user for which the manifest is requested.
/// \param [in] manifest The manifest identifier from the PURCHASE event.
/// \param [in] callback A callback function that will be called from \ref OriginUpdate when data arrives.
/// \param [in] pContext A user-provided data pointer for the callback function.
/// \param [in] timeout The amount of time the SDK will wait for a response.
/// \param[out] pHandle A handle for \ref OriginReadEnumeration functions to return \ref OriginOfferT structures.
OriginErrorT ORIGIN_SDK_API OriginQueryManifest(OriginUserT user, const OriginCharT* manifest, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup ecommerce
/// \brief Queries the result of a purchase transaction synchronically.
///
/// Queries what offers were purchased for a particular manifest. The manifest ID is returned by the \ref ORIGIN_EVENT_PURCHASE event
/// function as the parameter of an \ref ORIGIN_EVENT_PURCHASE event.
/// \param [in] user The user for which the manifest is requested.
/// \param [in] manifest Manifest identifier from the PURCHASE event.
/// \param[out] pTotalCount The total number of results in the enumeration.
/// \param [in] timeout The amount of time the SDK will wait for a response.
/// \param[out] pHandle A handle for \ref OriginReadEnumeration functions to return OriginOfferT structures.
OriginErrorT ORIGIN_SDK_API OriginQueryManifestSync(OriginUserT user, const OriginCharT* manifest, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup ecommerce
/// \brief Shows the lockbox checkout UI Asynchronously.
///
/// Displays the overlay-based checkout flow for the passed-in list of offers. Purchase completion and download
/// completion notification events will be sent as is appropriate.
/// \param [in] user The local user.
/// \param [in] pOfferList[] An array of offer identifiers. [Currently only one offer is supported]
/// \param [in] offerCount The number of offer identifiers in the offer list.
/// \param [in] callback A callback function that will be called from \ref OriginUpdate when data arrives.
/// \param [in] pContext A user-provided data pointer for the callback function.
OriginErrorT ORIGIN_SDK_API OriginShowCheckoutUI(OriginUserT user, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginErrorSuccessCallback callback, void* pContext);

/// \ingroup ecommerce
/// \brief Shows the lockbox checkout UI Synchronously.
///
/// Displays the overlay-based checkout flow for the passed-in list of offers. Purchase completion and download
/// completion notification events will be sent as is appropriate.
/// \param [in] user The local user.
/// \param [in] pOfferList[] An array of offer identifiers. [Currently only one offer is supported]
/// \param [in] offerCount The number of offer identifiers in the offer list.
OriginErrorT ORIGIN_SDK_API OriginShowCheckoutUISync(OriginUserT user, const OriginCharT *pOfferList[], OriginSizeT offerCount);

/// \ingroup ecommerce
/// \brief Do a direct checkout
///
/// This will perform a checkout directly, circumventing the IGO screens. This can only be used for Virtual Currency offers, when there is sufficient balance.
/// \param [in] user The local user.
/// \param [in] Currency The currency this transaction should be taken place in. (This is an optimization, to reduce query count on the server.)
/// \param [in] pOfferList[] An array of offer identifiers. [Currently only one offer is supported]
/// \param [in] offerCount The number of offer identifiers in the offer list.
/// \param [in] callback A callback function that will be called from \ref OriginUpdate when data arrives.
/// \param [in] pContext A user-provided data pointer for the callback function.
/// \param [in] timeout The amount of time the SDK will wait for a response.
OriginErrorT ORIGIN_SDK_API OriginCheckout(OriginUserT user, const OriginCharT * Currency, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginResourceCallback callback, void *pContext, uint32_t timeout);

/// \ingroup ecommerce
/// \brief Do a direct checkout
///
/// This will perform a checkout directly, circumventing the IGO screens. This can only be used for Virtual Currency offers, when there is sufficient balance.
/// \param [in] user The local user.
/// \param [in] Currency The currency this transaction should be taken place in. (This is an optimization, to reduce query count on the server.)
/// \param [in] pOfferList[] An array of offer identifiers. [Currently only one offer is supported]
/// \param [in] offerCount The number of offer identifiers in the offer list.
/// \param [in] timeout The amount of time the SDK will wait for a response.
OriginErrorT ORIGIN_SDK_API OriginCheckoutSync(OriginUserT user, const OriginCharT * Currency, const OriginCharT *pOfferList[], OriginSizeT offerCount, uint32_t timeout);


/// \ingroup ecommerce
/// \brief Queries the entitlements for the user.  [deprecated use OriginQueryEntitlementsEx]
///
/// The list can be filtered by category, offer or item IDs. The different filters are additive and many combinations
/// will return no results. For example, passing an offer-id and catalog-id which are not associated with each other
/// will never enumerate any items.
/// \param [in] user The local user.
/// \param [in] pCategoryList[] A list of category names. This parameter is reserved and should be NULL.
/// \param [in] categoryCount The number of category names (0=query all categories). This parameter is reserved and should be 0.
/// \param [in] pOfferList[] An array of offer identifiers.
/// \param [in] offerCount The number of offer identifiers (0=query all offers). Only one offer is supported at this time.
/// \param [in] pEntitlementTagList[] An array of item identifiers.
/// \param [in] entitlementTagCount The number of item identifiers (0=query all items). Only one item is supported at this time.
/// \param [in] pGroup The group the entitlements belong to ("" or NULL means all groups).
/// \param [in] callback A callback function that will be called from \ref OriginUpdate as data arrives.
/// \param [in] pContext A data pointer for the callback function.
/// \param [in] timeout The amount of time the SDK will wait for a response.
/// \param[out] pHandle A handle for \ref OriginReadEnumeration functions to return \ref OriginItemT structures.
OriginErrorT ORIGIN_SDK_API OriginQueryEntitlements(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, const OriginCharT *pEntitlementTagList[], OriginSizeT entitlementTagCount, const OriginCharT *pGroup, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup ecommerce
/// \brief Queries the entitlements for the user. [deprecated use OriginQueryEntitlementsSyncEx]
///
/// \param [in] user The local user.
/// \param [in] pCategoryList[] A list of category names. This parameter is reserved and should be NULL.
/// \param [in] categoryCount The number of category names (0=query all categories). This parameter is reserved and should be 0.
/// \param [in] pOfferList[] An array of offer identifiers.
/// \param [in] offerCount The number of offer identifiers (0=query all offers). Only one offer is supported at this time.
/// \param [in] pEntitlementTagList[] An array of item identifiers.
/// \param [in] entitlementTagCount The number of item identifiers (0=query all items). Only one item is supported at this time.
/// \param [in] pGroup The group the entitlements belong to ("" or NULL means all groups).
/// \param[out] pTotalCount The total number of results in the enumeration.
/// \param [in] timeout The amount of time the SDK will wait for a response.
/// \param[out] pHandle A handle for \ref OriginReadEnumeration functions to return \ref OriginItemT structures.
OriginErrorT ORIGIN_SDK_API OriginQueryEntitlementsSync(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, const OriginCharT *pEntitlementTagList[], OriginSizeT entitlementTagCount, const OriginCharT *pGroup, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup ecommerce
/// \brief Queries the entitlements for the user.
///
/// The list can be filtered by category, offer or item IDs. The different filters are additive and many combinations
/// will return no results. For example, passing an offer-id and catalog-id which are not associated with each other
/// will never enumerate any items.
/// \param [in] user The local user.
/// \param [in] pOfferList[] An array of offer identifiers.
/// \param [in] offerCount The number of offer identifiers (0=query all offers). Only one offer is supported at this time.
/// \param [in] pEntitlementTagList[] An array of item identifiers.
/// \param [in] entitlementTagCount The number of item identifiers (0=query all items). Only one item is supported at this time.
/// \param [in] pGroupList[] The groups the entitlements belong to ("" or NULL means all groups).
/// \param [in] groupCount The number of groups (0=query all groups).
/// \param [in] includeChildGroups Indicates whether the group hierarchy should be traversed when finding items.
/// \param [in] callback A callback function that will be called from \ref OriginUpdate as data arrives.
/// \param [in] pContext A data pointer for the callback function.
/// \param [in] timeout The amount of time the SDK will wait for a response.
/// \param[out] pHandle A handle for \ref OriginReadEnumeration functions to return \ref OriginItemT structures.
OriginErrorT ORIGIN_SDK_API OriginQueryEntitlementsEX(OriginUserT user, const OriginCharT *pOfferList[], OriginSizeT offerCount, const OriginCharT *pEntitlementTagList[], OriginSizeT entitlementTagCount, const OriginCharT *pGroupList[], OriginSizeT groupCount, bool includeChildGroups, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup ecommerce
/// \brief Queries the entitlements for the user.
///
/// \param [in] user The local user.
/// \param [in] pOfferList[] An array of offer identifiers.
/// \param [in] offerCount The number of offer identifiers (0=query all offers). Only one offer is supported at this time.
/// \param [in] pEntitlementTagList[] An array of item identifiers.
/// \param [in] entitlementTagCount The number of item identifiers (0=query all items). Only one item is supported at this time.
/// \param [in] pGroupList[] The groups the entitlements belong to ("" or NULL means all groups).
/// \param [in] groupCount The number of groups (0=query all groups).
/// \param [in] includeChildGroups Indicates whether the group hierarchy should be traversed when finding items.
/// \param[out] pTotalCount The total number of results in the enumeration.
/// \param [in] timeout The amount of time the SDK will wait for a response.
/// \param[out] pHandle A handle for \ref OriginReadEnumeration functions to return \ref OriginItemT structures.
OriginErrorT ORIGIN_SDK_API OriginQueryEntitlementsSyncEX(OriginUserT user, const OriginCharT *pOfferList[], OriginSizeT offerCount, const OriginCharT *pEntitlementTagList[], OriginSizeT entitlementTagCount, const OriginCharT *pGroupList[], OriginSizeT groupCount, bool includeChildGroups, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup ecommerce
/// \brief Consumes an entitlement.
///
/// This function will consume uses of an entitlement. If called with a non-consumable entitlement, it will return an
/// error. If called on a consumable entitlement without enough uses, it will also return an error and will NOT consume
/// the remaining uses.
/// \param [in] user The local user.
/// \param [in] uses The number of uses to consume.
/// \param [in] bOveruse True if the function should return success; zero usage count if uses is greater than the available items.
/// \param [in] pItem The entitlement to consume.
/// \param [in] callback A callback to be called when the entitlement is consumed or when an error occurs.
/// \param [in] pContext A data pointer for the callback function.
/// \param [in] timeout The amount of time the SDK will wait for a response.
OriginErrorT ORIGIN_SDK_API OriginConsumeEntitlement(OriginUserT user, uint32_t uses, bool bOveruse, OriginItemT *pItem, OriginEntitlementCallback callback, void *pContext, uint32_t timeout);

/// \ingroup ecommerce
/// \brief Consumes an entitlement.
///
/// This function will consume uses of an entitlement. If called with a non-consumable entitlement, it will return an
/// error. If called on a consumable entitlement without enough uses, it will also return an error and will NOT consume
/// the remaining uses.
/// \param [in] user The local user.
/// \param [in] uses The number of uses to consume.
/// \param [in] bOveruse True if the function should return success, and zero usage count if uses is greater than the available items.
/// \param[in, out] pItem The entitlement to consume. The updated entitlement will replace the current.
/// \param [in] timeout The amount of time the SDK will wait for a response.
/// \param[out] pHandle A handle to the allocated resources. Use \ref OriginDestroyHandle to free the associated resources.
OriginErrorT ORIGIN_SDK_API OriginConsumeEntitlementSync(OriginUserT user, uint32_t uses, bool bOveruse, OriginItemT *pItem, uint32_t timeout, OriginHandleT *pHandle);

/// Not for match
//OriginQueryResource[Sync]
//Create an enumerator to query all the users entitled content which matches a particular game defined resource ID.
//
//See also XContentCreateEnumerator(DWORD dwUserIndex, XCONTENTDEVICEID DeviceID, DWORD dwContentType, DWORD dwContentFlags, DWORD cItem, PDWORD pcbBuffer, PHANDLE phEnum)
//
//Parameters:
//	[in] pUser=local user
//	[in] pResource=entitlements must match this resource identifier
//	[in] iOffset=index of resource to start retrieving at
//	[in] iMaxCount=maximum number of items to return
//	[in, optional] callback=a callback function that will be called from \ref OriginUpdate as data arrives
//	[in, optional] pContext=a data pointer for the callback function
//	[out] pTotalCount=total number of results in the enumeration
//	[out] pHandle=handle for \ref OriginReadEnumeration functions to return OriginItemT structures
//
//int32 OriginQueryResource(const OriginUserT *pUser,
//const OriginChar *pResource, int32 iOffset, int32 iMaxCount,
//OriginResourceCallbackT callback, void* pContext,
//uint32 timeout, HANDLE *pHandle);
//
//int32 OriginQueryResourceSync(const OriginUserT *pUser,
//const OriginChar *pResource, int32 iOffset, int32 iMaxCount,
//uint32 *pTotalCount, uint32 timeout, HANDLE *pHandle);



#ifdef __cplusplus
}
#endif



#endif //__ORIGIN_COMMERCE_H__
