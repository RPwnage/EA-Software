#ifndef __ORIGIN_TYPES_H__
#define __ORIGIN_TYPES_H__

#include "Origin.h"

// Get the definitions for our integers. This is supported on MAC, PC, and XBOX
#if !defined(INCLUDED_eabase_H)

#if _MSC_VER>=1600
#include <stdint.h>
#include <stddef.h>
#include <yvals.h>
#elif defined ORIGIN_MAC
#include <stdint.h>
#include <stddef.h>
#else
#define _STDINT
#include "OriginIntTypes.h"
#endif

#endif

// Include definitions of language strings.
#include "OriginEnums.h"
#include "OriginLanguage.h"
#include "OriginBaseTypes.h"

#if defined ORIGIN_PC
#include "windows/OriginWinTypes.h"
#elif defined ORIGIN_MAC
#include "mac/OriginMacTypes.h"
#endif

#pragma pack(push, 8)


/// \ingroup types
/// \brief A structure input to \ref OriginStartup that gives information about the game.
struct OriginStartupInputT
{
	const OriginCharT*	ContentId;		///< The content ID of this product. If the game is started through Origin an environment variable will be set with the content Id of the game. eg ContentId=71055 for FIFA12. If your executable has multiple version you can use this to know what version is started. [This is not a DRM solution.]
	const OriginCharT*	Title;			///< The title of the Game. Origin will use the ContentId to look up the actual name of the game in its list of entitlements. This title is only used when Origin cannot find an account matching the contentId.
	const OriginCharT*	MultiplayerId;	///< Identifies games that can play together. This id is also used to find a fitting entitlement in Origins entitlement list, if ContentId doesn't match any entitlement in Origins entitlement list.
	const OriginCharT*	Language;		///< The ISO IETF language setting. See <a href="http://online.ea.com/confluence/display/nucleus/EA+Locale">http://online.ea.com/confluence/display/nucleus/EA+Locale</a>. [Not used.]
};

/// \ingroup types
/// \brief A structure returned by \ref OriginStartup that gives information about the OriginSDK.
struct OriginStartupOutputT
{
	const OriginCharT*	Version;		///< Origin version string.
    const OriginCharT*  ContentId;      ///< The content Id associated with the game.
    OriginInstanceT     Instance;       ///< A handle to the Origin Instance.
};

/// \ingroup types
/// \brief Detailed information about the user.
struct OriginProfileT						
{
	OriginUserT			UserId;			///< The identity of the user.
	OriginPersonaT		PersonaId;		///< The identity of the persona.
	const OriginCharT	*PersonaName;	///< The textual persona name.
	const OriginCharT	*AvatarId;		///< The ImageId to the Avatar.
	const OriginCharT	*Country;	    ///< The country code of the user.
    bool                IsUnderAge;     ///< Indicates whether the user is under age.
    const OriginCharT   *GeoCountry;    ///< The country the users IP is in. 
    const OriginCharT   *CommerceCountry;   ///< The country used for commerce transactions.
    const OriginCharT   *CommerceCurrency;  ///< The currency commerce transactions will be in.
};

/// \ingroup types
/// \brief Detailed information about the user's friend.
struct OriginFriendT							
{
	OriginUserT			UserId;			///< The identity of the friend.
	OriginPersonaT		PersonaId;		///< The identity of the persona.
	const OriginCharT	*Persona;		///< The textual persona name.
    const OriginCharT	*AvatarId;		///< The ImageId to the Avatar.
	const OriginCharT	*Group;			///< The chat group this friend belongs to.
	enumPresence		PresenceState;	///< The online state (see \ref enumPresence).
	enumFriendState		FriendState;	///< The friend's state (see \ref ORIGIN_FRIEND_DECLINED etc.).
	const OriginCharT	*TitleId;		///< The title that the user is in.
	const OriginCharT	*Title;			///< The localized title string.
	const OriginCharT   *MultiplayerId;	///< An id to identify games that can play together.
	const OriginCharT	*Presence;		///< The rich presence description string.
	const OriginCharT	*GamePresence;	///< A game-specific presence string.
};
   

