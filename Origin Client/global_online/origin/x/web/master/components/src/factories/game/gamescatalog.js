/**
 * @file game/gamescatalog.js
 */
(function() {
    'use strict';

    function GamesCatalogFactory(ComponentsLogFactory, ComponentsConfigFactory, AuthFactory) {
        /**
         * @typedef uniqueEntObject
         * @type {object}
         * @property {String} offerId
         * @property {Date} grantDate
         */
        /** an array of uniqueEntObjects
         */
        var catalog = {},
            myEvents = new Origin.utils.Communicator(),
            criticalCatalogRetrieved = false,
            DEFAULT_BOX_ART = 'packart-placeholder.jpg',
            UNOWNEDONLY = 'UNOWNEDONLY',
            OWNEDONLY = 'OWNEDONLY';
        var stateTransitionTimers = {}; // Map of timers indexed by Offer ID.
        //self = this; /* you can't use this in a function for jshint */

        /**
         * sets the property name to value if catalog[id] exists
         * @param {String} id - catalog offerId
         * @param {[String]} nameArray - property names
         * @param {object} value - value of the property
         * @return {void}
         * @method setCatalogProperty
         */
        function setCatalogProperty(id, nameArray, value) {
            var cat,
                len;

            //can't use truthy because value of 0 or false is valid
            if (!catalog[id] || value === null || typeof(value) === 'undefined') {
                return;
            }

            cat = catalog[id];
            len = nameArray.length;

            for (var i = 0; i < len; i++) {
                if (i === len - 1)  //at the end
                {
                    cat [nameArray[i]] = value;
                } else {
                    if (typeof cat[nameArray[i]] === 'undefined') {
                        cat [nameArray[i]] = {};
                    }
                    cat = cat [nameArray[i]];
                }
            }

//            catalog[id][name] = value;
        }

        /**
         * determines if the type of object is 'Primary'
         * @param {Object} item
         * @return {boolean} true/false based on type === 'Primary'
         * @method isPrimary
         */
        function isPrimary(item) {
            return item && item.type === 'Primary';
        }

        /**
         * Generates a pipeline filter that would be true for all the items with given country code
         * @param {string} countryCode - country code to match items against
         * @return {boolean}
         */
        function matchesCountryCode(countryCode) {
            return function(item) {
                return item && item.countryCode === countryCode;
            };
        }

        /**
         * determines if the type of object is 'Alternate'
         * @param {Object} item
         * @return {boolean} true/false based on type === 'Alternate'
         * @method isAlternate
         */
        function isAlternate(item) {
            return item && item.type === 'Alternate';
        }

        /**
         * extracts out the masterTitleIds from the response if any exists
         * @param {Object} response
         * @return {Object} primary and alternate masterTitleIds
         * @method fetchMasterTitleIds
         */
        function fetchMasterTitleIds(response) {
            var mdmHierarchy = Origin.utils.getProperty(response, ['mdmHierarchies', 'mdmHierarchy']),
                primaryItems,
                primaryItem,
                alternateItems,
                obj = { 'primary': null, 'alternates': [] };

            //mdmHeirarchy may not exist (e.g. with extra content), so we need to guard against it
            if (mdmHierarchy) {
                primaryItems = mdmHierarchy.filter(isPrimary);
                if (primaryItems.length) {
                    primaryItem = primaryItems[primaryItems.length - 1]; //use the last one
                    obj.primary = Origin.utils.getProperty(primaryItem, ['mdmMasterTitle', 'masterTitleId']);
                }
                alternateItems = mdmHierarchy.filter(isAlternate);
                for (var i = 0; i < alternateItems.length; i++) {
                    obj.alternates.push(Origin.utils.getProperty(alternateItems[i], ['mdmMasterTitle', 'masterTitleId']));
                }
            }
            return obj;
        }

        function isKnownDisplayType(displayType) {
            var knownTypes = ['Full Game', 'Full Game Plus Expansion', 'Expansion', 'Addon', 'None', 'Game Only'];
            return (knownTypes.indexOf(displayType) > -1);
        }

        function setOriginDisplayType(id, response) {
            var displayType = Origin.utils.getProperty(response, ['publishing', 'publishingAttributes', 'originDisplayType']);
            if (typeof displayType === 'undefined' || !isKnownDisplayType(displayType)) {
                if (catalog[id].downloadable) {
                    displayType = 'Addon';
                } else {
                    displayType = 'Game Only';
                }
            }
            catalog[id].originDisplayType = displayType;
        }

        function setPurchasable(id, response) {
            var countryList,
                countryCode,
                matchingCountry,
                isPurchasable = false;

            countryList = Origin.utils.getProperty(response, ['countries', 'country']);
            //guard against offers that don't have countries specified
            if (countryList) {
                countryCode = Origin.locale.countryCode();
                matchingCountry = countryList.filter(matchesCountryCode(countryCode));

                if (matchingCountry.length === 0) {
                    //try default
                    countryCode = 'DEFAULT';
                    matchingCountry = countryList.filter(matchesCountryCode(countryCode));
                }

                if (matchingCountry.length > 0) { //found one
                    isPurchasable = Origin.utils.getProperty(matchingCountry[0], ['attributes', 'isPurchasable']);
                } else {
                    isPurchasable = false;
                }
            }
            setCatalogProperty(id, ['countries', 'isPurchasable'], isPurchasable);
        }

        /**
         * returns the platform specific object
         * @param {String} offerId
         * @param {String} platform
         * @return {Object} returns the platform object, if none existed, it creates one with just platform attribute
         * @method getPlatformObj
         */
        function getPlatformObj(id, platform) {
            if (!catalog[id].platforms.hasOwnProperty(platform)) {
                //doesn't yet exist, so create one for this platform
                catalog[id].platforms[platform] = {
                    platform: platform
                };
            }
            return (catalog[id].platforms[platform]);
        }

        /**
         * extracts out the properties we use under softwareAttributes
         * @param {Object} response
         * @return {[Object]} an array of object that contains multiPlayerId, packageType, platform for each platform
         * @method fetchSoftwareAttributes
         */
        function fetchSoftwareAttributes(response) {
            var software = Origin.utils.getProperty(response, ['publishing', 'softwareList', 'software']),
                fulfillmentAttributes = null,
                platform = null,
                multiPlayerId = null,
                packageType = null,
                executePathOverride = null,
                softwareAttributes = [];

            //for some titles (e.g. extra content), software might not exist so we need to guard against it
            if (software) {
                for (var i = 0; i < software.length; i++) {
                    fulfillmentAttributes = software[i].fulfillmentAttributes;
                    if (fulfillmentAttributes) {
                        platform = software[i].softwarePlatform;
                        //if platform isn't specified for some reason, assume PCWIN
                        if (!platform) {
                            platform = "PCWIN";
                        }
                        multiPlayerId = fulfillmentAttributes.multiPlayerId;
                        packageType = fulfillmentAttributes.downloadPackageType;
                        executePathOverride = fulfillmentAttributes.executePathOverride;
                        softwareAttributes.push({
                            platform: platform,
                            multiPlayerId: multiPlayerId,
                            packageType: packageType,
                            executePathOverride: executePathOverride
                        });
                    }
                }
            }

            return softwareAttributes;
        }

        /*
         * retrieves the properties under software attributes and sets the properties of the cached catalog
         * @param {String} id - offerId
         * @param {Object} response - catalog response from server
         * @method setSoftwareAttributes
         */
        function setSoftwareAttributes(id, response) {
            var softwareAttributes = fetchSoftwareAttributes(response),
                attribute = null,
                platform = null,
                xpathOverride = '';

            if (softwareAttributes) {
                for (var i = 0; i < softwareAttributes.length; i++) {
                    attribute = softwareAttributes[i];
                    platform = getPlatformObj(id, attribute.platform);
                    platform.multiPlayerId = attribute.multiPlayerId;
                    platform.packageType = attribute.packageType;

                    //store is only interested if executePathOverride is a web url (for web-based game), if it's not, then just leave it null
                    xpathOverride = attribute.executePathOverride;
                    if (!xpathOverride || (xpathOverride.indexOf('http') === -1)) {
                        xpathOverride = null;
                    }
                    platform.executePathOverride = xpathOverride;
                }
            }
        }


        /**
         * extracts out the control dates from the catalog response
         * @param {Object} response
         * @return {[Object]} an array of objects that contains releaseDate, downloadStartDate, useEndDate, platform for each platform
         * @method fetchSoftwareControlDates
         */
        function fetchSoftwareControlDates(response) {
            var softwareControlDateArray = Origin.utils.getProperty(response, ['publishing', 'softwareControlDates', 'softwareControlDate']),
                softwareControlObj = null,
                platform = null,
                softwareControlDates = [];

            if (softwareControlDateArray) {
                for (var i = 0; i < softwareControlDateArray.length; i++) {
                    softwareControlObj = softwareControlDateArray[i];
                    platform = softwareControlObj.platform;
                    //if not specified for some reason, assume PC
                    if (!platform) {
                        platform = "PCWIN";
                    }
                    softwareControlDates.push({
                        platform: platform,
                        releaseDate: softwareControlObj.releaseDate,
                        downloadStartDate: softwareControlObj.downloadStartDate,
                        useEndDate: softwareControlObj.useEndDate
                    });
                }
            }
            return softwareControlDates;
        }

        /*
         * retrieves the properties under software control dates and sets the properties of the cached catalog
         * @param {String} id - offerId
         * @param {Object} response - catalog response from server
         * @method setSoftwareControlDates
         */
        function setSoftwareControlDates(id, response) {
            var softwareControlDates = fetchSoftwareControlDates(response),
                dateObj = null,
                platform = null;

            if (softwareControlDates) {
                for (var i = 0; i < softwareControlDates.length; i++) {
                    dateObj = softwareControlDates[i];
                    platform = getPlatformObj(id, dateObj.platform);
                    platform.releaseDate = dateObj.releaseDate ? new Date(dateObj.releaseDate) : null;
                    platform.downloadStartDate = dateObj.downloadStartDate ? new Date(dateObj.downloadStartDate) : null;
                    platform.useEndDate = dateObj.useEndDate ? new Date(dateObj.useEndDate) : null;
                }
            }
        }

        function fireReleaseStateUpdateForOffer(offerId) {

            // Return an anonymous function to fire the event.
            return function() {

                // Fire the event.
                myEvents.fire('GamesCatalogFactory:offerStateChange', {
                    signal: 'releaseStateUpdate:' + offerId,
                    eventObj: {}
                });

                // Remove the current timer and re-check the offer for any further events.
                delete stateTransitionTimers[offerId];
                queueCatalogTimingEventsForOffer(offerId);
            };
        }

        /**
         * Sets up events to fire when the release date is passed.  Assuming catalog data is
         * available for the supplied offer ID, it sets up appropriate timers.
         *
         * @param {string} offerId - The offer to set the timer
         */
        function queueCatalogTimingEventsForOffer(offerId) {

            // Prepare to fire an event when the offer's release date is passed.
            var currentTime = Origin.datetime.getTrustedClock();
            var platformObj = getPlatformObj(offerId, Origin.utils.os());
            var downloadStartDate = platformObj.downloadStartDate;
            var releaseDate = platformObj.releaseDate;
            var useEndDate = platformObj.useEndDate;

            // Only one timer per offer is used.  Once fired, it will re-check for any further timers needed.
            if (! (stateTransitionTimers.hasOwnProperty(offerId))) {

                // Unreleased to Preloadable
                if (downloadStartDate && (currentTime < downloadStartDate)) {
                    var millisecondsUntilPreloadable = downloadStartDate - currentTime;
                    stateTransitionTimers[offerId] = setTimeout(fireReleaseStateUpdateForOffer(offerId), millisecondsUntilPreloadable);
                }

                // Preloadable to Released
                else if (releaseDate && (currentTime < releaseDate)) {
                    var millisecondsUntilReleased = releaseDate - currentTime;
                    stateTransitionTimers[offerId] = setTimeout(fireReleaseStateUpdateForOffer(offerId), millisecondsUntilReleased);
                }

                // Released to Sunset
                else if (useEndDate && (currentTime < useEndDate)) {
                    var millisecondsUntilSunset = useEndDate - currentTime;
                    stateTransitionTimers[offerId] = setTimeout(fireReleaseStateUpdateForOffer(offerId), millisecondsUntilSunset);
                }
            }
        }

        function setPlatformFromBaseAttributes(id, baseAttributes) {
            var platformList = [],
                platformObj = {};

            platformList = baseAttributes.split('/');
            for (var i = 0; i < platformList.length; i++) {
                platformObj = {
                    platform: platformList[i]
                };
                catalog[id].platforms[platformList[i]] = platformObj;
            }
        }

        function setPackArt(id, response) {
            var packArt,
                packArtTypes = ['packArtSmall', 'packArtMedium', 'packArtLarge'];

            angular.forEach (packArtTypes, function(value) {
                packArt = Origin.utils.getProperty(response, ['localizableAttributes', value]);
                if (packArt) {
                    packArt = response.customAttributes.imageServer + packArt;
                } else {
                    packArt = ComponentsConfigFactory.getImagePath(DEFAULT_BOX_ART);
                }
                setCatalogProperty (id, ['i18n', value], packArt);
            });
        }

        function setSoftwareLocales(id, response) {
            var locales = Origin.utils.getProperty(response, ['publishing', 'softwareLocales', 'locale']);
            catalog[id].softwareLocales = [];
            angular.forEach(locales, function(value) {
                catalog[id].softwareLocales.push(value.value);
            });
        }

        /*
         * given a LARS3 catalog response from the server, sets up the cached catalog info catalog[offerId]
         * @param {String} id - offerId
         * @param {Object} response - catalog response from server
         * @method setInfoFromCatalog
         */
        function setInfoFromCatalog(id, response) {
            var publishingAttr = response.publishing.publishingAttributes,
                subType = publishingAttr.gameDistributionSubType,
                baseAttributes = response.baseAttributes.platform;

            catalog[id] = {
                offerId: id,
                extraContent: response.ecommerceAttributes.availableExtraContent,
                downloadable: publishingAttr.isDownloadable,
                trial: ((subType === 'Limited Trial') ||
                    (subType === 'Weekend Trial') ||
                    (subType === 'Limited Weekend Trial')),
                limitedTrial: ((subType === 'Limited Trial') ||
                    (subType === 'Limited Weekend Trial')),
                trialLaunchDuration: publishingAttr.trialLaunchDuration || '',
                oth: subType === 'Game Giveaway',
                demo: subType === 'Demo',
                alpha: subType === 'Alpha',
                beta: subType === 'Beta',
                suppressedOfferIds: [],
                platforms: {},
                countries: {},
                i18n: {},
                vaultInfo: {},
                vault: false,
                //comment the price out for now as not all catalog info currently has it
                //price: response.countries.DEFAULT.prices.price.price,
                //LARS3 no longer contains platformFacetKey
                //platformFacetKey: response.customAttributes.platformFacetKey
            };

            // extra content offers
            if (response.offerType === 'Extra Content') {
                catalog[id].extraContentType = publishingAttr.originDisplayType;
            }

            // Suppression attributes
            if (publishingAttr.suppressedOfferIds) {
                catalog[id].suppressedOfferIds = publishingAttr.suppressedOfferIds.split(',');
            }

            var masterTitleIds = fetchMasterTitleIds(response);
            setCatalogProperty(id, ['masterTitleId'], masterTitleIds.primary);
            setCatalogProperty(id, ['alternateMasterTitleIds'], masterTitleIds.alternates);

            setPurchasable(id, response);
            setOriginDisplayType(id, response);

            //set platform dependent attributes
            setSoftwareAttributes(id, response);
            setSoftwareControlDates(id, response);

            //if platforms is not specified via softwareAttributes or softwareControlDates, then try to at least set platform from baseAttributes
            if (Object.keys(catalog[id].platforms).length === 0) {
                setPlatformFromBaseAttributes(id, baseAttributes);
            }

            //TODO: LARS3 doesn't yet contain countries info, once it is added we'll need to parse it
            setCatalogProperty(id, ['i18n', 'longDescription'], Origin.utils.getProperty(response, ['localizableAttributes', 'longDescription']));
            setCatalogProperty(id, ['i18n', 'shortDescription'], Origin.utils.getProperty(response, ['localizableAttributes', 'shortDescription']));
            setCatalogProperty(id, ['i18n', 'displayName'], Origin.utils.getProperty(response, ['localizableAttributes', 'displayName']));
            setPackArt (id, response);

            setSoftwareLocales(id, response);

            queueCatalogTimingEventsForOffer(id);
        }

        function getModifiedOriginDisplayType(displayType, downloadable) {
            if (displayType === null || typeof displayType === 'undefined' || !isKnownDisplayType(displayType)) {
                if (downloadable) {
                    displayType = 'Addon';
                } else {
                    displayType = 'Game Only';
                }
            }
            return displayType;
        }

        function getDateFromStr(dateStr) {
            if (dateStr) {
                return new Date(dateStr);
            }
            return null;
        }

        function extractCountryObj(responseCountryObj) {
            var countryObj = {};

            for (var ckey in responseCountryObj) {
                if (!responseCountryObj.hasOwnProperty(ckey)) {
                    continue;
                }

                if (ckey === 'isPurchasable' || ckey === 'hasSubscriberDiscount') {
                    //convert to a bool
                    countryObj[ckey] = (responseCountryObj[ckey] === 'Y');
                } else {
                //make a copy so that we don't hold onto the supercat response
                    countryObj[ckey] = responseCountryObj[ckey];
                }
            }
            return countryObj;
        }

        function getPackArt(packArtUrl, imageServer, offerId) {
            var packArt;

            if (!imageServer) {
                ComponentsLogFactory.error (offerId + ':missing image server');
                packArt = ComponentsConfigFactory.getImagePath(DEFAULT_BOX_ART);
            } else if (packArtUrl) {
                packArt = imageServer + packArtUrl;
            } else {
                packArt = ComponentsConfigFactory.getImagePath(DEFAULT_BOX_ART);
            }
            return packArt;
        }

        /* set up the cached catalog info from supercat
         * @param {Object} response - supercat
         * @method setInfoFromCriticalCatalog
         */
        function setInfoFromCriticalCatalog(response) {
            var offer = {},
                responseOffer = {},
                platformObj = {},
                responsePlatform = {},
                i18nObj = {},
                responseI18nObj = {},
                subType = '';

            for (var i = 0; i < response.offers.length; i++) {
                if (catalog[response.offers[i].offerId]) {
                    continue;
                }

                offer = {};
                responseOffer = response.offers[i];
                offer.offerId = responseOffer.offerId;
                offer.offerType = responseOffer.offerType;
                offer.downloadable = (responseOffer.isDownloadable === 'True');

                subType = responseOffer.gameDistributionSubType;

                offer.trial = ((subType === 'Limited Trial') ||
                    (subType === 'Weekend Trial') ||
                    (subType === 'Limited Weekend Trial'));

                offer.limitedTrial = ((subType === 'Limited Trial') ||
                    (subType === 'Limited Weekend Trial'));

                offer.oth = subType === 'Game Giveaway';
                offer.demo = subType === 'Demo';
                offer.alpha = subType === 'Alpha';
                offer.beta = subType === 'Beta';

                offer.trialLaunchDuration = responseOffer.trialLaunchDuration || '';
                offer.extraContent = responseOffer.extraContent || [];
                offer.masterTitleId = responseOffer.masterTitleId;
                offer.alternateMasterTitleIds = responseOffer.alternateMasterTitleIds || [];
                offer.suppressedOfferIds = responseOffer.suppressedOfferIds || [];
                offer.originDisplayType = getModifiedOriginDisplayType(responseOffer.originDisplayType, offer.downloadable);

                offer.platforms = {};
                //platform dependent
                if (responseOffer.platforms && responseOffer.platforms.length > 0) {
                    for (var j = 0; j < responseOffer.platforms.length; j++) {
                        platformObj = {};
                        responsePlatform = responseOffer.platforms[j];
                        platformObj.platform = responsePlatform.platform;
                        platformObj.multiPlayerId = responsePlatform.multiPlayerId;
                        platformObj.packageType = responsePlatform.downloadPackageType;
                        platformObj.releaseDate = getDateFromStr(responsePlatform.releaseDate);
                        platformObj.downloadStartDate = getDateFromStr(responsePlatform.downloadStartDate);
                        platformObj.useEndDate = getDateFromStr(responsePlatform.useEndDate);
                        platformObj.executePathOverride = responsePlatform.executePathOverride;
                        offer.platforms[platformObj.platform] = platformObj;
                    }
                } else {
                    //just default to the PC one until we can get supecat fixed
                    platformObj = {
                        platform: 'PCWIN'
                    };
                    offer.platforms[platformObj.platform] = platformObj;
                }

                //rating system related
                offer.ratingSystemIcon = responseOffer.ratingSystemIcon;
                offer.gameRatingUrl = responseOffer.gameRatingUrl;
                offer.gameRatingPendingMature = (responseOffer.gameRatingPendingMature === 'true');
                offer.gameRatingDescriptionLong = responseOffer.gameRatingDescriptionLong;
                offer.gameRatingTypeValue = responseOffer.gameRatingTypeValue;
                offer.gameRatingReason = responseOffer.gameRatingReason;
                offer.gameRatingDesc = [];

                if (responseOffer.gameRatingDesc) {
                    for (var k = 0; k < responseOffer.gameRatingDesc.length; k++) {
                        offer.gameRatingDesc[k] = responseOffer.gameRatingDesc[k];
                    }
                }

                offer.franchiseFacetKey = responseOffer.franchiseFacetKey;
                offer.platformFacetKey = responseOffer.platformFacetKey;
                offer.publisherFacetKey = responseOffer.publisherFacetKey;
                offer.developerFacetKey = responseOffer.developerFacetKey;
                offer.gameTypeFacetKey = responseOffer.gameTypeFacetKey;
                offer.gameEditionTypeFacetKey = responseOffer.gameEditionTypeFacetKey;
                offer.numberOfPlayersFacetKey = responseOffer.numberOfPlayersFacetKey;
                offer.genreFacetKey = responseOffer.genreFacetKey;

                offer.mdmItemType = responseOffer.mdmItemType;
                offer.revenueModel = responseOffer.revenueModel;
                offer.defaultLocale = responseOffer.defaultLocale;
                offer.softwareLocales = responseOffer.softwareLocales;

                offer.masterTitle = responseOffer.masterTitle;
                offer.itemName = responseOffer.itemName;
                offer.itemType = responseOffer.itemType;
                offer.itemId = responseOffer.itemId;
                offer.rbuCode = responseOffer.rbuCode;
                offer.brand = responseOffer.brand;
                offer.storeGroupId = responseOffer.storeGroupId;

                offer.dynamicPricing = (responseOffer.dynamicPricing === 'Y');

                //default values
                offer.countries = {};

                if (responseOffer.countries) {
                    offer.countries = extractCountryObj(responseOffer.countries);
                }

                //i18n
                offer.i18n = {};
                if (responseOffer.i18n) {
                    responseI18nObj = responseOffer.i18n;
                    i18nObj = {};

                    for (var key in responseI18nObj) {
                        if (!responseI18nObj.hasOwnProperty(key)) {
                            continue;
                        }
                        if (key === 'packArtSmall' || key === 'packArtMedium' || key === 'packArtLarge') {
                            i18nObj[key] = getPackArt(responseI18nObj[key], responseOffer.imageServer, offer.offerId);
                        } else {
                            i18nObj [key] = responseI18nObj[key];
                        }
                    }
                    offer.i18n = i18nObj;
                } else {
                    //at least populate packArt and displayName so they are non-null so people don't have guard against it
                    offer.i18n.packArtSmall = ComponentsConfigFactory.getImagePath(DEFAULT_BOX_ART);
                    offer.i18n.packArtMedium = ComponentsConfigFactory.getImagePath(DEFAULT_BOX_ART);
                    offer.i18n.packArtLarge = ComponentsConfigFactory.getImagePath(DEFAULT_BOX_ART);
                    offer.i18n.displayName = 'DISPLAY NAME MISSING';
                }

                offer.vault = false;
                offer.vaultInfo = {};
                if (responseOffer.vault) {
                    offer.vault = true;
                    for (var vkey in responseOffer.vault) {
                        offer.vaultInfo[vkey] = responseOffer.vault[vkey];
                    }
                }

                if (offer.offerType === 'Extra Content') {
                    offer.extraContentType = responseOffer.originDisplayType; //set it to the actual displaytype, not the modified one
                }

                catalog[offer.offerId] = offer;

                queueCatalogTimingEventsForOffer(offer.offerId);
            }
            return catalog;
        }

        function handleCriticalCatalogInfoError(error) {
            //in qtwebkit, error.stack is undefined so send error.message instead
            ComponentsLogFactory.error('[GAMESCATALOGFACTORY:criticalCatalogInfo]:', error.stack ? error.stack : error.message);
            criticalCatalogRetrieved = true;
            myEvents.fire('GamesCatalogFactory:criticalCatalogInfoComplete', {
                signal: 'criticalCatalogInfoComplete',
                eventObj: false
            });
        }

        function handleCriticalCatalogInfoResponse(response) {
            criticalCatalogRetrieved = true;
            setInfoFromCriticalCatalog(response);
            myEvents.fire('GamesCatalogFactory:criticalCatalogInfoComplete', {
                signal: 'criticalCatalogInfoComplete',
                eventObj: true
            });
        }

        function retrieveCriticalCatalogInfoPriv() {
            return Origin.games.getCriticalCatalogInfo(Origin.locale.locale())
                .then(handleCriticalCatalogInfoResponse)
                .catch(handleCriticalCatalogInfoError);
        }

        function checkForAndRequestPrivateOffer(productId, parentOfferId, resolveList) {
            return function(error) {
                var promise = null;

                if (error.status === Origin.defines.http.ERROR_404_NOTFOUND && AuthFactory.isAppLoggedIn()) {
                    var locale = Origin.locale.locale();
                    ComponentsLogFactory.log('[GAMESCATALOG] - attempting private offer retriveval:', productId);
                    promise = Origin.games.catalogInfoPrivate(productId, locale, parentOfferId)
                        .then(handleCatalogInfoResponse(resolveList));
                } else {
                    promise = Promise.reject(error);
                }
                return promise;

            };
        }

        function handleCatalogInfoError(productId) {
            return function(error) {
                ComponentsLogFactory.log('handleCatalogInfoError:', productId, ' error =', error.status);
            };
        }


        function handleCatalogInfoResponse(resolveList) {
            return function(response) {
                var offerId = response.offerId;
                setInfoFromCatalog(offerId, response);
                resolveList[offerId] = catalog[offerId];
                return catalog[offerId];

            };
        }

        function addToResolveList(resolveList, offerId, info) {
            resolveList[offerId] = info;
            return info;

        }

        function getCatalogInfoByList(idList, parentOfferId, forceRetrieveFromServer) {
            var catalogRequestPromises = [],
                resolveList = {},
                promise;


            if (!idList.length) {
                promise = Promise.resolve(resolveList);
            } else {
                for (var i = 0; i < idList.length; i++) {
                    var offerId = idList[i];
                    if (catalog[offerId] && !forceRetrieveFromServer) {
                        catalogRequestPromises.push(Promise.resolve(addToResolveList(resolveList, offerId, catalog[offerId])));
                    } else {
                        catalogRequestPromises.push(
                            Origin.games.catalogInfo(offerId, Origin.locale.locale(), false, parentOfferId)
                            .then(handleCatalogInfoResponse(resolveList), checkForAndRequestPrivateOffer(offerId, parentOfferId, resolveList))
                            .catch(handleCatalogInfoError(offerId))
                        );
                    }
                }
                promise = Promise.all(catalogRequestPromises).then(function() {
                    myEvents.fire('GamesCatalogFactory:catalogUpdated', {
                        signal: 'catalogUpdated',
                        eventObj: resolveList
                    });
                    return resolveList;
                }).catch(function(error) {
                    ComponentsLogFactory.error('[GAMESCATALOG:getCatalogInfoByList]', error.message);
                });
            }
            return promise;

        }

        function ownedDLCFilter(entitlements) {
            return function(item) {
                return entitlements.indexOf(item) > -1;
            };
        }

        function unownedDLCFilter(entitlements) {
            return function(item) {
                return !ownedDLCFilter(entitlements)(item);
            };
        }

        function retrieveExtraContentCatalogInfoPriv(parentOfferId, entitlements, filter) {


            var promise = null,
                offerIds = [];
            //only retrieve if parent offerId exists, otherwise ignore
            if (!catalog[parentOfferId]) {
                promise = Promise.reject(new Error('retrieveExtraContentCatalogInfo: parent catalog not loaded:' + parentOfferId));
            } else {

                if (catalog[parentOfferId].extraContent.length > 0) {
                    offerIds = catalog[parentOfferId].extraContent;

                    if (filter === OWNEDONLY) {
                        offerIds = catalog[parentOfferId].extraContent.filter(ownedDLCFilter(entitlements));

                    } else if (filter === UNOWNEDONLY) {
                        offerIds = catalog[parentOfferId].extraContent.filter(unownedDLCFilter(entitlements));
                    }

                    ComponentsLogFactory.log('retrieving EC for:', parentOfferId, ' [', catalog[parentOfferId].extraContent.length, ']');
                    promise = getCatalogInfoByList(offerIds, parentOfferId);
                } else {
                    ComponentsLogFactory.log('retrieveExtraContentCatalogInfo: parent catalog has no extraContent', parentOfferId);
                    promise = Promise.resolve([]);
                }
            }
            return promise;
        }

        function isBaseGame(catalogObject) {
            return (catalogObject.originDisplayType === 'Full Game' || catalogObject.originDisplayType === 'Full Game Plus Expansion');
        }

        function getExistingCatalogInfo(offerId) {
            if (catalog[offerId]) {
                return catalog[offerId];
            }
            return null;
        }

        return {

            events: myEvents,

            isCriticalCatalogRetrieved: function () {
                return criticalCatalogRetrieved;
            },

            clearCriticalCatalogRetrieved: function () {
                criticalCatalogRetrieved = false;
            },

            retrieveCriticalCatalogInfo: retrieveCriticalCatalogInfoPriv,

            clearCatalog: function() {
                catalog = {};
            },

            getCatalog: function() {
                return catalog;
            },

            /**
             * Get the Catalog information
             * @param {string[]} a list of offerIds
             * @param {string} parentOfferId (optional) - if list is of extra content, provide parentOfferId so we can retrieve private offers if needed
             * @return {Promise} a promise for a list of catalog info
             * @method getCatalogInfo
             */
            getCatalogInfo: getCatalogInfoByList,

            /**
             * Get the Catalog information if it exists
             * @param {string} offerId
             * @return {Object} catalog info if it exists (i.e loaded), otherwise returns null
             * @method getExistingCatalogInfo
             */
            getExistingCatalogInfo: getExistingCatalogInfo,


            /**
             * given a parentOfferId, retrieves all extra content associated with that offer
             * @param {string} parentOfferId
             * @return {Promise<string>}
             * @method retrieveExtraContentCatalogInfo
             */
            retrieveExtraContentCatalogInfo: retrieveExtraContentCatalogInfoPriv,
            /**
             * returns true if the catalog object is a base game
             * @param {object} catalogObject
             * @return {boolean}
             * @method isBaseGame
             */
            isBaseGame:isBaseGame
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesCatalogFactorySingleton(ComponentsLogFactory, ComponentsConfigFactory, AuthFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesCatalogFactory', GamesCatalogFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesCatalogFactory
     * @description
     *
     * GamesCatalogFactory
     */
    angular.module('origin-components')
        .factory('GamesCatalogFactory', GamesCatalogFactorySingleton);
}());
