
/** 
 * @file /store/pack/scripts/pack.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pack';

    /**
    * The controller
    */
    function OriginStorePackCtrl($scope, UtilFactory) {
        /* Get localized strings */
        $scope.viewAllStr = UtilFactory.getLocalizedStr($scope.viewAllStr, CONTEXT_NAMESPACE, 'viewallstr');

    }
    /**
    * The directive
    */
    function originStorePack(ComponentsConfigFactory) {
        return {
            transclude: true,
            restrict: 'E',
            scope: {
                type: '@type',
                title: '@title',
                viewAllStr: '@viewallstr',
                viewAllGref: '@viewallhref'
            },
            controller: 'OriginStorePackCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/pack/views/pack.html')
        };
    }

    /**
     * A list of pack type options
     * @readonly
     * @enum {string}
     */
    /* jshint ignore:start */
    var PackTypeEnumeration = {
        "six": "six",
        "nine": "nine"
    };
    /* jshint ignore:end */
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePack
     * @restrict E
     * @element ANY
     * @scope
     * @param {PackTypeEnumeration} type can be one of two values 'six'/'nine'. this sets the style of the pack
     * @param {LocalizedString} title the title of the pack
     * @param {LocalizedString} viewallstr the text for the link
     * @param {LocalizedString} viewallhref the url for the link
     * @description
     * 
     * Store Game Pack
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-pack
     *              type="six"
     *              title="Most Popular"
     *              viewallstr="View All"
     *              viewallhref="/#/store/browse/most-popular">
     <origin-store-pack>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePackCtrl', OriginStorePackCtrl)
        .directive('originStorePack', originStorePack);
}()); 
