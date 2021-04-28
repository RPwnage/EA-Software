    /**
     * The parser functionality for origin locale
     * @return {object} public api methods
     */
    function localeParser() {
        /**
         * Get the first occurence of /xx-xx(/ or #) in the URL
         * @param {string} url the url to analyze
         * @return {string} the locale string eg. fr-fr
         */
        function parseLocale(url, defaultLocale) {
            var localeMatches = matchStandardUrlPattern.exec(url);

            if (localeMatches && localeMatches[2]) {
                return localeMatches[2].toLowerCase();
            }

            return defaultLocale;
        }

        function parseThreeLetterCountryCode(url, defaultCountry) {
            var localeMatches = matchStandardUrlPattern.exec(url);

            if (localeMatches && localeMatches[1]) {
                return localeMatches[1].toLowerCase();
            }

            return defaultCountry;
        }

        /**
         * Parse the URL into a collection and return an API object with the cached object
         * @param {string} url the input url to parse eg https://www.example.com/fr-fr/store/buy/2982992
         * @param {string} defaultLocale if a locale string is not found, default to the given string eg en-us
         * @return {Object} instance of API
         */
        function parse(url, defaultLocale, defaultCountry) {
            var locale = parseLocale(url, defaultLocale),
                country = parseThreeLetterCountryCode(url, defaultCountry),
                collection = {
                    locale: locale,
                    casedLocale: parseCasedLocale(locale),
                    languageCode: parseLanguageCode(locale),
                    countryCode: parseCountryCode(country),
                    threeLetterCountryCode: country,
                    threeLetterLanguageCode: parseThreeLetterLanguageCode(locale),
                    localeAlternates: parseLocaleAlternates(locale),
                    currencyCode: parseCurrencyCode(country)
                };

            return api(collection);
        }

        return {
            parse: parse
        };
    }
