/**
 * Jasmine functional test
 */

'use strict';

describe('Game Tile Violator Rendering Factory', function() {
    var gametileViolatorFactory;

    beforeEach(function() {
        window.OriginComponents = {};
        window.Origin = {};
        angular.mock.module('origin-components');

        module(function($provide) {
            $provide.factory('GameViolatorFactory', function() {
                var violatorType = {
                    automatic: 'automatic',
                    programmed: 'programmed'
                };

                return {
                    violatorType: violatorType,
                    isValidViolatorData: function() {return true;}
                };
            });

            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: function() {},
                    error: function() {}
                };
            });

            $provide.factory('SubscriptionFactory', function() {
                return {
                    events: {
                        'on': function() {},
                        'fire': function() {}
                    }
                };
            });

            $provide.factory('GamesDataFactory', function() {
                return {
                    events: {
                        'on': function() {},
                        'fire': function() {}
                    }
                };
            });

            $provide.factory('AuthFactory', function() {
                return {
                    events: {
                        'on': function() {},
                        'fire': function() {}
                    }
                };
            });

            $provide.factory('GamesEntitlementFactory', function() {
                return {
                    events: {
                        'on': function() {},
                        'fire': function() {}
                    }
                };
            });      
                        
        });
    });

    beforeEach(inject(function(_GametileViolatorFactory_) {
        gametileViolatorFactory = _GametileViolatorFactory_;
    }));

    describe('getInlineMessages - automatic violators', function() {
        it('will build a dom document for $compiling', function() {
            var offerId = 'OFB-EAST:12345',
                inlineViolatorCount = 1,
                violatorData = [{
                    'violatorType': 'automatic',
                    'label': 'game-sunset',
                    'priority': 'critical',
                    'offerid': offerId
                }];

            var htmlElements = gametileViolatorFactory.getInlineMessages(violatorData, offerId, inlineViolatorCount);

            expect(htmlElements[0].outerHTML)
                .toEqual('<origin-game-violator-automatic violatortype="game-sunset" priority="critical" offerid="OFB-EAST:12345" enddate="" timeremaining="undefined" timerformat="undefined"></origin-game-violator-automatic>');
        });

        it('will use an optional end date if provided', function() {
            var offerId = 'OFB-EAST:12345',
                inlineViolatorCount = 1,
                violatorData = [{
                    'violatorType': 'automatic',
                    'label': 'game-expiring',
                    'priority': 'critical',
                    'offerid': offerId,
                    'endDate': new Date('Thu Sep 08 2016 21:24:35 GMT+0000 (UTC)'),
                    'timerformat': 'fixed-date'
                }];

            var htmlElements = gametileViolatorFactory.getInlineMessages(violatorData, offerId, inlineViolatorCount);

            expect(htmlElements[0].outerHTML)
                .toContain('enddate');
        });
    });

    describe('getInlineMessages - programmed violators', function() {
        it('will build a dom document for $compiling including a pad for attaching targeting', function() {
            var offerId = 'OFB-EAST:12345',
                inlineViolatorCount = 1,
                violatorData = [{
                    'violatorType': 'programmed',
                    'priority': 'critical',
                    'title-str': 'foo',
                    'pass-thru': 'passed',
                    'cq-targeting-item': '',
                    'cq-targeting-subscribers': 'default'
                }];

            var htmlElements = gametileViolatorFactory.getInlineMessages(violatorData, offerId, inlineViolatorCount);

            expect(htmlElements[0].outerHTML)
                .toEqual('<div class="origin-cms-targeting-wrapper"><origin-game-violator-programmed violatortype="programmed" priority="critical" title-str="foo" pass-thru="passed" cq-targeting-item="" cq-targeting-subscribers="default" offerid="OFB-EAST:12345"></origin-game-violator-programmed></div>');
        });
    });
});