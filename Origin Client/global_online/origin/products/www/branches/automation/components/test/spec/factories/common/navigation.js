/**
 * Jasmine functional test
 */

'use strict';

describe('NavigationFactory', function() {

    var NavigationFactory, $state;

    beforeEach(function () {
        window.Origin = {
            'utils': {
                'Communicator': function() {}
            },
            'log': {
                'message': function() {}
            },
            'obfuscate': {
                'encode': function() {}
            }
        };

        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide) {

            $provide.factory('$state', function() {
                return {
                    is: function() {}
                };
            });

        });
    });

    beforeEach(inject(function(_NavigationFactory_, _$state_){
        NavigationFactory = _NavigationFactory_;
        $state = _$state_;
    }));

    describe('isState', function() {
        it('will return true if the state name matches', function() {
            spyOn($state, 'is').and.returnValue(true);

            expect(NavigationFactory.isState('some-state')).toEqual(true);
        });

        it('will return false is state name does not match', function() {
            spyOn($state, 'is').and.returnValue(false);

            expect(NavigationFactory.isState('some-state')).toEqual(false);
        });
    });
});