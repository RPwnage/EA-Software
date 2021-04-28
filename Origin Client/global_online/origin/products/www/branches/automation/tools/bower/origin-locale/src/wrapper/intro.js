(function() {

    'use strict';

    /**
     * Matches standard lowercase, 2 character, hyphen delimited langugage-country combinations
     * as well as the storefront that prefaces him.
     * @type {RegExp}
     * @see https://regex101.com/r/mP7jN9/6
     */
    var matchStandardUrlPattern = /\/([a-z]{3})\/([a-z]{2}-[a-z]{2})\//i;

    /**
    * Explodes a URL into a pattern where we extract the base url from a url
    * @type {RegExp}
    * @see https://regex101.com/r/eI2tX6/1
    */
     var genericUrlPattern = /^(https?:\/\/\/?[\w]*(?::[\w]*)?@?[\d\w\.-]+(?::(\d+))?)/;

     var twoLetterToThreeLetterCountryMap = {
         'us': 'usa',
         'ca': 'can',
         'gb': 'gbr',
         'au': 'aus',
         'be': 'bel',
         'br': 'bra',
         'de': 'deu',
         'dk': 'dnk',
         'es': 'esp',
         'fi': 'fin',
         'fr': 'fra',
         'hk': 'hkg',
         'ie': 'irl',
         'in': 'ind',
         'it': 'ita',
         'jp': 'jpn',
         'kr': 'kor',
         'mx': 'mex',
         'nl': 'nld',
         'no': 'nor',
         'nz': 'nzl',
         'pl': 'pol',
         'pt': 'prt',
         'ru': 'rus',
         'se': 'swe',
         'sg': 'sgp',
         'th': 'tha',
         'tw': 'twn',
         'za': 'zaf',
         'ch': 'deu',
         'ww': 'usa',
         'cn': 'chn',
         'default': 'irl'
     };

    /**
     * Extract the two letter country code from the three letter country code
     * @param  {string} country e.g. usa
     * @return {string} the country code eg. us
     */
    function parseCountryCode(country) {
        for (var key in twoLetterToThreeLetterCountryMap) {
            if (twoLetterToThreeLetterCountryMap.hasOwnProperty(key) && twoLetterToThreeLetterCountryMap[key] === country) {
                return key;
            }
        }
        return '';
    }

    /**
     * Extract the langueage code from the locale string
     * @param  {string} locale the locale string eg. en-us
     * @return {string} the language code eg. en
     */
    function parseLanguageCode(locale) {
        return locale.substr(0, 2);
    }

    /**
     * Get a property from a map or use the map's default if defined
     * @param {object} map the map to search
     * @param {string} the keyname to match
     * @return {string} the value of the key or default
     */
    function getProperty(map, key) {
        var defaultValue = map['default'];

        if (!key) {
            return defaultValue;
        }

        return map[key] || defaultValue;
    }
