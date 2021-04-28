/**
 * @file home/recommended/trial.js
 */
(function() {
    'use strict';

    function originHomeRecommendedActionTrial(ComponentsConfigFactory, ComponentsLogFactory, OcdHelperFactory, GamesDataFactory, GamesCatalogFactory, ViolatorModelFactory, DateHelperFactory, $compile) {

        function originHomeRecommendedActionTrialLink(scope, element) {

            var TEN_DAYS = 864000,
                timeoutHandle;

            /**
             * compiles the xml with the current directives scope
             * @param  {string} xml the raw uncompiled xml
             * @return {object}     returns an angular compiled html object
             */
            function compileXML(xml) {
                return $compile(xml)(scope);
            }

            /**
             * adds the element to the dom
             */
            function addToDom(compiledTag) {
                var container = element;
                container.html('');
                container.append(compiledTag);
                scope.$digest();

            }

            /**
             * gets to total seconds remaining in trial
             * @param  {number} trialTimeRemaining the time remaining
             * @param  {Date}   trialEndDate the trial end date
             * @return {number} returns the seconds remianing in trial
             */
            function getSecondsRemaining(trialTimeRemaining, trialEndDate) {
                return trialEndDate ? DateHelperFactory.getCountdownData(trialEndDate).totalSeconds : trialTimeRemaining;
            }

            /**
             * logs error message
             * @param  {Error} error the error object
             */
            function handleError(error) {
                ComponentsLogFactory.error('[originHomeRecommendedActionTrial]', error);
            }

            /**
             * switch to base game
             */
            function switchToBaseGame(offerId) {
                return function() {
                    Promise.resolve(buildTileMarkup('origin-home-recommended-action-trial-expired', offerId))
                        .then(compileXML)
                        .then(addToDom);
                };
            }

            /**
             * sets a switch timer based off the seconds remaining in the trial
             * @param {number} seconds the seconds remaining in trial
             */
            function setSwitchTimer(offerId) {
                return function(seconds) {
                    //only start a countdown if we are within 10 days and if we didn't expire
                    if (seconds && seconds < TEN_DAYS) {

                        if (seconds < 0) {
                            seconds = 0;
                        }

                        //setup the time out
                        timeoutHandle = setTimeout(switchToBaseGame(offerId), seconds * 1000);
                    }
                };
            }

            /**
             * clean up
             */
            function onDestroy() {
                clearTimeout(timeoutHandle);
            }

            /**
             * Get full base game associated with the trial
             * @param  {String} offerId offer id of trial
             * @return {Promise} offer id of full base game
             */
            function getBaseGameOfferId(offerId) {
                return function(catalogInfo) {
                    return GamesDataFactory.getOfferIdByPath(catalogInfo[offerId].freeBaseGame);
                };
            }

            /**
             * builds the tile markup
             * @param  {String} tag tag name for directive
             * @param  {String} offerId offer Id of the game to show
             * @return {string} the raw xml markup for the directive
             */
            function buildTileMarkup(tag, offerId) {
                return OcdHelperFactory.buildTag(tag, {
                    'offerid': offerId,
                    'discovertileimage': scope.discoverTileImage,
                    'discovertilecolor': scope.discoverTileColor
                });
            }

            function addTrialOrBaseGameToDOM(trialOfferId) {
                return function(baseGameOfferId, expired) {
                    var offerId = expired ? baseGameOfferId : trialOfferId,
                        tag = expired ? 'origin-home-recommended-action-trial-expired' : 'origin-home-recommended-action-trial-in-progress';

                    Promise.resolve(buildTileMarkup(tag, offerId))
                        .then(compileXML)
                        .then(addToDom);

                    return [baseGameOfferId, expired];
                };
            }

            function switchToBaseGameOnExpired(baseGameOfferId, expired) {
                if(!expired) {
                    Promise.all([
                        GamesDataFactory.getTrialTimeRemaining(scope.offerId),
                        GamesDataFactory.getTrialEndDate(scope.offerId),
                        GamesDataFactory.isTrialExpired(scope.offerId)

                    ])
                        .then(_.spread(getSecondsRemaining))
                        .then(setSwitchTimer(baseGameOfferId));
                }
            }

            Promise.all([
                GamesDataFactory.getCatalogInfo([scope.offerId])
                    .then(getBaseGameOfferId(scope.offerId)),
                GamesDataFactory.isTrialExpired(scope.offerId)
            ])
                .then(_.spread(addTrialOrBaseGameToDOM(scope.offerId)))
                .then(_.spread(switchToBaseGameOnExpired))
                .catch(handleError);

            scope.$on('$destroy', onDestroy);
        }

        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                discoverTileImage: '@',
                discoverTileColor: '@',
            },
            link: originHomeRecommendedActionTrialLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionTrial
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offerid of the game you want to interact with
     * @param {ImageUrl} discover-tile-image tile image 1000x250
     * @param {string} discover-tile-color the background color
     *
     * @description
     *
     * trial recommended next action
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-home-recommended-action-trial offerid="OFB-EAST:109552444"></origin-home-recommended-action-trial>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originHomeRecommendedActionTrial', originHomeRecommendedActionTrial);
}());