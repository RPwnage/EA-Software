/**
 * @file home/recommended/trial.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-recommended-action-trial';

    function OriginHomeRecommendedActionTrialCtrl($scope, $controller, $interval, GamesDataFactory, UtilFactory, ComponentsLogFactory) {

        var updateInterval = null,
            terminationDate,
            trialInfo = {};


        function getTimeLeft() {
            var termDate = Date.parse(terminationDate),
                currentDate = Origin.datetime.getTrustedClock(),
                timeLeftObj = {},
                timeLeftMinutes = Math.ceil((termDate - currentDate)/1000/60);

            timeLeftObj.minutes = timeLeftMinutes;
            timeLeftObj.hours = Math.round(timeLeftMinutes/60);

            return timeLeftObj;
        }


        /**
         * gets tileleft and started info about the trial entitlement
         * @param  {[type]} catalogInfo [description]
         * @return {[type]}             [description]
         */
        function getTrialInfo(launchDuration) {
            var started = true,
                timeLeftObj = {};

            if (terminationDate) {
                //if we have a termination date it means we've started the trial
                timeLeftObj = getTimeLeft();
            } else if (launchDuration !== '') {
                //otherwise if we have a trial duration we are a trial
                timeLeftObj.hours = launchDuration;
                timeLeftObj.minutes = launchDuration * 60;
                started = false;
            } else {
                //shouldn't get here
                ComponentsLogFactory.error('No trial duration set or termination date set, but its a trial.');
            }

            return {
                timeLeft: timeLeftObj,
                started: started,
            };
        }

        function setTerminationDate() {
            var ent = GamesDataFactory.getEntitlement($scope.offerId);
            terminationDate = (ent && ent.terminationDate) ? ent.terminationDate : null;
        }


        function setSubtitleAndDescription(trialInfo) {
            var tokeninfo = {
                    '%game%': $scope.gamename
                };

            if (!trialInfo.started) {
                //not started
                tokeninfo['%hours%'] = trialInfo.timeLeft.hours;
                $scope.subtitle = UtilFactory.getLocalizedStr($scope.subtitleNotStartedRaw, CONTEXT_NAMESPACE, 'subtitlenotstarted', tokeninfo);
                $scope.description = UtilFactory.getLocalizedStr($scope.descriptionNotStartedRaw, CONTEXT_NAMESPACE, 'descriptionnotstarted', tokeninfo, trialInfo.timeLeft.hours) + ' ';
            } else if (trialInfo.timeLeft.minutes <= 0) {
                // expired trial
                $scope.subtitle = UtilFactory.getLocalizedStr($scope.subtitleExpiredRaw, CONTEXT_NAMESPACE, 'subtitleexpired', tokeninfo);
                $scope.description = UtilFactory.getLocalizedStr($scope.descriptionExpiredRaw, CONTEXT_NAMESPACE, 'descriptionexpired', tokeninfo);
                $scope.displayPrice = true;
            } else {
                // trial in progress
                $scope.subtitle = UtilFactory.getLocalizedStr($scope.subtitleRaw, CONTEXT_NAMESPACE, 'subtitle', tokeninfo);
                if (trialInfo.timeLeft.minutes < 60) {
                    tokeninfo['%minutes%'] = trialInfo.timeLeft.minutes;
                    $scope.description = UtilFactory.getLocalizedStr($scope.descriptionRawMinutes, CONTEXT_NAMESPACE, 'descriptionminutes', tokeninfo, trialInfo.timeLeft.minutes) + ' ';
                } else {
                    tokeninfo['%hours%'] = trialInfo.timeLeft.hours;
                    $scope.description = UtilFactory.getLocalizedStr($scope.descriptionRaw, CONTEXT_NAMESPACE, 'description', tokeninfo, trialInfo.timeLeft.hours) + ' ';
                }

            }
        }

        function customSubtitleAndDescriptionFunction(catalogInfo) {

            setTerminationDate();
            trialInfo = getTrialInfo(catalogInfo.trialLaunchDuration);

            setSubtitleAndDescription(trialInfo);

            //set up an update timer
            if (trialInfo.started) {
                //seems really wasteful to do it every minute but the alternative would require
                //a bit of messy logic so leave it this way for now
                updateInterval = $interval(updateTrialInfoAndDescription, 1000*60);
            }
        }

        function updateTrialInfoAndDescription() {

            trialInfo.timeLeft = getTimeLeft();
            trialInfo.started = true;

            setSubtitleAndDescription(trialInfo);

            if (trialInfo.timeLeft.minutes <= 0) {
                cancelIntervalTimer();
            }
        }

        function cancelIntervalTimer() {
            if (updateInterval) {
                $interval.cancel(updateInterval);
                updateInterval = null;
            }
        }

        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and subtitle/description function (if needed)
        $controller('OriginHomeRecommendedActionCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customSubtitleAndDescriptionFunction: customSubtitleAndDescriptionFunction
        });

        $scope.$on('$destroy', cancelIntervalTimer);
    }

    function originHomeRecommendedActionTrial(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@',
                gamename: '@gamename',
                subtitleRaw: '@subtitle',
                descriptionRaw: '@description',
                descriptionRawMinutes: '@descriptionminutes',
                subtitleExpiredRaw: '@subtitleexpired',
                descriptionExpiredRaw: '@descriptionexpired',
                subtitleNotStartedRaw: '@subtitlenotstarted',
                descriptionNotStartedRaw: '@descriptionnotstrated',
                priceOfferId: '@priceofferid',
                offerId: '@offerid'
            },
            controller: 'OriginHomeRecommendedActionTrialCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionTrial
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl|OCD} image the url of the image
     * @param {LocalizedString|OCD} gamename the name of the game, if not passed it will use the name from catalog
     * @param {LocalizedString} subtitle the subtitle string
     * @param {LocalizedString} description the description string
     * @param {LocalizedString} descriptionminutes the description string to show minutes remaining
     * @param {LocalizedString} subtitleexpired the subtitle expired string
     * @param {LocalizedString} descriptionexpired the description expired string
     * @param {LocalizedString} subtitlenotstarted the subtitle not started string
     * @param {LocalizedString} descriptionnotstarted the description not started string
     * @param {OfferId} offerid the offerid of the game you want to interact with
     * @param {string} priceofferid the offerid of the game you want to show pricing for
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
        .controller('OriginHomeRecommendedActionTrialCtrl', OriginHomeRecommendedActionTrialCtrl)
        .directive('originHomeRecommendedActionTrial', originHomeRecommendedActionTrial);
}());