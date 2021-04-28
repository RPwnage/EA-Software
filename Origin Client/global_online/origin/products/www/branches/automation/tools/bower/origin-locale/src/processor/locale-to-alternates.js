    /**
     * Regional language alternates
     * @type {Object}
     */
    var languageAlternates = {
        'default': [
            'en',
            'nl',
            'es',
            'fr',
            'it',
            'da',
            'de',
            'se',
            'pt',
            'pl',
            'nb',
            'no',
            'kr',
            'ru',
            'fi',
            'zh',
            'th',
            'ja'
        ]
    };

    /**
     * Generate a list of alternate languages available for the locale
     * @param {string} locale the locale string to analyze
     * @return {array} a list of alternate languages for the region
     * @see https://support.google.com/webmasters/answer/2620865?hl=en
     */
    function parseLocaleAlternates(locale) {
        return getProperty(languageAlternates, locale);
    }