/// \ingroup types
/// \brief Presence information.
struct OriginPresenceT
{
   	OriginUserT			UserId;			///< The identity of the user.
   	enumPresence		PresenceState;	///< The online state (see \ref enumPresence).
   	const OriginCharT *	TitleId;		///< The title the user is in.
	const OriginCharT *	MultiplayerId;	///< An id to identify games that can play together.
   	const OriginCharT *	Title;			///< The localized title string.
   	const OriginCharT *	Presence;		///< The rich presence description string.
   	const OriginCharT *	GamePresence;	///< A game-specific presence string.
};


/// \ingroup types
/// \brief Information about a friend's status.
struct OriginIsFriendT
{
	OriginUserT			UserId;			///< The identity of the friend.
	enumFriendState     FriendState;	///< The friend's state (see #ORIGIN_FRIEND_DECLINED etc.).
};

/// \ingroup types
/// \brief A chat message.
struct OriginChatT
{
    const OriginCharT *  ChatId;    ///< The chat id
	OriginUserT          UserId;	///< The sender of the chat message.
	const OriginCharT *  Thread;	///< The thread associated with this message.
	const OriginCharT *  Message;	///< The message.
};

struct OriginChatInfoT
{
    const OriginCharT * ChatId;
    enumChatStates      ChatState;
};

/// \ingroup types
/// \brief Invite information.
struct OriginInviteT							 
{
	char				bInitial;				///< Indicates whether the game is started because of this invite.
	OriginUserT			FromId;					///< The user who sent the invitation.
	const OriginCharT	*SessionInformation;	///< The game session that the user is invited to.
};

/// \ingroup types
/// \brief Invite information.
struct OriginInvitePendingT
{
    OriginUserT			FromId;					///< The user who sent the invitation.
    const OriginCharT   *MultiplayerId;         ///< Indicator for what game the invite is.
};

/// \ingroup types
/// \brief A struct to hold the download status of an item.
struct OriginContentT
{
	const OriginCharT		   *ItemId;		///< The item belonging to this notification.
	float						Progress;	///< When a download is in progress this will indicate the progress of the download [0.0, 1.0].
	enumContentState            State;		///< Simplified state of the content. It can be any of the states defined in enumContentState
};


/// \ingroup types
/// \brief A structure that will be provided when an ORIGIN_EVENT_PROFILE event occurs.
struct OriginProfileChangeT
{
	OriginUserT					UserId;		///< For what user did the profile change.
	enumProfileChangeItem		Changed;	///< Enumeration to indicate what item of the profile has changed.
};


/// \ingroup types
/// \brief A struct to hold the image.
struct OriginImageT
{
	const OriginCharT * ImageId;			///< A string that identifies the image.
	int32_t Width;						    ///< The width of the image
	int32_t Height;						    ///< The height of the image
    const OriginCharT *ResourcePath;		///< The path to the cached resource. \note The resource maybe have a different size or format than specified. The information is only used for image lookup.
};

/// \ingroup types
/// \brief A struct to hold a data entitlement item.
struct OriginItemT
{
    const OriginCharT*	Type;				///< The item type (not displayed).
    const OriginCharT*	ItemId;				///< The item ID (not displayed).
    const OriginCharT*	EntitlementId;		///< The entitlement ID uniquely defines the entitlement in the entitlement cache.
    const OriginCharT*	EntitlementTag;		///< A game-specific string that unlocks the item in the game.
    const OriginCharT*	Group;				///< The group this entitlement belongs to.
    const OriginCharT*	ResourceId;			///< A game-defined resource ID (not displayed).
    OriginTimeT			TerminateDate;		///< The expiration date (zero indicates no expiration).
    OriginTimeT         GrantDate;          ///< The date on which the entitlement was granted. For consumables this will indicate the date of first purchase.
    OriginSizeT			QuantityLeft;		///< The number of plays remaining: a negative number indicates infinite plays (non consumable); a positive value indicates the number of uses remaining.
};

