/*
 * Marketing telemetry for Gifting modal
 */

/**
 *  * @file telemetry/custom-events/gifting.js
 */
(function () {
    'use strict';

    var NO_SEARCH_RESULTS_EVENT = 'telemetryGiftingNoSearchResults';
    var SEARCH_USER_TEXT_FIELD_SELECTOR = '.origin-telemetry-gift-friendslist-overlay .origin-telemetry-otkfield-input';
    var SELECTED_FRIEND_SELECTOR = 'origin-telemetry-store-gift-friendtile-selected';
    var OTHER_FRIEND_SELECTED_SELECTOR = 'origin-telemetry-store-gift-friendtile-dimmed';
    var SEARCH_RESULTS_THRESHOLD_LESS_THEN_FIVE = ' - 5'; // If user has 0 - 5 search attemps of no results
    var SEARCH_RESULTS_THRESHOLD_LESS_THEN_TEN = ' - 10'; // If user has 6 - 10 search attemps of no results
    var SEARCH_RESULTS_THRESHOLD_MORE_THEN_TEN = ' > 10'; // If user has 10+ search attemps of no results
    var ATTACH_EVENT_FUNC = 'on';
    var DETACH_EVENT_FUNC = 'off';

    function TelemetryGiftingCustomEventsFactory(TelemetryTrackerUtil, AppCommFactory, TELEMETRY_CONFIG) {

        var noSearchResultsCounter = 0;
        var eventHandlers = {};
        var previouslySelectedFriend = null;

        /**
         * When the user search yields no results
         */
        function onNoSearchResults() {

            var textFieldElement = angular.element(SEARCH_USER_TEXT_FIELD_SELECTOR);

            if (textFieldElement.length && textFieldElement.val()) {
                noSearchResultsCounter++;
            }
        }

        function onNextPageOrClose() {
            if (noSearchResultsCounter > 0) {
                var thresholdLabel;
                if (noSearchResultsCounter < 5) {
                    thresholdLabel = SEARCH_RESULTS_THRESHOLD_LESS_THEN_FIVE;
                } else if (noSearchResultsCounter < 10) {
                    thresholdLabel = SEARCH_RESULTS_THRESHOLD_LESS_THEN_TEN;
                } else {
                    thresholdLabel = SEARCH_RESULTS_THRESHOLD_MORE_THEN_TEN;
                }

                var giftingTelemetryData = getEventFromConfig(NO_SEARCH_RESULTS_EVENT);

                var data = {
                    category: giftingTelemetryData.category,
                    action: giftingTelemetryData.action,
                    label: giftingTelemetryData.label.replace('{threshold}', thresholdLabel)
                };

                TelemetryTrackerUtil.sendTelemetryEvent(data);

            }
        }


        function getEventFromConfig(eventType) {
            return _.get(TELEMETRY_CONFIG, ['stateSpecific', 'giftingTelemetryConfig', eventType]);
        }

        function handleMouseUpEvent(trigger, mouseUpEventData, event) {
            var selectionStatus;

            var parent = angular.element(event.target).parents(trigger);
            var nucleusId = parent.parent().attr('nucleus-id');
            if (parent.hasClass(SELECTED_FRIEND_SELECTOR)) {
                selectionStatus = 'deselect';
                previouslySelectedFriend = nucleusId;
            } else if (parent.hasClass(OTHER_FRIEND_SELECTED_SELECTOR)) {
                selectionStatus = 'select';
                previouslySelectedFriend = nucleusId;
            } else {
                if (previouslySelectedFriend === nucleusId) {
                    selectionStatus = 'reselect';
                    previouslySelectedFriend = nucleusId;
                } else {
                    selectionStatus = 'select';
                    previouslySelectedFriend = nucleusId;
                }
            }

            if (selectionStatus) {
                var data = {
                    category: mouseUpEventData.category,
                    action: mouseUpEventData.action,
                    label: mouseUpEventData.label[selectionStatus]
                };
                TelemetryTrackerUtil.sendTelemetryEvent(data);
            }
        }

        /**
         * Handles blur event for customize message step of gifting
         * @param trigger
         * @param blurEventData
         */
        function handleBlurEvent(trigger, blurEventData) {
            if (!angular.element(trigger).val()) {
                var data = {
                    category: blurEventData.category,
                    action: blurEventData.action,
                    label: blurEventData.label
                };
                TelemetryTrackerUtil.sendTelemetryEvent(data);
            }
        }

        /**
         * Adds/Removes Event handler to/from dom
         * @param configEvents config json
         * @param bodyElement top node to attach events to
         * @param eventFunc event to bind (on/off)
         * @param eventType type of event (click/mouseup)
         * @param eventHandler function to handle the event
         */
        function addEventHandler(configEvents, bodyElement, eventFunc, eventType, eventHandler) {

            _.forEach(configEvents, function (configEvent) {
                var triggers = _.get(configEvent, 'trigger');
                if (triggers) {
                    _.forEach(triggers, function (trigger) {
                        if (eventFunc === ATTACH_EVENT_FUNC) {
                            eventHandlers[trigger] = _.partial(eventHandler, trigger, configEvent);
                        }
                        bodyElement[eventFunc](eventType, trigger, eventHandlers[trigger]);
                    });
                }
            });
        }

        /**
         * Sends page view events and resets page specific variables
         * @param pageViewEvent
         */
        function sendPageViewEvent(pageViewEvent) {
            TelemetryTrackerUtil.sendPageViewEvent(pageViewEvent);
            noSearchResultsCounter = 0; //reset counter to avoid sending extra telemetry events
            previouslySelectedFriend = null;

        }

        /**
         * Invokes pageViews, blurEvent, mouseup & click events
         * @param eventFunc
         */
        function invokeEvent(eventFunc) {
            Origin.telemetry.events[eventFunc](NO_SEARCH_RESULTS_EVENT, onNoSearchResults);

            var pageViewEvents = getEventFromConfig('pageViewEvent');
            _.forEach(pageViewEvents, function (pageViewEvent, pageViewEventKey) {
                if (eventFunc === ATTACH_EVENT_FUNC) {
                    eventHandlers[pageViewEventKey] = _.partial(sendPageViewEvent, pageViewEvent);
                }
                Origin.telemetry.events[eventFunc](pageViewEventKey, eventHandlers[pageViewEventKey]);
            });

            var body = angular.element('body');
            var mouseUpEvents = getEventFromConfig('mouseUpCustomEvent');
            var blurEvents = getEventFromConfig('blurEvent');
            addEventHandler(mouseUpEvents, body, eventFunc, 'mouseup', handleMouseUpEvent);
            AppCommFactory.events[eventFunc]('dialog:hide', onNextPageOrClose);
            addEventHandler(blurEvents, body, eventFunc, 'blur', handleBlurEvent);
        }

        /**
         * Bind all events
         * Register all events here
         * Called by TelemetryTracker when route changes
         */
        function bindEvents() {
            invokeEvent(ATTACH_EVENT_FUNC);
        }

        /**
         * Unbind all events
         * Un-register all events here
         * Called by TelemetryTracker when route changes
         */
        function unbindEvents() {
            invokeEvent(DETACH_EVENT_FUNC);
            previouslySelectedFriend = null;
            noSearchResultsCounter = 0;
            eventHandlers = {};
        }

        return {
            bindEvents: bindEvents,
            unbindEvents: unbindEvents
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.TelemetryGiftingCustomEventsFactory
     * @description
     *
     * TelemetryUserStatisticsCustomEventsFactory
     */
    angular.module('originApp')
        .factory('TelemetryGiftingCustomEventsFactory', TelemetryGiftingCustomEventsFactory);
}());
