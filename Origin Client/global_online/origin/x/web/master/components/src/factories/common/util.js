/**
 * @file common/util.js
 */
(function() {
    'use strict';

    function UtilFactory(LocFactory, ComponentsLogFactory, $http) {

        var transitionEndStr = (function() {
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

        function extractData(response) {
            var status = response.status,
                data = response.data;

            if (status !== 200 && data !== 'object') {
                var error = new Error('ERROR: error retrieving JSON data from: ' + response.config.url + '(' + status + ')');
                error.status = status;
                return Promise.reject(error);
            }

            return data;
        }



        function objectToAttributes(data) {
            var attrs = [];
            var arrayAttrs = [];
            var curVal;
            var jsonify = '';
            if (typeof(data) === 'undefined') {
                return [];
            }
            for (var key in data) {
                if (data.hasOwnProperty(key)) {
                    curVal = data[key];
                    if (typeof(curVal) !== 'undefined') {
                        arrayAttrs = [];
                        if (curVal.constructor === Array) {
                            for (var item in curVal) {
                                jsonify = JSON.stringify(curVal[item]);
                                arrayAttrs.push(jsonify);
                            }
                            attrs.push(key + '=\'[' + arrayAttrs.join(',') + ']\'');
                        } else if (curVal.constructor === Object) {
                            jsonify = JSON.stringify(curVal);
                            attrs.push(key + '=\'' + jsonify + '\'');
                        } else {
                            attrs.push(key + '=\'' + curVal + '\'');
                        }
                    }

                }
            }
            return attrs;
        }

        return {
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
            getLocalizedStr: function(possibleStr, namespace, key, params, choice) {
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
            },
            
            /**
             * Returns the state the progress bar should display.
             * @param {Object} progressInfo - game.progressInfo
             * @return {String} state that progress bar should display
             * @method getDownloadState
             */
            getDownloadState: function(progressInfo) {
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
            },

            /**
             * Return true if element is on screen
             * @param {Element} elm jQlite element
             * @return {boolean}
             */
            isOnScreen: function(elm) {
                var win = angular.element(window),
                    elem = angular.element(elm),
                    vtop = win.scrollTop(),
                    vleft = win.scrollLeft(),
                    vright = vleft + win.width(),
                    vbottom = vtop + win.height(),
                    bleft = elem.offset().left,
                    btop = elem.offset().top,
                    bright = bleft + elem.outerWidth(),
                    bbottom = btop + elem.outerHeight();

                return (!(vright < bleft || vleft > bright || vbottom < btop || vtop > bbottom));
            },

            isAboveScreen: function(elm) {
                var vtop = angular.element(window).scrollTop();
                var btop = angular.element(elm).offset().top;
                var bbottom = btop + angular.element(elm).outerHeight();
                return (bbottom < vtop );
            },


            getElementHeight: function(elm) {
                return elm.height() + parseInt(elm.css('padding-top'), 10) + parseInt(elm.css('padding-bottom'), 10) + parseInt(elm.css('margin-top'), 10) + parseInt(elm.css('margin-bottom'), 10);
            },

            /**
             * Subscribe to the transition end event on an element
             * @param {Element} jQlite element
             * @param {Function} callback
             * @return {void}
             */
            onTransitionEnd: function(elm, callback, duration) {
                var called = false;
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
            },

            //REVIEW: Who wrote these functions? Where are the
            //comments??
            getMatrixValues: function(el) {
                var st = window.getComputedStyle(el, null);
                var tr = st.getPropertyValue("-webkit-transform") ||
                         st.getPropertyValue("-moz-transform") ||
                         st.getPropertyValue("-ms-transform") ||
                         st.getPropertyValue("-o-transform") ||
                         st.getPropertyValue("transform") ||
                         "FAIL";
                return tr.split('(')[1].split(')')[0].split(',');
            },

            getRotationAngle: function(el) {
                var values = this.getMatrixValues(el);
                var a = values[0];
                var b = values[1];

                return Math.round(Math.atan2(b, a) * (180/Math.PI));
            },

            getTranslationX: function(el) {
                var values = this.getMatrixValues(el);
                return values[4];
            },

            getTranslationY: function(el) {
                var values = this.getMatrixValues(el);
                return values[5];
            },

            /**
             * Get a time string in the format "HH:MM AM|PM"
             * @param {Number} Integer value representing the number of milliseconds since 1 January 1970 00:00:00 UTC
             * @return {String} formatted string result
             */
            getFormattedTimeStr: function(milliseconds) {
                var date, hours, minutes, pm = false;

                date = new Date(milliseconds);
                hours = date.getHours();
                minutes = date.getMinutes();

                if (hours > 12) {
                    pm = true;
                    hours -= 12;
                }

                minutes = minutes < 10 ? ('0' + minutes) : minutes;

                return hours + ':' + minutes + (pm ? ' PM' : ' AM');
            },

            /**
             * Returns a function that will call the fn parameter only once per specified delay, regardless of how many times the throttle function is called
             * @param {Function} Callback function
             * @param {Number} The specified delay in MS
             * @return {Function}
             */
            throttle: function(fn, delay) {
                var prev, deferTimer;
                return function() {
                    var context = this;
                    var now = +new Date(),
                        args = arguments;
                    if (prev && now < (prev + delay)) {
                        clearTimeout(deferTimer);
                        deferTimer = setTimeout(function() {
                            prev = now;
                            fn.apply(context, args);
                        }, delay);
                    } else {
                        prev = now;
                        fn.apply(context, args);
                    }
                };
            },

            /**
             * Returns a function that will call the fn parameter only once per second and eats any subsequent calls until the delay period is ended
             * @param {Function} Callback function
             * @param {Number} The specified delay in MS
             * @return {Function}
             */
            dropThrottle: function(fn, delay) {
                if (!delay) {
                    delay = 1000;
                }

                var throttleTimer;
                return function() {
                    var context = this;
                    var args = arguments;
                    if (!throttleTimer) {
                        fn.apply(context, args);

                        throttleTimer = setTimeout(function() {
                            throttleTimer = null;
                        }, delay);

                    }
                };
            },

            /**
             * This function returns a function that will call apply() on the fn parameter, after delay, providing that no other calls to the function are made. If the function is called before the delay timeout has passed, the 1 second delay is re-set.
             * @param {Function} Callback function
             * @param {Number} The specified delay in MS
             * @return {Function}
             */
            reverseThrottle: function(fn, delay) {
                if (!delay) {
                    delay = 1000;
                }
                var deferTimer = null;
                return function() {
                    var context = this;
                    var args = arguments;
                    if (deferTimer) {
                        clearTimeout(deferTimer);
                    }
                    deferTimer = setTimeout(function() {
                        deferTimer = null;
                        fn.apply(context, args);
                    }, delay);
                };
            },

            /**
             * Checks if the browser supports touch events
             * @return {boolean}
             */
            isTouchEnabled: function() {
                //for now disable touch on the embedded
                if (Origin.client.isEmbeddedBrowser()) {
                    return false;
                } else {
                    return ('ontouchstart' in window);
                }
            },

            /**
             * Writes out attributes inside of a Javascript Object. Converts them into directive parameters.
             * @param {Object}
             * @return {String}
             */
            objectToAttributes: objectToAttributes,

            /**
             * Creates a tag with a passed in directive object. (See DialogFactory.createDirective())
             * @param {Object Array} Array of directive objects.
             * @return {String} - the tag
             */
            buildTag: function(directives) {
                var iDirective = 0,
                    tag = '',
                    internalTag = '',
                    attrs = '',
                    currentDirectiveObj = null;

                for (iDirective = 0; iDirective < directives.length; iDirective++) {
                    currentDirectiveObj = directives[iDirective];
                    if (!currentDirectiveObj || !currentDirectiveObj.name) {
                        break;
                    }

                    // Directive has embedded directives
                    internalTag = '';
                    if (currentDirectiveObj.directives && currentDirectiveObj.directives.length) {
                        internalTag = this.buildTag(currentDirectiveObj.directives);
                    }

                    attrs = objectToAttributes(currentDirectiveObj.data);
                    tag += '<' + currentDirectiveObj.name + ' ' + attrs.join(' ') + '>' + internalTag + '</' + currentDirectiveObj.name + '>';
                }
                return tag ? tag : '';
            },

            /**
             * Takes in a url and returns a JSON object
             * @param  {string} url the endpoint for the json object
             * @return {promise}
             */
            getJsonFromUrl: function(url) {
                return $http.get(url).then(extractData, extractData);
            },

            /**
             * Takes in a string and returns an attribute friendly version
             * @param  {string} string to be converted
             * @return {string}
             */
            toAttributeFriendlyText: function(str) {
                str = str.replace(/'/g, '\&#39;');
                str = str.replace(/</g, '&lt;');
                str = str.replace(/>/g, '&gt;');
                return str;
            },

            /**
             * Create a unique ID
             *
             * @return {string} String will be of the form XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXX
             */
            guid: function (){
                function s4() {
                    return Math.floor((1 + Math.random()) * 0x10000).toString(16).substring(1);
                }
                return s4() + s4() + '-' + s4() + '-' + s4() + '-' + s4() + '-' + s4() + s4() + s4();
            },

            /**
            * Get the parent element matching the class name
            * @param {Node} node
            * @param {String} className
            * @return {Node}
            * @method getParentByClassName
            */
            getParentByClassName: function(node, className) {
                var parent = node;
                while (parent.length) {
                    if (parent.hasClass(className)) {
                        return parent;
                    }
                    parent = parent.parent();
                }
            }

        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function UtilFactorySingleton(LocFactory, ComponentsLogFactory, $http, SingletonRegistryFactory) {
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
