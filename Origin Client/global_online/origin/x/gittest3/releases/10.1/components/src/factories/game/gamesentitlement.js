/**
 * @file game/gamesentitlement.js
 */
(function() {
    'use strict';

    function GamesEntitlementFactory(DialogFactory, ComponentsLogFactory) {
        //events triggered from this service:
        //  games:baseGameEntitlementsUpdate

        //type of entitlement
        /*jshint unused: true */
        var OriginPermissionsNormal = '0',
            OriginPermissions1102 = '1',
            OriginPermissions1103 = '2';
        /*jshint unused: false */

        /**
         * @typedef uniqueEntObject
         * @type {object}
         * @property {String} offerId
         * @property {Date} grantDate
         */
        /** an array of uniqueEntObjects
         */
        var uniqueEntitlements = [],
            extraContentEntitlements = {}, //list of entitlements indexed by masterTitleId
            initialRefreshFlag = false,
            myEvents = new Origin.utils.Communicator();


        function getEntitlement(offerId) {
            var ent;
            for (var i = 0; i < uniqueEntitlements.length; i++) {
                if (offerId === uniqueEntitlements[i].offerId) {
                    ent = uniqueEntitlements[i];
                    break;
                }
            }
            return ent;
        }

        function entitlementByMasterTitleId(masterTitleId, catalog) {
            //TODO: we need to add additional logic here so that if we have multiple offerids that we own associated with the mastertitle
            //we choose the one based off of the correct priority like we do in the client side RTP code
            var offer = null;
            for (var i = 0; i < uniqueEntitlements.length; i++) {
                var catalogInfo = catalog[uniqueEntitlements[i].offerId];
                if (catalogInfo && catalogInfo.masterTitleId && (Number(masterTitleId) === catalogInfo.masterTitleId)) {
                    offer = uniqueEntitlements[i].offerId;

                    break;
                }
            }
            return offer;
        }

        function handleEntitlementSuccess(response) {
            uniqueEntitlements = [];

            //for now, filter only those that are downloadable
            var len = response.length,
                i, ent;
            for (i = 0; i < len; i++) {
                ent = response[i];
                ent.productId = ent.offerId;
                if (ent.entitlementTag === 'ORIGIN_DOWNLOAD' && ent.productId !== 'undefined' && ent.entitlementId !== 'undefined') {
                    //see if it already exists
                    if (getEntitlement(ent.productId)) { //found one
                        if (ent.status === 'DELETED') { //this entitlement should be deleted
                            // GamesManager.baseGameEntitlements.splice(ndx, 1);
                            //TODO: emit signal to indicate entitlement has been deleted
                        } else {
                            // GamesManager.updateEntitlement(ndx, ent);
                        }
                    } else if (ent.status !== 'DELETED') {
                        uniqueEntitlements.push({
                            offerId: ent.productId,
                            grantDate: ent.grantDate ? new Date(ent.grantDate) : null,
                            terminationDate: ent.terminationDate ? new Date(ent.terminationDate) : null
                        });
                        //TODO: emit signal to indicate new entitlement
                    }
                }
            }
            initialRefreshFlag = true;
            myEvents.fire('GamesEntitlementFactory:baseGameEntitlementsUpdate', {
                signal: 'baseGameEntitlementsUpdate',
                eventObj: uniqueEntitlements
            });
        }

        function handleEntitlementError(error) {
            DialogFactory.openAlert({
                id: 'web-entitlement-server-down-warning',
                title: 'Entitlement server is down.',
                description: 'Not all elements of the page will load. Please refresh the page. If this happens for an extended period of time please email originxcore@ea.com',
                rejectText: 'OK'
            });
            initialRefreshFlag = true;
            ComponentsLogFactory.log('getBaseGameInfo: error', error);
            myEvents.fire('GamesEntitlementFactory:baseGameEntitlementsError', {
                signal: 'baseGameEntitlementsError',
                eventObj: []
            });
        }

        function catchEntitlementError(error) {
            initialRefreshFlag = true;
            ComponentsLogFactory.error('[GAMESENTITLEMENTFACTORY:baseGameEntitlements]', error.stack);
        }

        //force the call to go out to the serve to retrieve entitlements
        function retrieveBaseGameInfo() {
            ComponentsLogFactory.log('GamesEntitlementFactory: retrieveBaseGameInfo');

            Origin.games.baseGameEntitlements(true)
                .then(handleEntitlementSuccess, handleEntitlementError)
                .catch(catchEntitlementError);
        }

        function retrieveExtraContentEntitlements(catalog) {

            var extraContentEntitlementRequests = [];

            function handleExtraContentEntitlmentResponse(responseObj) {
                if (responseObj.masterTitleId) {
                    extraContentEntitlements[responseObj.masterTitleId] = responseObj.entitlements;
                }
                return responseObj.entitlements;
            }

            function handleExtraContentEntitlementError(errorObj) {
                ComponentsLogFactory.error('handleExtraContentEntitlementError:', errorObj.masterTitleId, errorObj.stack);
                return [];
            }

            //clear previously retrieved
            extraContentEntitlements = {}; //list of entitlements indexed by masterTitleId

            //iterate thru each entitlement, and extract the masterTitleId from catalog associated with entitlement
            //pass that masterTitleId to retrieve the owned extra content entitlement associated with the masterTitleId
            for (var i = 0; i < uniqueEntitlements.length; i++) {
                var entOfferId = uniqueEntitlements[i].offerId;
                if (catalog[entOfferId] && catalog[entOfferId].extraContent.length > 0) {
                    //check and make sure we don't already have it for this masterTitleId
                    if (catalog[entOfferId].masterTitleId) {
                        var masterTitleId = catalog[entOfferId].masterTitleId;
                        //make sure extra content exists
                        if (!extraContentEntitlements[masterTitleId]) {
                            extraContentEntitlementRequests.push(Origin.games.extraContentEntitlements(masterTitleId, true)
                                .then(handleExtraContentEntitlmentResponse, handleExtraContentEntitlementError));
                        }
                    } else {
                        ComponentsLogFactory.warn('retrieveAllExtraContent - missing masterTitleId:', entOfferId);
                    }
                } else {
                    if (!catalog[entOfferId]) {
                        ComponentsLogFactory.warn('retrieveAllExtraContent - catalog not loaded for:', entOfferId);
                    } else {
                        ComponentsLogFactory.warn('retrieveAllExtraContent - no extra content for:', entOfferId);
                    }
                }
            }

            return Promise.all(extraContentEntitlementRequests).then(function() {
                return extraContentEntitlements;
            });

        }

        function isPrivate(entitlement) {
            return entitlement && (entitlement.originPermissions === OriginPermissions1102 || entitlement.originPermissions === OriginPermissions1103);
        }

        function getPrivateEntitlementsPriv() {
            return extractOfferIds(uniqueEntitlements.filter(isPrivate));
        }

        function baseEntitlementOfferIdList() {
            return extractOfferIds(uniqueEntitlements);
        }

        function extractOfferIds(entitlements) {
            return entitlements.map(function (ent) {
                return ent.offerId;
            });
        }

        function offerExists(offerId) {
            return function(ent) {
                return (ent.offerId === offerId);
            };
        }

        function ownsEntitlementPriv(offerId) {
            var foundEnt = uniqueEntitlements.filter(offerExists(offerId));
            return (typeof foundEnt !== 'undefined' && foundEnt.length > 0);
        }

        function nonOriginGamePriv(offerId) {
            //for now always return false
            return false;
        }

        function clearEntitlements() {
            uniqueEntitlements = [];
            initialRefreshFlag = false;

            //signal that we've modified the entitlements
            myEvents.fire('GamesEntitlementFactory:baseGameEntitlementsUpdate', {
                signal: 'baseGameEntitlementsUpdate',
                eventObj: uniqueEntitlements
            });
            ComponentsLogFactory.log('GAMESENTITLEMENTFACTORY: fired event GamesEntitlementFactory:baseGameEntitlementsUpdate from clearEntitlements');
        }

        return {

            events: myEvents,

            clearEntitlements: clearEntitlements,

            /**
             * returns true if the initial base game entitlement request completed
             * @return {boolean}
             * @method initialRefreshCompleted
             */
            initialRefreshCompleted: function() {
                ComponentsLogFactory.log('GamesEntitlementFactory: initialRefreshCompleted', initialRefreshFlag);
                return initialRefreshFlag;
            },

            /**
             * sets initialRefreshFlag
             * @method initialRefreshCompleted
             */
            setInitialRefreshFlag: function(boolval) {
                initialRefreshFlag = boolval;
            },

            /**
             * return the cached unique base game entitlements
             * @return {string[]}
             * @method baseGameEntitlements
             */
            baseGameEntitlements: function() {
                return uniqueEntitlements;
            },


            /**
             * returns just the list of offerIds for each entitlement
             * @return {string[]}
             * @method baseEntitlementOfferIdList
             */
            baseEntitlementOfferIdList: baseEntitlementOfferIdList,
            /**
             * force retrieve the base game entitlements from the
             * @return {promise<string[]>}
             * @method refreshEntitlements
             */
            refreshEntitlements: retrieveBaseGameInfo,

            /**
             * given a master title id we return an owned base game offer, return null if no offer
             * @return {string}
             * @method checkEntitlementByMasterTitleId
             */
            getEntitlementByMasterTitleId: entitlementByMasterTitleId,

            /**
             * given an offerId, returns the entitlement object, otherwise it returns NULL
             * @return {string}
             * @method getEntitlement
             */
            getEntitlement: getEntitlement,

            /**
             * force retrieve from server ALL the extra content entitlements associated with each offer in the user's entitlement list
             * assumes base game entitlements and associated catalog info have already been retrieved
             * @return {promise<string[]>}
             * @method refreshExtraContentEntitlements
             */
            refreshExtraContentEntitlements: retrieveExtraContentEntitlements,

            /**
             * given a masterTitleId, returns all OWNED extra content entitlements
             * @param {string} masterTitleId
             * @return {string[]} a list of owned entitlements
             */
            getExtraContentEntitlementByMasterTitleId: function(masterTitleId) {
                return extraContentEntitlements[masterTitleId];
            },

            /**
             * returns an array of private entitlements
             * @return {string[]}
             * @method getPrivateEntitlements
             */
            getPrivateEntitlements: getPrivateEntitlementsPriv,

            /**
             * returns true if an entitlement exists for the offer
             * @param {string} offerId
             * @return {boolean}
             * @method ownsEntitlement
             */
            ownsEntitlement: ownsEntitlementPriv,

            /**
             * returns true if an offer is a non-Origin game
             * @param {string} offerId
             * @return {boolean}
             * @method nonOriginGame
             */
            nonOriginGame: nonOriginGamePriv
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesEntitlementFactorySingleton(DialogFactory, ComponentsLogFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesEntitlementFactory', GamesEntitlementFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesEntitlementFactory

     * @description
     *
     * GamesEntitlementFactory
     */
    angular.module('origin-components')
        .factory('GamesEntitlementFactory', GamesEntitlementFactorySingleton);
}());