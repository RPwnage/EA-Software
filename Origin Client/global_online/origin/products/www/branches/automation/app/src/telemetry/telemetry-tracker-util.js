/**
 * Factory for telemetry utilities
 * @file telemetry/telemetry-tracker-util.js
 */
(function() {
    'use strict';
    var RECOMMENDATION_ITEM_TYPE_INDEX = 0,
        RECOMMENDATION_TAG_INDEX = 1,
        RECOMMENDATION_ITEM_TYPE_NAME = 2,
        RECOMMENDATION_ITEM_TYPE_POSITION = 3;

    function TelemetryTrackerUtil(ComponentsLogFactory) {
        /**
         * convert the label string from the telemetry object to an object consumed by the recommendation API
         * @param  {object} item the telemetry object
         * @return {object} recommendation api object
         */
        function convertAttrToRecommendationAPIObject(item) {
            var recoInfoString = _.get(item, 'label') || '',
                recoInfoArray = recoInfoString.split(' ');

            return {
                'item_type': _.get(recoInfoArray, RECOMMENDATION_ITEM_TYPE_INDEX) || '',
                'item_name': _.get(recoInfoArray, RECOMMENDATION_ITEM_TYPE_NAME) || '',
                'tile_position': _.get(recoInfoArray, RECOMMENDATION_ITEM_TYPE_POSITION) || ''
            };
        }

        /**
         * take an item and pull out the tag
         * @param  {object} item the telemetry object
         * @return {string} the tracking tag
         */
        function extractTrackingTag(item) {
            var recoInfoString = _.get(item, 'label') || '',
                recoInfoArray = recoInfoString.split(' ');
            return _.get(recoInfoArray, RECOMMENDATION_TAG_INDEX) || '';
        }

        /**
         * send recommendation to PIN
         * @param  {object[]} dataArray array of objects tracking info pulled from DOM
         */
        function sendRecommendationTrackingEvent(dataArray) {
            var datalist = _.map(dataArray, convertAttrToRecommendationAPIObject),
                trackingTagList = _.map(dataArray, extractTrackingTag),
                //the action is always the same for the whole group, so just take the first one from the array
                action = _.get(dataArray, [0, 'action']) || '';

            if (action.length) {
                Origin.pin.trackRecommendations(action, trackingTagList, datalist);
            }
        }

        /**
         * Send telemetry data
         * @param {object} data - Telemetry payload to send
         * @param {boolean} includePinEvent - we also send telemetry data to PinEvent
         */
        function sendTelemetryEvent(data, includePinEvent) {
            if (data) {
                ComponentsLogFactory.debug('Telemetry - Marketing Event Sent: ', data);
                Origin.telemetry.sendMarketingEvent(data.category, data.action, data.label, data.value, data.params, data.additionalPinParams);
                if (includePinEvent) {
                  Origin.telemetry.sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING,
                      'message', Origin.client.isEmbeddedBrowser() ? 'client' : 'web', 'success', data.additionalPinParams);
                }
            }
        }

        function sendPageViewEvent(data) {
            if (data) {
                ComponentsLogFactory.debug('Telemetry - Page View Event Sent: ', data);
                Origin.telemetry.sendPageView(data.page, data.title, data.params);
            }
        }

        return {
            sendTelemetryEvent: sendTelemetryEvent,
            sendPageViewEvent: sendPageViewEvent,
            sendRecommendationTrackingEvent: sendRecommendationTrackingEvent
        };
    }

    angular
        .module('originApp')
        .factory('TelemetryTrackerUtil', TelemetryTrackerUtil);
}());
