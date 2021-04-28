/**
 * @file home/recommended/trial.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-recommended-action-trial';

    function OriginHomeRecommendedActionTrialCtrl($scope, GamesDataFactory, LocFactory, UtilFactory, ComponentsLogFactory) {
        function onInitialEntitlementUpdateCompleted() {

            // get the client data
            var game = GamesDataFactory.getClientGame($scope.offerId);
            $scope.isDownloading = (typeof(game) !== 'undefined') ? (game.downloading || game.installing) : false;
            $scope.offerBased = true;

            /**
             * If the game is downloading then make sure we show the game info
             * @return {void}
             * @method updateDownloadState
             */
            function updateDownloadState() {
                if (typeof(game) !== 'undefined') {
                    $scope.isDownloading = (game.downloading || game.installing) ? true : false;
                    $scope.$digest();
                }
            }

            /**
            * clean up
            * @return {void}
            * @method onDestroy
            */
            function onDestroy() {
                GamesDataFactory.events.off('games:update:' + $scope.offerId, updateDownloadState);
                GamesDataFactory.events.off('games:baseGameEntitlementsUpdate', onInitialEntitlementUpdateCompleted);
            }

            GamesDataFactory.events.on('games:update:' + $scope.offerId, updateDownloadState);
            $scope.$on('$destroy', onDestroy);

            GamesDataFactory.getCatalogInfo([$scope.offerId])
                .then(function(response) {

                    var hoursLeft = 0;
                    var terminationDate;
                    var ent = GamesDataFactory.getEntitlement($scope.offerId);
                    if (ent) {
                        terminationDate = ent.terminationDate;
                    }
                    var now = new Date();

                    if (terminationDate && now > terminationDate) {
                        // expired trial
                        $scope.title = $scope.titleExpired || 'Experience the full game.';
                        $scope.subtitle = $scope.subtitleExpired || 'Your trial has expired.';
                        $scope.description = $scope.descriptionExpired || 'Keep playing and earn more achivements. Purchase %game% now to continue playing.';
                        $scope.ctaText = LocFactory.trans('Learn More');
                        $scope.displayPrice = true;


                    } else if (terminationDate && now <= terminationDate) {
                        // valid trial
                        hoursLeft = (terminationDate.getHours() - now.getHours());
                    } else if (response[$scope.offerId].trialLaunchDuration !== '') {
                        // un-initiated trial
                        hoursLeft = response[$scope.offerId].trialLaunchDuration;
                    }

                    // Because of translation choice issue for 0.5 and 1.5 hours we will round up for
                    // initial release.
                    hoursLeft = Math.ceil(hoursLeft);

                    var gameName = response[$scope.offerId].displayName;

                    $scope.title = UtilFactory.getLocalizedStr($scope.titlestr, CONTEXT_NAMESPACE, 'title', {
                        '%game%': gameName
                    });

                    $scope.subtitle = UtilFactory.getLocalizedStr($scope.subtitlestr, CONTEXT_NAMESPACE, 'subtitle', {
                        '%game%': gameName
                    });

                    $scope.description = UtilFactory.getLocalizedStr($scope.descriptiontr, CONTEXT_NAMESPACE, 'description', {
                        '%game%': gameName,
                        '%hours%': hoursLeft
                    }, hoursLeft) + ' ';

                    $scope.image = $scope.image || response[$scope.offerId].packArt;
                    $scope.displayFriends = true;

                })
                .catch(function(error) {
                    ComponentsLogFactory.error('[origin-home-recommended-action-trial] GamesDataFactory.getCatalogInfo', error.stack);
                });
        }


        //setup a listener for any entitlement updates
        if (GamesDataFactory.initialRefreshCompleted()) {
            onInitialEntitlementUpdateCompleted();
        } else {
            GamesDataFactory.events.once('games:baseGameEntitlementsUpdate', onInitialEntitlementUpdateCompleted);
        }
    }

    function originHomeRecommendedActionTrial(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@',
                title: '@titlestr',
                subtitle: '@subtitlestr',
                description: '@descriptionstr',
                titleExpired: '@titleexpiredstr',
                subtitleExpired: '@subtitleexpiredstr',
                descriptionExpired: '@descriptionexpiredstr',
                offerId: '@offerid',
                friendsPlayingDescription: '@friendsplayingdescriptionstr'
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
     * @param {ImageUrl} image the url of the image
     * @param {LocalizedString|OCD} title the title string
     * @param {LocalizedString|OCD} subtitle the subtitle string
     * @param {LocalizedString|OCD} description the description string
     * @param {LocalizedString|OCD} titleexpired the title expired string
     * @param {LocalizedString|OCD} subtitleexpired the subtitle expired string
     * @param {LocalizedString|OCD} descriptionexpired the description expired str
     * @param {LocalizedString|OCD} friendsplayingdescription the friends playing string
     * @param {string} offerid the offerid of the game you want to interact with
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