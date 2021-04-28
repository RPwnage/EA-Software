/**
 * Factory for telemetry parser utilities
 * @file telemetry/telemetry-tracker-parser-util.js
 */
(function () {
    'use strict';

    var RECOID = 'recoid';

    function TelemetryTrackerParser($state, TelemetryTrackerUtil) {

        /**
         * Get the token value
         * @param {object} data - Config data for the token
         * @param {string} ele - Element user interacted with
         * @returns {string} - Token value
         */
        function getActualTokenValue(data, ele) {
            var $ele = angular.element(ele);
            var root = _.get(data, 'root');
            var $rootElement = root && ($ele.is(root) ? $ele : $ele.parents(root));
            var selectorData = _.get(data, 'selector');
            var selector = getSelector(selectorData);
            var attribute = getAttribute(selectorData);
            var value;

            if ($rootElement && $rootElement.length) {
                if (selector) {
                    if (attribute) {
                        value = $rootElement.find(selector).attr(attribute);
                    } else {
                        value = $rootElement.find(selector).text();
                    }
                } else if (attribute) {
                    value = $rootElement.attr(attribute);
                }
            } else {
                var $selector = angular.element(selector);

                if (selector) {
                    if (attribute) {
                        value = $selector.attr(attribute);
                    } else {
                        value = $selector.text();
                    }
                } else if (attribute) {
                    value = $ele.attr(attribute);
                }
            }

            return value;
        }

        /**
         * Get the token value for each word in the value
         * @param {string} str - String that contains tokens that needs to be replaced
         * @param {object} ele - Element user interacted with
         * @param {object} data - Config data for token
         * @param {string} tokenName - Name of the token (ie - labelToken)
         * @returns {string} - String with all tokens replaced with their actual values
         */
        function getAllTokenValue(str, ele, data, tokenName) {
            var words = str.split(' ');
            var filledInWords = _.map(words, function(item) {
                if (isToken(item)) {
                    return getTokenValue(item, data, tokenName, ele);
                } else {
                    return item;
                }
            });

            return filledInWords.join(' ');
        }

        /**
         * Get attribute value from string
         * Attribute value is designated as "[]"
         * @param {string} str - String that contains attribute
         * @returns {string} - Attribute
         */
        function getAttribute(str) {
            var start = str.indexOf('[');
            var end   = str.indexOf(']');
            if (start >= 0 && end > start) {
                return str.substring(start + 1, end);
            }
        }

        /**
         * Get the priority value for the token
         * @param {object} priorityData - Config data for priority data
         * @param {object} ele - Element user interacted with
         * @returns {string} - Value of item
         */
        function getPriorityTokenValue(priorityData, ele) {
            var value;

            _.forEach(priorityData, function(priority) {
                value = getActualTokenValue(priority, ele);

                if (value) {
                    return false;
                }
            });

            return value;
        }

        /**
         * Get CSS selector from string
         * @param {string} str - String that contains selector
         * @returns {string} - Selector
         */
        function getSelector(str) {
            var start = str.indexOf('[');
            return str.substring(0, start);
        }

        /**
         * Get telemetry data for ele by using data
         * @param {object} ele - DOM element interacted with
         * @param {object} data - Data from config that relates to ele
         * @returns {object} - Telemetry data
         */
        function getTelemetry(ele, data) {
            var label = _.get(data, 'label');

            if (label) {
                var labelTokenValue = getAllTokenValue(label, ele, data, 'labelToken');

                if (labelTokenValue) {
                    label = labelTokenValue;
                }
            }

            return {
                category: _.get(data, 'category'),
                action: _.get(data, 'action'),
                label: label,
                value: _.get(data, 'value')
            };
        }

        /**
         * Get the token value for the specified tokenName
         * @param {string} item - Name of label (ie - {id}, {url})
         * @param {object} data - Config data for token
         * @param {string} tokenName - Name of the token (ie - labelToken)
         * @param {object} ele - Element user interacted with
         * @returns {string} - Value of item
         */
        function getTokenValue(item, data, tokenName, ele) {
            var tokenData = _.get(data, [tokenName, item]);
            var priorityData = _.get(tokenData, 'priority');

            if (priorityData) {
                return getPriorityTokenValue(priorityData, ele);
            } else if (tokenData) {
                return getActualTokenValue(tokenData, ele);
            }
        }

        /*
         * Check if config is a global event
         * note - Global events do not have state names
         * @param {object} config - Config data
         * @returns {boolean} - If config is a Global event
         */
        function isGlobalEvent(config) {
            return !_.get(config, 'stateName');
        }

        /**
         * Check if ele is in viewport
         * @param {object} ele - Element to check if is in viewport
         * @returns {boolean} - If ele is in viewport
         */
        function isInViewport(ele) {
            var $ele = angular.element(ele);
            var top = $ele.offset().top;
            var left = $ele.offset().left;
            var width = $ele.width();
            var height = $ele.height();

            return (
                top < (window.pageYOffset + window.innerHeight) &&
                left < (window.pageXOffset + window.innerWidth) &&
                (top + height) >= window.pageYOffset &&
                (left + width) >= window.pageXOffset
            );
        }

        /**
         * Check if ele is trigger
         * @param {object} ele - Element to verify
         * @param {string/object} trigger - Selector(s) to verify element with
         * @returns {boolean} - If ele is trigger
         */
        function isValidItem(ele, trigger) {
            var $ele = angular.element(ele);
            if (typeof(trigger) === 'object') {
                var flag = false;

                _.forEach(trigger, function(path) {
                    if ($ele.is(path)) {
                        flag = true;
                        return false;
                    }
                });

                return flag;
            }
        }

        /**
         * Check if current page is stateName
         * @param {string|Array} stateName - Route name to verify against
         * @returns {boolean} - If current page is stateName
         */
        function isMatchingState(stateName) {
            var currentState = $state.current.name;
            return !_.isArray(stateName) ? (stateName &&
                (stateName === currentState ||
                currentState.indexOf(stateName)) >= 0) :
                (stateName.indexOf(currentState) >= 0);
        }

        /**
         * Check if str is a token (ie - {offer-id}, {title})
         * @param {string} str - String to check against
         * @returns {boolean} - If str is a token
         */
        function isToken(str) {
            return (
                str[0]              === '{' &&
                str[str.length - 1] === '}'
            );
        }

        /**
         * check to see if the tile is from a recommendation
         * @param  {object}  element the element to check
         * @param  {object}  event   the telemetry event
         * @return {Boolean}         returns true if is a recommendation element
         */
        function isRecommendation(element, event) {
            var recommendationEvent = _.get(event, 'recommendation');

            return recommendationEvent && (element.attr(RECOID) ||
                element.parents(_.get(recommendationEvent, 'recommendationTrigger')).attr(RECOID));
        }

        function appendPinDataToTelemetry(telemetry, scrollEvent, element) {
            var pinEventContent = _.get(scrollEvent, 'pinEvent.content'),
                pinEventMeta = _.get(scrollEvent, 'pinEvent.meta'),
                pinEventReqs = _.get(scrollEvent, 'pinEvent.reqs');

            var content = getTelemetry(element, pinEventContent),
                meta = getTelemetry(element, pinEventMeta),
                reqs = getTelemetry(element, pinEventReqs);

            if(content) {
                telemetry.additionalPinParams = {
                    'type': 'in-game',
                    'service': 'originx',
                    'format': 'live_tile',
                    'client_state': 'foreground',
                    'msg_id': telemetry.label,
                    'content': _.get(content, ['label'], ''),
                    'status': 'impression',
                    'destination_name' : _.get(pinEventContent, ['destination_name'], '')
                };
            }

            if (meta) {
                _.set(telemetry.additionalPinParams, ['meta'], {
                    'friend_id' : meta.label, 'friend_type' : 'nucleus'
                });
            }

            if (reqs) {
                _.set(telemetry.additionalPinParams, ['reqs'], {
                    'mutual_frd_cnt' : reqs.label
                });
            }
        }

        /**
         * Send telemetry for scroll event
         * @param {object} scrollEvent - Config data for the scroll event
         * @param {object} elements - Elements that the scroll event is assigned to watch
         * @param {string} category - Category assigned to the event
         */
        function sendScrollTelemetry(scrollEvent, elements, category) {
            var hasSeen = 'origin-telemetry-has-seen',
                recommendationTrackingInfo = [];

            angular.element(elements).each(function(elementKey, element) {
                var $element = angular.element(element);

                if (!$element.hasClass(hasSeen) && isInViewport(element)) {
                    var telemetry = getTelemetry(element, scrollEvent),
                        recommendationEvent = _.get(scrollEvent, 'recommendation');

                    if (isRecommendation($element, scrollEvent)) {
                        recommendationTrackingInfo.push(getTelemetry(element, recommendationEvent));
                    }

                    appendPinDataToTelemetry(telemetry, scrollEvent, element);

                    TelemetryTrackerUtil.sendTelemetryEvent(_.merge(telemetry, {
                        category: category
                    }), _.get(telemetry, ['additionalPinParams']));

                    $element.addClass(hasSeen);
                }
            });
            if (recommendationTrackingInfo.length) {
                TelemetryTrackerUtil.sendRecommendationTrackingEvent(recommendationTrackingInfo);
            }
        }

        return {
            getActualTokenValue: getActualTokenValue,
            getAllTokenValue: getAllTokenValue,
            getAttribute: getAttribute,
            getPriorityTokenValue: getPriorityTokenValue,
            getSelector: getSelector,
            getTelemetry: getTelemetry,
            getTokenValue: getTokenValue,
            isGlobalEvent: isGlobalEvent,
            isValidItem: isValidItem,
            isRecommendation: isRecommendation,
            isMatchingState: isMatchingState,
            isToken: isToken,
            sendScrollTelemetry: sendScrollTelemetry
        };
    }

    angular
        .module('originApp')
        .factory('TelemetryTrackerParser', TelemetryTrackerParser);
}());
