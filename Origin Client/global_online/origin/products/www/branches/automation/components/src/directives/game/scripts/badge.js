/**
 * @file game/badge.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-badge';

    function OriginGameBadgeCtrl($scope, UtilFactory, GamesDataFactory, ComponentsLogFactory) {

        // Initialize static members of the $scope
        $scope.badgeTextVals = {
            'test': UtilFactory.getLocalizedStr($scope.testStr, CONTEXT_NAMESPACE, 'test'),
            'new': UtilFactory.getLocalizedStr($scope.newStr, CONTEXT_NAMESPACE, 'new'),
            'trial': UtilFactory.getLocalizedStr($scope.trialStr, CONTEXT_NAMESPACE, 'trial'),
            'alpha': UtilFactory.getLocalizedStr($scope.alphaStr, CONTEXT_NAMESPACE, 'alpha'),
            'beta': UtilFactory.getLocalizedStr($scope.betaStr, CONTEXT_NAMESPACE, 'beta'),
            'demo': UtilFactory.getLocalizedStr($scope.demoStr, CONTEXT_NAMESPACE, 'demo')
        };

        function configureGameBadge() {

            // Refresh catalog data first, then update badge status.
            GamesDataFactory.getCatalogInfo([$scope.offerId])
                    .then(configureGameBadgeWithCatalogData)
                    .catch(function(error) {
                        ComponentsLogFactory.error('origin-game-badge - getCatalogInfo', error);
                    });
        }

        function setGameBadge(text, isNew, isVisible) {

            // Set default values for some parameters.
            isNew = typeof isNew !== 'undefined' ? isNew : false;
            isVisible = typeof isVisible !== 'undefined' ? isVisible : true;

            $scope.isNew = isNew;
            $scope.badgeText = text;
            $scope.isVisible = isVisible;
        }

        // Callback to set the game badge when game data is retrieved.
        function configureGameBadgeWithCatalogData(data) {

            // Prepeare to determine the applicable badge.
            var entitlement = GamesDataFactory.getEntitlement($scope.offerId);
            var gameData = data[$scope.offerId];
            var isPrivate = GamesDataFactory.isPrivate(entitlement);
            var isNew = entitlement.isNew;

            // Default to blank/invisible badge.
            setGameBadge('', false, false);

            // Determine badges in order of greatest to least priority.

            // Test Code (1102/1103)
            if (isPrivate) {
                setGameBadge($scope.badgeTextVals.test);
            }

            // Trial
            else if (gameData.trial || gameData.earlyAccess) {
                setGameBadge($scope.badgeTextVals.trial);
            }

            // New (game is considered new for 5 days)
            else if (isNew) {
                setGameBadge($scope.badgeTextVals.new, true);
            }

            // Alpha
            else if (gameData.alpha) {
                setGameBadge($scope.badgeTextVals.alpha);
            }

            // Beta
            else if (gameData.beta) {
                setGameBadge($scope.badgeTextVals.beta);
            }

            // Demo
            else if (gameData.demo) {
                setGameBadge($scope.badgeTextVals.demo);
            }

            // Guard against digest re-entrancy.
            if(!$scope.$$phase) {
                // Update watchers of our scope.
                $scope.$digest();
            }
        }

        function cleanUp() {
            GamesDataFactory.events.off('games:isNewUpdate:' + $scope.offerId, configureGameBadge);
        }

        // listen to any updates to our offer ID.
        GamesDataFactory.events.on('games:isNewUpdate:' + $scope.offerId, configureGameBadge);
        $scope.$on('$destroy', cleanUp);

        // Set the game badge.
        configureGameBadge();
    }

    // Factory to initialize the originGameBadge directive and options.
    function originGameBadge(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                testStr: '@test',
                newStr: '@new',
                trialStr: '@trial',
                alphaStr: '@alpha',
                betaStr: '@beta',
                demoStr: '@demo'
            },
            controller: 'OriginGameBadgeCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/views/badge.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameBadge
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offerid of the game you want to interact with
     * @param {LocalizedString} test string for test
     * @param {LocalizedString} new string for new
     * @param {LocalizedString} trial string for trial
     * @param {LocalizedString} alpha string for alpha
     * @param {LocalizedString} beta string for beta
     * @param {LocalizedString} demo string for demo
     * @description
     *
     * game badge
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-badge offerid="OFB-EAST:109549060"></origin-game-badge>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameBadgeCtrl', OriginGameBadgeCtrl)
        .directive('originGameBadge', originGameBadge);
}());
