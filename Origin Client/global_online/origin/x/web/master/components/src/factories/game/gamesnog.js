/**
 * @file game/gamesnog.js
 */

// jshint unused:false
(function() {
    'use strict';

    function GamesNonOriginFactory($timeout, ComponentsLogFactory, ComponentsConfigFactory, ObjectHelperFactory, DialogFactory) {

        // The events handler.
        var myEvents = new Origin.utils.Communicator();
        var initialStateReceivedFlag = false;
        var initialStateReceivedTimeout = 6000;
        var games = {};

        // Default Box Art
        var DEFAULT_BOX_ART = 'packart-placeholder.jpg';

        var defaultPackArt = ComponentsConfigFactory.getImagePath(DEFAULT_BOX_ART);

        // The template used to define the default structure of NOG data.  This template is
        // updated by properties from the Origin client.
        var nonOriginGameTemplate = {
            entitlement: {},
            catalog: {
                originDisplayType: 'Full Game',
                extraContent: {},
                downloadable: false,
                trial: false,
                limitedTrial: false,
                trialLaunchDuration: '',
                oth: false,
                demo: false,
                alpha: false,
                beta: false,
                supperessedOfferIds: [],
                platforms: {},
                countries: {
                    isPurchasable: false
                },
                i18n: {
                    packArtSmall: defaultPackArt,
                    packArtMedium: defaultPackArt,
                    packArtLarge: defaultPackArt
                }
            }
        };

        /**
         * Mark the initial state as having been received.  This could be used to send events
         * to notify any other factories that may be interested.
         */
        function markInitialStateAsReceived() {
            if (!initialStateReceivedFlag) {
                initialStateReceivedFlag = true;
            }
        }

        /**
         * Create an error handler with the specified message.
         * @param {string} message - the messsage to log.
         */
        function logError(message) {
            return function (error) {
                ComponentsLogFactory.error('GamesNonOriginFactory: ' + message);
                console.log(error);
            };
        }

        /**
         * Request a refresh of NOGs from the Origin client, then handle the updated data and any
         * errors.
         */
        function refreshNogs() {
            return Origin.client.games.getNonOriginGames()
                .then(handleNogRefresh)
                .catch(logError("Error refreshing non-Origin games."))
                .then(markInitialStateAsReceived);
        }

        /**
         * Handle updated/refreshed NOGs from the Origin client.
         * @param updatedGames
         */
        function handleNogRefresh(updatedGames) {
            return _.map(updatedGames, updateNog);
        }

        /**
         * Return a boolean indicating whether or not the gameToAdd as a data object satisfies
         * minimum requirements.  Currently this requires both 'catalog' and 'entitlement' sub-objects, and
         * that the 'entitlement' object has an 'offerId'.
         * @param {Object} gameToAdd - The object to validate.
         * @returns {boolean}
         */
        function nogSatisfiesMinimumRequirements(gameToAdd) {
            return (gameToAdd.catalog && gameToAdd.entitlement && gameToAdd.entitlement.offerId) ? true : false;
        }

        /**
         * Update game/stored NOG data for a given Non-Origin game.  The game data contains both
         * catalog and entitlement data.  The data is initialized from a prototype data structure,
         * providing defaults for most attributes, then the NOG data is stored indexed by Offer ID.
         *
         * @param {Object} gameToAdd - The updated game data structure, including both catalog
         *                 and entitlement data.
         * @returns {Promise<{Object}>} A promise that resolves when the NOG is completely updated.
         */
        function updateNog(gameToAdd) {

            // Bail if the game does not satisfy data requirements.
            if (! nogSatisfiesMinimumRequirements(gameToAdd)) {
                return Promise.resolve({});
            }

            var offerId = gameToAdd.entitlement.offerId;

            // If this is a new game,
            if (! games.hasOwnProperty(offerId)) {

                // Populate the game with the template.
                games[offerId] = _.cloneDeep(nonOriginGameTemplate);
            }

            // Update data from the real game to add.
            _.merge(games[offerId], gameToAdd);

            // Prepare to asynchronously update the box art.
            return Origin.client.games.getCustomBoxArtInfo(offerId)
                .then(updateCustomBoxArt(offerId))
                .catch(logError('Failure to get custom box art for ' + offerId))
                .then(sendUpdateEvent);
        }

        /**
         * Create a function to handle the customized box art for an offer.
         *
         * @param {string} offerId - the Offer ID to update
         * @returns {Function} A function taking {@link CustomBoxArtInfo}
         */
        function updateCustomBoxArt(offerId) {
            return function (boxArt) {

                if (games.hasOwnProperty(offerId)) {

                    var encodedBoxArt = boxArt.customizedBoxart;
                    var gameData = games[offerId].catalog.i18n;

                    gameData.packArtLarge = encodedBoxArt;
                    gameData.packArtMedium = encodedBoxArt;
                    gameData.packArtSmall = encodedBoxArt;

                    // TODO: support for cropped box art once ORCORE-1882 is implemented.
                }
            };
        }

        /**
         * Fires an update event, which can be used to notify directives of updated games.
         *
         * NOTE: this is currently not used, but is anticipated to be necessary when generic
         * support for customizing box art is added.
         *
         * @param {string} offerId - The offerId for which to send the update event.
         * @returns {Function}
         */
        function sendUpdateEvent(offerId) {
            return function () {

                // NOTE: this is currently not used, but is anticipated to be necessary when generic
                // support for customizing box art is added.
                myEvents.fire('GamesNonOriginFactory:update', {
                    signal: 'update:' + offerId,
                    eventObj: null
                });
            };
        }

        function initialRefreshCompleted() {
            return initialStateReceivedFlag;
        }

        /**
         * Retrieve catalog data for the specified offer IDs.  For symmetry with GamesCatalogFactory, this
         * function returns a promise, even though it has the data immediately available.
         *
         * @param {string[]} offerIds A list of Offer IDs to query.
         * @returns {Object} List of catalog data for any known offer IDs.
         */
        function getCatalogInfo(offerIds) {
            return _.mapValues(_.pick(games, offerIds), 'catalog');
        }

        /**
         * Determines if the supplied offer ID is a known non-Origin game.
         *
         * @param {string} offerId The Offer ID to look for.
         * @returns {boolean} true if the offer is a NOG.
         */
        function isNonOriginGame(offerId) {
            return games.hasOwnProperty(offerId);
        }

        /**
         * Retrieve All non-Origin game entitlement data.
         *
         * @returns {Array} An array of non-Origin games.
         */
        function baseGameEntitlements() {
            return _.map(games, 'entitlement');
        }

        /**
         * Returns the Offer IDs for all non-Origin games.
         *
         * @returns {List} An array of non-Origin games.
         */
        function baseEntitlementOfferIdList() {
            return Object.keys(games);
        }

        /**
         * Retrieve entitlement data for a single offer.
         *
         * @param {string} offerId - The offer ID to retriefve
         * @returns {null|Object} Any entitlement data for the offer, or null if not found.
         */
        function getEntitlement(offerId) {
            if (_.has(games, offerId)) {
                return games[offerId].entitlement;
            }
            return null;
        }

        /*
         * Initialization section.  Here, we perform whatever actions are necessary upon
         * construction.
         */

        //we fire a message here as received for a standalone browser since we cannot
        //differentiate whether the client is not online or just haven't gone online yet.
        //use timeout only for external browser; for embedded wait for the actual signal from the client
        if (!Origin.client.isEmbeddedBrowser()) {
            $timeout(markInitialStateAsReceived, initialStateReceivedTimeout, false);
        }

        //listen for status changes
        Origin.events.on(Origin.events.CLIENT_GAMES_CHANGED, refreshNogs);
        Origin.events.on(Origin.events.CLIENT_GAMES_BASEGAMESUPDATED, refreshNogs);

        return {

            events: myEvents,
            getCatalogInfo: getCatalogInfo,
            isNonOriginGame: isNonOriginGame,
            baseGameEntitlements: baseGameEntitlements,
            baseEntitlementOfferIdList: baseEntitlementOfferIdList,
            getEntitlement: getEntitlement,
            initialRefreshCompleted: initialRefreshCompleted,
            refreshNogs: refreshNogs
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesNonOriginFactorySingleton($timeout, ComponentsLogFactory, ComponentsConfigFactory, ObjectHelperFactory, DialogFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesNonOriginFactory', GamesNonOriginFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesNogFactory

     * @description
     *
     * GamesNogFactory manages the non-Origin games that are retrieved from the Origin client
     * by the GamesClientFactory.
     */
    angular.module('origin-components')
        .factory('GamesNonOriginFactory', GamesNonOriginFactorySingleton);
}());
// jshint unused:true
