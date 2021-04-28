
/** 
 * @file store/scripts/row.js
 */ 
(function(){
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    /**
    * The directive
    */
    function originStoreRow(ComponentsConfigFactory) {

        function originStoreRowLink(scope) {
            scope.equalContentHeight = scope.equalContentHeight === "true";
            scope.gradient = scope.gradient === "true";
        }

        return {
            restrict: 'E',
            scope: {
                equalContentHeight: '@',
                titleStr: '@',
                gradient: '@'
            },
            link: originStoreRowLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/row.html'),
            transclude: true,
            replace: true
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreRow
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str The title center aligned above the row
     * @param {BooleanEnumeration} equal-content-height whether content have equal size
     * @param {BooleanEnumeration} gradient Whether to include the gradient background at the bottom of the row
     * @description Store row container
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-row
     *          title-str="Some Title"
     *          gradient="true"
     *          equal-content-height="true">
     *     </origin-store-row>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreRow', originStoreRow);
}()); 
