/**
 * @file cta/addnonorigingame.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-cta-add-non-origin-game';
     
    var ButtonTypeEnumeration = {
        "primary": "primary",
        "secondary": "secondary",
        "transparent": "transparent"
    };

    function OriginCtaAddNonOriginGameCtrl($scope, UtilFactory) {
        $scope.type = $scope.type || ButtonTypeEnumeration.primary;
        $scope.btnText = UtilFactory.getLocalizedStr($scope.btnText, CONTEXT_NAMESPACE, 'description');

        $scope.onBtnClick = function() {
            Origin.client.games.addNonOriginGame();
        };

    }

    function originCtaAddNonOriginGame(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                btnText: '@description',
                type: '@'
            },
            controller: 'OriginCtaAddNonOriginGameCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html')
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaAddNonOriginGame
     * @restrict E
     * @element ANY
     * @scope
     * @param {ButtonTypeEnumeration} type the style of button
     * @param {LocalizedText} description string for button
     * @description
     *
     * AddNonOriginGame (takes you to AddNonOriginGame game popup) call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-add-non-origin-game description="add a non origin game" type="primary"></origin-cta-add-non-origin-game>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaAddNonOriginGameCtrl', OriginCtaAddNonOriginGameCtrl)
        .directive('originCtaAddNonOriginGame',  originCtaAddNonOriginGame);
}());