/// \ingroup types
/// \brief A struct that holds a commerce offer.
struct OriginOfferT
{
    const OriginCharT *	Type;				///< The offer type (not displayed).
    const OriginCharT *	OfferId;			///< The offer identifier (not displayed).
    const OriginCharT *	Name;				///< The display name.
    const OriginCharT *	Description;		///< The display description.
    const OriginCharT *	ImageId;			///< The image ID of the Offer.
    char				bIsOwned;			///< Indicates that the item is already owned by the customer.
    char				bIsHidden;			///< Indicates that the item is not to be displayed.
    char				bCanPurchase;		///< Indicates that the item can be purchased.
    OriginTimeT			PurchaseDate;		///< The earliest purchase date (non-zero).
    OriginTimeT			DownloadDate;		///< The earliest download date (non-zero).
    OriginTimeT			PlayableDate;		///< The earliest playable date (non-zero).
    uint64_t			DownloadSize;		///< The size in KB (non-zero).
    const OriginCharT *	Currency;			///< The currency identifier (ISO currency).
    double				Price;				///< The price in Currency units.
    const OriginCharT *	RegularPrice;		///< The normal price (localized string).
    OriginSizeT			QuantityAllow;		///< The maximum sellable inventory (count up).
    OriginSizeT			QuantitySold;		///< The amount of inventory sold (count up).
    OriginSizeT			QuantityAvail;		///< The available inventory (count down).
    OriginSizeT			EntitlementCount;	///< The number of entitlements associated with this offer.
    OriginItemT *  		Entitlements;		///< The entitlements associated with this offer.
};

/// \ingroup types
/// \brief A struct representing the available catalogs in a store.
struct OriginCatalogT
{
    const OriginCharT	*	Name;			///< The name of the catalog.
    const OriginCharT	*	Status;			///< The status of the catalog.
    const OriginCharT	*	CurrencyType;	///< The currency type associated with this Catalog.
    const OriginCharT	*	Group;			///< The group name of this catalog. 

    uint64_t				CatalogId;		///< The Id, associated with this catalog. Use this Id in the OriginSelectStore function.
};

/// \ingroup types
/// \brief A struct representing the store.

struct OriginStoreT
{
    const OriginCharT	*	Name;			///< The name of the store.
    const OriginCharT	*	Title;			///< The store title.
    const OriginCharT	*	Group;			///< The store group this store belongs to.
    const OriginCharT	*	Status;			///< The status of the store.
    const OriginCharT	*	DefaultCurrency;///< The default currency for this store.
    uint64_t				StoreId;		///< The Id of the store. This is the same Id as specified in the OriginSelectStore function
    char					bIsDemoStore;	///< Indicator whether this store is a real store.

    const OriginCatalogT *	pCatalogs;		///< The catalogs in the store.
    OriginSizeT				CatalogCount;	///< The Number of catalogs in the store.
};


/// \ingroup types
/// \brief A struct representing a catalog category for commerce functions.
struct OriginCategoryT
{
    const OriginCharT	*	Type;			///< The category type (not displayed).
    const OriginCharT	*	CategoryId;		///< The category identifier (not displayed).
    const OriginCharT	*	ParentId;		///< The parent category (not displayed).
    const OriginCharT	*	Name;			///< The display name.
    const OriginCharT	*	Description;	///< The display description.
    char					bMostPopular;	///< The featured category.
    const OriginCharT	*	ImageID;		///< The image name.
    OriginSizeT				CategoryCount;	///< The number of sub categories.
    const OriginCategoryT *	pCategories;	///< The subcategories of this category.
    OriginSizeT				OfferCount;		///< The number of offers in this category.
    const OriginOfferT	*	pOffers;		///< The offers in this category.
};

