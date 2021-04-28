
/** 
 * @file store/access/scripts/item.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-item';

    function OriginStoreAccessItemCtrl($scope, PriceInsertionFactory) {
        $scope.strings = {};

        PriceInsertionFactory
            .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.text, CONTEXT_NAMESPACE, 'text');
    }

    function originStoreAccessItem() {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                text: '@'
            },
            controller: OriginStoreAccessItemCtrl,
            template: '<li class="origin-store-access-item"><i class="otkicon otkicon-check origin-store-access-item-check"></i><span ng-bind-html="strings.text"></span></li>'
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessItem
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedTemplateString} text The text for this list item
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-item
     *          text="some text">
     *     </origin-store-access-item>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessItem', originStoreAccessItem);
}()); 
