(function() {
    'use strict';

    function LocFactory(PluralizeFactory, DISPLAY_FALLBACK, FALLBACK_PREFIX) {

        var dictionary = false,
            callbacks = {};

        /**
        * Once the dictionary has loaded execute all of the callbacks
        * that are waiting for the onReady event
        * @return {void}
        */
        function onReady() {
            var fnObj;
            if (callbacks.ready) {
                for (var i = 0, j = callbacks.ready.length; i < j; i++) {
                    fnObj = callbacks.ready[i];
                    fnObj.callback.apply(fnObj.scope || window, fnObj.args);
                }
                callbacks.ready = [];
            }
        }

        /**
        * Sets new dictionary
        * @param {Object} newValue - new dictionary to set
        * @return {void}
        */
        function setDictionary(newValue) {
            dictionary = newValue;
            onReady();
        }

        /**
        * Get the string from the dictionary
        * @param {String} key - the key of the string in the dictionary,
        *   this is the english string by default.
        * @param {String} namespace - *optional* the namespace that the translation belongs
        *   to, like mygames, home, etc.
        * @return {String} - the string in the dictionary, or the english string if its not found
        */
        function getDictionaryStr(key, namespace) {
            var str = !!namespace ? dictionary[namespace] && dictionary[namespace][key] : dictionary[key];
            if (!str) {
                throw 'Key not found in dictionary';
            } else {
                return str;
            }
        }

        return {

            /**
            * Need to initialize the loc factory, which makes
            * the request to get the dictionary.
            * @param {Object} newDictionary - the dictionary
            * @return {void}
            */
            init: setDictionary,

            /**
            * returns whether the dictionary is empty or not
            * @return {boolean}
            */
            loaded: function() {
                return !!dictionary;
            },

            /**
            * Function to get a translated string with variables substituted.
            * an example of the call would be.
            *
            * LocFactory.trans('I would like you to meet my best friend, %name%.', 'profile', {
            *   '%name%': 'Consuela'
            * });
            *
            * @param {String} key - the key of the string in the dictionary,
            *   this is the english string by default.
            * @param {Object} params - *optional*, a hash where the key is the
            *   name of the variable that you would like to be substituted and
            *   the value is the value of that variable.  For instance:
            *       {
            *           '%name%': 'Bob'
            *       }
            *   Where the variable must be enclosed by percent signs (%) as above
            * @param {String} namespace - *optional* the namespace that the translation belongs
            *   to, like mygames, home, etc.
            * @return {String} - the translated string with variables substituted
            */
            trans: function(key, params, namespace) {

                if (!dictionary) {
                    throw 'Dictionary has not been loaded yet';
                }

                var str = '';
                try {
                    str = getDictionaryStr(key, namespace);
                } catch (e) {
                    if (DISPLAY_FALLBACK) {
                        str = FALLBACK_PREFIX + key;
                    }
                }

                // We should also sanitize the string to make sure that if there is
                // markup it doesn't break things
                // str = sanitize(str);
                return this.substitute(str, params);

            },

            /**
            * Subtitute the the variables into the string
            * @param {String} str - the string to put the variable values into
            * @param {Object} params - a hash where the key is the
            *   name of the variable that you would like to be substituted and
            *   the value is the value of that variable.  For instance:
            *       {
            *           '%name%': 'Bob'
            *       }
            *   Where the variable must be enclosed by percent signs (%) as above
            * @return {String} - substituted string
            */
            substitute: function(str, params) {
                var substitutedStr = str, // substitutedString
                    re, key;

                if (typeof(params) !== 'undefined') {
                    for (key in params) {
                        if (params.hasOwnProperty(key)) {
                            re = new RegExp(key, 'g');
                            substitutedStr = substitutedStr.replace(re, params[key]);
                        }
                    }
                }
                return substitutedStr;
            },

            /**
            * Function to get a translated string where we also resolve any
            * pluralization associated with that string.
            *
            * LocFactory.transChoice('{0} I have no bananas | {1} I have a banana
            *   | [2,+Inf] I have many bananas', 22);
            *
            * @param {String} key - the key of the string in the dictionary,
            *   this is the english string by default.  This string will have
            *   various plural forms, delimited by a |
            *       e.g. '{0} I have no bananas | {1} I have a banana | [2,+Inf] I
            *               have many bananas'
            * @param {Number} choice - the number which determines which
            *   plural string form to use to use.
            * @param {Object} params - *optional*, a hash where the key is the
            *   name of the variable that you would like to be substituted and
            *   the value is the value of that variable.  For instance:
            *       {
            *           '%name%': 'Bob'
            *       }
            *   Where the variable must be enclosed by percent signs (%) as above
            * @param {String} namespace - *optional* the namespace that the translation belongs
            *   to, like mygames, home, etc.
            * @return {String} - the translated string with variables substituted
            *   in the proper plural form
            */
            // REVIEW: Consider consolidating this method with 'trans'.
            // This would help reducing the amount of copy-paste here
            // and reducing complexity of UtilFactory.getLocalizedStr
            transChoice: function(key, choice, params, namespace) {

                if (!dictionary) {
                    throw 'Dictionary has not been loaded yet';
                }

                var str = '';
                try {
                    str = getDictionaryStr(key, namespace);
                    str = PluralizeFactory.resolve(str, choice);
                } catch (e) {
                    // should this be here? hmm...
                    // REVIEW: this behaviour is different from the similar portion of 'trans' method. Why?
                    if (DISPLAY_FALLBACK) {
                        return FALLBACK_PREFIX + key;
                    }
                }
                // We should also sanitize the string to make sure that if there is
                // markup it doesn't break things
                // str = sanitize(str);

                return this.substitute(str, params);

            },

            /**
            * Resolve pluralization and substitute parameters for a string.  These will
            * resolve the strings that come directly from the CMS.
            *
            * @param {String} str - A string to resolve pluralization and substitute
            *   parameters.  This string will have various plural forms, delimited by a |
            *       e.g. '{0} I have no bananas | {1} I have a banana | [2,+Inf] I
            *               have many bananas'
            * @param {Number} choice - the number which determines which
            *   plural string form to use to use.
            * @param {Object} params - *optional*, a hash where the key is the
            *   name of the variable that you would like to be substituted and
            *   the value is the value of that variable.  For instance:
            *       {
            *           '%name%': 'Bob'
            *       }
            *   Where the variable must be enclosed by percent signs (%) as above
            * @return {String} - the translated string with variables substituted
            *   in the proper plural form
            */
            substituteChoice: function(str, choice, params) {
                var subedStr = '';
                try {
                    subedStr = PluralizeFactory.resolve(str, choice);
                } catch (e) {
                    if (DISPLAY_FALLBACK) {
                        return FALLBACK_PREFIX + str;
                    }
                }
                return this.substitute(subedStr, params);
            },

            /**
            * Listen to events
            * @paran {String} eventName - the event name to subscribe to
            * @param {Function} callback - the callback to fire on the event
            * @param {Object} scope - the scope to execute the callback on
            * @return {void}
            */
            on: function(eventName, callback, scope) {
                var args = [].slice.call(arguments, 3);
                if (eventName === 'ready' && dictionary) {
                    callback.apply(scope || window, args);
                } else {
                    if (!callbacks[eventName]) {
                        callbacks[eventName] = [];
                    }
                    callbacks[eventName].push({
                        'callback': callback,
                        'scope': scope,
                        'args': args
                    });
                }
            }

        };
    }

    angular.module('origin-i18n')
        .factory('LocFactory', LocFactory);
}());
