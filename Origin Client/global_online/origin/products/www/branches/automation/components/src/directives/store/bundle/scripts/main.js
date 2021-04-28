/**
 * @file store/bundle/main.js
 */
(function(){
    'use strict';
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var TextColorEnumeration = {
        "light": "light",
        "dark": "dark"
    };


    function originStoreBundleMainCtrl($scope, $timeout, StoreBundleFactory, AppCommFactory, FeatureDetectionFactory) {
        $scope.isShrunk = false;
        $scope.isPinned = false;
        $scope.stripeMargin = 0;

        $scope.supportsCssBlur = FeatureDetectionFactory.browserSupports('css-filters');
        $scope.isDarkText = $scope.textColor === TextColorEnumeration.dark;
        $scope.isScrimShown = $scope.hideScrim !== BooleanEnumeration.true;

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

    function originStoreBundleMain(ComponentsConfigFactory, AnimateFactory) {

        function originStoreBundleMainLink(scope, element) {
            var extraContent = angular.element('.origin-store-sitestripe-wrapper'),
                $hero = element.find('.origin-storebundlehero'),
                $offerList = element.find('.origin-bundlemain-offerlist');
            /**
             * Adjusts component's element height and position to pin it to the page's top
             * @return {void}
             */
            function adjustSize() {
                var scrollTop = window.pageYOffset || document.documentElement.scrollTop,
                    offset = extraContent.height(),
                    isShrunk = (scrollTop >= 100),
                    isPinned = (offset && scrollTop >= offset) || scrollTop !== 0; //pin if scroll is more than site stripe or page not scrolled

                if (scope.isShrunk !== isShrunk || scope.isPinned !== isPinned) {
                    scope.isShrunk = isShrunk;
                    scope.isPinned = isPinned;
                }
                updateMargin(isPinned, isShrunk);
            }

            function adjustSizeDigest(){
                adjustSize();
                scope.$digest();
            }

            /**
             * Update product list content margin base on hero height
             * @return {void}
             */
            function updateMargin(isPinned, isShrunk){
                var hero = $hero.get(0);
                if (hero && isPinned){
                    var newHeight = hero.offsetHeight - extraContent.height(); //Attemp to calculate the height so I don't have to wait for angular to process and browse to render.
                    if (isShrunk){
                        var header = $hero.find('.origin-storebundlehero-header').get(0);
                        newHeight -= header ? header.offsetHeight : 0;
                    }
                    $offerList.css('margin-top', newHeight);
                }else{
                    $offerList.css('margin-top', 0);
                }

                if (hero) {
                    scope.updateHeroMargin(scope.stripeMargin);
                }
            }
            // This is removed to fix issue ORSTOR-1902 this will need a new solution to make sure
            // that the bundle page is still functioning correctly
            AnimateFactory.addScrollEventHandler(scope, adjustSizeDigest, 100);
            AnimateFactory.addResizeEventHandler(scope, adjustSizeDigest, 100);

            /**
             * Update the hero margin, used to account for global sitestripes
             * @return {void}
             */
            scope.updateHeroMargin = function(margin) {
                scope.stripeMargin = margin;
                $hero.css('margin-top', scope.isPinned ? margin : 0);
            };

            // check sizing upon load in case we're scrolled down in the page
            adjustSize();
        }

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                bundleId: '@',
                titleStr: '@',
                description: '@',
                backgroundImage: '@',
                legalText: '@',
                buyNowText: '@',
                subtotalText: '@',
                totalText: '@',
                discountText: '@',
                yourBundleText: '@',
                emptyBundleText: '@',
                hideScrim: '@',
                textColor: '@'
            },
            link: originStoreBundleMainLink,
            controller: 'originStoreBundleMainCtrl',
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
     * @param {LocalizedText} title-str title of bundle
     * @param {LocalizedText} description description of bundle
     * @param {ImageUrl} background-image background image displayed in bundle area
     * @param {LocalizedText} legal-text (optional) legal text related to this bundle
     * @param {LocalizedString} your-bundle-text The 'your bundle' text override
     * @param {LocalizedString} buy-now-text The 'buy now' text override
     * @param {LocalizedString} subtotal-text The 'subtotal' text override
     * @param {LocalizedString} total-text The 'total' text override
     * @param {LocalizedString} discount-text The 'discount' text override
     * @param {LocalizedString} empty-bundle-text instructions to display when cart is empty
     * @param {BooleanEnumeration} hide-scrim Show or hide image scrim. Default to show. Check to hide.
     * @param {TextColorEnumeration} text-color The text/font color of the component. Default to 'light'.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-bundle-main
     *             facet-category-id="bundle"
     *             bundle-id="black-friday"
     *             title-str="Black Friday Sale"
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
