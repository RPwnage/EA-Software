/**
 * Jasmine functional test
 */

'use strict';

describe('Games Filter Factory', function() {
    var gamesFilterFactory,
        subscriptionFactory,
        gameTemplate = {
            gameNameFacetKey: 'test-facet',
            isAlphaBetaDemo: false,
            gameDistributionSubType: 'Normal Game',
            isSubscription: false,
            gameEditionTypeFacetKeyRankDesc: 3000,
            isInstalled: false,
            isTrial: false,
            isEarlyAccess: false,
            isReleased: true,
            isPreorder: false,
            isSuppressed: false
        };

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        window.Origin = {
            log: {
                message: angular.noop
            },
            utils: {
                Communicator: angular.noop
            },
            user: {
                userPid: function() {}
            },
            events: {
                on: angular.noop,
                off: angular.noop
            }
        };
        module(function($provide) {
            $provide.factory('GamesDataFactory', function() {
                return {};
            });
            $provide.factory('PlatformRefiner', function() {
                return {
                    isReleasedOnAnyPlatform: function() {
                        return function(model) {
                            return model.isReleased;
                        };
                    }
                };
            });
        });
    });

    beforeEach(inject(function(_GamesFilterFactory_, _SubscriptionFactory_) {
        gamesFilterFactory = _GamesFilterFactory_;
        subscriptionFactory = _SubscriptionFactory_;
    }));

    describe('Base game suppression', function() {
        var baseGame1, baseGame2, games, suppressed;
        beforeEach(function() {
            baseGame1 = _.cloneDeep(gameTemplate),
            baseGame2 = _.cloneDeep(gameTemplate);

            baseGame2.gameEditionTypeFacetKeyRankDesc = 9000;
        });

        it('will not suppress lesser base games under normal circumstances', function(done) {
            spyOn(subscriptionFactory,'isActive').and.returnValue(true);
            games = gamesFilterFactory.hideLesserEditions([baseGame1, baseGame2]),
            suppressed = _.filter(games, { 'isSuppressed': true });
            expect(suppressed.length).toBe(0);
            done();
        });
    });
    describe('Subscription suppression', function() {
        var subsGame, lesserGame, games, suppressed;
        beforeEach(function() {
            subsGame = _.cloneDeep(gameTemplate),
            lesserGame = _.cloneDeep(gameTemplate);

            subsGame.isSubscription = true;
        });

        it('will suppress lesser offers if user has an active subscription', function(done) {
            spyOn(subscriptionFactory,'isActive').and.returnValue(true);
            games = gamesFilterFactory.hideLesserEditions([subsGame, lesserGame]),
            suppressed = _.filter(games, { 'isSuppressed': true });
            expect(suppressed.length).toBe(1);
            done();
        });
        it('will not suppress lesser offers if users subscription has lapsed', function(done) {
            spyOn(subscriptionFactory,'isActive').and.returnValue(false);
            games = gamesFilterFactory.hideLesserEditions([subsGame, lesserGame]),
            suppressed = _.filter(games, { 'isSuppressed': true });
            expect(suppressed.length).toBe(0);
            done();
        });
    });
    describe('Early access suppression', function() {
        var baseGame, earlyAccessGame, games, suppressed;
        beforeEach(function() {
            baseGame = _.cloneDeep(gameTemplate),
            earlyAccessGame = _.cloneDeep(gameTemplate);

            earlyAccessGame.isEarlyAccess = true;
        });

        it('will not suppress early access offer if the base game is unreleased', function(done) {
            baseGame.isReleased = false;
            games = gamesFilterFactory.hideLesserEditions([baseGame, earlyAccessGame]),
            suppressed = _.filter(games, { 'isSuppressed': true });
            expect(suppressed.length).toBe(0);
            done();
        });
        it('will suppress early access offer if the base game is released', function(done) {
            games = gamesFilterFactory.hideLesserEditions([baseGame, earlyAccessGame]),
            suppressed = _.filter(games, { 'isSuppressed': true });
            expect(suppressed.length).toBe(1);
            done();
        });
    });
    describe('Trial suppression', function() {
        var baseGame1, baseGame2, trialGame, games, suppressed;
        beforeEach(function() {
            baseGame1 = _.cloneDeep(gameTemplate),
            baseGame2 = _.cloneDeep(gameTemplate),
            trialGame = _.cloneDeep(gameTemplate);

            trialGame.isTrial = true;
        });

        it('will not suppress the trial if no matching base game is found', function(done) {
            baseGame1.gameNameFacetKey = 'mismatched-facet';
            baseGame2.gameNameFacetKey = 'other-facet';
            games = gamesFilterFactory.hideLesserEditions([baseGame1, baseGame2, trialGame]),
            suppressed = _.filter(games, { 'isSuppressed': true });
            expect(suppressed.length).toBe(0);
            done();
        });
        it('will suppress the trial if a matching base game is found', function(done) {
            games = gamesFilterFactory.hideLesserEditions([baseGame1, baseGame2, trialGame]),
            suppressed = _.filter(games, { 'isSuppressed': true });
            expect(suppressed.length).toBe(1);
            done();
        });
    });
    describe('Alpha/beta/demo suppression', function() {
        var baseGame, demoGame, trialGame, games, suppressed;
        beforeEach(function() {
            baseGame = _.cloneDeep(gameTemplate),
            demoGame = _.cloneDeep(gameTemplate),
            trialGame = _.cloneDeep(gameTemplate);

            demoGame.isAlphaBetaDemo = true;
            trialGame.isTrial = true;
        });

        it('will not allow suppression of an alpha/beta/demo by a base game', function(done) {
            games = gamesFilterFactory.hideLesserEditions([baseGame, demoGame]),
            suppressed = _.filter(games, { 'isSuppressed': true });
            expect(suppressed.length).toBe(0);
            done();
        });
        it('will not allow suppression of a trial by an alpha/beta/demo', function(done) {
            games = gamesFilterFactory.hideLesserEditions([trialGame, demoGame]),
            suppressed = _.filter(games, { 'isSuppressed': true });
            expect(suppressed.length).toBe(0);
            done();
        });
    });
    describe('Preorder suppression', function() {
        var ownedGame, preorderGame1, preorderGame2, games, suppressed;
        beforeEach(function() {
            ownedGame = _.cloneDeep(gameTemplate),
            preorderGame1 = _.cloneDeep(gameTemplate),
            preorderGame2 = _.cloneDeep(gameTemplate);

            preorderGame1.isPreorder = true;
            preorderGame2.isPreorder = true;
        });

        it('will suppress the preorder if the user owns the game', function(done) {
            games = gamesFilterFactory.hideLesserEditions([ownedGame, preorderGame1, preorderGame2]),
            suppressed = _.filter(games, { 'isSuppressed': true });
            expect(suppressed.length).toBe(2);
            done();
        });
        it('will not allow suppression of a preorder by another preorder', function(done) {
            games = gamesFilterFactory.hideLesserEditions([preorderGame1, preorderGame2]),
            suppressed = _.filter(games, { 'isSuppressed': true });
            expect(suppressed.length).toBe(0);
            done();
        });
    });
});