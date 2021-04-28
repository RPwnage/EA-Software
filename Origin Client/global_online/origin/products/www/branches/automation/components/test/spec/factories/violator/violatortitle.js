/**
 * Jasmine functional test
 */

'use strict';

describe('Violator title factory', function() {
    var ViolatorTitleFactory,
        CONTEXT_NAMESPACE = 'origin-gamelibrary-ogd-violator';

    beforeEach(function() {
        window.OriginComponents = {};
        window.Origin = {
            locale: {
                languageCode: function() {
                    return '';
                },
                countryCode: function() {
                    return '';
                },
                threeLetterCountryCode: function() {
                    return '';
                },
                locale: function() {
                    return '';
                }
            }
        };
        angular.mock.module('origin-components');
        module(function($provide){
            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: function(){},
                    error: function(){}
                };
            });
        });
    });

    beforeEach(inject(function(_ViolatorTitleFactory_) {
        ViolatorTitleFactory = _ViolatorTitleFactory_;
    }));

    describe('getTitle', function() {
        var scope;

        beforeEach(function() {
            scope = {
                daystemplate: 'less than %days% days',
                hourstemplate: '{1} 1 hour or less | ]0, +Inf] less than %hours% hours',
                minutestemplate: ']-1, 5] less than 5 minutes | ]5, 15] less than 15 minutes | ]15, 30] less than 30 minutes | ]30,+Inf] less than an hour'
            };
        })

        it('will append a normal programmed title string', function() {
            scope.title = 'This is a title';

            expect(ViolatorTitleFactory.getTitle(scope, CONTEXT_NAMESPACE)).toEqual('This is a title');
        });

        it('will add a countdown timer directive for a fixed date string', function() {
            _.merge(scope, {
                title: 'promotion ends in %enddate%',
                enddate: '2016-01-20T00:00:00Z',
                livecountdown: 'fixed-date'
            });

            expect(ViolatorTitleFactory.getTitle(scope, CONTEXT_NAMESPACE)).toEqual('promotion ends in <origin-game-violator-timer enddate="2016-01-20T00:00:00Z" format=""></origin-game-violator-timer>');
        });

        it('will add a countdown timer for a limited time dynamic - less than 2 hours', function(){
            _.merge(scope, {
                title: 'Time left in Trial: %time%.',
                timeremaining: '7201',
                livecountdown: 'limited'
            });

            expect(ViolatorTitleFactory.getTitle(scope, CONTEXT_NAMESPACE)).toEqual('Time left in Trial: less than 3 hours.');
        });

        it('will add a countdown timer for a limited time dynamic - hours -> minutes threshold', function(){
            _.merge(scope, {
                title: 'Time left in Trial: %time%.',
                timeremaining: '3600',
                livecountdown: 'limited'
            });

            expect(ViolatorTitleFactory.getTitle(scope, CONTEXT_NAMESPACE)).toEqual('Time left in Trial: less than 2 hours.');
        });

        it('will add a countdown timer for a limited time dynamic - hours -> minutes threshold', function(){
            _.merge(scope, {
                title: 'Time left in Trial: %time%.',
                timeremaining: '3599',
                livecountdown: 'limited'
            });

            expect(ViolatorTitleFactory.getTitle(scope, CONTEXT_NAMESPACE)).toEqual('Time left in Trial: less than an hour.');
        });

        it('will add a countdown timer for a limited time dynamic timestamp - 5 minute warning', function(){
            _.merge(scope, {
                title: 'Time left in Trial: %time%.',
                timeremaining: '299',
                livecountdown: 'limited'
            });

            expect(ViolatorTitleFactory.getTitle(scope, CONTEXT_NAMESPACE)).toEqual('Time left in Trial: less than 5 minutes.');
        });

        it('will add a countdown timer for a limited time dynamic timestamp - no time left', function(){
            _.merge(scope, {
                title: 'Time left in Trial: %time%.',
                timeremaining: '0',
                livecountdown: 'limited'
            });

            expect(ViolatorTitleFactory.getTitle(scope, CONTEXT_NAMESPACE)).toEqual('Time left in Trial: .');
        });

        it('will append a timer that includes days for values that would look weird (more than a few days) expressed as hours', function() {
            _.merge(scope, {
                title: 'Time left in the universe: %time%.',
                timeremaining: '300000',
                livecountdown: 'days-hours-minutes'
            });

            expect(ViolatorTitleFactory.getTitle(scope, CONTEXT_NAMESPACE)).toEqual('Time left in the universe: less than 4 days.');
        });

        it('will append a timer that includes days - days -> hours threshold', function() {
            _.merge(scope, {
                title: 'Time left in the universe: %time%.',
                timeremaining: '86400',
                livecountdown: 'days-hours-minutes'
            });

            expect(ViolatorTitleFactory.getTitle(scope, CONTEXT_NAMESPACE)).toEqual('Time left in the universe: less than 2 days.');
        });

        it('will append a timer that includes days - days -> hours threshold', function() {
            _.merge(scope, {
                title: 'Time left in the universe: %time%.',
                timeremaining: '86399',
                livecountdown: 'days-hours-minutes'
            });

            expect(ViolatorTitleFactory.getTitle(scope, CONTEXT_NAMESPACE)).toEqual('Time left in the universe: less than 24 hours.');
        });
    });
});
