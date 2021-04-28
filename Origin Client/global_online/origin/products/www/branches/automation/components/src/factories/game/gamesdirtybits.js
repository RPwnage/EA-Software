/**
 * @file game/gamesdata.js
 */
(function() {
    'use strict';

    var ENTITLEMENT_REFRESH_TIMEOUT_INTERVAL = 2000;

    function GamesDirtybitsFactory(GamesCatalogFactory, GamesEntitlementFactory, ComponentsLogFactory, AuthFactory) {


        var myEvents = new Origin.utils.Communicator(),
            pendingDirtyEvents = [],
            entitlementRefreshTimeout = null;

        function handleDirtyBitsUpdateError(error) {
            ComponentsLogFactory.error('[GAMES DIRTYBITS UPDATE]:', error);
        }

        function doPostRefreshTasks() {
            GamesEntitlementFactory.doPostRefreshTasks(GamesCatalogFactory.getCatalog());
        }

        function fireDirtyBitsEntitlementUpdateEvent() {
            myEvents.fire('GamesDirtybitsFactory:entitlementUpdateEvent');
        }


        function hasValidOfferBaseGameOfferIds(data) {
            return (data && (data.msgType === 'xOfferChange') && (data.xBaseGameOfferIds && data.xBaseGameOfferIds.length));
        }

        function hasValidOfferMasterTitlesIds(data) {
            return (data && (data.msgType === 'xOfferChange') && (data.xMasterTitles && Object.keys(data.xMasterTitles).length));            
        }        

        function notifyCatalogUpdate(offers) {
            if (!_.isEmpty(offers)) {
                //GamesDataFactory will handle emitting signal for each offer in the offers list
                myEvents.fire('GamesDirtybitsFactory:catalogUpdated', {
                    signal: 'catalogUpdated',
                    eventObj: offers
                });
            }
        }

        function onDirtyBitsEntitlementUpdate() {
            // Reset the timeout if it's active and another dirty bits event comes in.
            if (entitlementRefreshTimeout) {
                clearTimeout(entitlementRefreshTimeout);
            }
            entitlementRefreshTimeout = setTimeout(onEntitlementRefreshTimeout, ENTITLEMENT_REFRESH_TIMEOUT_INTERVAL);
        }

        function onEntitlementRefreshTimeout() {
            entitlementRefreshTimeout = null;
            GamesEntitlementFactory.retrieveEntitlements(true)
                .then(GamesCatalogFactory.getCatalogInfo)
                .then(doPostRefreshTasks)
                .then(fireDirtyBitsEntitlementUpdateEvent)
                .catch(handleDirtyBitsUpdateError);
        }


        function onDirtyBitsCatalogUpdate(data) {

            var offerUpdateTimes = {};

            if (hasValidOfferBaseGameOfferIds(data)) {
                offerUpdateTimes = data.xOfferUpdateTimes ? data.xOfferUpdateTimes : {};
                GamesCatalogFactory.getCatalogInfo(data.xBaseGameOfferIds, null, true, offerUpdateTimes)
                    .then(notifyCatalogUpdate)
                    .catch(handleDirtyBitsUpdateError);
            }

            if (hasValidOfferMasterTitlesIds(data)) {
                var masterTitles = data.xMasterTitles;
                offerUpdateTimes = data.xOfferUpdateTimes ? data.xOfferUpdateTimes : {};

                for (var p in masterTitles) {
                    if (masterTitles.hasOwnProperty(p)) {
                        var parentOffer = GamesEntitlementFactory.getBaseGameEntitlementByMasterTitleId(p, GamesCatalogFactory.getCatalog());
                        GamesCatalogFactory.getCatalogInfo(masterTitles[p], parentOffer ? parentOffer.offerId : null, true, offerUpdateTimes)
                            .then(notifyCatalogUpdate)
                            .catch(handleDirtyBitsUpdateError);
                    }
                }
            }

        }

        /**
         * double checks expiration before renewing in case something else renewed the token since kicking off the backoff timer
         * @return {Promise} resolves when login finishes or immediately if the token is not expired
         */
        function relogin() {
            if (Origin.user.isAccessTokenExpired()) {
                return Origin.auth.reloginAccessTokenExpired();
            } else {
                return Promise.resolve();
            }
        }
        /**
         * triggerd from the backoff time out, we only attempt the back off once
         * @param  {Function} callback the callback we trigger off of the dirty bits event
         * @param  {object}   data     the dirty bits data
         */
        function loginAndRetryDirtyBitGameCallback(callback, data) {
            //remove from pending array
            pendingDirtyEvents = _.without(pendingDirtyEvents, data.timeOutHandle);
            data.timeOutHandle = null;

            //make the login call
            relogin()
                .then(_.partial(callback, data))
                .catch(function(error) {
                    ComponentsLogFactory.error('[GamesDirtyBits] error', error);
                });
        }

        /**
         * triggerd from the backoff time out, we only attempt the back off once
         * @param  {Function} callback the callback we trigger off of the dirty bits event
         * @param  {object}   data     the dirty bits data
         */
        function executeDirtyBitGameCallback(callback, data) {
            //remove from pending array
            pendingDirtyEvents = _.without(pendingDirtyEvents, data.timeOutHandle);
            data.timeOutHandle = null;

            callback(data);
        }

        /**
         * wraps the callback so we try to login after a backoff if the token has expired
         * @param  {Function} callback the callback to call
         */
        function reloginWrapper(callback, isEntitlement) {
            var BACKOFFTIME =  60000 *10;
            return function(data) {
                //we only care about entitlement and catalog changes with baseGameOfferIds and masterTitles
                if(isEntitlement || (hasValidOfferBaseGameOfferIds(data) || hasValidOfferMasterTitlesIds(data))) {
                    //see if the token is expired
                    if (Origin.user.isAccessTokenExpired()) {
                        //get a random time between 0 and BACKOFFTIME
                        //if its an entitlement event, and we're expired we want to relogin and run the logic right away
                        var backoff = isEntitlement ? 0 : _.random(0, BACKOFFTIME, false),
                            timeOutHandle;

                        //fire the time out to relogin and then call the callback with data
                        //if the login fails we just lose the dirty bit.
                        if(isEntitlement) {
                            timeOutHandle = setTimeout(_.partial(loginAndRetryDirtyBitGameCallback, callback, data), backoff);
                        } else {
                            //if its catalog we just want to execute the dirty bits game callback after a delay and not attempt a relogin
                            timeOutHandle = setTimeout(_.partial(executeDirtyBitGameCallback, callback, data), backoff);
                        }

                        data.timeOutHandle = timeOutHandle;
                        pendingDirtyEvents.push(timeOutHandle);
                    } else {

                        //if we have a valid authtoken just call the callback right away
                        callback(data);
                    }
                }
            };

        }

        function onAuthChanged(loginObj) {
            if (loginObj && !loginObj.isLoggedIn) {
                pendingDirtyEvents.forEach(function(item) {
                    clearTimeout(item);
                });
                if (entitlementRefreshTimeout) {
                    clearTimeout(entitlementRefreshTimeout);
                }
            }
        }

        //
        // DIRTY BITS END
        //

        //listen for dirty bit updates
        Origin.events.on(Origin.events.DIRTYBITS_ENTITLEMENT, reloginWrapper(onDirtyBitsEntitlementUpdate, true));
        Origin.events.on(Origin.events.DIRTYBITS_CATALOG, reloginWrapper(onDirtyBitsCatalogUpdate));
        AuthFactory.events.on('myauth:change', onAuthChanged);
        return {
            events: myEvents
        };
    }

    function GamesDirtybitsFactorySingleton(GamesCatalogFactory, GamesEntitlementFactory, ComponentsLogFactory, AuthFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesDirtybitsFactory', GamesDirtybitsFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesDirtybitsFactory

     * @description
     *
     * GamesDirtybitsFactory
     */
    angular.module('origin-components')
        .factory('GamesDirtybitsFactory', GamesDirtybitsFactorySingleton);
}());