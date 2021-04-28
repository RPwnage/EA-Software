/**
 * @file /store/sitestripe/scripts/Alert.js
 */
(function(){
    'use strict';

    function OriginStoreSitestripeAlertCtrl($scope) {
        $scope.edgecolor = '#c93507';
        $scope.alert = true;
    }

    function originStoreSitestripeAlert(ComponentsConfigFactory) {

        function originStoreSitestripeAlertLink(scope, elem, attrs, ctrl) {
            scope.siteStripeDismiss = ctrl.siteStripeDismiss;
        }

        return {
            require: '^originStoreSitestripeWrapper',
            restrict: 'E',
            replace: true,
            scope: {
                title: '@'
            },
            controller: 'OriginStoreSitestripeAlertCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/sitestripe/views/messageoftheday.html'),
            link: originStoreSitestripeAlertLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSitestripeAlert
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
     *             <origin-store-sitestripe-alert
     *                 title="some title">
     *             </origin-store-sitestripe-alert>
     *         </origin-store-sitestripe-wrapper>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreSitestripeAlertCtrl', OriginStoreSitestripeAlertCtrl)
        .directive('originStoreSitestripeAlert', originStoreSitestripeAlert);
}());