/**
 * @file factories/gamelibrary/ogdheaderobserver.js
 */
(function() {

    'use strict';

    function OgdHeaderObserverFactory(GamesDataFactory, ObjectHelperFactory, OcdHelperFactory, ComponentsLogFactory, UrlHelper, ObserverFactory, ObservableFactory, BeaconFactory, SubscriptionFactory, EntitlementStateRefiner, EventsHelperFactory) {
        /**
         * Get a field from ocd, catalog or defaults
         * @param {String} ocdField the OCD field name
         * @param {String} catalogField the catalog field name as a fallback
         * @param {Object} ocdInfo the ocd data collection
         * @param {Object} catalogData the catalog data collection
         * @param {Object} targetDirective the context namespace to use for loc factory
         */
        function getFrom(ocdField, catalogField, ocdInfo, catalogData) {
            if (_.has(ocdInfo, ocdField)) {
                return _.get(ocdInfo, ocdField);
            } else if (_.has(catalogData, catalogField)) {
                return _.get(catalogData, catalogField);
            }
        }

        /**
         * Partially applied chooser that will eventually fetch the requested arguments from their respective documents
         * @param {String} ocdField the OCD field name
         * @param {String} catalogField the catalog field name as a fallback
         * @return {Function}
         */
        function choose(ocdField, catalogField) {
            return _.partial(getFrom, ocdField, catalogField, _, _);
        }

        /**
         * Handle an error if the promise is rejected
         * @param {Exception} err the error message
         * @return {Promise<Data, Error>}
         */
        function handleError(err) {
            ComponentsLogFactory.error('Could not retrieve OGD header model', err);
            return Promise.resolve();
        }

        /**
         * The header view variables cascade from ocd > catalog
         * @type {Object}
         */
        var headerMap = {
            backgroundVideoId: choose(['backgroundvideoid']),
            backgroundImage: choose(['backgroundimage']),
            gameLogo: choose(['gamelogo']),
            gamePremiumLogo: choose(['gamepremiumlogo']),
            gameName: choose(['gamename'], ['i18n', 'displayName']),
            packartLarge: choose(undefined, ['i18n', 'packArtLarge'])
        };

        /**
         * Fetch values from the map
         * @param  {Object} ocdInfo the gamehub document from OCD
         * @param  {Object} catalogInfo the catalog data from EADP
         * @param {Object} entitlementInfo  the entitlement info
         * @param {Object} clientInfo the client info
         * @param {Boolean} isvalidPlatform the user is on an Origin enabled platform
         * @param {Boolean} isActiveSubscription the user has an active subscription
         * @param {Boolean} isTrialExpired is the user's trial expired
         * @return {Object} the view model for OGD content
         */
        function generateHeaderViewModel(ocdInfo, catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription, isTrialExpired, targetDirective, isPlayingGame) {
            var model = {};

            _.forEach(headerMap, function(value, key) {
                model[key] = value.apply(undefined, [ocdInfo, catalogInfo, targetDirective]);
            });

            model.isNog = GamesDataFactory.isNonOriginGame(catalogInfo.offerId);
            model.accessLogo = GamesDataFactory.isSubscriptionEntitlement(catalogInfo.offerId);
            model.isGameActionable = EntitlementStateRefiner.isGameActionable(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription, isTrialExpired, model.isNog, GamesDataFactory.isPrivate(entitlementInfo));
            model.isPlayingGame = isPlayingGame;

            return model;
        }

        /**
         * Get the view model
         * @param  {String} offerId         the offer ID to use
         * @param  {String} targetDirective the target directive name for the OGD header
         * @return {Object} the view model
         */
        function getModel(offerId, targetDirective) {
            return Promise.all([
                    GamesDataFactory.getOcdByOfferId(offerId)
                        .then(OcdHelperFactory.findDirectives(targetDirective))
                        .then(OcdHelperFactory.flattenDirectives(targetDirective))
                        .then(_.head),
                    GamesDataFactory.getCatalogInfo([offerId])
                        .then(ObjectHelperFactory.getProperty(offerId)),
                    GamesDataFactory.getEntitlement(offerId),
                    GamesDataFactory.getClientGamePromise(offerId),
                    BeaconFactory.isInstallable(),
                    SubscriptionFactory.isActive(),
                    GamesDataFactory.isTrialExpired(offerId),
                    targetDirective,
                    Origin.client.games.isGamePlaying()                    
                ])
                .then(_.spread(generateHeaderViewModel))
                .catch(handleError);
        }

        /**
         * Build an observer for use in the controller
         * @param {string} offerId the offerId to clear
         * @param {string} targetDirective the context namespace
         * @param {Object} scope the current scope
         * @param {string} bindTo the scope variable name to bind to
         * @return {Observer}
         */
        function getObserver(offerId, targetDirective, scope, bindTo) {
            var observable = ObservableFactory.observe([offerId, targetDirective]),
            handles;

            /**
             * Call commit in the observable's context to prevent inheriting the event scope frame
             */
            function callbackCommit() {
                observable.commit.apply(observable);
            }

            handles = [
                GamesDataFactory.events.on(['games', 'catalogUpdated', offerId].join(':'), callbackCommit),
                GamesDataFactory.events.on(['games', 'startedPlaying'].join(':'), callbackCommit),
                GamesDataFactory.events.on(['games', 'finishedPlaying'].join(':'), callbackCommit)
            ];

            var destroyEventListeners = function() {
                EventsHelperFactory.detachAll(handles);
            };

            scope.$on('$destroy', destroyEventListeners);

            return ObserverFactory
                .create(observable)
                .addFilter(_.spread(getModel))
                .attachToScope(scope, bindTo);
        }

        return {
            getObserver: getObserver,
            getModel: getModel
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.OgdHeaderObserverFactory
     * @description
     *
     * The view model for ogd header
     */
    angular.module('origin-components')
        .factory('OgdHeaderObserverFactory', OgdHeaderObserverFactory);
}());
