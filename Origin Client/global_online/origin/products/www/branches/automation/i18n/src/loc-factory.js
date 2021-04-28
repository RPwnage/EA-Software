(function () {
    'use strict';

    function LocFactory(PluralizeFactory, LocDictionaryFactory, FALLBACK_PREFIX) {

        //show the missing strings off our query param
        var displayFallback = false;

        /**
         * boolean to determine whether we show the fallback message or not
         * @return {boolean} returns true if we should show the fallback string
         */
        function shouldDisplayFallbackString() {
            return displayFallback;
        }
        
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
        function substitute(str, params) {
            var key;

            if (typeof(params) !== 'object') {
                return str;
            }

            for (key in params) {
                if (params.hasOwnProperty(key)) {
                    str = str.split(key).join(String(params[key]));
                }
            }

            return str;
        }

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
        function trans(key, params, namespace) {

            if (!LocDictionaryFactory.isLoaded()) {
                throw 'Dictionary has not been loaded yet';
            }

            var str = '';
            try {
                str = LocDictionaryFactory.getDictionaryStr(key, namespace);
            } catch (e) {
                // note that the prefix here is different than the prefix
                // that we have at each entry.
                if (shouldDisplayFallbackString()) {
                    str = FALLBACK_PREFIX + key;
                }
            }

            // We should also sanitize the string to make sure that if there is
            // markup it doesn't break things
            // str = sanitize(str);
            return substitute(str, params);

        }

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
        function transChoice(key, choice, params, namespace) {

            if (!LocDictionaryFactory.isLoaded()) {
                throw 'Dictionary has not been loaded yet';
            }

            var str = '';
            try {
                str = LocDictionaryFactory.getDictionaryStr(key, namespace);
                str = PluralizeFactory.resolve(str, choice);
            } catch (e) {
                // should this be here? hmm...
                // REVIEW: this behaviour is different from the similar portion of 'trans' method. Why?
                if (shouldDisplayFallbackString()) {
                    return FALLBACK_PREFIX + key;
                }
            }
            // We should also sanitize the string to make sure that if there is
            // markup it doesn't break things
            // str = sanitize(str);

            return substitute(str, params);

        }

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
        function substituteChoice(str, choice, params) {
            var subedStr = '';
            try {
                subedStr = PluralizeFactory.resolve(str, choice);
            } catch (e) {
                if (shouldDisplayFallbackString()) {
                    return FALLBACK_PREFIX + str;
                }
            }
            return substitute(subedStr, params);
        }

        function init(displayFallbackOverride) {
            displayFallback = displayFallbackOverride;
        }

        return {
            init: init,
            trans: trans,
            substitute: substitute,
            transChoice: transChoice,
            substituteChoice: substituteChoice
        };
    }

    angular.module('origin-i18n')
        .factory('LocFactory', LocFactory);
}());
