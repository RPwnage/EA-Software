/**
 * @file game/badge.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-badge';

    function OriginGameBadgeCtrl($scope, UtilFactory) {
        $scope.isNew = false;
        $scope.isVisible = false;
        $scope.badgeTextVals = {
            'demo': UtilFactory.getLocalizedStr($scope.demoStr, CONTEXT_NAMESPACE, 'demo'),
            'alpha': UtilFactory.getLocalizedStr($scope.alphaStr, CONTEXT_NAMESPACE, 'alpha'),
            'beta': UtilFactory.getLocalizedStr($scope.betaStr, CONTEXT_NAMESPACE, 'beta'),
            'trial': UtilFactory.getLocalizedStr($scope.trialStr, CONTEXT_NAMESPACE, 'trial'),
            'new': UtilFactory.getLocalizedStr($scope.newStr, CONTEXT_NAMESPACE, 'new')
        };
        $scope.badgeText = $scope.badgeTextVals.demo; // for now call it demo
    }

    function originGameBadge(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                demoStr: '@demo',
                alphaStr: '@alpha',
                betaStr: '@beta',
                trialStr: '@trial',
                newStr: '@new'
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
     * @param {LocalizedString} demo string for demo
     * @param {LocalizedString} alpha string for alpha
     * @param {LocalizedString} beta string for beta
     * @param {LocalizedString} trial string for trial
     * @param {LocalizedString} new string for new
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