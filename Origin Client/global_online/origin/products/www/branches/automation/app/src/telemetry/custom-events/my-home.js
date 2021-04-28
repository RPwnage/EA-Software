/**
 * Factory for custom telemetry tracking events on My Home
 * File gets consumed by /app/src/telemetry/telemetry-tracker.js and is configured in /tools/config/telemetry/my-home.json
 * @file telemetry/custom-events/my-home.js
 */
(function () {
    'use strict';

    function TelemetryMyHomeCustomEventsFactory(TelemetryTrackerUtil, AppCommFactory) {

        var EVENT_TAKEOVER_IMPRESSION = 'originTakeoverVisible';

        var STATE_LOGGED_IN = 'app.home_loggedin';
        var STATE_HOME = 'app.home_home';

        var ATTRIBUTE_CTID = 'ctid';
        var ATTRIBUTE_IMAGE = 'image';
        var ATTRIBUTE_KEY_MESSAGE = 'key-message';
        var ATTRIBUTE_OCD_PATH = 'ocd-path';
        var ATTRIBUTE_TITLE_STR = 'title-str';

        var CATEGORY_MY_HOME = 'My Home';
        var ACTION_IMPRESSION = 'Impression';
        var ACTION_PAGE_SCROLL = 'Page Scroll';

        var VIDEO_STARTED = 'Video Play - Start',
            VIDEO_ENDED = 'Video Play - End';

        var scrollPercentageStateChange;

        /**
         * Send GA/PIN event when user starts or stops a video on Home Page
         * Category: 'My Home'
         * Action: 'Video Play - Start' or 'Video Play - End'
         * Label: 'Video Id'
         * Value: 'Time Played in seconds (only in case of video end event)
         * AdditionalPinParams: Pin telemetry data object
         */
        function sendStartStopVideoTelemetryEvent(data) {
            var eventData = {
                category: CATEGORY_MY_HOME,
                action: (data.playedTime)? VIDEO_ENDED : VIDEO_STARTED,
                label: data.videoId,
                additionalPinParams: data.additionalPinEvents // needed for PIN telemetry
            };

            //GA error out with empty value so we need the following check
            if(data.playedTime) {
                eventData.value = parseInt(data.playedTime);
            }

            TelemetryTrackerUtil.sendTelemetryEvent(eventData, true); // true is for "includePinEvent" param
        }

        /**
         * Bind all events
         * Register all events here
         * Called by TelemetryTracker when route changes
         */
        function bindEvents() {
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_TILE_VIDEO_STARTED, sendStartStopVideoTelemetryEvent);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_TILE_VIDEO_ENDED, sendStartStopVideoTelemetryEvent);
            Origin.telemetry.events.on(EVENT_TAKEOVER_IMPRESSION, sendTakeoverImpression);
            scrollPercentageStateChange = AppCommFactory.events.on('uiRouter:stateChangeStart', sendScrollPercentage);
        }

        /**
         * Unbind all events
         * Un-register all events here
         * Called by TelemetryTracker when route changes
         */
        function unbindEvents() {
            Origin.telemetry.events.off(Origin.telemetry.events.TELEMETRY_TILE_VIDEO_STARTED, sendStartStopVideoTelemetryEvent);
            Origin.telemetry.events.off(Origin.telemetry.events.TELEMETRY_TILE_VIDEO_ENDED, sendStartStopVideoTelemetryEvent);
            Origin.telemetry.events.off(EVENT_TAKEOVER_IMPRESSION, sendTakeoverImpression);

            if (_.isFunction(scrollPercentageStateChange)) {
                scrollPercentageStateChange();
            }
        }

        /**
         * Send scroll percentage when user leaves the Home Page
         * Category: 'My Home'
         * Action: 'Page Scroll'
         * Label: Quatet (25%, 50%, 75%, 100%) user has seen
         * Value: Percentage the user has scrolled down the page
         */
        function sendScrollPercentage(event, toState, toParams, fromState) {
            if (
                fromState.name === STATE_LOGGED_IN &&
                toState.name !== STATE_LOGGED_IN &&
                toState.name !== STATE_HOME
            ) {
                var pxToTop = window.scrollY;
                var viewPosition = pxToTop + document.documentElement.clientHeight;
                var documentHeight = document.body.scrollHeight;
                var percentage = pxToTop === 0 ? 0 : Math.floor(viewPosition / documentHeight * 100); // If user has not scrolled
                var quartetScrolled = Math.floor(percentage / 25) * 25; // Has the user seen 25%, 50%, 70%, or 100%
                var data = {
                    category: CATEGORY_MY_HOME,
                    action: ACTION_PAGE_SCROLL,
                    label: quartetScrolled,
                    value: percentage
                };

                TelemetryTrackerUtil.sendTelemetryEvent(data);
            }
        }

        /**
         * When user sees Takeover
         * Category: 'My Home'
         * Action: 'Impression'
         * Label: ID that identifies which takeover user has seen
         */
        function sendTakeoverImpression() {
            var takeoverEle = angular.element('.origin-tealium-takeover:visible');

            if (takeoverEle.length) {
                var rootEle = angular.element(takeoverEle).parent();
                var label = rootEle.attr(ATTRIBUTE_CTID) || 
                            rootEle.attr(ATTRIBUTE_OCD_PATH) || 
                            rootEle.attr(ATTRIBUTE_IMAGE) || 
                            rootEle.attr(ATTRIBUTE_TITLE_STR) || 
                            rootEle.attr(ATTRIBUTE_KEY_MESSAGE);
                var data = {
                    category: CATEGORY_MY_HOME,
                    action: ACTION_IMPRESSION,
                    label: label
                };

                TelemetryTrackerUtil.sendTelemetryEvent(data);
            }
        }

        return {
            bindEvents: bindEvents,
            unbindEvents: unbindEvents
        }; 
    }

    angular
        .module('originApp')
        .factory('TelemetryMyHomeCustomEventsFactory', TelemetryMyHomeCustomEventsFactory);
}());