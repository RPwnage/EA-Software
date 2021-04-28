/**
 * Jasmine functional test
 */

'use strict';

describe('Pad zeroes filter', function() {
    var filter;

    beforeEach(function(){
        angular.mock.module('origin-components');
    });

    beforeEach(inject(function(_$filter_) {
        filter = _$filter_;
    }));

    describe('pad zeroes filter', function() {
        it('will pad a string with up to 8 chars in length', function() {
            var padZeroes = filter('padZeroes');
            expect(padZeroes('1', 8)).toBe('00000001');
        });
    });
});