/**
 * Jasmine unit test
 */

'use strict';

describe('Origin Locale', function() {
    describe('Intitialization of a collection in a hash demarcated spa', function() {
        it('will parse the url field from in a non-usa region', function() {
            var locale = OriginLocale.parse("https://www.origin.com/gbr/en-gb/store/pdp/DR%3A225064100/info", 'en-us', 'usa');

            expect(locale.getLocale()).toBe('en-gb');
            expect(locale.getCasedLocale()).toBe('en_GB');
            expect(locale.getLanguageCode()).toBe('en');
            expect(locale.getCountryCode()).toBe('gb');
            expect(locale.getThreeLetterCountryCode()).toBe('gbr');
            expect(locale.getThreeLetterLanguageCode()).toBe('eng');
            expect(locale.getLocaleAlternates().length).toBeGreaterThan(1);
            expect(locale.getCurrencyCode()).toBe('gbp');
        });
        it('will parse the url field from in a usa region', function() {
            var locale = OriginLocale.parse("https://www.origin.com/store/pdp/DR%3A225064100/info", 'en-us', 'usa');

            expect(locale.getLocale()).toBe('en-us');
            expect(locale.getCasedLocale()).toBe('en_US');
            expect(locale.getLanguageCode()).toBe('en');
            expect(locale.getCountryCode()).toBe('us');
            expect(locale.getThreeLetterCountryCode()).toBe('usa');
            expect(locale.getThreeLetterLanguageCode()).toBe('eng');
            expect(locale.getLocaleAlternates().length).toBeGreaterThan(1);
            expect(locale.getCurrencyCode()).toBe('usd');
        });
        it('will parse the url with uppercase letters', function() {
            var locale = OriginLocale.parse("https://www.origin.com/GBR/en-gb/store/pdp/DR%3A225064100/info", 'en-us', 'usa');

            expect(locale.getLocale()).toBe('en-gb');
            expect(locale.getCasedLocale()).toBe('en_GB');
            expect(locale.getLanguageCode()).toBe('en');
            expect(locale.getCountryCode()).toBe('gb');
            expect(locale.getThreeLetterCountryCode()).toBe('gbr');
            expect(locale.getThreeLetterLanguageCode()).toBe('eng');
            expect(locale.getLocaleAlternates().length).toBeGreaterThan(1);
            expect(locale.getCurrencyCode()).toBe('gbp');

            var locale = OriginLocale.parse("https://www.origin.com/gbr/EN-gb/store/pdp/DR%3A225064100/info", 'en-us', 'usa');

            expect(locale.getLocale()).toBe('en-gb');
            expect(locale.getCasedLocale()).toBe('en_GB');
            expect(locale.getLanguageCode()).toBe('en');
            expect(locale.getCountryCode()).toBe('gb');
            expect(locale.getThreeLetterCountryCode()).toBe('gbr');
            expect(locale.getThreeLetterLanguageCode()).toBe('eng');
            expect(locale.getLocaleAlternates().length).toBeGreaterThan(1);
            expect(locale.getCurrencyCode()).toBe('gbp');

            var locale = OriginLocale.parse("https://www.origin.com/gbr/en-GB/store/pdp/DR%3A225064100/info", 'en-us', 'usa');

            expect(locale.getLocale()).toBe('en-gb');
            expect(locale.getCasedLocale()).toBe('en_GB');
            expect(locale.getLanguageCode()).toBe('en');
            expect(locale.getCountryCode()).toBe('gb');
            expect(locale.getThreeLetterCountryCode()).toBe('gbr');
            expect(locale.getThreeLetterLanguageCode()).toBe('eng');
            expect(locale.getLocaleAlternates().length).toBeGreaterThan(1);
            expect(locale.getCurrencyCode()).toBe('gbp');

            var locale = OriginLocale.parse("https://www.origin.com/GBR/EN-GB/store/pdp/DR%3A225064100/info", 'en-us', 'usa');

            expect(locale.getLocale()).toBe('en-gb');
            expect(locale.getCasedLocale()).toBe('en_GB');
            expect(locale.getLanguageCode()).toBe('en');
            expect(locale.getCountryCode()).toBe('gb');
            expect(locale.getThreeLetterCountryCode()).toBe('gbr');
            expect(locale.getThreeLetterLanguageCode()).toBe('eng');
            expect(locale.getLocaleAlternates().length).toBeGreaterThan(1);
            expect(locale.getCurrencyCode()).toBe('gbp');

        });

    });

    describe('Create URL from language code', function() {
        it('will create a new language variant of the URL for the USA', function() {
            var locale = OriginLocale.parse("https://www.origin.com/store/pdp/DR%3A225064100/info", 'en-us', 'usa');

            expect(locale.createUrl('https://www.origin.com/store/pdp/DR%3A225064100/info', 'es'))
                .toBe('https://www.origin.com/usa/es-us/store/pdp/DR%3A225064100/info');
        });

        it('will create an english link for the USA', function() {
            var locale = OriginLocale.parse("https://www.origin.com/usa/es-us/store/pdp/DR%3A225064100/info", 'en-us', 'usa');

            expect(locale.createUrl('https://www.origin.com/usa/es-us/store/pdp/DR%3A225064100/info', 'en'))
                .toBe('https://www.origin.com/store/pdp/DR%3A225064100/info');

            expect(locale.createUrl('https://www.origin.com/usa/es-us/store/pdp/DR%3A225064100/info', 'en'))
                .toBe('https://www.origin.com/store/pdp/DR%3A225064100/info');
        });

        it('will create a new language variant for the URL in Europe', function() {
            var locale = OriginLocale.parse("https://www.origin.com/gbr/en-gb/store/pdp/DR%3A225064100/info", 'en-gb', 'gbr');

            expect(locale.createUrl('https://www.origin.com/gbr/en-gb/store/pdp/DR%3A225064100/info', 'fr'))
                .toBe('https://www.origin.com/gbr/fr-gb/store/pdp/DR%3A225064100/info');
        });

    });
});
