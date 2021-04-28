
/**
 * @file store/home/promo/scripts/static.js
 */
(function(){
    'use strict';

    function originStoreHomePromoStaticCtrl($scope, $window, $location) {
        $scope.onBtnClick = function() {
            if ($scope.href.substring(0, 4).toLowerCase() === 'http')
            {
                // support for full/external URLs
                $window.open($scope.href, '_blank');
            }
            else
            {
                // support for internal URLs
                $location.path($scope.href);
            }
        };
    }

    function originStoreHomePromoStatic(ComponentsConfigFactory) {
        /**
        * The directive link
        */
        return {
            transclude: true,
            restrict: 'E',
            scope: {
                "image" : "@",
                "title" : "@",
                "text" : "@",
                "href" : "@"
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/home/promo/views/static.html'),
            controller: originStoreHomePromoStaticCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreHomePromoStatic
     * @restrict E
     * @element ANY
     * @scope
     * @description Static promo module
     * @param {ImageUrl} image The background image for this module.
     * @param {LocalizedString} title promo title
     * @param {LocalizedString} text promo text
     * @param {string} href href to link to
     *
     *
     * @example
     * <example module="origin-components">
     *     <origin-store-home-promo-static
     *         image="http://docs.x.origin.com/origin-x-design-prototype/images/store/wide-ggg.jpg"
     *         title="We Stand By Our Games."
     *         text="If you don't love it, return it."
     *         href="greatgameguarantee"
     *     >
     *        <origin-cta-loadurlinternal
     *             href="greatgameguarantee"
     *             description="Learn more"
     *             type="transparent">
     *        </origin-cta-loadurlinternal>
     *     </origin-store-home-promo-static>
     * </example>
     */
    angular.module('origin-components')
        .controller('originStoreHomePromoStaticCtrl', originStoreHomePromoStaticCtrl)
        .directive('originStoreHomePromoStatic', originStoreHomePromoStatic);
}());
