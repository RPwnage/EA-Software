    /**
     * Get the cased country code by locale string
     * @param  {string} locale the locale string eg. en-us
     * @return {string} the cased locale sting eg. en_US
     */
    function parseCasedLocale(locale) {
        var boom = locale.split('-');
        return [boom[0], boom[1].toUpperCase()].join('_');
    }
