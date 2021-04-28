'use strict';

describe('Origin Locale API', function() {
    describe('locale getters when uninitialized', function() {
        it('Will default to the USA locale', function() {
            expect(Origin.locale.locale()).toEqual('en_US');
            expect(Origin.locale.countryCode()).toEqual('US');
            expect(Origin.locale.threeLetterCountryCode()).toEqual('USA');
            expect(Origin.locale.currencyCode()).toEqual('USD');
        });
    });

    describe('locale setters will set a value and fire an event', function() {
        beforeEach(function() {
            spyOn(Origin.events, 'fire').and.callThrough();
        });

    	it('Will update the locale - case sensitive input and output', function() {
            Origin.locale.setLocale('en_GB');
            expect(Origin.events.fire).toHaveBeenCalledWith(Origin.events.LOCALE_CHANGED);
            expect(Origin.locale.locale()).toBe('en_GB');
        });

        it('Will update the countryCode  - case insensitive input - uppercase output', function() {
            Origin.locale.setCountryCode('Gb');
            expect(Origin.events.fire).toHaveBeenCalledWith(Origin.events.COUNTRY_CODE_CHANGED);
            expect(Origin.locale.countryCode()).toBe('GB');
        });

        it('Will update the threeLetterCountryCode - case insensitive input - uppercase output', function() {
            Origin.locale.setThreeLetterCountryCode('Gbr');
            expect(Origin.events.fire).toHaveBeenCalledWith(Origin.events.THREE_LETTER_COUNTRY_CODE_CHANGED);
            expect(Origin.locale.threeLetterCountryCode()).toBe('GBR');
        });

        it('Will update the currencyCode - case insensitive input - uppercase output', function() {
            Origin.locale.setCurrencyCode('gbp');
            expect(Origin.events.fire).toHaveBeenCalledWith(Origin.events.CURRENCY_CODE_CHANGED);
            expect(Origin.locale.currencyCode()).toBe('GBP');
        });
    });

    describe('attempting to set the locale again to the same locale', function () {
        it('will be ignored', function() {
            Origin.locale.setLocale('en_US');
            spyOn(Origin.events, 'fire').and.callThrough();
            Origin.locale.setLocale('en_US');

            expect(Origin.events.fire).not.toHaveBeenCalled();
        });
    });
});
