/**
 * @file models/Violatorbserver.js
 */
(function() {

    'use strict';

    var CALLBACK_DEBOUNCE_MS = 500;

    function ViolatorObserverFactory(ObservableFactory, ObserverFactory, ObjectHelperFactory, ComponentsLogFactory, AuthFactory, GamesDataFactory, GamesClientFactory, GameViolatorFactory, EventsHelperFactory, OgdHeaderViolatorFactory, GametileViolatorFactory) {
        /**
         * Generate a model using a common interface factory, the map will map violator view types
         * to a factory that can generate the correct HTML elements for the model
         * @type {Object}
         */
        var viewTypeRenderers = {
            gametile: GametileViolatorFactory,
            importantinfo: OgdHeaderViolatorFactory,
            myhometrial: OgdHeaderViolatorFactory
        };

        /**
         * Create the model object
         * @param {Object} catalogData the data from catalog for the give offer id
         * @param {Object} violatorData the active violator list
         * @param {string} viewContext the view conext of the violator (eg gametile, importantinfo)
         * @param {Number} inlineCount the number of inline violators to show
         * @param {Object} messages the messages collection from scope
         * @return {Object} view model
         */
        function generateModel(catalogData, violatorData, viewContext, inlineCount, messages) {
            var model = {},
                renderer = viewTypeRenderers[viewContext];

            if (!GameViolatorFactory.isValidViolatorData(violatorData)) {
                return violatorData;
            }

            model.inlineMessages = renderer.getInlineMessages(violatorData, catalogData.offerId, inlineCount);

            if (violatorData.length > inlineCount && messages) {
                var gamemessages = _.isFunction(_.get(messages, 'gamemessages')) ? messages.gamemessages({'%game%': _.get(catalogData, ['i18n', 'displayName'])}) : _.noop,
                    viewmoremessagesctaCallback = _.isFunction(messages.viewmoremessagesctaCallback) ? messages.viewmoremessagesctaCallback : _.noop,
                    closemessagescta = _.get(messages, 'closemessagescta', '');

                model.readmore = renderer.getDialogMessages(violatorData, catalogData.offerId, gamemessages, closemessagescta);
                model.readmoreText = renderer.getReadMoreLinkHtml(violatorData.length, inlineCount, viewmoremessagesctaCallback);
            }

            return model;
        }

        /**
         * Handle errors from the service
         * @param {Error} err the error message
         */
        function handleError(err) {
            ComponentsLogFactory.error('[ViolatorObserverFactory] error', err.stack);
            return Promise.resolve();
        }

        /**
         * Build the view model for the controller
         * @param {string} offerId the offerId to clear
         * @param {string} viewContext the view conext of the violator (eg gametile, importantinfo)
         * @param {Number} inlineCount the number of inline violators to show
         * @param {Object} messages the messages collection from scope
         * @return {Promise.<Object, Error>}
         */
        function getModel(offerId, viewContext, inlineCount, messages) {
            return Promise.all([
                    GamesDataFactory.getCatalogInfo([offerId])
                        .then(ObjectHelperFactory.getProperty(offerId)),
                    GameViolatorFactory.getViolators(offerId, viewContext),
                    viewContext,
                    inlineCount,
                    messages
                ])
                .then(_.spread(generateModel))
                .catch(handleError);
        }

        /**
         * Build an observer for use in the controller
         * @param {string} offerId the offerId to clear
         * @param {string} viewContext the view context of the violator (eg gametile, importantinfo)
         * @param {Number} inlineCount the number of inline violators to show
         * @param {Object} messages the messages collection from the controller's scope
         * @param {Object} scope the current scope
         * @param {Function} callbackFunc the callback function to run
         * @return {Observer}
         */
        function getObserver(offerId, viewContext, inlineCount, messages, scope, callbackFunc) {
            var observable = ObservableFactory.observe([offerId, viewContext, inlineCount, messages]),
                handles;

            /**
             * Call commit in the observable's context to prevent inheriting the event scope frame
             */
            function callbackCommit() {
                observable.commit.apply(observable);
            }

            /*
             * Due to the large number of possible committers, debounce the number of simultaneous
             * event triggers that can regenerate the model
             */
            var debouncedCallbackCommit = _.debounce(callbackCommit, CALLBACK_DEBOUNCE_MS);

            /**
             * Clean up violator event listeners
             */
            function destroyEventListeners() {
                EventsHelperFactory.detachAll(handles);
            }

            /**
             * Handle the client signal for game updates
             * @param  {Object} evt The event data
             */
            function showViolatorsForOffer(evt) {
                if(!_.has(evt, 'signal')) {
                    return;
                }

                // signal looks like update:OFFER_ID or progressupdate:OFFER_ID
                var signal = _.clone(evt.signal),
                    updatedOfferId = signal.replace(/(update|progressupdate):/, '');

                if (updatedOfferId === offerId) {
                    debouncedCallbackCommit();
                }
            }

            handles = [
                GamesDataFactory.events.on(['games', 'catalogUpdated', offerId].join(':'), debouncedCallbackCommit),
                GamesDataFactory.events.on(['games', 'baseGameEntitlementsUpdate'].join(':'), debouncedCallbackCommit),
                GamesDataFactory.events.on(['games', 'update', offerId].join(':'), debouncedCallbackCommit),
                GamesDataFactory.events.on(['game', 'hidden', offerId].join(':'), debouncedCallbackCommit),
                GamesDataFactory.events.on(['game', 'unhidden', offerId].join(':'), debouncedCallbackCommit),
                AuthFactory.events.on(['myauth', 'clientonlinestatechangeinitiated'].join(':'), debouncedCallbackCommit),
                GamesClientFactory.events.on(['GamesClientFactory', 'update'].join(':'), showViolatorsForOffer),
                GameViolatorFactory.events.on(['violator', 'dismiss', offerId].join(':'), debouncedCallbackCommit),
                GameViolatorFactory.events.on(['violator', 'enddateReached', offerId].join(':'), debouncedCallbackCommit)
            ];

            scope.$on('$destroy', destroyEventListeners);

            return ObserverFactory
                .create(observable)
                .addFilter(_.spread(getModel))
                .onUpdate(scope, callbackFunc, ObserverFactory.noDigest);
        }

        return {
            getObserver: getObserver,
            getModel: getModel
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ViolatorObserverFactory

     * @description
     *
     * Helpers to build the violators and overflow zones of the owned game details header
     */
    angular.module('origin-components')
        .factory('ViolatorObserverFactory', ViolatorObserverFactory);
}());
