    /**
     * An api for accessing locale attributes
     * @param  {object} collection a collection cache of static locale information
     * @return {object} public api methods
     */
    function api(collection) {
        /**
         * Remap a US locale to a standard locale http://www.foo.com/home -> http://www.foo.com/usa/es-us/home
         * @param  {string} languageCode the new language code
         * @return {function} a function or replace to execute
         */
        function convertToStandardUrlLocale(languageCode) {
            /**
             * This function takes the replacement group and maps the new language code into it
             * @param  {string} replacementGroup the replacement group from the regex  eg
             * @return {string} the string replacement eg. /de-gb
             */
            return function(replacementGroup) {
                return [replacementGroup, '/', getThreeLetterCountryCode(), '/', languageCode, '-us'].join('');
            };
        }

        /**
         * Remap a standard URL scheme /gbr/en-gb/home -> /gbr/de-gb/home
         * @param  {string} languageCode the new language code
         * @return {function} a function or replace to execute
         */
        function replaceStandardUrlLocale(languageCode) {
            /**
             * This function takes the replacement group and maps the new language code into it
             * @param  {string} replacementGroup the replacement group from the regex delimited with () eg. /gbr/en-gb/
             * @return {string} the string replacement eg. /gbr/de-gb
             */
            return function(replacementGroup) {
                var matchChars = replacementGroup.split('');
                var languageChars = languageCode.split('');
                matchChars[5] = languageChars[0];
                matchChars[6] = languageChars[1];
                return matchChars.join('');
            };
        }

        /**
         * Get a value from the collection by key
         * @param  {string} key the keyname
         * @return {Mixed}  the corresponding collection value
         */
        function get(key) {
            return collection[key];
        }

        /**
         * Get the url friendly locale string
         * @return {string} the url-friendly locale string eg. en-us
         */
        function getLocale() {
            return get('locale');
        }

        /**
         * Get a cased locale string representation
         * @return {string} the legacy locale string eg. en_US
         */
        function getCasedLocale() {
            return get('casedLocale');
        }

        /**
         * Get the language code
         * @return {string} the ISO-639-1 language code eg. en
         */
        function getLanguageCode() {
            return get('languageCode');
        }

        /**
         * Get the country code
         * @return {string} the ISO_3166-1_alpha-2 country code eg. us
         */
        function getCountryCode() {
            return get('countryCode');
        }

        /**
         * Get the three letter language code
         * @return {string} the ISO-639-2 country code eg. eng
         */
        function getThreeLetterLanguageCode() {
            return get('threeLetterLanguageCode');
        }

        /**
         * Get the three letter country code
         * @return {string} the ISO_3166-1_alpha-3 country code eg. usa
         */
        function getThreeLetterCountryCode() {
            return get('threeLetterCountryCode');
        }

        /**
         * Get an aray of alternaate language codes for this locale
         * @return {Array} a list of ISO-639-1 language codes eg. ['en', 'es', ...]
         */
        function getLocaleAlternates() {
            return get('localeAlternates');
        }

        /**
         * Get the currency code for this region
         * @return {string} the applicable curency code eg. usd
         */
        function getCurrencyCode() {
            return get('currencyCode');
        }

        /**
         * Remap the URL for the desired language
         * @param {string} The current URL
         * @param {string} ISO-639-1 two character lower case language code
         * @return {string} the input URL with the new language code
         * @see https://regex101.com/r/iA0kI3/1
         *      https://regex101.com/r/mP7jN9/6
         */
        function createUrl(url, languageCode) {
            if (getCountryCode() === 'us' && getLanguageCode() === 'en') {
                if (!url.match(matchStandardUrlPattern)) {
                     return url.replace(genericUrlPattern, convertToStandardUrlLocale(languageCode));
                 } else {
                     return url;
                 }
            } else {
                if (getCountryCode() === 'us' && languageCode === 'en') {
                    return url.replace(matchStandardUrlPattern, '/');
                } else {
                    return url.replace(matchStandardUrlPattern, replaceStandardUrlLocale(languageCode));
                }
            }
        }

        return {
            getLocale: getLocale,
            getCasedLocale: getCasedLocale,
            getLanguageCode: getLanguageCode,
            getCountryCode: getCountryCode,
            getThreeLetterCountryCode: getThreeLetterCountryCode,
            getThreeLetterLanguageCode: getThreeLetterLanguageCode,
            getLocaleAlternates:  getLocaleAlternates,
            getCurrencyCode: getCurrencyCode,
            createUrl: createUrl
        };
    }
