/**
 * Jasmine functional test
 */

'use strict';

describe('MD5 Factory', function() {
    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide){
            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: function(message){}
                }
            })
        });
    });

    var md5;

    beforeEach(inject(function(_Md5Factory_){
        md5 = _Md5Factory_;
    }));

    describe('When using arrays as input', function() {
        it('Should provide a unique fingerprint for each array if the array has changed', function() {
            var fingerprint1 = md5.md5(['abc123', 'foobazbat']);
            var fingerprint2 = md5.md5(['barfoo', 'mass effect']);
            var fingerprint3 = md5.md5(['abc123', 'foobazbat']);

            expect(fingerprint1).toEqual('15fcbd8a46bdb881a3b87a1e0ed671ad');
            expect(fingerprint2).toEqual('47af783c1f4a8172a8f629c866d87510');
            expect(fingerprint1).not.toEqual(fingerprint2);
            expect(fingerprint1).toEqual(fingerprint3);
        });

        it('Should proceed as designed if provided with a string', function() {
            var fingerprint1 = md5.md5('abc123foobazbat');
            expect(fingerprint1).toEqual('15fcbd8a46bdb881a3b87a1e0ed671ad');
        });
    });
});