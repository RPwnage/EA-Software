/**
 * Factory for telemetry tracking services
 * @file telemetry/telemetry-tracker.js
 */
(function () {
    'use strict';

    var BIND_EVENTS = 'bindEvents';
    var UNBIND_EVENTS = 'unbindEvents';

    function TelemetryTrackerFactory(TELEMETRY_CONFIG, TelemetryTrackerUtil, TelemetryTrackerParser, TelemetryMyHomeCustomEventsFactory, TelemetryGiftingCustomEventsFactory, TelemetryWishlistCustomEventsFactory, TelemetryUserStatisticsCustomEventsFactory, AnimateFactory, AppCommFactory) {
        
        var configs = []; // Aggregate of both Global configs + State Specific configs
        var globalConfigs = []; // Global configs (ie - Navigation, Site Stripe)

        var SCROLL_THROTTLE_MS = 250; // Scroll impression tracking

        // Updated customEventsMap for each injected custom event factory
        var customEventsMap = {
            'TelemetryMyHomeCustomEventsFactory': TelemetryMyHomeCustomEventsFactory,
            'TelemetryUserStatisticsCustomEventsFactory' : TelemetryUserStatisticsCustomEventsFactory,
            'TelemetryWishlistCustomEventsFactory' : TelemetryWishlistCustomEventsFactory,
            'TelemetryGiftingCustomEventsFactory' : TelemetryGiftingCustomEventsFactory
        };

        /**
         * Bind events
         */
        function bindEvents() {
            AnimateFactory.addScrollEventHandler(null, onScrollImpressionEvents, SCROLL_THROTTLE_MS);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_ELEMENT_DOM_LOADED, onScrollImpressionEvents);            
            angular.element('body').on('mouseup', onMouseUpEvent);
        }

        /**
         * Bind custom callbacks in configs
         * @param {string} customEventFunction name of the function to invoke
         */
        function invokeCustomEventFuncOnAllConfig(customEventFunction) {
            _.forEach(configs, function(config, configKey) {
                invokeCustomEventFactoryFunction(config, configKey, customEventFunction);
            });
        }

        /**
         * Initialize Telemetry Tracker
         */
        function init() {
            bindEvents();
            setGlobalConfigs();

            // Track states when route changes
            AppCommFactory.events.on('uiRouter:stateChangeSuccess', onStateChange);
        }

        /**
         * When user mouse up on element, send mouse up telemetry data
         * @param {object} e - Event data
         */
        function onMouseUpEvent(e) {
            _.forEach(configs, function(config) {
                
                // If config file is suppose to handle mouseup events on this state
                if (TelemetryTrackerParser.isGlobalEvent(config) || TelemetryTrackerParser.isMatchingState(_.get(config, 'stateName'))) {      
                    var $ele = angular.element(e.target);
                    var mouseUpEvents = _.get(config, 'mouseUpEvents', []);

                    // Loop through each of the mouseup events
                    _.forEach(mouseUpEvents, function(mouseUpEvent) {
                        var trigger = _.get(mouseUpEvent, 'trigger'),
                            recommendation = _.get(mouseUpEvent, 'recommendation');

                        // Send telemetry if mouseup-ed element is suppose to be tracked
                        // Do not break the for loop as multiple events can be assigned to the same element
                        if (TelemetryTrackerParser.isValidItem($ele, trigger)) {
                            var telemetry = TelemetryTrackerParser.getTelemetry($ele, mouseUpEvent);
                            TelemetryTrackerUtil.sendTelemetryEvent(_.merge(telemetry, {category: config.category}));

                            if(TelemetryTrackerParser.isRecommendation($ele, mouseUpEvent)) {
                                TelemetryTrackerUtil.sendRecommendationTrackingEvent([TelemetryTrackerParser.getTelemetry($ele, recommendation)]);
                            }
                        }
                    });
                }
            });
        }

        /**
         * When user scrolls, send impression telemetry data
         */
        function onScrollImpressionEvents() {
            _.forEach(configs, function(config) {

                // If config file is suppose to handle mouseup events on this state/route
                if (TelemetryTrackerParser.isGlobalEvent(config) || (TelemetryTrackerParser.isMatchingState(_.get(config, 'stateName')))) {
                    var scrollImpressionEvents = _.get(config, 'scrollImpressionEvents', []);

                    _.forEach(scrollImpressionEvents, function(scrollImpressionEvent) {
                        var triggers = _.get(scrollImpressionEvent, 'trigger');
                        
                        if (_.isObject(triggers)) {
                            _.forEach(triggers, function(trigger) {
                                TelemetryTrackerParser.sendScrollTelemetry(scrollImpressionEvent, trigger, config.category);
                            });
                        }
                    });
                }
            });
        }

        /**
         * When the state changes
         */
        function onStateChange() {
            invokeCustomEventFuncOnAllConfig(UNBIND_EVENTS);
            configs = [];
            var stateSpecificConfigs = _.get(TELEMETRY_CONFIG, 'stateSpecific');

            // Load state specific configs
            _.forEach(stateSpecificConfigs, function(stateSpecificConfig) {
                var stateName = _.get(stateSpecificConfig, 'stateName');

                if (TelemetryTrackerParser.isMatchingState(stateName)) {
                    setConfigs(stateSpecificConfig);
                    invokeCustomEventFuncOnAllConfig(BIND_EVENTS);
                }
            });
        }


        /**
         * Merge State Config and Global Configs to Configs object
         * @param {object} stateConfig - Config data for current state
         */
        function setConfigs(stateConfig) {
            configs.push(stateConfig);

            _.forEach(globalConfigs, function(globalConfig) {
                configs.push(globalConfig);
            });
        }

        /**
         * Set global configs (ie - Navigation, Site Stripe)
         */
        function setGlobalConfigs() {
            var globalConfig = _.get(TELEMETRY_CONFIG, 'global');
            _.forEach(globalConfig, function(config, configKey) {
                globalConfigs.push(config);
                invokeCustomEventFactoryFunction(config, configKey, BIND_EVENTS);
            });
        }

        function invokeCustomEventFactoryFunction(config, configKey, func) {
            var factoryName = _.get(config, ['customEvents']);

            if (factoryName) {
                var factory = customEventsMap[factoryName];
                var functionToInvoke = _.get(factory, func);
                if (_.isFunction(functionToInvoke)) {
                    functionToInvoke();
                }
            }
        }

        return {
            init: init
        }; 
    }

    angular
        .module('originApp')
        .factory('TelemetryTrackerFactory', TelemetryTrackerFactory);
}());