/// \ingroup types
/// \brief A struct representing an achievement.
struct OriginAchievementT
{
    int32_t Id;                             ///< The Achievement id.
    int32_t Progress;                       ///< The progress in reaching the achievement. Limit defined in achievement definition.
    int32_t Total;                          ///< The number of points to score to reach the achievement.
    int32_t Count;                          ///< The number of times this achievement is achieved.
    const OriginCharT *     Name;           ///< The localized name of the achievement
    const OriginCharT *     Description;    ///< The localized description of the achievement
    const OriginCharT *     HowTo;          ///< The localized instructions on how to get this achievement.
    const OriginCharT *     ImageId;        ///< The image Id related to this achievement. Use OriginQueryImage to get a cached version of this image.
    OriginTimeT             GrantDate;      ///< The date when you got this achievement awarded. (Only valid when bAwarded == true)
    OriginTimeT             Expiration;     ///< The date when the achievement will expire. The achievment shouldn't be shown when expired.
};


struct OriginAchievementSetT
{
    const OriginCharT *     Name;               ///< The name of the achievement set.
    const OriginCharT *     GameName;           ///< The localized game name where these achievements belong to.

    OriginAchievementT *    pAchievements;      ///< The list of achievements.
    OriginSizeT             AchievementCount;   ///< The number of achievements in the list.
};


struct OriginChunkStatusT
{
    const OriginCharT *     ItemId;		    ///< The item belonging to this notification. This is the offerId of the game for full game installation, and the DLC offerId for DLC progressive installation.
	const OriginCharT *		Name;			///< The name associated with this chunk.
    int32_t                 ChunkId;        ///< The chunk for which this update is.
	enumChunkType			Type;			///< The type of the chunk.
    enumChunkState          State;          ///< The state the chunk is currently in.
    float					Progress;	    ///< When a download is in progress this will indicate the progress of the download [0.0, 1.0].
	uint64_t				Size;			///< The byte size of the chunk. The game game can use this information to calculate the progress of multiple chunks at a time. SUM(p1*s1 + p2*s2 + p3*s3 + ...)/SUM(s1 + s2 + s3 + ...)
	int						ChunkETA;		///< The estimated time it takes to download the chunk in milliseconds.
	int						TotalETA;		///< The estimated time it takes to download the whole content including this chunk in milliseconds.
};

struct OriginIsProgressiveInstallationAvailableT
{
	const OriginCharT *     ItemId;		    ///< The item belonging to this notification. This is the offerId of the game for full game installation, and the DLC offerId for DLC progressive installation.
	bool                    bAvailable;     ///< Is Progressive installation support available.
};

struct OriginAreChunksInstalledInfoT
{
    const OriginCharT *     ItemId;		    ///< The item belonging to this notification. This is the offerId of the game for full game installation, and the DLC offerId for DLC progressive installation.
    int32_t *               ChunkIds;       ///< An array of chunk Ids
    OriginSizeT             ChunkIdsCount;  ///< The number of elements in the chunk array.
    bool                    bInstalled;     ///< Are the chunks installed.
};

struct OriginChunkPriorityInfoT
{
	const OriginCharT *     ItemId;		    ///< The item belonging to this notification. This is the offerId of the game for full game installation, and the DLC offerId for DLC progressive installation.
	int32_t *               ChunkIds;       ///< An array of chunk Ids
	OriginSizeT             ChunkIdsCount;  ///< The number of elements in the chunk array.
};

struct OriginDownloadedInfoT 
{
    const OriginCharT *     ItemId;		    ///< The item belonging to this notification. This is the offerId of the game for full game installation, and the DLC offerId for DLC progressive installation.
    const OriginCharT *     FilePath;       ///< The path for which you asked whether it was downloaded.
    bool                    bDownloaded;    ///< Indicator on whether the file is downloaded.
};

