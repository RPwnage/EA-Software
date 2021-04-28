/**
 * Jasmine functional test
 */

'use strict';

describe('Ogd Header Violator Rendering Factory', function() {
    var ogdHeaderViolatorFactory, dialogFactory;

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
            
            $provide.factory('DialogFactory', function() {
                return {
                    open: function() {}
                };
            })
        });
    });

    beforeEach(inject(function(_OgdHeaderViolatorFactory_, _DialogFactory_) {
        ogdHeaderViolatorFactory = _OgdHeaderViolatorFactory_;
        dialogFactory = _DialogFactory_;
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

            var htmlElements = ogdHeaderViolatorFactory.getInlineMessages(violatorData, offerId, inlineViolatorCount);

            expect(htmlElements[0].outerHTML)
                .toEqual('<origin-gamelibrary-ogd-violator violatortype="game-sunset" priority="critical" offerid="OFB-EAST:12345" enddate="" timeremaining="undefined" timerformat="undefined"></origin-gamelibrary-ogd-violator>');
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

            var htmlElements = ogdHeaderViolatorFactory.getInlineMessages(violatorData, offerId, inlineViolatorCount);

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

            var htmlElements = ogdHeaderViolatorFactory.getInlineMessages(violatorData, offerId, inlineViolatorCount);

            expect(htmlElements[0].outerHTML)
                .toEqual('<div class="origin-cms-targeting-wrapper"><origin-gamelibrary-ogd-important-info violatortype="programmed" priority="critical" title-str="foo" pass-thru="passed" cq-targeting-item="" cq-targeting-subscribers="default" offerid="OFB-EAST:12345"></origin-gamelibrary-ogd-important-info></div>');
        });
    });

    describe('getDialogMessages', function() {
        it('will return a {ReadMoreDialog} if the number of violator messages is greater than the inline message limit - integrators can use the .show() function to pop the dialog', function() {
            var offerId = 'OFB-EAST:12345',
                inlineViolatorCount = 1,
                gamemessages = 'Battlefield dialog title',
                closemessagescta = 'Close',
                violatorData = [{
                    'violatorType': 'automatic',
                    'label': 'game-sunset',
                    'priority': 'critical',
                    'offerid': offerId
                }, {
                    'violatorType': 'programmed',
                    'priority': 'critical',
                    'title-str': 'foo',
                    'pass-thru': 'passed',
                    'cq-targeting-item': '',
                    'cq-targeting-subscribers': 'default'
                }];

            spyOn(dialogFactory, 'open').and.callThrough();

            var readMoreDialog = ogdHeaderViolatorFactory.getDialogMessages(violatorData, offerId, gamemessages, closemessagescta);
            readMoreDialog.show();

            expect(dialogFactory.open).toHaveBeenCalled();
        });
    });

    describe('getReadMoreLinkHtml(totalMessageCount, inlineViolatorCount, viewmmoremessagesctaCallback)', function() {
        it('Will return a message for the number of remaining messages for use with modal links', function() {
            var offerId = 'OFB-EAST:12345',
                inlineViolatorCount = 1,
                messages = {
                    viewmmoremessagesctaCallback: function() {}
                },
                violatorData = [{
                    'violatorType': 'automatic',
                    'label': 'game-sunset',
                    'priority': 'critical',
                    'offerid': offerId
                }, {
                    'violatorType': 'programmed',
                    'priority': 'critical',
                    'title-str': 'foo',
                    'pass-thru': 'passed',
                    'cq-targeting-item': '',
                    'cq-targeting-subscribers': 'default'
                }];

            spyOn(messages, 'viewmmoremessagesctaCallback').and.returnValue('+1 more message');

            var readmoreLinkHtml = ogdHeaderViolatorFactory.getReadMoreLinkHtml(violatorData.length, inlineViolatorCount, messages.viewmmoremessagesctaCallback);

            expect(readmoreLinkHtml).toEqual('<div class="origin-ogd-message origin-ogd-messages-readmore"><a class="origin-ogd-messages-readmore-link" ng-click="readmore.show()">+1 more message</a></div>');
            expect(messages.viewmmoremessagesctaCallback).toHaveBeenCalledWith({'%messages%': 1}, 1);
        });
    });
});