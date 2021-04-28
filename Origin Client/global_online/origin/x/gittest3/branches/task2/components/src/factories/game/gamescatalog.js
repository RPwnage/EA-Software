/**
 * @file game/gamescatalog.js
 */
(function() {
    'use strict';

    function GamesCatalogFactory(ComponentsLogFactory, ComponentsConfigFactory) {
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
            DEFAULT_BOX_ART = 'default-boxart.jpg',
            UNOWNEDONLY = 'UNOWNEDONLY',
            OWNEDONLY = 'OWNEDONLY';
        //self = this; /* you can't use this in a function for jshint */

        /**
         * sets the property name to value if catalog[id] exists
         * @param {String} id - catalog offerId
         * @param {String} name - property name
         * @param {object} value - value of the property
         * @return {void}
         * @method setCatalogProperty
         */
        function setCatalogProperty(id, name, value) {
            //can't use truthy because value of 0 or false is valid
            if (!catalog[id] || value === null || typeof(value) === 'undefined') {
                return;
            }

            catalog[id][name] = value;
        }

        /**
         * determines if the type of object is 'Primary'
         * @param {Object} item
         * @return {boolean} true/false based on type === 'Primary'
         * @method fetchMasterTitleId
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
         * extracts out the masterTitleId from the response if it exists, otherwise returns null
         * @param {Object} response
         * @return {String} masterTitleId or null
         * @method fetchMasterTitleId
         */
        function fetchMasterTitleId(response) {
            var mdmHierarchy = Origin.utils.getProperty(response, ['mdmHierarchies', 'mdmHierarchy']),
                primaryItems,
                primaryItem,
                masterTitleId = null;

            //mdmHeirarchy may not exist (e.g. with extra content), so we need to guard against it
            if (mdmHierarchy) {
                primaryItems = mdmHierarchy.filter(isPrimary);
                if (primaryItems.length) {
                    primaryItem = primaryItems[primaryItems.length - 1]; //use the last one
                    masterTitleId = Origin.utils.getProperty(primaryItem, ['mdmMasterTitle', 'masterTitleId']);
                }
            }
            return masterTitleId;
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
                isPurchasable;

            countryList = Origin.utils.getProperty(response, ['countries', 'country']);
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
            setCatalogProperty(id, 'purchasable', isPurchasable);
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
                catalog[id].platforms[platform] = {platform: platform};
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
                        softwareAttributes.push({
                            platform: platform,
                            multiPlayerId: multiPlayerId,
                            packageType: packageType
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
                platform = null;

            if (softwareAttributes) {
                for (var i = 0; i < softwareAttributes.length; i++) {
                    attribute = softwareAttributes[i];
                    platform = getPlatformObj(id, attribute.platform);
                    platform.multiPlayerId = attribute.multiPlayerId;
                    platform.packageType = attribute.packageType;
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
                    platform = softwareControlObj.softwarePlatform;
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

        /*
         * given a LARS3 catalog response from the server, sets up the cached catalog info catalog[offerId]
         * @param {String} id - offerId
         * @param {Object} response - catalog response from server
         * @method setInfoFromCatalog
         */
        function setInfoFromCatalog(id, response) {
            var publishingAttr = response.publishing.publishingAttributes,
                gameSubType = publishingAttr.gameDistributionSubType;

            catalog[id] = {
                offerId: id,
                packArt: response.customAttributes.imageServer + response.localizableAttributes.packArtLarge,
                displayName: response.localizableAttributes.displayName,
                extraContent: response.ecommerceAttributes.availableExtraContent,
                downloadable: publishingAttr.isDownloadable,
                trial: ((gameSubType === 'Limited Trial') ||
                    (gameSubType === 'Weekend Trial') ||
                    (gameSubType === 'Limited Weekend Trial')),
                limitedTrial: ((gameSubType === 'Limited Trial') ||
                    (gameSubType === 'Limited Weekend Trial')),
                trialLaunchDuration: publishingAttr.trialLaunchDuration || '',
                oth: gameSubType === 'Game Giveaway',
                platforms: {}
                //comment the price out for now as not all catalog info currently has it
                //price: response.countries.DEFAULT.prices.price.price,
                //LARS3 no longer contains platformFacetKey
                //platformFacetKey: response.customAttributes.platformFacetKey
            };

            //if there is no packArt lets use the default boxart
            if(!catalog[id].packArt) {
                catalog[id].packArt = ComponentsConfigFactory.getImagePath(DEFAULT_BOX_ART);
            }

            // extra content offers
            if (response.offerType === 'Extra Content') {
                catalog[id].extraContentType = publishingAttr.originDisplayType;
            }

            setCatalogProperty(id, 'masterTitleId', fetchMasterTitleId(response));
            setPurchasable(id, response);
            setOriginDisplayType(id, response);

            //set platform dependent attributes
            setSoftwareAttributes(id, response);
            setSoftwareControlDates(id, response);
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

        /* set up the cached catalog info from supercat
         * @param {Object} response - supercat
         * @method setInfoFromCriticalCatalog
         */
        function setInfoFromCriticalCatalog(response) {
            var offer = {},
                responseOffer = {},
                platformObj = {},
                responsePlatform = {},
                distributionSubType = '';

            for (var i = 0; i < response.offers.length; i++) {
                if (catalog[response.offers[i].offerId]) {
                    continue;
                }

                offer = {};
                responseOffer = response.offers[i];

                offer.offerId = responseOffer.offerId;
                offer.offerType = responseOffer.offerType;
                offer.packArt = responseOffer.packArt || ComponentsConfigFactory.getImagePath(DEFAULT_BOX_ART);
                offer.displayName = responseOffer.displayName;
                offer.downloadable = (responseOffer.isDownloadable === 'True');

                distributionSubType = responseOffer.gameDistributionSubType;

                offer.trial = ((distributionSubType === 'Limited Trial') ||
                    (distributionSubType === 'Weekend Trial') ||
                    (distributionSubType === 'Limited Weekend Trial'));

                offer.limitedTrial = ((distributionSubType === 'Limited Trial') ||
                    (distributionSubType === 'Limited Weekend Trial'));

                offer.oth = distributionSubType === 'Game Giveaway';

                offer.trialLaunchDuration = responseOffer.trialLaunchDuration || '';
                offer.extraContent = responseOffer.extraContent || [];
                offer.masterTitleId = responseOffer.masterTitleId;
                offer.purchasable = (responseOffer.isPurchasable === 'Y');
                offer.originDisplayType = getModifiedOriginDisplayType(responseOffer.originDisplayType, offer.downloadable);

                offer.platforms = {};
                //platform dependent
                if (responseOffer.platforms) {
                    for (var j = 0; j < responseOffer.platforms.length; j++) {
                        platformObj = {};
                        responsePlatform = responseOffer.platforms[j];
                        platformObj.platform = responsePlatform.platform;
                        platformObj.multiPlayerId = responsePlatform.multiPlayerId;
                        platformObj.packageType = responsePlatform.downloadPackageType;
                        platformObj.releaseDate = getDateFromStr(responsePlatform.releaseDate);
                        platformObj.downloadStartDate = getDateFromStr(responsePlatform.downloadStartDate);
                        platformObj.useEndDate = getDateFromStr(responsePlatform.downloadStartDate);
                        offer.platforms[platformObj.platform] = platformObj;
                    }
                }

                if (offer.offerType === 'Extra Content') {
                    offer.extraContentType = responseOffer.originDisplayType; //set it to the actual displaytype, not the modified one
                }

                catalog[offer.offerId] = offer;
            }
            return catalog;
        }

        function handleCriticalCatalogInfoError(error) {
            ComponentsLogFactory.error('[GAMESCATALOGFACTORY:criticalCatalogInfo]:', error.stack);
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

        function checkForAndRequestPrivateOffer(productId, entitlements, parentOfferId, resolveList) {
            return function(error) {
                var promise = null;

                if (error.status === Origin.defines.http.ERROR_404_NOTFOUND && Origin.auth.isLoggedIn()) {
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

        function getCatalogInfoByList(idList, parentOfferId, entitlements) {
            var catalogRequestPromises = [],
                resolveList = {},
                promise;


            if (!idList.length) {
                promise = Promise.resolve(resolveList);
            } else {
                for (var i = 0; i < idList.length; i++) {
                    var offerId = idList[i];
                    if (catalog[offerId]) {
                        catalogRequestPromises.push(Promise.resolve(addToResolveList(resolveList, offerId, catalog[offerId])));
                    } else {
                        catalogRequestPromises.push(
                            Origin.games.catalogInfo(offerId, Origin.locale.locale(), false, parentOfferId)
                            .then(handleCatalogInfoResponse(resolveList), checkForAndRequestPrivateOffer(offerId, entitlements, parentOfferId, resolveList))
                            .catch(handleCatalogInfoError(offerId))
                        );
                    }
                }
                promise = Promise.all(catalogRequestPromises).then(function() {
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
                    promise = getCatalogInfoByList(offerIds, parentOfferId, entitlements);
                } else {
                    ComponentsLogFactory.log('retrieveExtraContentCatalogInfo: parent catalog has no extraContent', parentOfferId);
                    promise = Promise.resolve([]);
                }
            }
            return promise;
        }

        return {

            events: myEvents,

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
             * given a parentOfferId, retrieves all extra content associated with that offer
             * @param {string} parentOfferId
             * @return {Promise<string>}
             * @method retrieveExtraContentCatalogInfo
             */
            retrieveExtraContentCatalogInfo: retrieveExtraContentCatalogInfoPriv
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesCatalogFactorySingleton(ComponentsLogFactory, ComponentsConfigFactory, SingletonRegistryFactory) {
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