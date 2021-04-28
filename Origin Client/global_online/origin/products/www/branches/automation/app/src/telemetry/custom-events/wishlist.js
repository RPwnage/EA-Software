/*
 * Marketing telemetry for Wishlist
 */

/**
 *  * @file telemetry/custom-events/wishlist.js
 */
(function () {
    'use strict';

    var ATTACH_EVENT_FUNC = 'on';
    var DETACH_EVENT_FUNC = 'off';

    function TelemetryWishlistCustomEventsFactory($state, TelemetryTrackerUtil, TelemetryTrackerParser, TELEMETRY_CONFIG) {

        //list of eventHandlers to detach events correctly.
        var eventHandlers = {};

        /**
         * Handles an event.
         * @param event
         * @param eventDataConfig
         */
        function handleEvent(event, eventDataConfig) {
            var configTelemetryEventData =_.get(eventDataConfig, ['telemetryEventData']);
            var telemetryEventData = {
                category : configTelemetryEventData.category,
                action: _.get(configTelemetryEventData, ['action', $state.current.name], configTelemetryEventData.action)
            };
            if (TelemetryTrackerParser.isToken(configTelemetryEventData.label)) {
                var offerId = angular.element(event.target).parents('.origin-telemetry-profile-wishlist-gametile').data('telemetry-offer-id');
                if (offerId) {
                    telemetryEventData.label = offerId;
                    TelemetryTrackerUtil.sendTelemetryEvent(telemetryEventData);
                }
            } else {
                telemetryEventData.label = configTelemetryEventData.label;
                TelemetryTrackerUtil.sendTelemetryEvent(telemetryEventData);
            }
        }

        /**
         *
         * @param contentElement top level element to attach the listener to
         * @param eventFunc attach (on)/ detach (off) from dom
         * @param clickEventData data from json
         * @param eventType click / mouse up
         */
        function invokeEventHandler(contentElement, eventFunc, eventType, clickEventData) {
            //get dom element to attach the listener to from config
            var triggers = _.get(clickEventData, 'trigger');
            triggers = !_.isArray(triggers) ? [triggers] : triggers;
            _.forEach(triggers, function (trigger) {
                if (eventFunc === ATTACH_EVENT_FUNC) {
                    eventHandlers[trigger] = _.partialRight(handleEvent, clickEventData);
                }
                contentElement[eventFunc](eventType, trigger, eventHandlers[trigger]);
            });
        }


        /**
         * Loads event specific config from TELEMETRY_CONFIG
         * @param eventType (mouseup/click)
         * @returns {*}
         */
        function getEventConfig(eventType) {
            return _.get(TELEMETRY_CONFIG, ['stateSpecific', 'wishlistTelemetryConfig', eventType]);
        }


        /**
         * Attaches/detaches event handlers
         * @param eventFunc
         */
        function manageEvents(eventFunc) {
            var clickEvents = getEventConfig('clickEvents');
            var mouseUpEvents = getEventConfig('mouseUpCustomEvent');
            var contentElement = angular.element('body');
            var clickHandler = _.partial(invokeEventHandler, contentElement, eventFunc, 'click');
            var mouseUpHandler = _.partial(invokeEventHandler, contentElement, eventFunc, 'mouseup');

            _.forEach(clickEvents, clickHandler);
            _.forEach(mouseUpEvents, mouseUpHandler);
        }

        /**
         * Bind all events
         * Register all events here
         * Called by TelemetryTracker when route changes
         */
        function bindEvents() {
            manageEvents(ATTACH_EVENT_FUNC);
        }

        /**
         * Unbind all events
         * Un-register all events here
         * Called by TelemetryTracker when route changes
         */
        function unbindEvents() {
            manageEvents(DETACH_EVENT_FUNC);
            eventHandlers = {};
        }

        return {
            bindEvents: bindEvents,
            unbindEvents: unbindEvents
        };
    }


    /**
     * @ngdoc service
     * @name origin-components.factories.TelemetryWishlistCustomEventsFactory
     * @description
     *
     * TelemetryUserStatisticsCustomEventsFactory
     */
    angular.module('originApp')
        .factory('TelemetryWishlistCustomEventsFactory', TelemetryWishlistCustomEventsFactory);
}());

