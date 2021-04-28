/**
 * @file home/recommendedstory.js
 */
(function() {
    'use strict';

    function RecommendedStoryFactory($timeout, FeedFactory, GamesDataFactory, ComponentsCMSFactory, UtilFactory, AuthFactory, SubscriptionFactory) {
        var recStoryTypes = [];

        function createReturnObject(incomingUseThis) {
            return {
                useThis: incomingUseThis
            };
        }

        function createOfferResponse(offerId, secondaryCheck) {
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
            if (!AuthFactory.isAppLoggedIn()) {
                notLoggedIn = true;
            }

            returnObj = createReturnObject(notLoggedIn);
            return returnObj;
        }

        function checkJustAcquired() {
            return createOfferResponse(FeedFactory.getJustAcquired());
        }

        function checkSubNoneInstalled() {
            return createOfferResponse(FeedFactory.getMostRecentSubsEntitlementNotInstalled());
        }

        function checkTrial() {
            return createOfferResponse(FeedFactory.getNewestTrial());
        }

        function checkNewGameReleased() {
            return createOfferResponse(FeedFactory.getNewestRelease());
        }

        function checkGameReadyForPreload() {
            return createOfferResponse(FeedFactory.getNewestPreload());
        }

        function checkUnderAgeNoGames() {
            return createReturnObject(Origin.user.underAge() && !GamesDataFactory.baseGameEntitlements().length);
        }

        function checkNonSubNoPlayable() {
            return createReturnObject(!SubscriptionFactory.userHasSubscription() && !GamesDataFactory.baseGameEntitlements().length);
        }

        function checkSubNoPlayable() {
            return createReturnObject(SubscriptionFactory.userHasSubscription() && !GamesDataFactory.baseGameEntitlements().length);
        }

        function checkNonSubPlayableNoRecent() {
            return createReturnObject(!SubscriptionFactory.userHasSubscription() && !FeedFactory.getPlayedAGameRecently(30));
        }

        function checkSubPlayableNoRecent() {
            return createReturnObject(SubscriptionFactory.userHasSubscription() && !FeedFactory.getPlayedAGameRecently(30));
        }

        function checkDownloadedRecently() {
            return createOfferResponse(FeedFactory.getDownloadedGameNewerLastPlayed());
        }

        function checkLastPlayedReady() {
            return createOfferResponse(FeedFactory.getLastPlayedGame());
        }

        function checkSubRecentlyCancelled() {
            return createReturnObject(false);
        }

        function checkNoRecommendedAction() {
            return createReturnObject(true);
        }
        /**
         * helper to add to our recommended action list
         * @param {string}  directiveName           the name of the associated directive
         * @param {function}  stateCheckFunction    function to check if we should select this action
         * @param {Boolean} isOCD                   does this action use OCD
         */
        function addRecommendedStoryType(directiveName, stateCheckFunction, isOCD) {
            recStoryTypes.push({
                directiveName: directiveName,
                checkState: stateCheckFunction,
                isOCD: isOCD
            });

        }
        /**
         * waits until the feed factory is ready the resolves the promise
         * @return {promise} resolves when the feed factory has built the needed data
         */
        function waitForFeedReady() {
            return new Promise(function(resolve, reject) {
                var storiesReady = FeedFactory.isOwnedGameInfoReady() || !AuthFactory.isAppLoggedIn(),
                    clientStateReady = (GamesDataFactory.initialClientStatesReceived() || !AuthFactory.isAppLoggedIn()),
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

        /**
         * finds the recommended action to show
         * @return {object} reutrns an object that contanins directive name and offer id (if applicable)
         */
        function findSelectedStory() {
            var chosenStory = recStoryTypes[recStoryTypes.length - 1],
                storyInfoObj = null,
                i;

            for (i = 0; i < recStoryTypes.length; i++) {
                storyInfoObj = recStoryTypes[i].checkState();
                if (storyInfoObj.useThis) {
                    chosenStory = recStoryTypes[i];
                    chosenStory.offerId = storyInfoObj.offerId || null;
                    break;
                }
            }
            return chosenStory;
        }

        /**
         * adds an offerId as an attribute to the non-ocd directives
         * @param  {object} response directive tag with overriden values coming from a server
         * @return {string}          the updated directive tag with the offerid added
         */
        function createDirectiveElement(response) {
            var directiveElement = angular.element(response.xml).children();
            if (response.offerId) {
                directiveElement.attr('offerid', response.offerId);
            }
            return directiveElement[0];
        }

        /**
         * returns an object to be passed in to UtilFactory.buildTag
         * @param  {object} chosenStory contains the offerId and direective name
         * @return {object}             returns an array of an object with the directive name and a data object that will become attributes
         */
        function setupDirectivesObject(chosenStory) {
            return function(dataObject) {
                dataObject.offerId = chosenStory.offerId;
                return [{
                    name: chosenStory.directiveName,
                    data: dataObject
                }];
            };
        }

        /**
         * retrieves the OCD Info, then builds a directive with the OCD Info as attributes
         * @param  {object} chosenStory contains the offerId and direective name
         */
        function buildDirectiveWithOCD(chosenStory) {
            return GamesDataFactory.getOcdDirectiveByOfferId(chosenStory.directiveName, chosenStory.offerId, Origin.locale.locale())
                .then(setupDirectivesObject(chosenStory))
                .then(UtilFactory.buildTag);
        }

        /**
         * give a chose recommended action, return the directive with attributes as a string
         * @param  {object} chosenStory contains the offerId and direective name
         * @return {string}             the directive with attributes
         */
        function retrieveStoryXML(chosenStory) {
            var promise = null;

            if (chosenStory.directiveName) {
                //if its OCD build the directive using OCD info, other wise grab the full directive for now
                if (chosenStory.isOCD) {
                    promise = buildDirectiveWithOCD(chosenStory);
                } else {
                    promise = ComponentsCMSFactory.retrieveRecommendedStoryData(chosenStory.directiveName, chosenStory.offerId)
                        .then(createDirectiveElement);
                }
            } else {
                promise = Promise.resolve(null);
            }
            return promise;
        }


        //the order of these calls is the order of priority for the
        //recommended next action check
        addRecommendedStoryType('origin-home-recommended-action-notloggedin', checkUserNotLoggedIn);
        addRecommendedStoryType('origin-home-recommended-action-justacquired', checkJustAcquired, true);
        addRecommendedStoryType('origin-home-recommended-action-submostrecentnotinstalled', checkSubNoneInstalled, true);
        addRecommendedStoryType('origin-home-recommended-action-trial', checkTrial, true);
        addRecommendedStoryType('origin-home-recommended-action-newreleased', checkNewGameReleased, true);
        addRecommendedStoryType('origin-home-recommended-action-preload', checkGameReadyForPreload, true);
        addRecommendedStoryType(null, checkUnderAgeNoGames);
        addRecommendedStoryType('origin-home-recommended-action-downloadedrecently', checkDownloadedRecently, true);
        addRecommendedStoryType('origin-home-recommended-action-subnotplayable', checkSubNoPlayable);
        addRecommendedStoryType('origin-home-recommended-action-nonsubnotplayable', checkNonSubNoPlayable);
        addRecommendedStoryType('origin-home-recommended-action-nonsubnotrecentlyplayed', checkNonSubPlayableNoRecent);
        addRecommendedStoryType('origin-home-recommended-action-subnotrecentlyplayed', checkSubPlayableNoRecent);
        addRecommendedStoryType('origin-home-recommended-action-lastgameplayed', checkLastPlayedReady, true);
        addRecommendedStoryType('origin-home-recommended-action-subrecentlycanceled', checkSubRecentlyCancelled);
        //this is the fallback action. if none of the requirements above are met, we will hit this
        addRecommendedStoryType(null, checkNoRecommendedAction);
        return {
            /**
             * based on the current state of the user return a directive with attributes to show as the recommended next action
             * @return {string}             the directive with attributes
             */
            getStoryDirective: function() {
                return waitForFeedReady()
                    .then(findSelectedStory)
                    .then(retrieveStoryXML);
            }
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function RecommendedStoryFactorySingleton($timeout, FeedFactory, GamesDataFactory, ComponentsCMSFactory, UtilFactory, AuthFactory, SubscriptionFactory, SingletonRegistryFactory) {
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