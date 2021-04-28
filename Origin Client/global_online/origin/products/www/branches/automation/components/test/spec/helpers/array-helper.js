/**
 * ArrayHelper global helper functions
 */

'use strict';

describe('ArrayHelper - helper functions', function() {
    var arrayHelper;

    beforeEach(function () {
        window.OriginComponents = {};
        module('origin-components');
    });

    beforeEach(inject(function (_ArrayHelperFactory_) {
        arrayHelper = _ArrayHelperFactory_;
    }));
    describe('shuffle', function(){
        it('will shuffle an array with random function', function(){
            var array = [1, 2, 3];
            //Cuases array to rotate left
            var randomFn = function(){
                return 0;
            };
            var result = [2, 3, 1];
            arrayHelper.shuffle(array, randomFn);
            expect(array).toEqual(result);
        });
        
    });
    
    describe('concat', function() {
        it('will concat arbitary number of array parameters filters out non array arguments', function() {
            var args = [[1, 2], [3, 4, 5], [6], {a: 1}, '12'];
            var results = [1, 2, 3, 4, 5, 6];
            expect(arrayHelper.concat.apply(null, args)).toEqual(results);
        });
    });
});