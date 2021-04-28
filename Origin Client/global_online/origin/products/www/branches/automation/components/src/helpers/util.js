/**
 * @file common/util.js
 */
(function() {
    'use strict';
    var SPECIAL_CHARS_REGEXP = /([\:\-\_]+(.))/g,
        MOZ_HACK_REGEXP = /^moz([A-Z])/,
        IS_ABSOLUTE_URL = new RegExp('^(https|http).', 'i'),
        PERCENTAGE_PLACEHOLDER = '{{PERCENTAGE}}',
        PERCENTAGE_REPLACEMENT_REGEX = new RegExp('%', 'g'),
        PERCENTAGE_PLACEHOLDER_REPLACEMENT_REGEX = new RegExp(PERCENTAGE_PLACEHOLDER, 'g'),
        ONE_SECOND_IN_MILLIS = 1000,
        NO_DELAY = 0,
        BYTES_KB = 1024,
        BYTES_MB = 1048576,
        BYTES_GB = 1073741824,
        BYTES_TB = 1099511627776,
        WINDOWS_XP_OR_VISTA_REGEXP = new RegExp('Windows NT (5\.1|6\.0).', 'i'),
        BASE_HREF = document.querySelector('head base') && document.querySelector('head base').getAttribute('href') || '',
        PDP_EDITIONS_NAMESPACE = 'origin-store-pdp-editions',
        VALID_NEUCLUS_ID = /^\d+$/;


    function UtilFactory(LocFactory) {
        /**
         * This function is specialized to use cases where the framework converts html attributes to camel cased scope attributes
         * It handles the conversion of attribute keys in a way that is specific to angular's private jqlite::camelCase function
         * This function has a special treatment of numbers in the middle of strings.
         * <origin-element my-attribute-1="foo"></origin-element> => $scope.myAttribute1.
         *
         * @param name the html attribute name eg my1attribute or my-attribute
         * @return {string} the scope attribute name eg. my1attribute or myAttribute
         */
        function htmlToScopeAttribute(htmlAttribute) {
            return String(htmlAttribute).
                replace(SPECIAL_CHARS_REGEXP, function(_, separator, letter, offset) {
                    return offset ? letter.toUpperCase() : letter;
                }).
                replace(MOZ_HACK_REGEXP, 'Moz$1');
        }

        /**
         * Get the window size
         *
         * @return {Object} object with width and height properties
         */
        function getWindowSize() {
            var width = window.innerWidth ? window.innerWidth : document.documentElement.clientWidth ? document.documentElement.clientWidth : screen.width;
            var height = window.innerHeight ? window.innerHeight : document.documentElement.clientHeight ? document.documentElement.clientHeight : screen.height;
            return {
                width: width,
                height: height
            };
        }

        /**
         * Create a popup window and center the window with a given width and height
         *
         * @param {String} url - url to show popup
         * @param {String} title - title for the popup window
         * @param {Number} width - width of the popup
         * @param {Number} height - height of the popup
         */
        function popupCenter(url, title, w, h) {
            // Fixes dual-screen position                         Most browsers      Firefox
            var dualScreenLeft = window.screenLeft !== undefined ? window.screenLeft : screen.left;
            var dualScreenTop = window.screenTop !== undefined ? window.screenTop : screen.top;

            var windowSize = getWindowSize();

            var left = ((windowSize.width / 2) - (w / 2)) + dualScreenLeft;
            var top = ((windowSize.height / 2) - (h / 2)) + dualScreenTop;
            var newWindow = window.open(url, title, 'location=no,menubar=no,resizable=no,scrollbars=yes,status=no,titlebar=no,toolbar=no, width=' + w + ', height=' + h + ', top=' + top + ', left=' + left, false);

            // Puts focus on the newWindow
            if (window.focus) {
                newWindow.focus();
            }

            return newWindow;
        }

        /**
         * If the string already exists as an attribute passed
         * down from the cms, then use that overwrite string which
         * is already translated, and substitute the parameters.
         * If the string doesn't come from the CMS then go and
         * fetch the string from the loc factory.
         *
         * @param {String} possibleStr - the attribute/value of the
         *   parameter that is passed down as a parameter to the
         *   directive.  This will either be the already translated
         *   string or it will be empty.
         * @param {String} namespace the dictionary entry that contains the default
         *   translations for the translation lookup in case possibleStr is not defined
         * @param {String} key - the English key for the string to
         *   translate.
         * @param {object} params - *optional* this is the optional list
         *   of parameters to substitute the string with.
         * @param {object} choice - *optional* this is the optional number value that
         *   determines if pluralization within the @possibleStr or @key
         *   needs to be resolved.
         *
         * @return {String} the localized string with parameters substituted
         *
         */
        function getLocalizedStr(possibleStr, namespace, key, params, choice) {
            // list of global variables to replace in geo merchandised strings automatically
            // TODO: Move this to another factory - these aren't global to utilfactory.
            //NOTE: if you modify this list, please update HtmlTransformer as well :(
            var GLOBAL_REPLACEMENTS = {
                '%url-locale%': Origin.locale.languageCode() + '-' + Origin.locale.countryCode().toLowerCase(),
                '%url-country%': Origin.locale.threeLetterCountryCode().toLowerCase(),
                '%url-country-code%': Origin.locale.countryCode().toLowerCase(),
                '%country-code%': Origin.locale.countryCode(),
                '%url-language%': Origin.locale.languageCode(),
                '%locale%': Origin.locale.locale()
            };

            params = params ? _.merge(params, GLOBAL_REPLACEMENTS) : GLOBAL_REPLACEMENTS;

            if (!!possibleStr) {
                if (typeof choice !== 'undefined') {
                    return LocFactory.substituteChoice(possibleStr, choice, params);
                } else {
                    return LocFactory.substitute(possibleStr, params);
                }
            } else {
                if (typeof choice !== 'undefined') {
                    return LocFactory.transChoice(key, choice, params, namespace);
                } else {
                    return LocFactory.trans(key, params, namespace);
                }
            }
        }

        /**
        * Create a function that passes the same context namespace to getLocalizedStr
        *
        * @param {String} contextNamespace
        * @return {Function}
        */
        function getLocalizedStrGenerator(contextNamespace) {
            /**
            * Function that calls getLocalizedStr with remaining parameters
            * @param {String} possibleStr - the attribute/value of the
            *   parameter that is passed down as a parameter to the
            *   directive.  This will either be the already translated
            *   string or it will be empty.
            * @param {String} key - the English key for the string to
            *   translate.
            * @param {object} params - *optional* this is the optional list
            *   of parameters to substitute the string with.
            * @param {object} choice - *optional* this is the optional number value that
            *   determines if pluralization within the @possibleStr or @key
            *   needs to be resolved.
            *
            * @return {String} the localized string with parameters substituted
            */
            return function(possibleStr, key, params, choice) {
                return getLocalizedStr(possibleStr, contextNamespace, key, params, choice);
            };

        }

        /**
         * Returns the state the progress bar should display.
         *
         * @param {Object} progressInfo - game.progressInfo
         * @return {String} state that progress bar should display
         * @method getDownloadState
         */
        function getDownloadState(progressInfo) {
            var state = '';

            switch (progressInfo.progressState) {
                case 'State-Active':
                    if (typeof(progressInfo.progress) !== 'undefined') {
                        if (progressInfo.progress === 1) {
                            state = 'complete';
                        } else {
                            state = 'active';
                        }
                    }
                break;
                case 'State-Paused':
                    state = 'paused';
                break;
                case 'State-Indeterminate':
                    state = 'preparing';
                break;
            }

            return state;
        }

        /**
         * Subscribe to the transition end event on an element
         *
         * @param {Element} jQlite element
         * @param {Function} callback
         * @param {Number} duration duration in milliseconds
         * @see https://jonsuh.com/blog/detect-the-end-of-css-animations-and-transitions-with-javascript/
         */
        function onTransitionEnd(elm, callback, duration) {
            var called = false,
                transitionEndStr = (function() {
                    var div = document.createElement('div'),
                        transitions = {
                            'WebkitTransition': 'webkitTransitionEnd',
                            'MozTransition': 'transitionend',
                            'OTransition': 'oTransitionEnd otransitionend',
                            'transition': 'transitionend'
                        };

                    for (var t in transitions) {
                        if (div.style[t] !== undefined) {
                            return transitions[t];
                        }
                    }
                }());

            elm.one(transitionEndStr, function(){
                called = true;
                if(callback) {
                    callback();
                }
            });

            if(duration) {
                var t = function() {
                    if (!called) {
                        elm.trigger(transitionEndStr);
                    }
                };
                setTimeout(t, duration);
            }
        }

        /**
         * Get a locale sensitive time string in the format "HH:MM AM|PM"
         *
         * @param {Number} Integer value representing the number of milliseconds since 1 January 1970 00:00:00 UTC
         * @return {String} formatted string result
         */
        function getFormattedTimeStr(milliseconds) {
            return moment(milliseconds).format('LT');
        }

        /**
         * Checks if the browser supports touch events
         *
         * @return {boolean}
         */
        function isTouchEnabled() {
            //for now disable touch on the embedded
            if (Origin.client.isEmbeddedBrowser()) {
                return false;
            } else {
                return ('ontouchstart' in window) || !!window.navigator.msMaxTouchPoints || !!window.navigator.maxTouchPoints;
            }
        }

        /**
         * Build an html tag using jqlite
         *
         * @param  {string} elementName The element Name
         * @param  {Object} elementAttributes An attribute list
         * @return {Object} HTMLElement
         */
        function buildTag(elementName, elementAttributes) {
            var tagElement = document.createElement(elementName);

            _.forEach(elementAttributes, function(value, key) {
                tagElement.setAttribute(key, value);
            });

            return tagElement;
        }

        /**
         * Create a unique ID
         *
         * @return {string} String will be a string of the form XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX where x is a hex number
         * @see http://stackoverflow.com/questions/105034/create-guid-uuid-in-javascript
         */
        function guid() {
            function s4() {
                return Math.floor((1 + Math.random()) * 0x10000).toString(16).substring(1);
            }

            return s4() + s4() + '-' + s4() + '-' + s4() + '-' + s4() + '-' + s4() + s4() + s4();
        }

        /**
        * Get the parent element matching the class name
        *
        * @param {object} node the jqlite node
        * @param {String} className classname to search
        * @return {Node} the parent jqlite node
        */
        function getParentByClassName(node, className) {
            var parent = node;
            while (parent.length) {
                if (parent.hasClass(className)) {
                    return parent;
                }
                parent = parent.parent();
            }
        }

        /**
        * Get formatted value for bytes in KB,MB,GB and TB
        *
        * @param {string} Bytes
        * @return {string} Formatted Value
        */
        function formatBytes(bytes) {
            if (bytes < BYTES_KB) {
                return bytes.toFixed(2);
            }
            else if (bytes < BYTES_MB) {
                return (bytes / BYTES_KB).toFixed(2);
            }
            else if (bytes < BYTES_GB) {
                return (bytes / BYTES_MB).toFixed(2);
            }
            else if (bytes < BYTES_TB) {
                return (bytes / BYTES_GB).toFixed(2);
            }
            else {
                return (bytes / BYTES_TB).toFixed(2);
            }
        }

        /**
         * Generate a base url
         *
         * @return {string} the url string https://www.origin.com/usa/en-us
         */
        function generateBaseUrl() {
            return window.location.protocol +
                '//' +
                window.location.host +
                '/' +
                Origin.locale.threeLetterCountryCode().toLowerCase() +
                '/' +
                Origin.locale.languageCode() +
                '-' +
                Origin.locale.countryCode().toLowerCase();
        }

        /**
         * DEPRECATED
         * Throttle the invocation of a callback function and its arguments
         * note that the return value from this decorator will not contain the return values from fn.
         * @param  {Function} fn the function to throttle
         * @param  {Number} delay the throttle interval in milliseconds
         * TODO: Deprecate this function in favor of _.throttle() or _.debounce()
         * @see https://css-tricks.com/debouncing-throttling-explained-examples/
         */
        function throttle(fn, delay) {
            var prev, deferTimer;
            delay = delay || NO_DELAY;

            return function() {
                var context = this,
                    now = +new Date(),
                    args = arguments;

                if (prev && now < (prev + delay)) {
                    clearTimeout(deferTimer);
                    deferTimer = setTimeout(function() {
                        prev = now;
                        if (_.isFunction(fn)) {
                            fn.apply(context, args);
                        }
                    }, delay);
                } else {
                    prev = now;
                    if (_.isFunction(fn)) {
                        fn.apply(context, args);
                    }
                }
            };
        }

        /**
         * DEPRECATED
         * Returns a function that will immediately invoke the fn with its arguments and
         * drop all subsequent atempts to invoke fn until the timer is reset
         * not that the refun values from the function will not contain the retun values from fn
         *
         * @param {Function} fn Callback function
         * @param {Number} delay The specified delay in milliseconds (defaults to 1s)
         * @return {Function}
         * TODO: Deprecate this function in favor of _.throttle() or _.debounce()
         * @see https://css-tricks.com/debouncing-throttling-explained-examples/
         */
        function dropThrottle(fn, delay) {
            var throttleTimer;
            delay = delay || ONE_SECOND_IN_MILLIS;

            return function() {
                var context = this,
                    args = arguments;

                if (!throttleTimer) {
                    if (_.isFunction(fn)) {
                        fn.apply(context, args);
                    }

                    throttleTimer = setTimeout(function() {
                        throttleTimer = null;
                    }, delay);
                }
            };
        }

        /**
         * This function returns a function that will call apply() on the fn parameter
         * after the specified delay, providing that no other calls to the function are made. If the function
         * is called before the delay timeout has passed, the delay is reset.
         *
         * @param {Function} fn Callback function
         * @param {Number} delay The specified delay in milliseconds (defaults to 1s)
         * @return {Function}
         */
        function reverseThrottle(fn, delay) {
            var deferTimer;
            delay = delay || ONE_SECOND_IN_MILLIS;

            return function() {
                var context = this,
                    args = arguments;

                if (deferTimer) {
                    clearTimeout(deferTimer);
                }

                deferTimer = setTimeout(function() {
                    deferTimer = null;
                    if (_.isFunction(fn)) {
                        fn.apply(context, args);
                    }
                }, delay);
            };
        }

        /**
         * Serialize a list of attributes for use with HTML Attributes
         * @param  {Object} data flat data map to serialize string|key: mixed|value
         * @return {Object} a serialized map to pass to a tag builder key={...Json...}
         */
        function serializeHtmlAttributes(data) {
            var attrs = {};

            if (!data) {
                return attrs;
            }

            _.forEach(data, function(value, key){
                attrs[key] = JSON.stringify(value);
            });

            return attrs;
        }

        /**
         * Given an input string and a map of static string replacements replace all occurences of the
         * search value with the replace value in a performant way (not using regexes)
         * @param {string} text the input text to analyze
         * @param {Object} replacementMap the replacement map {'search': 'replace'}
         * @return {string} the string with replaced values
         */
        function replaceAll(text, replacementMap) {
            if(!_.isObject(replacementMap) || !_.isString(text)) {
                return text;
            }

            _.forEach(replacementMap, function(replace, search) {
                text = text.split(search).join(replace);
            });

            return text;
        }

        function isAbsoluteUrl(url){
            return IS_ABSOLUTE_URL.test(url);
        }

        /**
         * Given two friends, it'll sort them by originId
         * @param friend1
         * @param friend2
         * @returns {number}
         */
        function compareOriginId(friend1, friend2) {
            return friend1.originId.localeCompare(friend2.originId, undefined, {numeric: true});
        }

        /**
         * Escapes % symbol so decodeURIComponent does not throw URL malformed error
         * @param {String} string
         * @returns {String} decoded string
         */
        function escapeAndDecodeURIComponent(string){
            string = string.replace(PERCENTAGE_REPLACEMENT_REGEX, PERCENTAGE_PLACEHOLDER);
            string = decodeURIComponent(string);
            string = string.replace(PERCENTAGE_PLACEHOLDER_REPLACEMENT_REGEX, '%');
            return string;
        }

        /**
         * Test if OS is XP or Vista
         * @returns {boolean}
         */
        function isXPOrVista(){
            var userAgent = navigator.userAgent || '';
            return WINDOWS_XP_OR_VISTA_REGEXP.test(userAgent);
        }

        /**
         * Returns the current base path for the SPA
         */
        function getBaseHref(){
            return BASE_HREF;
        }

        /**
         * This function removes the base href so the relative URL will be
         * in Angular context so Angular $location.url can use the URL
         *
         * @param {String} relativeUrl
         *
         * @return {String} relative url without base href
         */
        function getRelativeUrlInAngularContext(relativeUrl){
            var baseHref = getBaseHref();
            baseHref = baseHref === '/' ? '' : baseHref;
            return relativeUrl.indexOf(baseHref) > -1 ? relativeUrl.replace(baseHref, '') : relativeUrl;
        }

        /**
         * Unwraps a promise and calls the designated calback when executed
         *
         * @param {Function} successCallback the callback to call on success
         * @param {Function} errorCallback he callback to call on error
         * @return {Function}
         */
        function unwrapPromiseAndCall(successCallback, errorCallback) {
            successCallback = successCallback || function() {};
            errorCallback = errorCallback || function() {};

            /**
             * Process the promise and call a function with arguments callback(Promise1 [, Promise 2, ...])
             *
             * @param {Promise.<data, Error>} callback the callback will be called with spread params
             */
            return function(promise) {
                if(!promise || !_.isFunction(promise.then)) {
                    return;
                }

                var executeSuccessCallback = function(data) {
                    successCallback.apply(undefined, _.isArray(data) ? data : [data]);
                };

                return promise.then(executeSuccessCallback)
                    .catch(errorCallback);
            };
        }

        /**
         * Translates edition names becuase we dont get i18n values from catalog
         *
         * @param {String} edition The untranslated edition name from catalog
         * @return {String} the translated string or the original string if a translation does not exist
         */
        function getLocalizedEditionName(edition) {
            return LocFactory.trans(_.kebabCase(edition), {}, PDP_EDITIONS_NAMESPACE) || edition;
        }

        /**
         * return true if it looks like a valid nuclues id
         * @param nucleusId
         * @returns {boolean} return true if looks like a nuclues id.
         */
        function isValidNucleusId(nucleusId) {
            return VALID_NEUCLUS_ID.test(nucleusId);
        }

        return {
            getLocalizedStr: getLocalizedStr,
            getLocalizedStrGenerator: getLocalizedStrGenerator,
            getDownloadState: getDownloadState,
            onTransitionEnd: onTransitionEnd,
            getFormattedTimeStr: getFormattedTimeStr,
            throttle: throttle,
            dropThrottle: dropThrottle,
            reverseThrottle: reverseThrottle,
            isTouchEnabled: isTouchEnabled,
            buildTag: buildTag,
            htmlToScopeAttribute: htmlToScopeAttribute,
            popupCenter: popupCenter,
            getWindowSize: getWindowSize,
            guid: guid,
            getParentByClassName: getParentByClassName,
            formatBytes: formatBytes,
            generateBaseUrl: generateBaseUrl,
            serializeHtmlAttributes: serializeHtmlAttributes,
            replaceAll: replaceAll,
            isAbsoluteUrl: isAbsoluteUrl,
            escapeAndDecodeURIComponent: escapeAndDecodeURIComponent,
            isXPOrVista: isXPOrVista,
            getBaseHref: getBaseHref,
            compareOriginId: compareOriginId,
            getRelativeUrlInAngularContext: getRelativeUrlInAngularContext,
            unwrapPromiseAndCall: unwrapPromiseAndCall,
            getLocalizedEditionName: getLocalizedEditionName,
            isValidNucleusId : isValidNucleusId
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    // This needs to be a singleton for the popout/OIG windows to work properly please don't remove this singleton without proper testing of these features
    function UtilFactorySingleton(LocFactory, $http, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('UtilFactory', UtilFactory, this, arguments);
    }
    /**
     * @ngdoc service
     * @name origin-components.factories.UtilFactory

     * @description
     *
     * UtilFactory
     */
    angular.module('origin-components')
        .factory('UtilFactory', UtilFactorySingleton);

}());
