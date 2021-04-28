/*jshint strict: false */
/*jshint unused: false*/
define([
    'promise',
    'core/logger',
    'core/utils',
    'core/user',
    'core/urls',
    'core/dataManager',
    'core/errorhandler',
    'core/events',
    'core/locale',
    'modules/games/subscription',
    'core/anonymoustoken',
    'modules/client/client'
], function(Promise, logger, utils, user, urls, dataManager, errorhandler, events, configService, subscription, anonymoustoken, client) {
    /**
     * @module module:games
     * @memberof module:Origin
     */
    var defaultLMD = 'Sat, 01 Jan 2000 00:00:00 GMT',
        entReqNum = 0, //to keep track of the entitlement requests
        lastEntitlementRetrievedDate; //temp for now until we get the trusted clock

    /**
     * Returns the cached information from Local Storage for given productInfo
     * @param    productId
     * @return   offerInfo object = r:[LMD], dirty:true/false, o:[LMD]
     *           if it doesn't exist in local storage, returns undefined object
     */

    function getCacheInfo(productId) {
        var offerInfoLSstr = null,
            offerInfo;

        try {
            offerInfoLSstr = localStorage.getItem('lmd_' + productId);
        } catch (error) {
            logger.error('getCacheInfo: cannot get item from local storage - ', error.message);
        }

        if (offerInfoLSstr !== null) {
            //logger.log('getCacheInfo:', productId, ':', offerInfoLSstr);
            offerInfo = JSON.parse(offerInfoLSstr);
        } else {
            //logger.log('no localstorage for:', productId);
        }
        return offerInfo;
    }
    /**
     * description
     */

    function setCacheInfo(productId, offerInfo) {
        var jsonS = JSON.stringify(offerInfo);
        try {
            localStorage.setItem('lmd_' + productId, jsonS);
        } catch (error) {
            logger.error('setCacheInfo: unable to write to local storage', error.message);
        }
        //logger.log('setCacheInfo:', productId, ':', jsonS, 'offerInfo:', offerInfo.r, ',', offerInfo.o);
    }

    function parseLMD(response) {
        var returnObj = Date.parse(defaultLMD);
        //realize it's a bit odd to handle two types of responses here
        //alternative was to take the cached data and put it back in the same format as the network response
        //but netowrk response has the LMD as a datestring, and it seemed silly to convert the numeric value back to string to mimic the response
        //only to convert it back to the numeric value in this function.
        if (!isNaN(Number(response))) {
            //was just passed in a cached value, so just use that
            returnObj = response;
        } else if (utils.isChainDefined(response, ['offer', 0, 'updatedDate'])) {
            returnObj = Date.parse(response.offer[0].updatedDate);
        }

        return returnObj;
    }

    /**
     * This will return a promise for the value to use as the cache parameter for the catalog offer request
     *
     * @param    productId
     * @return   a promise
     *
     */
    function catalogCacheParameter(productId, updatedLMD) {
        //retrieve our cache-buster value
        var offerInfo = getCacheInfo(productId),
            promise = null;

        //if we're getting passed in an lmdvalue, then we received it via dirtybits
        if (updatedLMD) {
            promise = Promise.resolve(updatedLMD);
        } else {
            //not requested previously or isDirty = true means we need to go retrieve the Last-Modified-Date (LMD)
            if ((typeof offerInfo === 'undefined') || offerInfo.dirty === true) {
                //make request to ML to retrieve LMD
                var endPoint = urls.endPoints.catalogInfoLMD,
                    config = {
                        atype: 'GET',
                        headers: [{
                            'label': 'Accept',
                            'val': 'application/json; x-cache/force-write'
                        }],
                        parameters: [{
                            'label': 'productId',
                            'val': productId
                        }],
                        appendparams: [],
                        reqauth: false,
                        requser: false
                    };
                //just use 1 as outstanding since this is a no-cache request
                promise = dataManager.enQueue(endPoint, config, 1 /* outstanding */ );
            } else {
                //here if outstanding was already updated (e.g. when base entitlement was retrieved)
                promise = Promise.resolve(offerInfo.o);
            }
        }

        return promise;
    }

    function updateOfferCacheInfo(productId) {
        return function(lmdValue) {
            var offerCacheInfo = getCacheInfo(productId);
            if (!offerCacheInfo) {
                offerCacheInfo = {};
                offerCacheInfo.r = 0;
                offerCacheInfo.dirty = true;
            }

            offerCacheInfo.o = lmdValue;
            setCacheInfo(productId, offerCacheInfo);

            return lmdValue;
        };
    }

    function markCacheNotDirty(productId) {
        return function(response) {
            var offerCacheInfo = getCacheInfo(productId);
            offerCacheInfo.r = offerCacheInfo.o;
            offerCacheInfo.dirty = false;
            setCacheInfo(productId, offerCacheInfo);

            return response;

        };
    }

    function retrievePrivateCatalogInfo(lmdValue, productId, locale, parentOfferId, forcePrivate) {
        var token = user.publicObjs.accessToken();
        //if offline, token would be empty but we still want to retrieve it from the cache
        if (token.length > 0 || !client.onlineStatus.isOnline()) {
            var endPoint = urls.endPoints.catalogInfoPrivate,

                setLocale = locale || 'DEFAULT',
                country2letter = configService.countryCode(),
                encodedProductId = encodeURIComponent(productId),
                config = {
                    atype: 'GET',
                    headers: [{
                        'label': 'Accept',
                        'val': 'application/json; x-cache/force-write'
                    }],
                    parameters: [{
                        'label': 'productId',
                        'val': encodedProductId
                    }, {
                        'label': 'locale',
                        'val': setLocale
                    }],
                    appendparams: [{
                        'label': 'country',
                        'val' : country2letter
                    }, {
                        'label': 'lmd',
                        'val': lmdValue
                    }],
                    reqauth: true,
                    requser: false
                };

            if (forcePrivate) {
                //a temporary hack to get around the issue that EC2 isn't allowing /public endpoints
                //we'll get back a 401 if we don't own the offer but we don't want to re-initiate
                //a login in the case.  so just allow it to fail
                config.dontRelogin = true;
            }

            //want this to be suppressed when offline to prevent OPTIONS call
            dataManager.addHeader(config, 'AuthToken', token);

            //if parentId passed in, then this would be a request for extra content catalog
            if (parentOfferId) {
                config.appendparams.push({
                    'label': 'parentId',
                    'val': parentOfferId
                });
            }

            //error is caught in catalogInfo
            return dataManager.enQueue(endPoint, config, lmdValue);

        } else {
            //if token doesn't exist then don't bother making the request because it will trigger
            //a loop of trying to log in
            return errorhandler.promiseReject('private catalog url missing token:' + productId);
        }

    }

    function retrieveCatalogInfo(productId, locale, usePrivateUrl, parentOfferId) {
        return function(lmdValue) {
            var forcePrivate = false,
                promise = null,
                country2letter = configService.countryCode(),
                setLocale;

            //may need to remap locale to one that is recognized by EADP
            if (locale) {
                setLocale = configService.eadpLocale(locale, country2letter);
            } else {
                setLocale = 'DEFAULT';
            }

            if (usePrivateUrl || forcePrivate) {
                promise = retrievePrivateCatalogInfo(lmdValue, productId, setLocale, parentOfferId);
            } else {
                var endPoint = urls.endPoints.catalogInfo,
                    encodedProductId = encodeURIComponent(productId),
                    config = {
                        atype: 'GET',
                        headers: [{
                            'label': 'Accept',
                            'val': 'application/json; x-cache/force-write'
                        }],
                        parameters: [{
                            'label': 'productId',
                            'val': encodedProductId
                        }, {
                            'label': 'locale',
                            'val': setLocale
                        }],
                        appendparams: [{
                            'label': 'country',
                            'val': country2letter
                        },{
                            'label': 'lmd',
                            'val': lmdValue
                        }],
                        reqauth: false,
                        requser: false
                    };

                //error is caught in cataloginfo
                promise = dataManager.enQueue(endPoint, config, lmdValue);
            }
            return promise;
        };

    }

    function updateOfferLMDInfo(offerId, newLMD) {
        var offerInfo = getCacheInfo(offerId);
        if (typeof offerInfo === 'undefined') {
            offerInfo = {};
            offerInfo.r = 0;
        }
        offerInfo.o = newLMD;
        offerInfo.dirty = false;
        setCacheInfo(offerId, offerInfo);
    }


    function filteredLogAndCleanup(msg) {
        return function(error) {
            //to prevent spamming we of the console log we don't want to log the following failure causes for catalog info retrieval. The occur due to the way we retrieve public/private offers for LARS3 and are a part of the normal work flow
            if (utils.isChainDefined(error, ['response', 'failure', 'cause']) && (error.response.failure.cause === 'UNKNOWN_STOREGROUP' || error.response.failure.cause === 'UNKNOWN_OFFER' || error.response.failure.cause === 'OFFER_NOT_OWNED')) {
                return Promise.reject(error);
            } else {
                return errorhandler.logAndCleanup(msg)(error);
            }
        };
    }


    /**
     * This will return a promise for catalog info
     *
     * @param {string} productId The product id of the offer.
     * @param {string} locale locale of the offer
     * @return {promise<catalogInfo>}
     */
    function catalogInfo(productId, locale, usePrivateUrl, parentOfferId, updatedLMD) {
        return catalogCacheParameter(productId, updatedLMD)
            .then(parseLMD, parseLMD)
            .then(updateOfferCacheInfo(productId))
            .then(retrieveCatalogInfo(productId, locale, usePrivateUrl, parentOfferId))
            .then(markCacheNotDirty(productId), filteredLogAndCleanup('GAMES:catalogInfo FAILED'));

    }

    function updateLastEntitlementRetrievedDateFromHeader(headers) {
        //date should eventually come server
        lastEntitlementRetrievedDate = new Date();
        lastEntitlementRetrievedDate = lastEntitlementRetrievedDate.toISOString();
        lastEntitlementRetrievedDate = lastEntitlementRetrievedDate.substring(0, lastEntitlementRetrievedDate.lastIndexOf(':')) + 'Z';
    }

    function handleConsolidatedEntitlementsResponse(responseAndHeaders) {
        var response = responseAndHeaders.data,
            headers = responseAndHeaders.headers;

        updateLastEntitlementRetrievedDateFromHeader(headers);

        //need to update the offer LMD for each offer in the entitlement
        var len = response.entitlements.length;
        for (var i = 0; i < len; i++) {
            var lmdValue = Date.parse(response.entitlements[i].updatedDate); //convert to numeric value
            updateOfferLMDInfo(response.entitlements[i].offerId, lmdValue);
        }
        return response.entitlements;
    }

    function onDirtyBitsCatalogUpdate(dirtyBitData) {
        var offerUpdateTimes = dirtyBitData.offerUpdateTimes;
        //update the lmd
        for (var p in offerUpdateTimes) {
            if (offerUpdateTimes.hasOwnProperty(p)) {
                updateOfferLMDInfo(p, offerUpdateTimes[p]);
            }
        }
    }

    function handleBaseGameOfferIdByMasterTitleIdResponse(response) {
        if (response && response.offer) {
            return response.offer;
        }
        return [];
    }

    function djb2Code(str) {
        var chr,
            hash = 5381;

        for (var i = 0; i < str.length; i++) {
            chr = str.charCodeAt(i);
            hash = hash * 33 + chr;
        }
        return hash;
    }



    /**
     * Fetch the auth token for a user. Either and Anonymous token or a proper token
     * @return {string} The required token.
     */
    // TODO Remove Promise here.
    function getToken() {
        return user.publicObjs.accessToken();
    }
    /**
     * Create the config object used by the dataManager
     * @param  {string} offerId The offerId
     * @param  {token} token   A uniques identifier (the users Auth Token)
     * @param  {string} currency a currrency code, e.g. 'USD', 'CAD'
     * @param  {string} atype TBD
     * @return {objet}         The config object used by the dataManager
     */
    function createRatingsConfig(offerId, token, currency, atype) {
        atype = atype || 'GET';
        var config = {atype: atype},
            eadpLocale;

        if (token) {
            config.reqauth = true;
            dataManager.addHeader(config, 'AuthToken', token);
        }

        dataManager.addHeader(config, 'Accept', 'application/json');
        dataManager.addHeader(config, 'Content-Type', 'application/json');

        // query params
        dataManager.appendParameter(config, 'country', configService.countryCode());

        //may need to remap to eadpLocale
        eadpLocale = configService.eadpLocale(configService.locale(), configService.countryCode());
        dataManager.appendParameter(config, 'locale', eadpLocale);

        dataManager.appendParameter(config, 'pid', user.publicObjs.userPid());
        dataManager.appendParameter(config, 'currency', currency || configService.currencyCode());
        dataManager.appendParameter(config, 'offerIds', offerId);

        return config;
    }

    /**
     * Get the price from the ratings service
     * @param  {string} offerId The offerId which price to fetch
     * @param  {string} currency a currrency code, e.g. 'USD', 'CAD'
     * @return {Object}         The personalized price data for that offer.
     */
    function fetchPrice(offerId, currency) {
        var token = getToken();
        var config = createRatingsConfig(offerId, token, currency);
        var endPoint = token ? urls.endPoints.ratingsOffers : urls.endPoints.anonRatingsOffers;

        return dataManager.enQueue(endPoint, config, entReqNum)
            .then(function(result) {
                return result.offer;
            });
    }

    function getPricingFormatter() {
        var endPoint = urls.endPoints.currencyFormatter;
        var config = {
            atype: 'GET',
            headers: [{
                'label': 'Accept',
                'val': 'application/json; x-cache/force-write'
            }],
            parameters: [],
            appendparams: [],
            reqauth: false,
            requser: false,
            responseHeader: true
        };
        return dataManager.enQueue(endPoint, config, entReqNum)
            .then(function(response) {
                return response.data;
            });
    }

    function getPricingList(offerIdList, currency) {
        var pricePromises = [];
        pricePromises.push(fetchPrice(offerIdList, currency));
        return Promise.all(pricePromises);
    }

    function consolidatedEntitlements(forceRetrieve) {
        var endPoint = urls.endPoints.consolidatedEntitlements;
        var config = {
            atype: 'GET',
            headers: [{
                'label': 'Accept',
                'val': 'application/vnd.origin.v3+json; x-cache/force-write'
            }],
            parameters: [{
                'label': 'userId',
                'val': user.publicObjs.userPid()
            }],
            appendparams: [],
            reqauth: true,
            requser: true,
            responseHeader: true
        };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'AuthToken', token);
        }

        if (typeof forceRetrieve !== 'undefined' && forceRetrieve === true) {
            entReqNum++; //update the request #
        }

        //                var promise = dataManager.dataRESTauth(baseUrl, config);

        return dataManager.enQueue(endPoint, config, entReqNum)
            .then(handleConsolidatedEntitlementsResponse, errorhandler.logAndCleanup('GAMES:consolidatedEntitlements FAILED'));
    }

    events.on(events.DIRTYBITS_CATALOG, onDirtyBitsCatalogUpdate);

    return /** @lends module:Origin.module:games */ {

        /**
         * @typedef platformObject
         * @type {object}
         * @property {string} platform
         * @property {string} multiPlayerId
         * @property {string} downloadPackageType
         * @property {Date} releaseDate
         * @property {Date} downloadStartDate
         * @property {Date} useEndDate
         * @property {string} executePathOverride - null unless it's a url (web game)
         * @property {string} achievementSet
         */

        /**
         * not sure how to represent this, but basically, the object looks like this:
         * {
         *   PCWIN: {platformObject},
         *   MAC: {platformObject}
         * }
         * but each one is optional, i.e. the object could be empty, or it could just have PCWIN or just MAC or it could have both
         *
         * @typedef platformsObject
         * @type {object}
         */

        /**
         * @typedef countriesObject
         * @type {object}
         * @property {booelan} isPurchasable
         * @property {string} inStock
         * @property {string} catalogPrice
         * @property {string} countryCurrency
         * @property {string[]} catalogPriceA - array of prices
         * @property {string[]} countryCurrencyA - array of currencies
         * @property {boolean} hasSubscriberDiscount
         */

        /**
         * @typedef i18nObject
         * @type {object}
         * @property {string} franchiseFacetKey
         * @property {string} systemRequirements
         * @property {string} platformFacetKey
         * @property {string} longDescription
         * @property {string} officialSiteURL
         * @property {string} publisherFacetKey
         * @property {string} developerFacetKey
         * @property {string} shortDescription
         * @property {string} onlineDisclaimer
         * @property {string} eulaURL
         * @property {string} gameForumURL
         * @property {string} gameTypeFacetKey
         * @property {string} numgerofPlayersFacetKey
         * @property {string} genreFacetKey
         * @property {string} franchisePageLink
         * @property {string} brand
         * @property {string} displayName
         * @property {string} preAnnouncementDisplayDate
         * @property {string} ratingSystemIcon - url for icon image
         * @property {string} packArtSmall - url for packArt
         * @property {string} packArtMedium - url for packArt
         * @property {string} packArtLarge - url for packArt
         * @property {string} gameManualURL
         */

        /**
         * @typedef vaultObject
         * @type {object}
         * @property {string} offerId
         * @property {boolean} isUpgradeable
         */

        /**
         * @typedef catalogInfo
         * @type {object}
         * @property {string} offerId
         * @property {string} offerType
         * @property {string[]} extraContent - a list of extra content offerIds
         * @property {boolean} isDownloadable
         * @property {string} gameDistributionSubType
         * @property {string} trialLaunchDuration
         * @property {string} masterTitleId
         * @property {string[]} alternateMasterTitleIds - array of alternate masterTitleIds
         * @property {string[]} suppressedOfferIds - array of suppressedOffers
         * @property {string} originDisplayType
         * @property {platformsObject} platforms
         * @property {string} ratingSystemIcon - url for icon
         * @property {string} gameRatingUrl
         * @property {boolean} gameRatingPendingMature
         * @property {string} gameRatingDescriptionLong
         * @property {string} gameRatingTypeValue
         * @property {string[]} gameRatingDesc - array of descriptions
         * @property {string} franchiseFacetKey
         * @property {string} mdmItemType
         * @property {string} platformFacetKey
         * @property {string} publisherFacetKey
         * @property {string} imageServer - domain portion of packArt Url
         * @property {string} developerFacetKey
         * @property {string} revenueModel
         * @property {string[]} softwareLocales - array of locales
         * @property {string} gameTypeFacetKey
         * @property {string} masterTitle
         * @property {string} gameEditionTypeFacetKey
         * @property {string} numberofPlayersFacetKey
         * @property {string} genreFacetKey
         * @property {string} itemName
         * @property {string} itemType
         * @property {string} itemId
         * @property {string} rbuCode
         * @property {string} storeGroupId
         * @property {boolean} dynamicPricing
         * @property {countriesObject} countries
         * @property {i18nObject} i18n
         * @property {vaultObject} vault
         * @property {string[]} includeOffers
         * @property {string} contentId
         * @property {string} gameNameFacetKey
         * @property {string} gameEditionTypeFacetKeyRankDesc
         * @property {string} offerPath
         */

        /**
         * This will return a promise for catalog info
         *
         * @param {string} productId The product id of the offer.
         * @param {string} locale locale of the offer
         * @return {promise<module:Origin.module:games~catalogInfo>}
         * @method
         */
        catalogInfo: catalogInfo,

        /**
         * This will return a promise for the private catalog info
         *
         * @param {string} productId The product id of the offer.
         * @param {string} locale locale of the offer
         * @return {promise<module:Origin.module:games~catalogInfo>}
         */
        catalogInfoPrivate: function(productId, locale, parentOfferId) {
            return catalogInfo(productId, locale, true /*usePrivateUrl*/ , parentOfferId);
        },

        /**
         * @typedef entitlementObject
         * @type {object}
         * @property {string} entitlementId TBD
         * @property {string} offerId TBD
         * @property {string} entitlementTag TBD
         * @property {date} grantDate TBD
         * @property {string} status TBD
         * @property {number} useCount TBD
         * @property {string} entitlementType TBD
         * @property {string} originPermissions TBD
         * @property {string} cdKey TBD
         * @property {date} updatedDate TBD
         * @property {string} entitlementSource TBD
         * @property {string} productCatalog TBD
         * @property {boolean} isConsumable TBD
         * @property {number} version TBD
         * @property {string} offerAccess TBD
         * @property {string} offerType TBD
         * @property {string[]} suppressedOfferIds TBD
         * @property {string[]} suppressedBy TBD
         * @property {string[]} masterTitleIds TBD
         */
        /**
         * This will return a promise for the user's Entitlements
         *
         * @param {boolean} forceRetrieve If true grab latest info from server.
         * @return {promise<module:Origin.module:games~entitlementObject[]>}
         *
         */
         consolidatedEntitlements: function(forceRetrieve) {
            if(client.isEmbeddedBrowser()) {
                return client.games.retrieveConsolidatedEntitlements(urls.endPoints.consolidatedEntitlements, forceRetrieve)
                       .then(handleConsolidatedEntitlementsResponse, errorhandler.logAndCleanup('GAMES:consolidatedEntitlements FAILED'));
            } else {
                return consolidatedEntitlements(forceRetrieve);
            }
        },

        /**
         * This is the direct entitlement response object.
         *
         * @typedef directEntitlementObject
         * @type {object}
         * @property {boolean} success success state of entitlement request
         * @property {string} message optional message explaining success
         * @property {object} extra optional extra data explaining error
         */

        /**
         * Direct entitlement of free product
         *
         * @param {String} offerId Offer ID
         * @return {promise<module:Origin.module:games~directEntitlementObject>}
         * @method
         */
        directEntitle: function(offerId) {
            var authToken = user.publicObjs.accessToken(),
                eadpLocale;

            if (!authToken) {
                return new Promise(function(resolve){
                    resolve({success:false, message: 'requires-auth'});
                });
            }

            var endPoint = urls.endPoints.directEntitle;
            var config = {atype: 'POST', reqauth: true, requser: true};
            dataManager.addHeader(config, 'authToken', authToken);
            dataManager.addAuthHint(config, 'authToken', '{token}');
            dataManager.addHeader(config, 'Accept', 'application/json');
            dataManager.addParameter(config, 'offerId', offerId);
            dataManager.addParameter(config, 'userId', user.publicObjs.userPid());

            //may need to remap to eadpLocale
            eadpLocale = configService.eadpLocale(configService.locale(), configService.countryCode());
            dataManager.appendParameter(config, 'locale', eadpLocale);

            dataManager.appendParameter(config, 'cartName', 'store-cart-direct');

            return dataManager.enQueue(endPoint, config, entReqNum)
                .then(function(){
                    // trigger dirtybits entitlement update event
                    events.fire(events.DIRTYBITS_ENTITLEMENT);
                    return {success: true};
                }).catch(function(data) {
                    var failure = (data.response && data.response.error && data.response.error.failure) || {};

                    if (failure.cause === 'ALREADY_OWN') {
                        return {success: true, message: 'already-own'};
                    } else {
                        return {success: false, message: 'exception', extra: failure};
                    }
                });
        },

        /**
         * This will return a promise of a list of catalog info
         *
         * @return {promise<module:Origin.module:games~catalogInfoObject[]>}
         */
        getCriticalCatalogInfo: function(locale) {
            var setLocale,
                country2letter = configService.countryCode(),
                endPoint = urls.endPoints.criticalCatalogInfo,
                config = {
                    atype: 'GET',
                    headers: [{
                        'label': 'Accept',
                        'val': 'application/json'
                    }],
                    parameters: [{
                        'label': 'country2letter',
                        'val': country2letter
                    }],
                    appendparams: [],
                    reqauth: false,
                    requser: false
                };

            //may need to remap locale to one that is recognized by EADP
            if (locale) {
                setLocale = configService.eadpLocale(locale, country2letter);
            } else {
                setLocale = 'DEFAULT';
            }
            dataManager.addParameter(config, 'locale', setLocale);

            return dataManager.enQueue(endPoint, config, 0).catch(errorhandler.logAndCleanup('GAMES:getCriticalCatalogInfo FAILED'));
        },

        /**
         * @typedef path2offerIdObject
         * @type {object}
         * @property {string} storePath
         * @property {string} country
         * @property {string} offerId
         */
        /**
         * This will return a promise for path2offerId object
         *
         * @param {string} path
         * @return {promise<module:Origin.module:games~path2offerIdObject>} response
         */
        getOfferIdByPath: function(path) {
            var country2letter = configService.countryCode().toUpperCase(),
                endPoint = urls.endPoints.offerIdbyPath,
                config = {
                    atype: 'GET',
                    headers: [{
                        'label': 'Accept',
                        'val': 'application/json'
                    }],
                    parameters: [{
                        'label': 'path',
                        'val': path
                    }, {
                        'label': 'country2letter',
                        'val': country2letter
                    }],
                    appendparams: [],
                    reqauth: false,
                    requser: false
                };
            return dataManager.enQueue(endPoint, config, 0)
                .catch(errorhandler.logAndCleanup('GAMES:getOfferIdByPath FAILED'));
        },

        /**
         * @typedef customAttribObject
         * @type {object}
         * @property {string} gameEditionTypeFacetKeyRankDesc
         */

        /**
         * @typedef masterTitle2offerIdObject
         * @type {object}
         * @property {customAttribObject} customAttributes
         * @property {string} offerId
         */
        /**
         * Given a masterTitleId, this will return a promise for an array of objects of purchasable offerId with assciated gameEditionTypeFacetKeyRankDesc
         *
         * @param {string} masterTitleId
         * @return {promise<module:Origin.module:games~masterTitle2offerIdObject[]>} response
         */

        getBaseGameOfferIdByMasterTitleId: function(masterTitleId) {
            var country2letter = configService.countryCode().toUpperCase(),
                endPoint = urls.endPoints.basegameOfferIdByMasterTitleId,
                config = {
                    atype: 'GET',
                    headers: [{
                        'label': 'Accept',
                        'val': 'application/json'
                    }],
                    parameters: [{
                        'label': 'masterTitleId',
                        'val': masterTitleId
                    }, {
                        'label': 'country2letter',
                        'val': country2letter
                    }],
                    appendparams: [],
                    reqauth: false,
                    requser: false
                };
            return dataManager.enQueue(endPoint, config, 0)
                .then(handleBaseGameOfferIdByMasterTitleIdResponse)
                .catch(errorhandler.logAndCleanup('GAMES:getBaseGameOfferIdByMasterTitleId FAILED'));
        },

        /**
         * This will return a promise for the OCD object for the specified path
         *
         * @param {string} locales - should be locale+country, e.g. en-us.usa
         * @param {string} path
         * @return {promise<Object>} responsename OCD object for the specified path in CQ5 game tree
         *
         */
        getOcdByPath: function(locale, path) {
            var localeLower = locale.toLowerCase(),
                endPoint = urls.endPoints.ocdByPath,

                config = {
                    atype: 'GET',
                    headers: [{
                        'label': 'Accept',
                        'val': 'application/json; x-cache/force-write'
                    }],
                    parameters: [],
                    appendparams: [],
                    reqauth: false,
                    requser: false,
                    responseHeader: true
                };

            endPoint +=  path + '.' + localeLower + '.ocd';

            return dataManager.enQueue(endPoint, config, 0).catch(errorhandler.logAndCleanup('GAMES:getOcdByPath FAILED'));
        },

        /**
         * @typedef ratingObject
         * @type {object}
         * @property {float} finalTotalPrice
         * @property {float} originalTotalPrice
         * @property {float} originalTotalUnitPrice
         * @property {promotionsObject} promotions - could be empty
         * @property {integer} quantity
         * @property {recommendedPromotionsObject} recommendedPromotions - could be empty
         * @property {float} totalDiscountAmount
         * @property {float} totalDiscountRate
         */

        /**
         * @typedef priceObj
         * @type {object}
         * @property {string} offerId
         * @property {string} offerType
         * @property {ratingObject} rating associative array indexed by currency country e.g. rating['USD']
         */

        /**
         * This will return a list of pricing information from the ratingsEngine
         *
         * @param {string[]} offerIdList list of offers for which to retrieve price
         * @param {string} country 3-letter country
         * @param {string} currency a currrency code, e.g. 'USD', 'CAD'
         * @return {promise<module:Origin.module:games~priceObj[]>} responsename a promise for a list of priceObject
         */
        getPrice: function(offerIdList, country, currency) {
            return getPricingList(offerIdList, currency);
        },
        /**
         * Fetch the formatting rules for each locale from the server.
         *
         * @return {Promise} The HTTP response promise
         */
        getPricingFormatter: getPricingFormatter,

        /**
         * @typedef bundlePromotionsObj
         * @type {object}
         * @param {string} promotionRuleId
         * @param {float} discountAmount
         * @param {float} discountRate
         * @param {string} promotionRuleType
         */

        /**
         * @typedef bundleRatingObj
         * @type {object}
         * @property {float} finalTotalPrice
         * @property {float} originalTotalPrice
         * @property {float} originalTotalUnitPrice
         * @property {bundlePromotionsObject} promotions - could be empty
         * @property {integer} quantity
         * @property {recommendedPromotionsObject} recommendedPromotions - could be empty
         * @property {float} totalDiscountAmount
         * @property {float} totalDiscountRate
         */

        /**
         * @typedef lineitemPriceObj
         * @type {object}
         * @property {string} bundleType
         * @property {bundleRatingObj} rating
         */

        /**
         * @typedef bundlePriceObj
         * @type {object}
         * @property {boolean} isComplete
         * @property {lineitemPriceObj[]} lineitems;
         */

        /**
         * This will return a promise for the ODC profile data
         *
         * @param {string} odcProfile the ODC profile ID
         * @param {string} language The two-letter language code, i.e. 'en'
         * @return {promise<Object>} responsename ODC data for the given ODC profile
         *
         */
        getOdcProfile: function(odcProfile, language) {
            var endPoint = urls.endPoints.odcProfile;
            var config = {
                atype: 'GET',
                headers: [{
                    'label': 'Accept',
                    'val': 'application/json; x-cache/force-write'
                }],
                parameters: [{
                    'label': 'profile',
                    'val': odcProfile
                }, {
                    'label': 'language',
                    'val': language
                }],
                appendparams: [],
                reqauth: false,
                requser: false,
                responseHeader: false
            };

            return dataManager.enQueue(endPoint, config, 0)
                .catch(errorhandler.logAndCleanup('GAMES:getOdcProfile FAILED'));
        },

    };
});
