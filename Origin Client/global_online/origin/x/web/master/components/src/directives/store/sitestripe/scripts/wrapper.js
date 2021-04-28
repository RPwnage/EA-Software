
/**
 * @file store/sitestripe/scripts/wrapper.js
 */
(function(){
    'use strict';
    var LOCALSTORAGE_KEY = 'siteStripeDismissed';

    /**
    * The controller
    */
    function OriginStoreSitestripeWrapperCtrl($scope, LocalStorageFactory) {
        // Check localStorage key and hide site-stripe if set
        $scope.siteStripeVisible = !LocalStorageFactory.get(LOCALSTORAGE_KEY);

        /**
         * Hide the site-stripe and update localStorage key to record the "dismissed" state
         * @return {void}
         */
        this.siteStripeDismiss = function() {
            $scope.$evalAsync(function () {
                $scope.siteStripeVisible = false;
            });
            LocalStorageFactory.set(LOCALSTORAGE_KEY, true);
        };
    }
    /**
    * The directive
    */
    function originStoreSitestripeWrapper(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                lastModified: '@'
            },
            controller: 'OriginStoreSitestripeWrapperCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/sitestripe/views/wrapper.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSitestripeWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {string} last-modified The date and time the mod page was last modified.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-sitestripe-wrapper
     *              last-modified="June 5, 2015 06:00:00 AM PDT">
     *          </origin-store-sitestripe-wrapper>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreSitestripeWrapperCtrl', OriginStoreSitestripeWrapperCtrl)
        .directive('originStoreSitestripeWrapper', originStoreSitestripeWrapper);
}());