struct OriginCreateChunkInfoT
{
	const OriginCharT *		ItemId;		    ///< The item belonging to this notification. This is the offerId of the game for full game installation, and the DLC offerId for DLC progressive installation.
	uint32_t				ChunkId;		///< The Assigned Chunk Id.
};


/// \ingroup types
/// \brief The asynchronous callback signature for the \ref OriginCatalogT record.
/// 
/// A callback will receive a pointer to the root list of catalogs in the store.
/// \param pContext The user-provided context to the callback system. This context can contain anything.
/// Most of the time it will be a pointer to an application's handler class.
/// \param ppCatalogs The returned store root catalogs.
/// \param count The number of catalogs in the root folder.
/// \param error An error code indicating whether the transaction completed successfully.
typedef OriginErrorT (*OriginStoreCallback) (void * pContext, OriginStoreT * pStore, OriginSizeT size, OriginErrorT error);

/// \ingroup types
/// \brief The asynchronous callback signature for the \ref OriginCategoryT record.
/// 
/// A callback will receive the pointer to the root list of categories of the catalog set in SelectStore.
/// \param pContext The user-provided context to the callback system. This context can contain anything.
/// Most of the time it will be a pointer to an application's handler class.
/// \param ppCategories The returned catalog root categories.
/// \param count The number of categories in the root folder.
/// \param error An errorcode indicating whether the transaction completed successfully.
typedef OriginErrorT (*OriginCatalogCallback) (void * pContext, OriginCategoryT ** ppCategories, OriginSizeT numberOfCategories, OriginErrorT error);

/// \ingroup types
/// \brief The asynchronous callback signature for the \ref OriginItemT record.
///
/// A callback will receive the link to the root list of categories of the catalog.
/// \param pContext The user-provided context to the callback system. This context can contain anything.
/// Most of the time it will be a pointer to an application's handler class.
/// \param item The returned catalog root categories.
/// \param count The number of categories in the root folder.
/// \param error An error code indicating whether the transaction completed successfully.
typedef OriginErrorT (*OriginEntitlementCallback) (void * pContext, OriginItemT * item, OriginSizeT count, OriginErrorT error);

/// \ingroup types
/// \brief The asynchronous callback signature for the \ref OriginAchievementT.
///
/// A callback will receive a list of achievement sets requested.
/// \param pContext The user-provided context to the callback system. This context can contain anything.
/// Most of the time it will be a pointer to an application's handler class.
/// \param pAchievement The achievement granted.
/// \param size The size of the AchievementT structure.
/// \param error An error code indicating whether the transaction completed successfully.
typedef OriginErrorT (*OriginAchievementCallback) (void * pContext, OriginAchievementT * pAchievement, OriginSizeT size, OriginErrorT error);

/// \ingroup types
/// \brief The asynchronous callback signature for the \ref OriginAchievementSetT list.
///
/// A callback will receive a list of achievement sets requested.
/// \param pContext The user-provided context to the callback system. This context can contain anything.
/// Most of the time it will be a pointer to an application's handler class.
/// \param ppAchievementSets The list of achievement sets returned.
/// \param numberOfAchievementSets The number of achievement sets returned.
/// \param error An error code indicating whether the transaction completed successfully.
typedef OriginErrorT (*OriginAchievementSetCallback) (void * pContext, OriginAchievementSetT ** ppAchievementSets, OriginSizeT numberOfAchievementSets, OriginErrorT error);

/// \ingroup types
/// \brief The asynchronous callback signature for the OriginTicketT record.
///
/// A function with this signature can be used to receive ticket information. 
/// \param pContext The user-provided context to the callback system. This context can contain anything.
/// Most of the time it will be a pointer to an application's handler class.
/// \param balance The returned balance.
/// \param size This parameter can be ignored.
/// \param error An errorcode indicating whether the transaction completed successfully.
typedef OriginErrorT (*OriginWalletCallback) (void * pContext, int64_t * balance, OriginSizeT size, OriginErrorT error);

