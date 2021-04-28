/**
 * @file factories/gamelibrary/ogdheaderobserver.js
 */
(function() {

    'use strict';

    var WISHLIST_EVENTS = 'wishlist',
        ADD_OFFER_EVENT = 'addOffer',
        REMOVE_OFFER_EVENT = 'removeOffer',
        PRIVATE_ACCOUNT_HTTP_CODE = '409';

    function WishlistObserverFactory(ObservableFactory, ObserverFactory, WishlistFactory, EventsHelperFactory, GamesDataFactory, GamesClientFactory) {
        /**
         * Create a view model
         * @param  {object} wishlistData the wishlist data
         * @return {Object} the view model
         */
        function generateSuccessfulWishlistModel(data) {
            return {
                offerList: data,
                errorMessage: undefined,
                isPrivate: false
            };
        }

        /**
         * Pass through the error and privacy information
         * @param {String} err the error code to parse
         * @return {Object} the view model
         */
        function generateErrorWishlistModel(err) {
            var errorMessage = _.get(err, 'message'),
                isPrivate = _.includes(errorMessage, PRIVATE_ACCOUNT_HTTP_CODE);

            return {
                offerList: [],
                errorMessage: errorMessage,
                isPrivate: isPrivate
            };
        }

        /**
         * Get the view model
         * @param {string} userId the user Id to observe
         * @return {Promise.<Object, Error>} the view model
         */
        function getModel(userId) {
            var wishlist = WishlistFactory.getWishlist(userId);

            return wishlist.getOfferList()
                .then(generateSuccessfulWishlistModel)
                .catch(generateErrorWishlistModel);
        }

        /**
         * Build an observer for use in the controller
         * @param {string} userId the user Id to observe
         * @param {Object} scope the current scope
         * @param {Function} callback a function to call
         * @return {Observer}
         */
        function getObserver(userId, scope, callback) {
            var observable = ObservableFactory.observe([userId]),
            handles;

            /**
             * Call commit in the observable's context to prevent inheriting the event scope frame
             */
            function callbackCommit() {
                observable.commit.apply(observable);
            }

            handles = [
                WishlistFactory.events.on([WISHLIST_EVENTS, ADD_OFFER_EVENT, userId].join(':'), callbackCommit),
                WishlistFactory.events.on([WISHLIST_EVENTS, REMOVE_OFFER_EVENT, userId].join(':'), callbackCommit),
                GamesDataFactory.events.on(['games', 'baseGameEntitlementsUpdate'].join(':'), callbackCommit),
                GamesDataFactory.events.on(['games', 'extraContentEntitlementsUpdate'].join(':'), callbackCommit),
                GamesClientFactory.events.on(['GamesClientFactory', 'update'].join(':'), callbackCommit)
            ];

            var destroyEventListeners = function() {
                EventsHelperFactory.detachAll(handles);
            };

            scope.$on('$destroy', destroyEventListeners);

            return ObserverFactory
                .create(observable)
                .addFilter(_.spread(getModel))
                .onUpdate(scope, callback);
        }

        return {
            getObserver: getObserver,
            getModel: getModel
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.WishlistObserverFactory
     * @description
     *
     * The view model for wishlist
     */
    angular.module('origin-components')
        .factory('WishlistObserverFactory', WishlistObserverFactory);
}());
