/**
 * @file game/gamescatalog.js
 */
(function() {
    'use strict';

    function GamesCatalogFactory(ComponentsLogFactory, ComponentsConfigFactory, AuthFactory, GameRefiner, GamesEntitlementFactory) {
        var catalog = {},
            path2offerId = {},
            masterTitleId2offerId = {},
            lowestRankedPurchsableBasegames = {},
            myEvents = new Origin.utils.Communicator(),
            criticalCatalogRetrieved = false,
            stateTransitionTimers = {},// Map of timers indexed by Offer ID.
            MAX_INT32 = 0x7FFFFFFF,
            DEFAULT_BOX_ART = 'packart-placeholder.jpg',
            UNOWNEDONLY = 'UNOWNEDONLY',
            OWNEDONLY = 'OWNEDONLY',
            freegamePaths = [],
            NORMAL_SUBTYPE = 'Normal Game';
        var REG_CHARS = new RegExp('&[#0-9a-z]+;', 'gi');
        var REG_SPACES = new RegExp('[^a-z0-9]', 'gi');
        var REG_DASHES = new RegExp('--', 'g');
        var ORIGIN_DISPLAY_TYPES = {
                'Full Game': 1,
                'Full Game Plus Expansion': 1,
                'Expansion': 1,
                'Addon': 1,
                'None': 1,
                'Game Only': 1
            };
        var GAME_TYPE_FACET_KEYS = { 'dlc': 1, 'currency': 1 , 'expansion': 1, 'addon': 1 };
        var PACKART_TYPES = { 'packArtSmall': 1, 'packArtMedium': 1, 'packArtLarge': 1 };
        var GAME_SUBTYPES = GameRefiner.SUBTYPES;


        function defaultBoxArtUrl() {
            return ComponentsConfigFactory.getImagePath(DEFAULT_BOX_ART);
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

        function fireReleaseStateUpdateForOffer(offerId) {

            // Return an anonymous function to fire the event.
            return function() {

                // Fire the event.
                myEvents.fire('GamesCatalogFactory:offerStateChange', {
                    signal: 'releaseStateUpdate:' + offerId,
                    eventObj: {}
                });

                resetCatalogTimingEventsForOffer(offerId)();
            };
        }

        function resetCatalogTimingEventsForOffer(offerId) {
            return function() {
                // Remove the current timer and re-check the offer for any further events.
                delete stateTransitionTimers[offerId];
                queueCatalogTimingEventsForOffer(offerId);
            };
        }

        function queueCatalogTimingEventForOffer(offerId, msecUntilTransition) {
            var fn;
            if (msecUntilTransition < MAX_INT32) {
                fn = fireReleaseStateUpdateForOffer(offerId);
            } else {
                fn = resetCatalogTimingEventsForOffer(offerId);
                msecUntilTransition = MAX_INT32 - 1; // Not sure the -1 is necessary but better to avoid overflow.
            }
            stateTransitionTimers[offerId] = setTimeout(fn, msecUntilTransition);
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
                    if (millisecondsUntilPreloadable > 0) {
                        queueCatalogTimingEventForOffer(offerId, millisecondsUntilPreloadable);
                    }
                }

                // Preloadable to Released
                else if (releaseDate && (currentTime < releaseDate)) {
                    var millisecondsUntilReleased = Math.floor(releaseDate - currentTime);
                    if (millisecondsUntilReleased > 0) {
                        queueCatalogTimingEventForOffer(offerId, millisecondsUntilReleased);
                    }
                }

                // Released to Sunset
                else if (useEndDate && (currentTime < useEndDate)) {
                    var millisecondsUntilSunset = Math.floor(useEndDate - currentTime);
                    if (millisecondsUntilSunset > 0) {
                        queueCatalogTimingEventForOffer(offerId, millisecondsUntilSunset);
                    }
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
            var runOffer,i;    
            var offerIdList = masterTitleId2offerId[response.masterTitleId];

            if(offerIdList) {   // remove the entry for this offerId, the parser will put it in right spot
                for(i=0; i < offerIdList.length; i++){  
                    runOffer = offerIdList[i];
                    if(response.offerId === runOffer.offerId){
                        offerIdList.splice(i,1);               // remove the only entry and done
                        break;
                    }
                }
            }

            catalog[id] = parseOfferSupercatFormat(response);
            queueCatalogTimingEventsForOffer(id);
        }

 
        function getDateFromStr(dateStr) {
            if (_.isString(dateStr)) {
                return new Date(dateStr);
            } else if (_.isObject(dateStr) && dateStr.hasOwnProperty('$date')) {
                return new Date(dateStr.$date);
            }

            return null;
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


        //replace spaces with -
        function hyphenate(pathkey) {
            return pathkey ? pathkey.toLowerCase().replace(REG_CHARS, '').replace(REG_SPACES, '-').replace(REG_DASHES, '-') : pathkey;
        }


       
        function parseOfferSupercatFormat(responseOffer) {
            // This procedure is heavily optimezed for speed - it is executed more than a thousant time on a critical path [the overall loop is 4-6 x faster then with the predecessor]
            var needle, offerIdList, titleId, i, tempIsUpgradeable, offersOnPath, runOffer, states;  // temp holders for a few lines from the first use 
            var responsePlatform = {};
            var offerPlatformsDict = {};
            var gameDistSubType = responseOffer.gameDistributionSubType;

            if(gameDistSubType && GAME_SUBTYPES[gameDistSubType]){
                needle = GAME_SUBTYPES[gameDistSubType];

                responseOffer.oth          = needle.indexOf('oth')           >= 0;
                responseOffer.limitedTrial = needle.indexOf('limited trial') >= 0;
                responseOffer.demo         = needle.indexOf('demo')          >= 0; 
                responseOffer.alpha        = needle.indexOf('alpha')         >= 0;
                responseOffer.beta         = needle.indexOf('beta')          >= 0;
                responseOffer.earlyAccess  = needle.indexOf('early access')  >= 0;

                responseOffer.trial = responseOffer.earlyAccess ? false : (needle.indexOf('trial') >= 0 ? true : responseOffer.type === 'trial' || responseOffer.gameTypeFacetKey === 'TRIAL');
            }
            else {
                responseOffer.limitedTrial = responseOffer.oth = responseOffer.demo = responseOffer.alpha = responseOffer.beta = responseOffer.earlyAccess = false;
                responseOffer.trial = responseOffer.type === 'trial' || responseOffer.gameTypeFacetKey === 'TRIAL';        
            } 

            responseOffer.downloadable = responseOffer.isDownloadable === 'True';   // should change the name

            // set banch of defaults if not present in the feed (someone told me that this is easier to read formatted like this than the original 5 liner :)
            if(!responseOffer.trialLaunchDuration ) {  
                responseOffer.trialLaunchDuration = '';    
            }

            if(!responseOffer.extraContent){  
                responseOffer.extraContent = [];           
            }

            if(!responseOffer.alternateMasterTitleIds) {  
                responseOffer.alternateMasterTitleIds = [];
            }

            if(!responseOffer.suppressedOfferIds) {  
                responseOffer.suppressedOfferIds = [];     
            }

            if(!responseOffer.includeOffers) {  
                responseOffer.includeOffers = [];          
            }

            if(!responseOffer.bundleOffers) {  
                responseOffer.bundleOffers=[];             
            }

            if(!responseOffer.originDisplayType || !ORIGIN_DISPLAY_TYPES[responseOffer.originDisplayType]) { 
                responseOffer.originDisplayType = responseOffer.downloadable ? 'Addon' : 'Game Only'; 
            }

            //platform dependent
            if(responseOffer.platforms && responseOffer.platforms.length > 0) {  // moves things from the array fromat inot a dictionary format with a bit of data corasing
                for (var j = 0; j < responseOffer.platforms.length; j++) {
                    responsePlatform = responseOffer.platforms[j];
                    responsePlatform.releaseDate                  = getDateFromStr(responsePlatform.releaseDate);
                    responsePlatform.downloadStartDate            = getDateFromStr(responsePlatform.downloadStartDate);
                    responsePlatform.useEndDate                   = getDateFromStr(responsePlatform.useEndDate);
                    responsePlatform.originSubscriptionUseEndDate = getDateFromStr(responsePlatform.originSubscriptionUseEndDate);
                    responsePlatform.showSubsSaveGameWarning      = responsePlatform.showSubsSaveGameWarning === 'true';
                    responsePlatform.achievementSet               = responsePlatform.achievementSetOverride;    // why do we rename catalog entries?
                    responsePlatform.packageType                  = responsePlatform.downloadPackageType;
                    offerPlatformsDict[responsePlatform.platform] = responsePlatform;  //temporal map 
                }
                responseOffer.platforms = offerPlatformsDict;
            } 
            else {
                responseOffer.platforms       = {};
                responseOffer.platforms.PCWIN = {platform: 'PCWIN'};
            }
 
            //rating system related
            if(responseOffer.i18n.ratingSystemIcon) { 
                responseOffer.ratingSystemIcon = responseOffer.i18n.ratingSystemIcon; 
            }

            if(!responseOffer.gameRatingDesc) { 
                responseOffer.gameRatingDesc   = []; 
            }

            if(!responseOffer.gameNameFacetKey) { 
                responseOffer.gameNameFacetKey = hyphenate(responseOffer.masterTitle); 
            }

            responseOffer.gameRatingPendingMature         = responseOffer.gameRatingPendingMature === 'true';
            responseOffer.franchiseFacetKey               = hyphenate(responseOffer.franchiseFacetKey);
            responseOffer.gameTypeFacetKey                = hyphenate(responseOffer.gameTypeFacetKey);
            responseOffer.gameEditionType                 = hyphenate(responseOffer.gameEditionTypeFacetKey);
            responseOffer.gameEditionTypeFacetKeyRankDesc = parseInt(responseOffer.gameEditionTypeFacetKeyRankDesc, 10);
            // FA new attribute on the object itself to dierectly support the ranking
            responseOffer.rank = (responseOffer.gameEdditionTypeFacetKeyRankDes) ? responseOffer.gameEdditionTypeFacetKeyRankDes : 0;  

            if(GAME_TYPE_FACET_KEYS[responseOffer.gameTypeFacetKey]) {
                responseOffer.gameEditionTypeFacetKey = responseOffer.i18n.displayName;
                responseOffer.gameEditionType         = hyphenate(responseOffer.i18n.displayName);
            }
            responseOffer.dynamicPricing = responseOffer.dynamicPricing === 'Y';

            if(responseOffer.countries) {
                states = responseOffer.countries;  // to help the optimization compiler/interpreter, and the line size :) 

                states.isPurchasable         = states.isPurchasable ? states.isPurchasable === 'Y' : false;
                responseOffer.isPurchasable  = states.isPurchasable && typeof states.isPublished !== 'undefined' && states.isPublished;
                states.hasSubscriberDiscount = states.hasSubscriberDiscount ? states.hasSubscriberDiscount === 'Y' : false;
                responseOffer.giftable       = states.giftable ? states.giftable === 'Y' : false;
            }
            else {
                responseOffer.countries     = {};
                responseOffer.isPurchasable = responseOffer.giftable =false;  
            } 

            //i18n - simplify
            if(responseOffer.i18n) {
                for(var key in PACKART_TYPES) {                     
                    responseOffer.i18n[key] = getPackArt(responseOffer.i18n[key], responseOffer.imageServer, responseOffer.offerId);
                }
            } 
            else { //at least populate packArt and displayName so they are non-null so people don't have guard against it
                responseOffer.i18n              = {};
                responseOffer.i18n.packArtSmall = responseOffer.i18n.packArtMedium = responseOffer.i18n.packArtLarge = ComponentsConfigFactory.getImagePath(DEFAULT_BOX_ART);
                responseOffer.i18n.displayName  = 'DISPLAY NAME MISSING';
            }
            if(responseOffer.vault && responseOffer.vault.path) {
                responseOffer.vaultInfo     = responseOffer.vault;
                responseOffer.vaultOcdPath  = responseOffer.vaultInfo.path;
                tempIsUpgradeable           = responseOffer.vault.isUpgradeable && responseOffer.vault.isUpgradeable === 'Y';
                responseOffer.vault         = !responseOffer.vault.isUpgradeable || responseOffer.vault.isUpgradeable === 'N';
                responseOffer.isUpgradeable = tempIsUpgradeable;
            }
            else {
                responseOffer.vaultInfo   = {};
                responseOffer.vaultOcdPath = null;
                responseOffer.vault        = responseOffer.isUpgradeable = false;
            }

            if(!responseOffer.durationUnit) { 
                responseOffer.durationUnit = ''; 
            }
          
            responseOffer.isThirdPartyTitle = responseOffer.isThirdPartyTitle === 'true' ? true : false;  
            
            // pop cap hero bundle should be a shell bundle, fixing it here until - service can fix it.
            //console.log("responseOffer.isShellBundle = "+responseOffer.isShellBundle + "    responseOffer.gameNameFacetKey = "+responseOffer.gameNameFacetKey);
            responseOffer.isShellBundle = responseOffer.gameNameFacetKey === 'popcap-hero-bundle'? true : (responseOffer.isShellBundle ? responseOffer.isShellBundle === 'Y' : false );

            if (responseOffer.trial || responseOffer.beta || responseOffer.demo || responseOffer.earlyAccess || responseOffer.limitedTrial || responseOffer.alpha) {

                freegamePaths.push({
                    offerPath: responseOffer.offerPath,
                    offerId: responseOffer.offerId,
                    masterTitleId: responseOffer.masterTitleId,
                    alternateMasterTitleIds: responseOffer.alternateMasterTitleIds
                });
            } 
              
            if(responseOffer.offerPath) {
                offersOnPath = path2offerId[responseOffer.offerPath];
                if(offersOnPath){ 
                    if(responseOffer.isPurchasable) { 
                        offersOnPath.unshift(responseOffer.offerId); 
                    }
                    else { 
                        offersOnPath.push(responseOffer.offerId);    
                    }
                }
                else {
                    path2offerId[responseOffer.offerPath] = [responseOffer.offerId];
                }
            }  

            titleId      = responseOffer.masterTitleId;    // temp variables tempId, offerIdList
            offerIdList = masterTitleId2offerId[titleId];  // the list cannot already have this offerid in (removed ahead when catalog updates are done)

            if(offerIdList) {
                for(i = 0; i < offerIdList.length; i++) {  // i is used after the loop exits
                    runOffer = offerIdList[i];
                    if(responseOffer.rank >= runOffer.rank) { 
                        offerIdList.splice(i, 0, responseOffer); 
                        break;
                    }
                }
                if(i === offerIdList.length) { offerIdList.push(responseOffer); }   // we did not break out of the loop, or the array was empty [this could happen for the canalog updates]
            } 
            else {
                masterTitleId2offerId[titleId] = [responseOffer];
            }

            if((!lowestRankedPurchsableBasegames[titleId] || responseOffer.rank < lowestRankedPurchsableBasegames[titleId].rank) &&
                (responseOffer.isPurchasable && responseOffer.isBaseGame && !responseOffer.alpha && !responseOffer.beta && !responseOffer.demo && !responseOffer.earlyAccess && !responseOffer.trial)) {

                lowestRankedPurchsableBasegames[titleId] = responseOffer;
            }
            return responseOffer;
        }

        /* set up the cached catalog info from supercat
         * @param {Object} response - supercat
         * @method setInfoFromCriticalCatalog
         */

        function setInfoFromCriticalCatalog(response) {  
            for (var i = 0; i < response.offers.length; i++) {
                if (catalog[response.offers[i].offerId]) {
                    continue;
                }

                catalog[response.offers[i].offerId] =  parseOfferSupercatFormat(response.offers[i]);
                queueCatalogTimingEventsForOffer(response.offers[i].offerId);
            }
        }

        function handleCriticalCatalogInfoError(error) {
            //in qtwebkit, error.stack is undefined so send error.message instead
            ComponentsLogFactory.error('[GAMESCATALOGFACTORY:criticalCatalogInfo]:', error);
            criticalCatalogRetrieved = true;

            //not sure if we want to send perf telemetry in this case, but we do want to mark the end
            Origin.performance.endTime('COMPONENTS:supercat');

            myEvents.fire('GamesCatalogFactory:criticalCatalogInfoComplete', {
                signal: 'criticalCatalogInfoComplete',
                eventObj: false
            });
        }


        // add basegame path to trials, demo, batas and alphas (speed optimized)
        function handleFreeGameBaseGames(data) {
            var seenPaths = {};
            var baseGameData = [];  // optimized version of the input for the procedure (no dups and blanks) enhanced with path version to match against

            // if you have to do something at all, first remove the duplicate pahts in the provided data (that shrinks it from 120+ to 18)
            // in the same prepare step compute the shorter "mastertitle"-like path (first part of the path) 
            // then the actuall work is to loop over the free-games and upudate those that are for the given "master title like" paths 

            if (data && data.length >0  && freegamePaths.length > 0) { 
                _.forEach(data,function(path) {
                    var matchingPath;                 
                    if (path && !seenPaths[path]) {
                        seenPaths[path]=true;
                        matchingPath = path.substr(0,path.lastIndexOf('/')+1);
                        if (matchingPath) {
                            baseGameData.push([matchingPath,path]);
                        }
                    }
                });

                _.forEach(freegamePaths, function(model) {
                    _.forEach(baseGameData, function(paths) {
                        if (model.offerPath && model.offerPath.indexOf(paths[0]) !== -1 && catalog[model.offerId]) {
                            catalog[model.offerId].freeBaseGame =  paths[1];
                        }
                    });
                });
            }

            criticalCatalogRetrieved = true; // those flags are semantically wery questionable ... needs deeper analysis
            myEvents.fire('GamesCatalogFactory:criticalCatalogInfoComplete', {
                signal: 'criticalCatalogInfoComplete',
                eventObj: true
            });
        }

        // get basegame path for trials, demos, alphas and betas
        function getFreeGameBaseGames() {
            var freeGameSiblings = [];

            _.forEach(freegamePaths, function(value) {
                if(value.alternateMasterTitleIds && value.alternateMasterTitleIds[0]) {
                    freeGameSiblings.push(getPurchasablePathByMasterTitleId(value.alternateMasterTitleIds[0], true));
                } else {
                    freeGameSiblings.push(getPurchasablePathByMasterTitleId(value.masterTitleId, true));
                }
            });

            Promise.all(freeGameSiblings)
                .then(handleFreeGameBaseGames)
                .catch(handleFreeGameBaseGames(null));
        }

        function handleCriticalCatalogInfoResponse(response) {
            setInfoFromCriticalCatalog(response);
            Origin.performance.endTimeMeasureAndSendTelemetry('COMPONENTS:supercat');
            Origin.performance.sendMeasureTelemetry('PREFETCH:supercat');
            getFreeGameBaseGames();
        }

        function clearSupercatPromise() {
            if(_.get(OriginKernel, ['promises', 'getSupercat'])) {
                OriginKernel.promises.getSupercat = null;
            }
        }

        function retrieveCriticalCatalogInfoPriv() {
            Origin.performance.beginTime('COMPONENTS:supercat');

            if(!OriginKernel.promises.getSupercat) {
                OriginKernel.promises.getSupercat =  Origin.games.getCriticalCatalogInfo(Origin.locale.locale());
            }

            return OriginKernel.promises.getSupercat
                .then(handleCriticalCatalogInfoResponse)
                .catch(handleCriticalCatalogInfoError)
                .then(clearSupercatPromise);
        }

        function checkForAndRequestPrivateOffer(productId, parentOfferId, resolveList) {
            return function(error) {
                var promise = null,
                    isPrivateEligible = (GamesEntitlementFactory.ownsEntitlement(productId) ||
                    (GamesEntitlementFactory.ownsEntitlement(parentOfferId) && GamesEntitlementFactory.getPrivateEntitlements().indexOf(parentOfferId) >= 0));

                //we don't want to even attempt the private call if we don't own the offer or we don't own the parent offer with an 110x entitlement to avoid spamming EC2.
                //EC2 /private API will return HTTP 401 unless the user owns the offer OR the user owns the parent offer with 110x permissions, so there is no point
                //in making the call unless either of these conditions is met.
                if (error.status === Origin.defines.http.ERROR_404_NOTFOUND && AuthFactory.isAppLoggedIn() && isPrivateEligible) {
                    var locale = Origin.locale.locale();
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
                ComponentsLogFactory.sendError('[GamesCatalogFactory:handleCatalogInfoError] offerId:' + productId, error);
            };
        }

        function handleCatalogInfoResponse(resolveList) {
            return function(response) {
                var offerId = response.offerId;
                setInfoFromCatalog(offerId, response);
                resolveList[offerId] = catalog[offerId];
                getFreeGameBaseGames();
                return catalog[offerId];
            };
        }

        function addToResolveList(resolveList, offerId, info) {
            resolveList[offerId] = info;
            return info;

        }

        function getCatalogInfoByList(idList, parentOfferId, forceRetrieveFromServer, updateTimesObject) {
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
                        if(offerId) {
                            var lmd = _.get(updateTimesObject, offerId);
                            catalogRequestPromises.push(
                                Origin.games.catalogInfo(offerId, Origin.locale.locale(), false, parentOfferId, lmd)
                                    .then(handleCatalogInfoResponse(resolveList), checkForAndRequestPrivateOffer(offerId, parentOfferId, resolveList))
                                    .catch(handleCatalogInfoError(offerId))
                            );
                        } else {
                            ComponentsLogFactory.error('[getCatalogInfoByList]: offerId is null');
                            catalogRequestPromises.push(Promise.resolve());
                        }
                    }
                }
                promise = Promise.all(catalogRequestPromises).then(function() {
                    return resolveList;
                }).catch(function(error) {
                    ComponentsLogFactory.error('[GAMESCATALOG:getCatalogInfoByList]', error);
                });
            }
            return promise;

        }

        function handleOfferIdByPathResponse(response) {
            return response.offerId;
        }

        function getOfferIdByPathSynchronous(path) {
            return path2offerId[path];
        }

        function getOfferIdByPath(path, purchasable) {
            var offerId,
                id,
                purchaseOnly = angular.isUndefined(purchasable) ? true : purchasable;

            if (path2offerId[path]) {
                for (var i = 0; i < path2offerId[path].length; i++) {
                    id = path2offerId[path][i];
                    if (!purchaseOnly) {
                        offerId = id;
                        break;
                    } else {
                        //there's two ways that offerId gets put into path2offerId
                        //1. was already loaded in catalog
                        //2. used ML's API to retrieve offerId
                        //for case 1, we can check catalog[id] directly for purchasability
                        //for case 2, ML's API only returns purchasable offers so even if offer isn't loaded into catalog, we can assume purchasability
                        if (!catalog[id]) {  //must have come from ML's API
                            offerId = id;
                            break;
                        } else {
                            if (_.get(catalog, [id, 'isPurchasable'], false)) {
                                offerId = id;
                                break;
                            }
                        }
                    }
                }

                if(offerId) {
                    return Promise.resolve(offerId);
                } else { // if we still don't have an offerId check with the server sometimes preorders change purchasable state.
                    return Origin.games.getOfferIdByPath(path).then(handleOfferIdByPathResponse);
                }

            } else {
                return Origin.games.getOfferIdByPath(path)
                    .then(handleOfferIdByPathResponse);     //JSSDK catches the error
            }
        }

        function getCatalogByOfferId(path) {
            return function(offerId) {
                var promise;
                if (offerId) {
//                    ComponentsLogFactory.log('GamesCatalogFactory:getCatalogByPath - found offerid for:', path, ', offerId=', offerId);
                    promise = getCatalogInfoByList([offerId]);
                } else {
                    ComponentsLogFactory.log('GamesCatalogFactory:getCatalogByPath - offerId not found for', path);
                    promise = Promise.resolve({});
                }
                return promise;
            };
        }

        function handleGetCatalogByPathError(error) {
            ComponentsLogFactory.warn('GamesCatalogFactory:getCatalogByPath - error:', error);
        }

        function getCatalogByPath(path, purchasable) {
            //first convert path to offerid
            return getOfferIdByPath(path, purchasable)
                .then(getCatalogByOfferId(path))
                .catch(handleGetCatalogByPathError);
        }

        function handleCatalogResponseForOfferIdByMasterTitleId(offerId) {
            return function(catalogdata) {
                return catalogdata[offerId].offerPath;
            };
        }

        function facetKeyRankDescSort(offerObj) {
            var rankDesc = Origin.utils.getProperty(offerObj, ['customAttributes', 'gameEditionTypeFacetKeyRankDesc']);
            return rankDesc ? parseInt(offerObj.customAttributes.gameEditionTypeFacetKeyRankDesc, 10) : 0;
        }

        function handleGetBaseGameOfferIdByMasterTitleIdResponse(masterTitleId, returnPath) {
            return function(response) {

                var offerIdByRankDesc,
                    offerId,
                    rank,
                    subtype;

                //nothing in the response, early return (shouldn't really happen tho)
                if (response.length === 0) {
                    return null;
                }

                //response is an array of object {customAttributes:{gameEditionTypeFacetKeyRankDesc:rank}, publishing: {publishingAttributes: gameDistributionSubType: 'Normal Game'}, offerId}
                offerIdByRankDesc = _.sortBy(response,  facetKeyRankDescSort);

                masterTitleId2offerId [masterTitleId] = [];

                //_.sortBy puts it in ascending order
                _.forEachRight(offerIdByRankDesc, function(offerObj) {

                    offerId = _.get(offerObj, 'offerId');
                    rank = parseInt(_.get(offerObj, ['customAttributes', 'gameEditionTypeFacetKeyRankDesc']));
                    subtype = _.get(offerObj, ['publishing', 'publishingAttributes', 'gameDistributionSubType']);

                    // handle empty subtypes, shouldnt happen, but if it does default to normal game
                    if(!subtype) {
                        subtype = NORMAL_SUBTYPE;
                    }

                    if (isNaN(rank)) {
                        rank = 0;
                    }

                    masterTitleId2offerId[masterTitleId].push({offerId:offerId, rank: rank});

                    if(GameRefiner.isNormalGame({'gameDistributionSubType': subtype})) {
                        lowestRankedPurchsableBasegames[masterTitleId] = {offerId:offerId, rank: rank};
                    }
                });

                if (returnPath) {
                    //want the actual path, so need to get the catalog data
                    return getCatalogInfoByList([masterTitleId2offerId[masterTitleId][0].offerId])
                        .then(handleCatalogResponseForOfferIdByMasterTitleId(masterTitleId2offerId[masterTitleId][0].offerId)); //let errors fall thru
                } else {
                    return (masterTitleId2offerId[masterTitleId][0].offerId);
                }
            };
        }

        function handleGetBaseGameOfferIdByMasterTitleIdError() {
            return null;
        }

        function getPurchasableByMasterTitleId(masterTitleId, baseGameOnly, returnPath) {
            if (!masterTitleId2offerId[masterTitleId]) {
                return Origin.games.getBaseGameOfferIdByMasterTitleId(masterTitleId)
                    .then(handleGetBaseGameOfferIdByMasterTitleIdResponse(masterTitleId, returnPath))
                    .catch(handleGetBaseGameOfferIdByMasterTitleIdError);
            } else {
                var offerId,
                    offerIdList = masterTitleId2offerId[masterTitleId],
                    retVal = null;

                for (var i = 0; i < offerIdList.length; i++) {
                    //find the first purchasable title (basegames were preferentially placed at the front of the arraay when mapping was created)
                    offerId = offerIdList[i].offerId;
                    if (catalog[offerId]) {
                        if (catalog[offerId].countries && catalog[offerId].countries.isPurchasable) {
                            if (baseGameOnly && (!GameRefiner.isBaseGame(catalog[offerId]) || GameRefiner.isAlphaTrialDemoOrBeta(catalog[offerId]))) {
                                continue;
                            } else {
                                retVal = offerId;
                                if(returnPath) {
                                    retVal = catalog[offerId].offerPath;
                                    if (retVal) {
                                        break;
                                    }
                                } else {
                                    break;
                                }

                            }
                        }
                    }
                }
                return Promise.resolve(retVal);
            }
        }

        function getPurchasablePathByMasterTitleId(masterTitleId, baseGameOnly) {
            return getPurchasableByMasterTitleId(masterTitleId, baseGameOnly, true);
        }

        function getPurchasableOfferIdByMasterTitleId(masterTitleId, baseGameOnly) {
            return getPurchasableByMasterTitleId(masterTitleId, baseGameOnly, false);
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

        function getOwnedItemIds(cataloglist, ownedECofferIds) {
            var ownedItemIds = [];
            angular.forEach(cataloglist, function(catalog) {
                if (ownedECofferIds.indexOf(catalog.offerId) > -1) {
                    ownedItemIds.push(catalog.itemId);
                }
            });
            return ownedItemIds;
        }

        function filterUnownedByItemIdList(catalogList, ownedECofferIds, ownedItemIds) {
            var filtered = {};

            angular.forEach(catalogList, function(o) {
                if ((ownedECofferIds.indexOf(o.offerId) >= 0) ||
                   (ownedItemIds.indexOf(o.itemId) < 0)) {  //unowed but itemId doesn't match any owned itemId
                    filtered[o.offerId] = o;
                }
            });
            return filtered;
        }

        function filterAnyUnownedByItemId(ownedECofferIds, filter) {
            var filteredUnownedByItemIdList = {},
                ownedItemIds = [];

            return function(response) {
                //response has no unowned so we don't need todo this check, just pass it thru
                if (filter === OWNEDONLY) {
                    return response;
                } else {
                    //if the offer is unowned, need to check and see if we already have an owned offer with the same itemId
                    //get the list of ownedItemIds
                    ownedItemIds = getOwnedItemIds(response, ownedECofferIds);

                    filteredUnownedByItemIdList = filterUnownedByItemIdList(response, ownedECofferIds, ownedItemIds);
                    return filteredUnownedByItemIdList;
                }
            };
        }

        function filterBySuppression(suppressedOffers) {
            var filteredResponse = {};

            return function (response) {
                angular.forEach(response, function(o) {
                    if (suppressedOffers.indexOf(o.offerId) < 0) { //doesn't exist in suppressedOffer list
                        filteredResponse[o.offerId] = o;
                    }
                });
                return filteredResponse;
            };
        }

        function filterUnownedByPurchasableAndPublished(ownedECOfferIds, addOnPreviewAllowed) {
            return function (response) {
                var purchasablePublishedResponse = {};

                angular.forEach(response, function(o) {
                    //if owned, automatically add it
                    if (ownedECOfferIds.indexOf(o.offerId) >= 0) {
                        purchasablePublishedResponse[o.offerId] = o;
                    } else if (o.countries && o.countries.isPurchasable) {
                        //from C++ logic
                        // To show unowned, unpublished offers, the offer must be flagged as store-preview content and the parent must have special permissions.
                        // This must be calculated here instead of ContentConfiguration::purchasable() because the extracontent may have multiple parent base games
                        // and some parents may have special permissions while others do not.
                        if (o.countries.isPublished || addOnPreviewAllowed) {
                            purchasablePublishedResponse[o.offerId] = o;
                        }
                    }
                });
                return purchasablePublishedResponse;
            };
        }

        function retrieveExtraContentCatalogInfoPriv(parentOfferId, extraContentEntitlements, suppressedOffers, addOnPreviewAllowed, filter) {
            var promise = null,
                offerIds = [],
                parentCatalogInfo = catalog[parentOfferId],
                ownedECOfferIdsArray = _.map(extraContentEntitlements, 'offerId');
            //only retrieve if parent offerId exists, otherwise ignore
            if (!parentCatalogInfo) {
                promise = Promise.reject(new Error('retrieveExtraContentCatalogInfo: parent catalog not loaded:' + parentOfferId));
            } else {

                if (parentCatalogInfo.extraContent.length > 0) {
                    offerIds = parentCatalogInfo.extraContent;

                    if (filter === OWNEDONLY) {
                        offerIds = parentCatalogInfo.extraContent.filter(ownedDLCFilter(ownedECOfferIdsArray));

                    } else if (filter === UNOWNEDONLY) {
                        offerIds = parentCatalogInfo.extraContent.filter(unownedDLCFilter(ownedECOfferIdsArray));
                    }

                    promise = getCatalogInfoByList(offerIds, parentOfferId)
                        .then(filterAnyUnownedByItemId(ownedECOfferIdsArray, filter))
                        .then(filterBySuppression(suppressedOffers))
                        .then(filterUnownedByPurchasableAndPublished(ownedECOfferIdsArray, addOnPreviewAllowed));
                } else {
                    promise = Promise.resolve([]);
                }
            }
            return promise;
        }

        function getExistingCatalogInfo(offerId) {
            if (catalog[offerId]) {
                return catalog[offerId];
            }
            return null;
        }

        // return lowest ranked purchsable basegame offer id from list by master title
        function returnLowestRankedPurchsableBasegameOfferId(masterTitleId) {
            return function() {
                return lowestRankedPurchsableBasegames[masterTitleId].offerId;
            };
        }

        function checkMasterTitle(masterTitleId) {
            if (!lowestRankedPurchsableBasegames[masterTitleId]) {
                return Origin.games.getBaseGameOfferIdByMasterTitleId(masterTitleId)
                    .then(handleGetBaseGameOfferIdByMasterTitleIdResponse(masterTitleId, false))
                    .then(returnLowestRankedPurchsableBasegameOfferId(masterTitleId))
                    .catch(handleGetBaseGameOfferIdByMasterTitleIdError);
            } else {
                return Promise.resolve(lowestRankedPurchsableBasegames[masterTitleId].offerId);
            }
        }

        function handleBaseGameResponses(masterTitlelist) {
            return function() {
                // Get the first offerId from lowestRankedPurchsableBasegames that also exists in masterTitlelist
                var foundBaseGame = _.head(_.filter(_.pick(lowestRankedPurchsableBasegames, masterTitlelist), 'offerId'));
                return foundBaseGame.offerId;
            };
        }

        function getLowestRankedPurchsableBasegame(game) {
            var masterTitlelist = [game.masterTitleId],
                promises;

            if(game.alternateMasterTitleIds.length > 0) {
                _.merge(masterTitlelist, game.alternateMasterTitleIds);
            }

            promises = _.map(masterTitlelist, function (masterTitle) {
                return checkMasterTitle(masterTitle);
            });

            return Promise.all(promises)
                    .then(handleBaseGameResponses(masterTitlelist))
                    .catch(handleGetBaseGameOfferIdByMasterTitleIdError);
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
             * Given a path of franchise/game/edition, get the corresponding catalog information
             * @param {string} offer path (franchise/game/edition)
             * @param {boolean} purchasable set to return only offer that is purchasable, defaults to true
             * @return {Promise} a promise for the catalog info
             * @method getCatalogByPath
             */
            getCatalogByPath: getCatalogByPath,

            /**
             * Given a path of franchise/game/edition, get the corresponding offerId
             * @param {string} offer path (franchise/game/edition)
             * @param {boolean} purchasable set to true to return only offer that is purchasable, defaults to true
             * @return {Promise} a promise for the offerId
             * @method getOfferIdByPath
             */
            getOfferIdByPath: getOfferIdByPath,

            /**
             * Given a path of franchise/game/edition, get the corresponding offerId.
             * ******DO NOT USE*************
             * THIS IS A SYNCHRONOUS CALL AND IF THE MAP IS NOT AVAILABLE WE JUST RETURN 'UNDEFINED'
             * THIS WILL BE REMOVED ONCE WE GET THE PATH IN THE ENTITLEMENT
             *
             * @param {string} offer path (franchise/game/edition)
             * @param {boolean} purchasable set to true to return only offer that is purchasable, defaults to true
             * @return {string} the offerId
             * @method getOfferIdByPathSynchronous
             */
            getOfferIdByPathSynchronous: getOfferIdByPathSynchronous,

            /**
             * Given a masterTitleId, returns a promise for the path to a purchasable offer associated with the masterTitleId
             * @param {string} masterTitleId
             * @param {boolean} baseGameOnly set to true if you only want a path for a base game, defaults to false
             * @return {Promise} a promise for the path, null if none found
             * @method getPurchasablePathByMasterTitleId
             */
            getPurchasablePathByMasterTitleId: getPurchasablePathByMasterTitleId,

            /**
             * Given a masterTitleId, returns a promise for the offer id to a purchasable offer associated with the masterTitleId
             * @param {string} masterTitleId
             * @param {boolean} baseGameOnly set to true if you only want a path for a base game, defaults to false
             * @return {Promise} a promise for the offer id, null if none found
             * @method getPurchasableOfferIdByMasterTitleId
             */
            getPurchasableOfferIdByMasterTitleId: getPurchasableOfferIdByMasterTitleId,

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
             * given a masterId, retrieves the offerIds associated with the masterId
             * @param {string} masterId
             * @return {array} list of offerIds
             */
            getOfferIdsFromMasterId: function (masterId) {
                return masterTitleId2offerId[masterId];
            },

            /*
             * returns the url for the default box art
             * @return {string}
             * @method defaultBoxArtUrl
             */
            defaultBoxArtUrl: defaultBoxArtUrl,

            /**
             * given a game model, retrieves the lowest ranked basegames offerId associated with the masterId
             * @param {object} game
             * @return {string} lowest ranked basegames offerId
             */
            getLowestRankedPurchsableBasegame: getLowestRankedPurchsableBasegame,

            /**
             * given a path, return a list of offer ids that map to that path
             * @param {String} path an ocdPath
             * @return {Object} map of offer ids to path
             */
            getPath2OfferIdMap: function(path) {
                return _.get(path2offerId, path, {});
            }
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesCatalogFactorySingleton(ComponentsLogFactory, ComponentsConfigFactory, AuthFactory, GameRefiner, GamesEntitlementFactory, SingletonRegistryFactory) {
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
