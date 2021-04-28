    /**
     * Remap locale to the three letter language code (used by EA Madrid translation services)
     * locales assigned by marketing
     * @type {Object}
     */
    var localeToThreeLetterLanguageCode = {
        'en': 'eng',
        'nl': 'dut',
        'es': 'spa',
        'fr': 'fre',
        'it': 'ita',
        'da': 'dan',
        'de': 'ger',
        'se': 'swe',
        'pt': 'por',
        'pl': 'pol',
        'nb': 'nor',
        'no': 'nor',
        'kr': 'kor',
        'ru': 'rus',
        'fi': 'fin',
        'zh': 'cht',
        'th': 'tha',
        'ja': 'jpn',
        'default': 'eng'
    };

    /**
     * Given a locale, select the appropriate 3 letter language code
     * @param  {string} locale the locale string eg. en-us
     * @return {string} the three letter language code eg. eng
     */
    function parseThreeLetterLanguageCode(locale) {
        return getProperty(localeToThreeLetterLanguageCode, parseLanguageCode(locale));
    }
