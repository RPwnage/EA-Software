/**
 * @file /store/sitestripe/scripts/Info.js
 */
(function(){
    'use strict';

    function OriginStoreSitestripeInfoCtrl($scope) {
        $scope.edgecolor = '#269dcf';
        $scope.info = true;
    }

    function originStoreSitestripeInfo(ComponentsConfigFactory) {

        function originStoreSitestripeInfoLink(scope, elem, attrs, ctrl) {
            scope.siteStripeDismiss = ctrl.siteStripeDismiss;
        }

        return {
            require: '^originStoreSitestripeWrapper',
            restrict: 'E',
            replace: true,
            scope: {
                title: '@'
            },
            controller: 'OriginStoreSitestripeInfoCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/sitestripe/views/messageoftheday.html'),
            link: originStoreSitestripeInfoLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSitestripeInfo
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title The title for this site stripe.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-sitestripe-wrapper
     *             last-modified="June 5, 2015 06:00:00 AM PDT">
     *             <origin-store-sitestripe-info
     *                 title="some title">
     *             </origin-store-sitestripe-info>
     *         </origin-store-sitestripe-wrapper>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreSitestripeInfoCtrl', OriginStoreSitestripeInfoCtrl)
        .directive('originStoreSitestripeInfo', originStoreSitestripeInfo);
}());