/// \ingroup types
/// \brief The asynchronous callback signature for the OriginTicketT record.
///
/// A function with this signature can be used to receive ticket information. 
/// \param pContext The user-provided context to the callback system. This context can contain anything.
/// Most of the time it will be a pointer to an application's handler class.
/// \param ticket Receives an \ref OriginHandleT handle when the resource is received.
/// \param length Receives the size of the resource.
/// \param error An errorcode indicating whether the transaction completed successfully.
typedef OriginErrorT (*OriginTicketCallback) (void * pContext, const char ** ticket, OriginSizeT length, OriginErrorT error);

/// \ingroup types
/// \brief The asynchronous callback signature for the AuthCode string.
///
/// A function with this signature can be used to receive authcode information. 
/// \param pContext The user-provided context to the callback system. This context can contain anything.
/// Most of the time it will be a pointer to an application's handler class.
/// \param authcode Receives an \ref OriginHandleT handle when the resource is received.
/// \param length Receives the size of the resource.
/// \param error An errorcode indicating whether the transaction completed successfully.
typedef OriginErrorT (*OriginAuthCodeCallback) (void * pContext, const char ** authcode, OriginSizeT length, OriginErrorT error);

/// \ingroup types
/// \brief The asynchronous callback signature for the OriginResourceT record.
///
/// A function with this signature can be used to receive ticket information. 
/// \param pContext The user-provided context to the callback system. This context can contain anything.
/// Most of the time it will be a pointer to an application's handler class.
/// \param resource Receives an \ref OriginHandleT handle when the resource is received.
/// \param length Receives the size of the resource.
/// \param error An errorcode indicating whether the transaction completed successfully.
typedef OriginErrorT (*OriginResourceCallback) (void * pContext, void * resource, OriginSizeT length, OriginErrorT error);

/// \ingroup types
/// \brief The asynchronous callback signature for incoming Origin events.
///
/// A function with this signature can be used to receive event information.
/// \param eventId One (and only one, not a bitwise combination) of the ORIGIN_EVENT_* values (see \ref ORIGIN_EVENT_INVITE etc.).
/// \param pContext The user-provided context to the callback system.  This context can contain anything.
/// Most of the time it will be a pointer to an application's handler class.
/// \param eventData The data associated with the event. The type depends on the eventId.
/// \param count The number of items of eventData in this callback.
typedef OriginErrorT (*OriginEventCallback) (int32_t eventId, void * pContext, void* eventData, OriginSizeT count);

/// \ingroup types
/// \brief The asynchronous callback signature for callbacks that are used to signal success or failure.
///
/// This is the signature of a function that will be called when the OriginXXX function completes. It doesn't return any data, only an error status.
/// \param pContext The user-provided context to the callback system. This context can contain anything. Most of the time it will be a pointer to an application's handler class.
/// \param pDummy This will be NULL.
/// \param dummyCount This will receive 0
/// \param error An errorcode indicating whether the transaction completed successfully.
typedef OriginErrorT (*OriginErrorSuccessCallback) (void * pContext, void * pDummy, OriginSizeT dummyCount, OriginErrorT error);

/// \ingroup types
/// \brief The asynchronous callback signature for reporting errors encountered by the Origin SDK.
///
/// A function with this signature can be registered to receive error reports with precise debug information directly
/// during Origin code execution.
///
/// This callback may be called at any moment, either from Origin functions called by the user or from private Origin
/// threads. The user is reponsible for synchronization.
/// \param pcontext The user-provided context to the error callback.  This context can contain anything, it is only used to pass back to the user callback.
/// \param error_code The Origin error code being reported.  This code will also generally be returned from the function that was executing.
/// \param message_text Some errors are reported with additional message text.
/// \param file The Origin source file that is reporting the error.
/// \param line The line of the Origin source file that is reporting the error.
typedef void (*OriginErrorCallback) (void * pcontext, OriginErrorT error_code, const char* message_text, const char* file, int line);


