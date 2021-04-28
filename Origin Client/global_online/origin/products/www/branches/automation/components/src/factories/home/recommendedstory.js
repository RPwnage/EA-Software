/**
 * @file home/recommendedstory.js
 */
(function() {
    'use strict';

    function RecommendedStoryFactory(FeedFactory, GamesDataFactory, SubscriptionFactory, OcdHelperFactory, ObjectHelperFactory, UtilFactory, VaultRefiner) {
        var recStoryTypes = [];

        function hasSubscriptionWithSupportedPlatform() {
            return SubscriptionFactory.userHasSubscription() && SubscriptionFactory.userIsOnSupportedPlatform();
        }
        function hasSubscriptionButUnsupportedPlatform() {
            return SubscriptionFactory.userHasSubscription() && !SubscriptionFactory.userIsOnSupportedPlatform();
        }

        function createReturnObject(incomingUseThis, offerid) {
            return {
                useThis: incomingUseThis,
                offerId: offerid
            };
        }

        function createOfferResponse(offerId) {
            return createReturnObject(!!offerId, offerId);
        }

        function checkNextUnopenedGift() {
            return FeedFactory.getNextUnopenedGift().then(createOfferResponse);
        }

        function checkJustAcquired() {
            return FeedFactory.getJustAcquired().then(createOfferResponse);
        }

        function checkSubNoneInstalled() {
            return FeedFactory.getMostRecentSubsEntitlementNotInstalled().then(createOfferResponse);
        }

        function checkTrial() {
            return FeedFactory.getNewestTrial(false).then(createOfferResponse);
        }

        function checkTrialExpired() {
            return FeedFactory.getNewestTrial(true).then(createOfferResponse);
        }

        function checkNewGameReleased() {
            return FeedFactory.getNewestRelease().then(createOfferResponse);
        }

        function checkGameReadyForPreload() {
            return FeedFactory.getNewestPreload().then(createOfferResponse);
        }

        function checkUnderAgeNoGames() {
            return FeedFactory.getBaseGameEntitlements().then(function(entitlementsArray) {
                return createReturnObject(Origin.user.underAge() && !entitlementsArray.length);
            });
        }

        function checkNonSubNoPlayable() {
            return FeedFactory.getBaseGameEntitlements().then(function(entitlementsArray) {
                return createReturnObject(!entitlementsArray.length && (!hasSubscriptionWithSupportedPlatform() || hasSubscriptionButUnsupportedPlatform()));
            });
        }

        function checkSubNoPlayable() {
            return FeedFactory.getBaseGameEntitlements().then(function(entitlementsArray) {
                return createReturnObject(hasSubscriptionWithSupportedPlatform() && !entitlementsArray.length);
            });
        }

        function checkNonSubPlayableNoRecent() {
            return FeedFactory.getPlayedAGameRecently(30).then(function(playedRecently) {
                return createReturnObject(!playedRecently && (!hasSubscriptionWithSupportedPlatform() || hasSubscriptionButUnsupportedPlatform()));
            });
        }

        function checkSubPlayableNoRecent() {
            return FeedFactory.getPlayedAGameRecently(30).then(function(playedRecently) {
                return createReturnObject(hasSubscriptionWithSupportedPlatform() && !playedRecently);
            });
        }

        function checkDownloadedRecently() {
            return FeedFactory.getDownloadedGameNewerLastPlayed().then(createOfferResponse);
        }

        function checkLastPlayedReady() {
            return FeedFactory.getLastPlayedGame().then(createOfferResponse);
        }

        function checkSubRecentlyCancelled(contextName) {
            return function() {
                var daysSinceCancel = Number(UtilFactory.getLocalizedStr(null, contextName, 'daysSinceCancel')) || 15,
                    show = SubscriptionFactory.isExpired() && VaultRefiner.isSubsExpiredForAtLeastNumDays(SubscriptionFactory.getExpiration(), daysSinceCancel);

                 return Promise.resolve(createReturnObject(show));
            };
        }

        function checkNoRecommendedAction() {
            return Promise.resolve(createReturnObject(true));
        }
        /**
         * helper to add to our recommended action list
         * @param {string}  directiveName           the name of the associated directive
         * @param {function}  stateCheckFunction    function to check if we should select this action
         */
        function addRecommendedStoryType(directiveName, stateCheckFunction) {
            recStoryTypes.push({
                directiveName: directiveName,
                checkState: stateCheckFunction
            });
        }

        function handleError() {
            return {};
        }

        /**
         * finds the recommended action to show
         * @return {object} object that contanins directive name and offer id (if applicable)
         */
        function findSelectedStory() {
            var chosenStory = recStoryTypes[recStoryTypes.length - 1],
                storyInfoObj = null,
                feedPromises = [],
                i;

            //add all the dependencies needed to figure out what we are supposed to show
            for (i = 0; i < recStoryTypes.length; i++) {
                feedPromises.push(recStoryTypes[i].checkState().catch(handleError));
            }

            //when they are all resolved lets go through in order and figure out which is the first match
            return Promise.all(feedPromises).then(function(response) {
                for (var i = 0; i < response.length; i++) {
                    storyInfoObj = response[i];
                    if (storyInfoObj.useThis) {
                        chosenStory = recStoryTypes[i];
                        chosenStory.offerId = storyInfoObj.offerId || null;
                        break;
                    }
                }
                return chosenStory;
            });

        }

        /**
         * Use this callback if an OCD override is defined
         * @return {Function}
         */
        function slingDataToDomCallback(elementName, elementAttributes) {
            /**
             * @param {mixed} data the OCD response to process
             * @return {Object} HTMLElement
             */
            return function(data) {
                data[elementName] = _.merge(elementAttributes, data[elementName]);
                return OcdHelperFactory.slingDataToDom(data);
            };
        }

        /**
         * Use this callback to build a fallback tag in case there's no OCD override
         * @param  {string} elementName       The element Name
         * @param  {Object} elementAttributes An attribute list
         * @return {Function}
         */
        function buildTagCallback(elementName, elementAttributes) {
            /**
             * @return {Object} HTMLElement
             */
            return function() {
                return OcdHelperFactory.buildTag(elementName, elementAttributes);
            };
        }

        /**
        * Put the data into a wrapper object with the directive name as the key
        * @param {String} directiveName
        * @return {Function}
        */
        function stuffIntoDirectiveName(directiveName) {
            /**
            * @param {Object} data - data to stuff into wrapper
            * @return {Object}
            */
            return function(data) {
                var obj = {};
                obj[directiveName] = data;
                return obj;
            };
        }


        /**
         * Retrieves OCD data and if an override exists, it is rendered, otherwise fall back to a default tak and offer id
         * with the same name as the story.
         * @param  {object} chosenStory contains the offerId and directive name
         * @return {Promise.<Object, Error>} The first tag
         */
        function buildDirectiveWithOCD(chosenStory) {
            return Promise.all([
                GamesDataFactory.getOcdByOfferId(chosenStory.offerId)
                    .then(OcdHelperFactory.findDirectives('origin-game-defaults'))
                    .then(OcdHelperFactory.flattenDirectives('origin-game-defaults'))
                    .then(_.head)
                    .then(stuffIntoDirectiveName(chosenStory.directiveName)),
                GamesDataFactory.getOcdByOfferId(chosenStory.offerId)
                    .then(OcdHelperFactory.findDirectives(chosenStory.directiveName))
                    .then(_.head)
                ])
                .then(_.spread(_.merge))
                .then(ObjectHelperFactory.maybe(
                    slingDataToDomCallback,
                    [
                        chosenStory.directiveName,
                        { offerid: chosenStory.offerId }
                    ],
                    buildTagCallback,
                    [
                        chosenStory.directiveName,
                        { offerid: chosenStory.offerId }
                    ]
                ));
        }


        /**
         * give a chose recommended action, return the directive with attributes as a string
         * @param  {object} chosenStory contains the offerId and direective name
         * @return {string}             the directive with attributes
         */
        function retrieveStoryXML(chosenStory) {
            if (chosenStory.directiveName) {
                if (chosenStory.offerId) {
                    return buildDirectiveWithOCD(chosenStory);
                } else {
                    //no need to retrieve ocd, just build a tag from the directive name
                    return Promise.resolve(OcdHelperFactory.buildTag(chosenStory.directiveName, {}));
                }
            } else {
                return Promise.resolve();
            }
        }

        //the order of these calls is the order of priority for the
        //recommended next action check
        addRecommendedStoryType('origin-home-recommended-action-gift', checkNextUnopenedGift);
        addRecommendedStoryType('origin-home-recommended-action-justacquired', checkJustAcquired);
        addRecommendedStoryType('origin-home-recommended-action-submostrecentnotinstalled', checkSubNoneInstalled);
        addRecommendedStoryType('origin-home-recommended-action-trial', checkTrial);
        addRecommendedStoryType('origin-home-recommended-action-trial', checkTrialExpired);
        addRecommendedStoryType('origin-home-recommended-action-newreleased', checkNewGameReleased);
        addRecommendedStoryType('origin-home-recommended-action-preload', checkGameReadyForPreload);
        addRecommendedStoryType(null, checkUnderAgeNoGames);
        addRecommendedStoryType('origin-home-recommended-action-downloadedrecently', checkDownloadedRecently);
        addRecommendedStoryType('origin-home-recommended-action-subnotplayable', checkSubNoPlayable);
        addRecommendedStoryType('origin-home-recommended-action-nonsubnotplayable', checkNonSubNoPlayable);
        addRecommendedStoryType('origin-home-recommended-action-nonsubnotrecentlyplayed', checkNonSubPlayableNoRecent);
        addRecommendedStoryType('origin-home-recommended-action-subnotrecentlyplayed', checkSubPlayableNoRecent);
        addRecommendedStoryType('origin-home-recommended-action-lastgameplayed', checkLastPlayedReady);
        addRecommendedStoryType('origin-home-recommended-action-subrecentlycanceled', checkSubRecentlyCancelled('origin-home-recommended-action-subrecentlycanceled'));
        //this is the fallback action. if none of the requirements above are met, we will hit this
        addRecommendedStoryType(null, checkNoRecommendedAction);

        return {
            getStoryDirective: function() {
                return findSelectedStory()
                    .then(retrieveStoryXML);

            }
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function RecommendedStoryFactorySingleton(FeedFactory, GamesDataFactory, SubscriptionFactory, OcdHelperFactory, ObjectHelperFactory, UtilFactory, VaultRefiner, SingletonRegistryFactory) {
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
