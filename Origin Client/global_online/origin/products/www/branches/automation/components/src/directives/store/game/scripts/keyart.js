
/** 
 * @file store/game/scripts/keyart.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-game-keyart';
    var HOVER_CLASS = 'origin-store-game-keyart-foreground-hover';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function OriginStoreGameKeyartCtrl($scope, DirectiveScope, PdpUrlFactory, ProductInfoHelper) {
        function setOnClick() {
            $scope.onElementClick = function() {
                PdpUrlFactory.goToPdp($scope.model);
            };

            $scope.isFree = ProductInfoHelper.isFree($scope.model);
        }

        DirectiveScope
            .populateScopeWithOcdAndCatalog($scope, CONTEXT_NAMESPACE, $scope.ocdPath)
            .then(setOnClick);
    }

    function originStoreGameKeyart(ComponentsConfigFactory) {
        function originStoreGameKeyartLink(scope, element) {
            /* This adds a hover class when the infobubble is hovered */
            scope.$on('infobubble-show', function() {
                element.addClass(HOVER_CLASS);
            });

            /* this removes the class */
            scope.$on('infobubble-hide', function() {
                element.removeClass(HOVER_CLASS);
            });
        }

        return {
            restrict: 'E',
            scope: {
                ocdPath: '@',
                overrideVault: '@',
                background: '@',
                oth: '@'
            },
            controller: OriginStoreGameKeyartCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/game/views/keyart.html'),
            link: originStoreGameKeyartLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreGameKeyart
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {OcdPath} ocd-path The offer ocd path
     * @param {ImageUrl} background The background image
     * @param {LocalizedString} oth The on the house string
     * @param {BooleanEnumeration} override-vault Override the "included with vault" text
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-game-keyart />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreGameKeyart', originStoreGameKeyart);
}()); 