typedef OriginErrorT (*OriginIsProgressiveInstallationAvailableCallback) (void * pContext, OriginIsProgressiveInstallationAvailableT * availableInfo, OriginSizeT dummy, OriginErrorT error);

/// \ingroup types
/// \brief The signature for the OriginAreChunksInstalled callback.
/// \param pContext The user-provided context to the callback system. This context can contain anything. Most of the time it will be a pointer to an application's handler class.
/// \param bInstalled A pointer to a boolean that contains the result of the are chunk installed query.
/// \param count this value can be ignored, it should contain 1 when the request successfully succeeds.
/// \param error An error returned to the callback when the query couldn't complete. In the case error != ORIGIN_SUCCESS the bInstalled value is not valid.
typedef OriginErrorT (*OriginAreChunksInstalledCallback) (void * pContext, OriginAreChunksInstalledInfoT * installInfo, OriginSizeT count, OriginErrorT error);

/// \ingroup types
/// \brief The signature for the OriginAreChunksInstalled callback.
/// \param pContext The user-provided context to the callback system. This context can contain anything. Most of the time it will be a pointer to an application's handler class.
/// \param bInstalled A pointer to a boolean that contains the result of the are chunk installed query.
/// \param count this value can be ignored, it should contain 1 when the request successfully succeeds.
/// \param error An error returned to the callback when the query couldn't complete. In the case error != ORIGIN_SUCCESS the bInstalled value is not valid.
typedef OriginErrorT (*OriginChunkStatusCallback) (void * pContext, OriginChunkStatusT * status, OriginSizeT count, OriginErrorT error);

/// \ingroup types
/// \brief The signature for the OriginIsFileDownloaded callback.
/// \param pContext The user-provided context to the callback system. This context can contain anything. Most of the time it will be a pointer to an application's handler class.
/// \param bInstalled A pointer to a boolean that contains the result of the are chunk installed query.
/// \param count this value can be ignored, it should contain 1 when the request successfully succeeds.
/// \param error An error returned to the callback when the query couldn't complete. In the case error != ORIGIN_SUCCESS the bInstalled value is not valid.
typedef OriginErrorT (*OriginIsFileDownloadedCallback) (void * pContext, struct OriginDownloadedInfoT * downloadInfo, OriginSizeT count, OriginErrorT error);

/// \ingroup types
/// \brief The signature for the OriginGetChunkPriority callback.
/// \param pContext The user-provided context to the callback system. This context can contain anything. Most of the time it will be a pointer to an application's handler class.
/// \param chunkIds An array of chunkIds.
/// \param count The number of chunks in the array of chunks.
/// \param error An error returned to the callback when the query couldn't complete. In the case error != ORIGIN_SUCCESS the chunkIds value is not valid.
typedef OriginErrorT (*OriginChunkPriorityCallback) (void * pContext, const OriginChunkPriorityInfoT * chunkIdInfo, OriginSizeT count, OriginErrorT error);

/// \ingroup types
/// \brief The signature for the OriginGetChunkPriority callback.
/// \param pContext The user-provided context to the callback system. This context can contain anything. Most of the time it will be a pointer to an application's handler class.
/// \param chunkId The assigned chunkId.
/// \param count a dummy parameter (always 1).
/// \param error An error returned to the callback when the query couldn't complete. In the case error != ORIGIN_SUCCESS the chunkIds value is not valid.
typedef OriginErrorT (*OriginCreateChunkCallback) (void * pContext, const int * chunkId, OriginSizeT count, OriginErrorT error);

/// \ingroup types
/// \brief The callback signature for getting debug information from the SDK.
///
/// \param pContext This will receive a user-provided context.
/// \param pText The debug text.
typedef void (*PrintfHook)(void *pContext, const OriginCharT *pText);


#pragma pack(pop)

#endif //__ORIGIN_TYPES_H__
