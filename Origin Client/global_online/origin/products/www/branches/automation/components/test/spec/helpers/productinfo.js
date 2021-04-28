/**
 * ProductInfo global helper functions
 */

'use strict';

describe('ProductInfoHelper - helper functions', function() {
    var productInfoHelper;

    beforeEach(function () {
        window.OriginComponents = {};
        module('origin-components');
    });

    beforeEach(inject(function (_ProductInfoHelper_) {
        productInfoHelper = _ProductInfoHelper_;
    }));

    describe('ProductInfoHelper isReleased', function() {
        it('returns true if game is released in the past', function() {

            var model = {
                releaseDate : new Date('Mon Nov 17 2014 21:00:00 GMT-0800 (Pacific Standard Time)')
            };
            expect(productInfoHelper.isReleased(model)).toBeTruthy();
        });

        it('returns true if game is released in the past', function() {

            var model = {
                releaseDate : new Date('Mon Nov 17 2014 21:00:00 GMT-0800 (Pacific Standard Time)')
            };
            expect(productInfoHelper.isReleased(model)).toBeTruthy();
        });

        it('returns false if game is to be released in the future', function() {

            var model = {
                releaseDate : new Date('Mon Nov 17 2019 21:00:00 GMT-0800 (Pacific Standard Time)')
            };

            expect(productInfoHelper.isReleased(model)).toBeFalsy();
        });

        it('returns true if game is to be released right now', function() {

            var model = {
                releaseDate : new Date()
            };

            expect(productInfoHelper.isReleased(model)).toBeTruthy();
        });

        it('returns false if game release date is undefined', function() {

            var model = {
            };

            expect(productInfoHelper.isReleased(model)).toBeFalsy();
        });
    });
});