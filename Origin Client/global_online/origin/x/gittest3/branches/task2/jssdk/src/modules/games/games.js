/*jshint unused: false */
/*jshint strict: false */
define([
    'promise',
    'core/logger',
    'core/utils',
    'core/user',
    'core/urls',
    'core/dataManager',
    'core/errorhandler'
], function(Promise, logger, utils, user, urls, dataManager, errorhandler) {

    var defaultLMD = 'Sat, 01 Jan 2000 00:00:00 GMT',
        entReqNum = 0, //to keep track of the entitlement requests
        queue = [];

    //need to set all localStorage entries to be dirty to force retrieval of LMD
    (function markAllLMDdirty() {

        //do this only if we're online
        var online = true;
        if (typeof OriginOnlineStatus !== 'undefined') {
            online = OriginOnlineStatus.onlineState;
        }
        if (online) {
            for (var i = 0; i < localStorage.length; i++) {
                if (localStorage.key(i).indexOf('lmd_') === 0) {
                    var lmdEntryStr = localStorage.getItem(localStorage.key(i));
                    var offerCacheInfo = JSON.parse(lmdEntryStr);
                    //mark it as dirty
                    offerCacheInfo.dirty = true;

                    var jsonS = JSON.stringify(offerCacheInfo);
                    localStorage.setItem(localStorage.key(i), jsonS);
                }
            }
        }
    })();

    /**
     * Returns the cached information from Local Storage for given productInfo
     * @param    productId
     * @return   offerInfo object = r:[LMD], dirty:true/false, o:[LMD]
     *           if it doesn't exist in local storage, returns undefined object
     */

    function getCacheInfo(productId) {
        var offerInfoLSstr = localStorage.getItem('lmd_' + productId);
        var offerInfo;
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
        localStorage.setItem('lmd_' + productId, jsonS);
        //logger.log('setCacheInfo:', productId, ':', jsonS, 'offerInfo:', offerInfo.r, ',', offerInfo.o);
    }

    function parseLMD(response) {
        var returnObj = Date.parse(defaultLMD);
        if (utils.isChainDefined(response, ['offer', 0, 'updatedDate'])) {
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
    function catalogCacheParameter(productId) {
        //retrieve our cache-buster value
        var offerInfo = getCacheInfo(productId),
            promise = null;

        //not requested previously or isDirty = true means we need to go retrieve the Last-Modified-Date (LMD)
        if ((typeof offerInfo === 'undefined') || offerInfo.dirty === true) {
            //make request to ML to retrieve LMD
            var endPoint = urls.endPoints.catalogInfoLMD,
                config = {
                    atype: 'GET',
                    headers: [{
                        'label': 'Accept',
                        'val': 'application/json'
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
        if (token.length > 0) {
            var endPoint = urls.endPoints.catalogInfoPrivate,

                setLocale = locale || 'DEFAULT',
                encodedProductId = encodeURIComponent(productId),
                config = {
                    atype: 'GET',
                    headers: [{
                        'label': 'Accept',
                        'val': 'application/json'
                    }, {
                        'label': 'AuthToken',
                        'val': token
                    }],
                    parameters: [{
                        'label': 'productId',
                        'val': encodedProductId
                    }, {
                        'label': 'locale',
                        'val': setLocale
                    }],
                    appendparams: [{
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
                promise = null;
            if (usePrivateUrl || forcePrivate) {
                promise = retrievePrivateCatalogInfo(lmdValue, productId, locale, parentOfferId);
            } else {
                var endPoint = urls.endPoints.catalogInfo,
                    setLocale = locale || 'DEFAULT',
                    encodedProductId = encodeURIComponent(productId),
                    config = {
                        atype: 'GET',
                        headers: [{
                            'label': 'Accept',
                            'val': 'application/json'
                        }],
                        parameters: [{
                            'label': 'productId',
                            'val': encodedProductId
                        }, {
                            'label': 'locale',
                            'val': setLocale
                        }],
                        appendparams: [{
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

    /**
     * This will return a promise for catalog info
     *
     * @param {string} productId The product id of the offer.
     * @param {string} locale locale of the offer
     * @return {promise<catalogInfo>}
     */
    function catalogInfo(productId, locale, usePrivateUrl, parentOfferId) {
        return catalogCacheParameter(productId)
            .then(parseLMD, parseLMD)
            .then(updateOfferCacheInfo(productId))
            .then(retrieveCatalogInfo(productId, locale, usePrivateUrl, parentOfferId))
            .then(markCacheNotDirty(productId), errorhandler.logAndCleanup('GAMES:catalogInfo FAILED'));

    }

    function handleBaseGameEntitlementsResponse(response) {
        //need to update the offer LMD for each offer in the entitlement
        var len = response.entitlements.length;
        for (var i = 0; i < len; i++) {

            var lmd = response.entitlements[i].updatedDate;
            var lmdValue = Date.parse(lmd); //conver to numeric value
            var offerId = response.entitlements[i].offerId;
            var offerInfo = getCacheInfo(offerId);
            if (typeof offerInfo === 'undefined') {
                offerInfo = {};
                offerInfo.r = 0;
            }
            offerInfo.o = lmdValue;
            offerInfo.dirty = false;
            setCacheInfo(offerId, offerInfo);
        }
        return response.entitlements;
    }

    function handleExtraContentInfoResponse(masterTitleId) {
        return function(response) {

            //need to update the offer LMD for each offer in the entitlement
            var len = response.entitlements.length;
            for (var i = 0; i < len; i++) {

                var lmd = response.entitlements[i].updatedDate;
                var lmdValue = Date.parse(lmd); //conver to numeric value
                var offerId = response.entitlements[i].offerId;
                var offerInfo = getCacheInfo(offerId);
                if (typeof offerInfo === 'undefined') {
                    offerInfo = {};
                    offerInfo.r = 0;
                }
                offerInfo.o = lmdValue;
                offerInfo.dirty = false;
                setCacheInfo(offerId, offerInfo);
            }

            var responseObj = {};
            responseObj.masterTitleId = masterTitleId;
            responseObj.entitlements = response.entitlements;
            return responseObj;
        };

    }

    function handleExtraContentInfoError(masterTitleId) {
        return function(error) {
            error.masterTitleId = masterTitleId;
            return error;
        };

    }

    /** @namespace
     * @memberof Origin
     * @alias games
     */
    return {

        /**
         * @typedef softwareObject
         * @type {object}
         * @property {fulfillmentAttributesObject} fulfillmentAttributes TBD
         * @property {downloadURLsObject} downloadURLs TBD
         * @property {string} softwarePlatform TBD
         */
        /**
         * @typedef softwareListObject
         * @type {object}
         * @property {softwareObject[]} software TBD
         */
        /**
         * @typedef publishingAttributesObject
         * @type {object}
         * @property {string} contentId TBD
         * @property {boolean} greyMarketControls TBD
         * @property {boolean} isDownloadable TBD
         * @property {string} originDisplayType TBD
         * @property {boolean} isPublished TBD
         */
        /**
         * @typedef publishingObject
         * @type {object}
         * @property {publishingAttributesObject} publishingAttributes TBD
         * @property {softwareListObject} publishingAttributes TBD
         * @property {softwareLocalesObject} publishingAttributes TBD
         * @property {softwareControlDates} publishingAttributes TBD
         */
        /**
         * @typedef localizableAttributeObject
         * @type {object}
         * @property {string} longDescription TBD
         * @property {string} displayName TBD
         * @property {string} shortDescription TBD
         * @property {string} packArtSmall TBD
         * @property {string} packArtMedium TBD
         * @property {string} packArtLarge TBD
         */
        /**
         * @typedef customAttributeObject
         * @type {object}
         * @property {string} imageServer TBD
         */
        /**
         * @typedef baseAttributeObject
         * @type {object}
         * @property {string} platform TBD
         */
        /**
         * @typedef catalogInfo
         * @type {object}
         * @property {string} itemId TBD
         * @property {string} financeId TBD
         * @property {baseAttributeObject} baseAttributes TBD
         * @property {customAttributeObject} customAttributes TBD
         * @property {localizableAttributeObject} localizableAttributes TBD
         * @property {publishingObject} publishingObject TBD
         * @property {mdmHierarchiesObject} mdmHierarchies TBD
         * @property {ecommerceAttributesObject} ecommerceAttributes TBD
         * @property {string} updatedDate TBD
         * @property {string} offerType TBD
         * @property {string} offerId TBD
         */
        /**
         * This will return a promise for catalog info
         *
         * @param {string} productId The product id of the offer.
         * @param {string} locale locale of the offer
         * @return {promise<catalogInfo>}
         */
        catalogInfo: catalogInfo,

        /**
         * This will return a promise for the private catalog info
         *
         * @param {string} productId The product id of the offer.
         * @param {string} locale locale of the offer
         * @return {promise<catalogInfo>}
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
         */
        /**
         * This will return a promise for the base game Entitlements
         *
         * @param {boolean} forceRetrieve If true grab latest info from server.
         * @return {promise<entitlementObject[]>}
         *
         */
        baseGameEntitlements: function(forceRetrieve) {
            var endPoint = urls.endPoints.baseGameEntitlements;
            var config = {
                atype: 'GET',
                headers: [{
                    'label': 'Accept',
                    'val': 'application/vnd.origin.v2+json'
                }],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }],
                appendparams: [],
                reqauth: true,
                requser: true
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
                .then(handleBaseGameEntitlementsResponse, errorhandler.logAndCleanup('GAMES:baseGameEntitlements FAILED'));
        },


        /**
         * This will return a promise for the extra content entitlements
         *
         * @param {string} masterTitleId The masterTitleId for the offer.
         * @param {boolean} forceRetrieve If true grab latest info from server.
         * @param {string} platform specify OS (PCWIN/Mac) for the extra content
         * @return {promise<entitlementObject[]>}
         *
         */
        extraContentEntitlements: function(masterTitleId, forceRetrieve, platform) {

            var endPoint = urls.endPoints.extraContentEntitlements;

            var osPlatform = platform;
            if (!platform) {
                osPlatform = utils.os();
            }

            var config = {
                atype: 'GET',
                headers: [{
                    'label': 'Accept',
                    'val': 'application/vnd.origin.v2+json'
                }, {
                    'label': 'X-Origin-Platform',
                    'val': osPlatform
                }],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }, {
                    'label': 'masterTitleId',
                    'val': masterTitleId
                }, ],
                appendparams: [],
                reqauth: true,
                requser: true
            };

            var token = user.publicObjs.accessToken();
            if (token.length > 0) {
                dataManager.addHeader(config, 'AuthToken', token);
            }

            if (typeof forceRetrieve !== 'undefined' && forceRetrieve === true) {
                entReqNum++; //update the request #
            }

            return dataManager.enQueue(endPoint, config, entReqNum)
                .then(handleExtraContentInfoResponse(masterTitleId), errorhandler.logAndCleanup('GAMES:extraContentEntitlements FAILED', handleExtraContentInfoError(masterTitleId)));
        },

        /**
         * @typedef criticalCatalogInfoObject
         * @type {object}
         * @property {string} b (boxart url)
         * @property {string} d (0/1 downloadable)
         * @property {string} n (gametitle)
         * @property {string} o offerId
         * @property {string} r release date
         * @property {string} t (0/1 trial)
         */
        /**
         * This will return a promise of a list of critical catalog info
         *
         * @return {promise<criticalCatalogInfoObject[]>}
         */
        getCriticalCatalogInfo: function(locale) {
            var setLocale = locale || 'DEFAULT',
                endPoint = urls.endPoints.criticalCatalogInfo,
                config = {
                    atype: 'GET',
                    headers: [{
                        'label': 'Accept',
                        'val': 'application/json'
                    }],
                    parameters: [{
                        'label': 'locale',
                        'val': setLocale
                    }],
                    appendparams: [],
                    reqauth: false,
                    requser: false
                };

            //TODO: eventually will need to deal with LMD and cache-busting
            return dataManager.enQueue(endPoint, config, 0).catch(errorhandler.logAndCleanup('GAMES:getCriticalCatalogInfo FAILED'));
        },
        /**
         * [hide description]
         * @param  {[type]} productId [description]
         * @return {[type]}           [description]
         */
        hide: function(productId) {
            console.log('hide' + productId);
        },
        /**
         * [unhide description]
         * @param  {[type]} productId [description]
         * @return {[type]}           [description]
         */
        unhide: function(productId) {
            console.log('unhide' + productId);
        },


        /**
         * This will return a promise for the OCD object for the specified offer
         *
         * @param {string} offerId
         * @param {locale} locale of the offer
         * @return {promise <Object>} OCD object for the offer
         *
         */
        getOcdByOfferId: function(offerId, locale) {
            var setLocale = locale || '',
                setLocaleLower = setLocale.toLowerCase(),
                endPoint = urls.endPoints.ocdByOffer,
                encodedOfferId = encodeURIComponent(offerId),
                config = {
                    atype: 'GET',
                    headers: [{
                        'label': 'Accept',
                        'val': 'application/json'
                    }],
                    parameters: [{
                        'label': 'offerId',
                        'val': encodedOfferId
                    }, {
                        'label': 'locale',
                        'val': ((setLocaleLower.length > 0) ? '.' + setLocaleLower : ''),
                    }],
                    appendparams: [],
                    reqauth: false,
                    requser: false,
                    responseHeader: true
                };

            return dataManager.enQueue(endPoint, config, 0).catch(errorhandler.logAndCleanup('GAMES:getOcdByOffer FAILED'));
        },

        /**
         * This will return a promise for the OCD object for the specified path
         *
         * @param {string} locales
         * @param {string} franchise
         * @param {string} game - OPTIONAL
         * @param {string} edition - OPTIONAL
         * @return {promise <Object>} OCD object for the specified path in CQ5 game tree
         *
         */
        getOcdByPath: function(locale, franchise, game, edition) {
            var localeLower = locale.toLowerCase(),
                endPoint = urls.endPoints.ocdByPath,

                config = {
                    atype: 'GET',
                    headers: [{
                        'label': 'Accept',
                        'val': 'application/json'
                    }],
                    parameters: [],
                    appendparams: [],
                    reqauth: false,
                    requser: false,
                    responseHeader: true
                };

                //we need to contruct the full path
                //https://dev.sb3.x.origin.com:4502/content/web/app/games/simcity.en-US.ocd
                //franchise is REQUIRED
                endPoint += franchise;
                if (franchise) {
                    endPoint += game ? '/' + game : '';
                }
                if (franchise && game) {
                    endPoint += edition ? '/' + edition : '';
                }

                //locale is required
                endPoint += '.' + localeLower + '.ocd';

                return dataManager.enQueue(endPoint, config, 0).catch(errorhandler.logAndCleanup('GAMES:getOcdByPath FAILED'));
        },

    };
});