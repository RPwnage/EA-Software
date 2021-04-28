/**
 * @file store/bundle/main.js
 */
(function(){
    'use strict';

    function originStoreBundleMainCtrl($scope, $timeout, UtilFactory, StoreBundleFactory, AppCommFactory) {
        $scope.isShrunk = false;
        $scope.isPinned = false;

        /**
         * Notifies carousel element of the change that's happened to its size
         * @return {void}
         */
        $scope.notifyCarousel = function () {
            AppCommFactory.events.fire('carousel:resetup');
        };

        $scope.$watch('isShrunk', function (newValue, oldValue) {
            if (oldValue !== newValue) {
                $timeout($scope.notifyCarousel, 250); // catches slow scope/template updates
            }
        });

        StoreBundleFactory.setCurrentBundleId($scope.bundleId);
    }

    function originStoreBundleMain(ComponentsConfigFactory) {

        function originStoreBundleMainLink(scope) {
            var extraContent = angular.element('.origin-store-sitestripe-wrapper');

            /**
             * Adjusts component's element height and position to pin it to the page's top
             * @return {void}
             */
            function adjustSize() {
                var scrollTop = angular.element(window).scrollTop(),
                    offset = extraContent.height(),
                    isShrunk = (scrollTop >= 100),
                    isPinned = (scrollTop >= offset);

                if (scope.isShrunk !== isShrunk || scope.isPinned !== isPinned) {
                    scope.isShrunk = isShrunk;
                    scope.isPinned = isPinned;
                    scope.$evalAsync();
                }
            }

            angular.element('#storewrapper').on("DOMSubtreeModified propertychange", scope.notifyCarousel);
            angular.element(window).on("scroll", adjustSize);

            adjustSize();
        }

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                bundleId: '@',
                title: '@',
                description: '@',
                backgroundImage: '@',
                legalText: '@',
                buyNowText: '@',
                subtotalText: '@',
                totalText: '@',
                discountText: '@'
            },
            link: originStoreBundleMainLink,
            controller: "originStoreBundleMainCtrl",
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/bundle/views/main.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreBundleMain
     * @restrict E
     * @element ANY
     * @scope
     * @description Store Bundle main component
     *
     * @param {string} bundle-id unique string representing the current bundle
     * @param {LocalizedText} title title of bundle
     * @param {LocalizedText} description description of bundle
     * @param {ImageUrl} background-image background image displayed in bundle area
     * @param {LocalizedText} legal-text (optional) legal text related to this bundle
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-bundle-main
     *             bundle-id="black-friday"
     *             title="Black Friday Sale"
     *             description="Add 5 items and save 33%. Buy now and save big. Available until 12/23/15"
     *             background-image="https://Eaassets-a.akamaihd.net/origin-com-store-final-assets-prod/192294/231.0x326.0/1011172_LB_231x326_en_US_^_2013-10-11-10-22-33_0401f511c44c24445b2adaf00ae8e2d751bd0d97.jpg"
     *             legal-text="Valid only on www.origin.com. product will be entitled to purchaser’s origin account day of launch. internet connection, origin account, acceptance of product and origin end user license agreements (eulas), installation of the origin client software (www.origin.com/about) and registration with enclosed single-use serial code required to play and access online features. serial code registration is limited to one origin account per serial code. serial codes are non-transferable once used and shall be valid, at a minimum, so long as online features are available. you must be 13+ to access online features. ea online privacy and cookie policy and terms of service can be found at www.ea.com. eulas and additional disclosures can be found atwww.ea.com/1/product-eulas. ea may retire online features after 30 days notice posted on www.ea.com/1/service-updates. includes software that collects data online to provide in game advertising. ea may provide certain incremental content and/or updates for no additional charge, if and when available."
     *         ></origin-store-bundle-main>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreBundleMain', originStoreBundleMain)
        .controller('originStoreBundleMainCtrl', originStoreBundleMainCtrl);
}());
