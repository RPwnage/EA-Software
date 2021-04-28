/**
 * @file home/recommended/trial.js
 */
(function() {
    'use strict';

    /**
     * Use an i18n string.
     * @typedef {string} LocalizedString
     */

    /**
     * Expect a valid URL to an image
     * @typedef {string} ImageUrl
     */

    /**
     * TrialTypeEnumeration list of allowed types - we expect this var to use double quotes for strings so it is JSON compliant
     * @enum {string}
     * @see http://www.json.org
     */
    var TrialTypeEnumeration = {
        "expanded": "expanded",
        "list-only": "list-only"
    };

    function originHomeRecommendedActionTrialCtrl($scope, ClientDataFactory, GamesDataFactory, LocFactory, UtilFactory, ComponentsLogFactory) {
        function onInitialEntitlementUpdateCompleted() {
            // get the client data
            var game = ClientDataFactory.getGame($scope.offerId);
            $scope.isDownloading = (typeof(game) !== 'undefined') ? (game.downloading || game.installing) : false;
            $scope.offerBased = true;
            $scope.priority = $scope.priority;

            /**
             * If the game is downloading then make sure we show the game info
             * @return {void}
             * @method updateDownloadState
             */
            function updateDownloadState() {
                if (typeof(game) !== 'undefined') {
                    $scope.isDownloading = (game.downloading || game.installing) ? true : false;
                    $scope.$apply();
                }
            }

            ClientDataFactory.events.on('update:' + $scope.offerId, updateDownloadState);

            switch ($scope.trialType) {
                case TrialTypeEnumeration['expanded']:
                    $scope.trialtype = TrialTypeEnumeration['expanded'];
                    break;
                default:
                    $scope.trialtype = TrialTypeEnumeration['list-only'];
                    break;
            }

            GamesDataFactory.getCatalogInfo([$scope.offerId])
                .then(function(response) {

                    var hoursLeft = 0;
                    var terminationDate = GamesDataFactory.getEntitlement($scope.offerId).terminationDate;
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

                    $scope.title = UtilFactory.getLocalizedStr($scope.titlestr,
                        '%game% - It\'s Game Time!', {
                            '%game%': gameName
                        }
                    );

                    $scope.subtitle = UtilFactory.getLocalizedStr($scope.subtitlestr,
                        'Continue your trial!', {
                            '%game%': gameName
                        }
                    );

                    $scope.description = UtilFactory.getLocalizedStr($scope.descriptiontr,
                        '{0} Your trial has expired. | {1} You have an hour left. | ]1,+Inf] You have %hours% hours left in your trial.', {
                            '%game%': gameName,
                            '%hours%': hoursLeft
                        },
                        hoursLeft
                    ) + ' ';

                    $scope.image = $scope.image || response[$scope.offerId].packArt;
                    $scope.displayFriends = true;

                })
                .catch(function(error) {
                    ComponentsLogFactory.error('origin-components - getCatalogInfo', error);
                });
        }


        //setup a listener for any entitlement updates
        if (GamesDataFactory.initialRefreshCompleted()) {
            onInitialEntitlementUpdateCompleted();
        } else {
            GamesDataFactory.events.once('games:baseGameEntitlementsUpdate', onInitialEntitlementUpdateCompleted);
        }
    }

    function originHomeRecommendedActionTrial() {
        return {
            restrict: 'E',
            scope: {
                image: '@image',
                expireddatetime: '@expireddatetime',
                title: '@title',
                subtitle: '@subtitle',
                description: '@description',
                titleExpired: '@titleexpired',
                subtitleExpired: '@subtitleexpire',
                descriptionExpired: '@descriptionexpired',
                trialType: '@trialtype',
                priority: '@priority',
                href: '@href',
                baz: '@baz',
                offerid: '@offerid'
            },
            controller: 'originHomeRecommendedActionTrialCtrl',
            templateUrl: '@directive_bundle/home/recommended/action/views/tile.html'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionTrial
     * @restrict E
     * @element ANY
     * @scope
     * @param {DateTime} expireddatetime an ISO-8601 compliant date string
     * @param {ImageUrl} image the url of the image
     * @param {LocalizedString} title the title string
     * @param {LocalizedString} subtitle the subtitle string
     * @param {LocalizedText} description the description string
     * @param {(LocalizedString|string=)} [titleexpired=John Wayne] the title expired string
     * @param {LocalizedString|Object[]} subtitleexpired the subtitle expired string
     * @param {LocalizedString|OCD} [descriptionexpired] the description expired str
     * @param {Url} href the URL to the trial PDP
     * @param {string|string=} [baz=foo] the trial PDP idenifier
     * @param {number} priority - the priority value
     * @param {TrialTypeEnumeration|ProductInfo} trialtype a list of tile configurations
     * @param {Video} youtubevideoid the video id for a youtube video/playlist
     * @param {OfferId} offerid the offer id
     * @description Trial recommended next action
     * @see http://usejsdoc.org/tags-param.html
     */

    angular.module('origin-components')
        .controller('originHomeRecommendedActionTrialCtrl', originHomeRecommendedActionTrialCtrl)
        .directive('originHomeRecommendedActionTrial', originHomeRecommendedActionTrial);
}());
