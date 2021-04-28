/**
 * @file home/recommendedstory.js
 */
(function() {
    'use strict';

    function RecommendedStoryFactory($timeout, FeedFactory, GamesDataFactory, ComponentsCMSFactory) {
        var recStoryTypes = [];

        function createReturnObject(incomingUseThis) {
            return {
                useThis: incomingUseThis
            };
        }

        function createOfferReponse(offerId, secondaryCheck) {
            var returnObj = {};
            var extraCheck = true;
            if (typeof secondaryCheck !== 'undefined') {
                extraCheck = secondaryCheck;
            }

            if (offerId && extraCheck) {
                returnObj = createReturnObject(true);
                returnObj.offerId = offerId;
            } else {
                returnObj = createReturnObject(false);
            }
            return returnObj;
        }

        function checkUserNotLoggedIn() {
            var notLoggedIn = false;
            var returnObj = {};
            if (!Origin.auth.isLoggedIn()) {
                notLoggedIn = true;
            }

            returnObj = createReturnObject(notLoggedIn);
            return returnObj;
        }

        function checkITO() {
            return createOfferReponse(GamesDataFactory.getClientITOGameIndex());
        }

        function checkTrial() {
            return createOfferReponse(FeedFactory.getNewestTrial());
        }

        function checkNewGameReleased() {
            return createOfferReponse(FeedFactory.getNewestRelease());
        }

        function checkGameReadyForPreload() {
            return createOfferReponse(FeedFactory.getNewestPreload());
        }

        function checkUnderAgeNoGames() {
            return createReturnObject(Origin.user.underAge() && !GamesDataFactory.baseGameEntitlements().length);
        }

        function checkNonSubNoPlayable() {
            return createReturnObject(!Origin.user.subscription() && !GamesDataFactory.baseGameEntitlements().length);
        }

        function checkSubNoPlayable() {
            return createReturnObject(Origin.user.subscription() && !GamesDataFactory.baseGameEntitlements().length);
        }

        function checkNonSubPlayableNoRecent() {
            return createReturnObject(!Origin.user.subscription() && !FeedFactory.getPlayedAGameRecently(30));
        }

        function checkSubPlayableNoRecent() {
            return createReturnObject(Origin.user.subscription() && !FeedFactory.getPlayedAGameRecently(30));
        }

        function checkLastPlayedReady() {
            return createOfferReponse(FeedFactory.getLastPlayedGame());
        }

        function checkSubRecentlyChurnedOut() {
            return createReturnObject(false);
        }

        function addRecommendedStoryType(feedType, stateCheckFunction, visible) {
            recStoryTypes.push({
                feedType: feedType,
                checkState: stateCheckFunction,
                visible: visible
            });

        }

        function waitForFeedReady() {
            return new Promise(function(resolve, reject) {
                var storiesReady = FeedFactory.isOwnedGameInfoReady() || !Origin.auth.isLoggedIn(),
                    clientStateReady = (GamesDataFactory.initialClientStatesReceived() || !Origin.auth.isLoggedIn()),
                    timeoutPromise = null;

                function checkAllStatesReady() {
                    if (storiesReady) {
                        $timeout.cancel(timeoutPromise);
                        resolve();
                    }
                }

                function setStoryReady() {
                    storiesReady = true;
                    checkAllStatesReady();
                }

                function setClientStateReady() {
                    clientStateReady = true;
                    checkAllStatesReady();
                }

                //give it 30 seconds and time out
                timeoutPromise = $timeout(function() {
                    reject(new Error('[RecommendedStoryFactory:waitForFeedReady timedout]'));
                }, 30000, false);

                FeedFactory.events.once('story:ownedGameInfoReady', setStoryReady);
                GamesDataFactory.events.once('games:initialClientStateReceived', setClientStateReady);
                checkAllStatesReady();

            });
        }

        function findSelectedStory() {
            var chosenStory = recStoryTypes[recStoryTypes.length - 1],
                i;
            for (i = 0; i < recStoryTypes.length; i++) {
                var storyInfoObj = recStoryTypes[i].checkState();
                if (storyInfoObj.useThis) {
                    chosenStory = recStoryTypes[i];

                    if (storyInfoObj.offerId) {
                        chosenStory.offerId = storyInfoObj.offerId;
                    } else {
                        chosenStory.offerId = null;
                    }

                    break;
                }
            }
            return chosenStory;
        }

        function createDirectiveElement(response) {
            var directiveElement = angular.element(response.xml).children();
            if (response.offerId) {
                directiveElement.attr('offerid', response.offerId);
            }
            return directiveElement[0];
        }

        function retrieveStoryXML(chosenStory) {
            var promise = null;

            if (chosenStory.visible) {
                promise = ComponentsCMSFactory.retrieveRecommendedStoryData(chosenStory);
            } else {
                promise = Promise.resolve({xml:''});
            }
            return promise;
        }

        //the order of these calls is the order of priority for the
        //recommended next action check
        addRecommendedStoryType('not-logged-in', checkUserNotLoggedIn, true);
        addRecommendedStoryType('install-through-origin', checkITO, true);
        addRecommendedStoryType('trial', checkTrial, true);
        addRecommendedStoryType('og-just-released', checkNewGameReleased, true);
        addRecommendedStoryType('og-game-preload-ready', checkGameReadyForPreload, true);
        addRecommendedStoryType('underage-nogames', checkUnderAgeNoGames, false);
        addRecommendedStoryType('subscriber-no-playable-games', checkSubNoPlayable, true);
        addRecommendedStoryType('non-subscriber-no-playable-games', checkNonSubNoPlayable, true);
        addRecommendedStoryType('non-subscriber-explore-vault', checkNonSubPlayableNoRecent, true);
        addRecommendedStoryType('subscriber-explore-vault', checkSubPlayableNoRecent, true);
        addRecommendedStoryType('last-game-played', checkLastPlayedReady, true);
        addRecommendedStoryType('subscription-expired', checkSubRecentlyChurnedOut, true);

        return {
            getStoryDirective: function() {
                return waitForFeedReady()
                    .then(findSelectedStory)
                    .then(retrieveStoryXML)
                    .then(createDirectiveElement);
            }
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function RecommendedStoryFactorySingleton($timeout, FeedFactory, GamesDataFactory, ComponentsCMSFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('RecommendedStoryFactory', RecommendedStoryFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.RecommendedStoryFactory
     
     * @description
     *
     * RecommendedStoryFactory
     */
    angular.module('origin-components')
        .factory('RecommendedStoryFactory', RecommendedStoryFactorySingleton);
}());