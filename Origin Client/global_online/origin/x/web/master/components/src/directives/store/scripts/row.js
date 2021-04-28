
/** 
 * @file store/scripts/row.js
 */ 
(function(){
    'use strict';

    /* jshint ignore:start */
    var EqualContentHeightEnum = {
        "Yes" : true,
        "No" : false
    };
    /* jshint ignore:end */

    var BackgroundClassEnum = {
        "None" : null,
        "Light Grey Gradient" : "lightgrey-gradient"
    };

    /**
    * The directive
    */
    function originStoreRow() {

        function originStoreRowLink(scope, element, attributes) {

            var className = BackgroundClassEnum[attributes.backgroundClass];
            if (className) {
                element.addClass(className);
            }

            scope.equalContentHeight = Boolean(scope.equalContentHeight);
        }

        return {
            restrict: 'E',
            scope: {
                equalContentHeight: '@'
            },
            link: originStoreRowLink,
            template: '<div ng-transclude class="l-origin-store-row" ng-class="{\'flex-box\' : equalContentHeight}"></div>',
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
     * @param {EqualContentHeightEnum} equal-content-height whether content have equal size
     * @description Store row container
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-row equal-content-height="true"/>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreRow', originStoreRow);
}()); 
