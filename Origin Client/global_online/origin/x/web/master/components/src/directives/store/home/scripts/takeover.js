/**
 * @file store/home/takeover.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-home-takeover';

    /**
     * A list of text colors
     * @readonly
     * @enum {string}
     */
    var ColorsEnumeration = {
        "dark": "dark",
        "light": "light"
    };

    function originStoreHomeTakeoverCtrl($scope, $state, ProductFactory) {
        /**
         * Redirects the user to the store home page
         * @param {object} $event - Angular's event object
         * @return {void}
         */
        $scope.goToStore = function ($event) {
            $event.stopPropagation();
            $state.go('app.store.wrapper.home.notakeover');
        };

        /**
         * Redirects the user to the offer PDP page
         * @return {void}
         */
        $scope.goToPdp = function () {
            if ($scope.learnMoreUrl) {
                window.location.href = $scope.learnMoreUrl;
            } else {
                $state.go('app.store.wrapper.pdp', {
                    offerid: $scope.offerId
                });
            }
        };

        if ($scope.offerId) {
            ProductFactory.get($scope.offerId).attachToScope($scope, 'model');
        } else {
            $scope.model = {};
        }
    }

    function originStoreHomeTakeover(ComponentsConfigFactory, UtilFactory, CSSGradientFactory, $window) {

        function originStoreHomeTakeoverLink(scope, element, attributes) {
            var container = element.find('.l-origin-store-takeover');

            function onResize() {
                container.height($window.innerHeight);
            }

            angular.element($window).on('resize', onResize);
            onResize();


            scope.learnMoreText = UtilFactory.getLocalizedStr(attributes.learnMoreText, CONTEXT_NAMESPACE, 'learnmoretext');
            scope.continueToStoreText = UtilFactory.getLocalizedStr(attributes.continueToStoreText, CONTEXT_NAMESPACE, 'continuetostoretext');

            scope.hasLogo = (typeof scope.logo === 'string' && scope.logo.length > 0);
            scope.isDarkTextColor = (scope.textColor === ColorsEnumeration.dark);
            scope.fadeoutGradient = CSSGradientFactory.createGradientFadeout(scope.backgroundColor, 'top to bottom');

            container.css({
                'background-color': scope.backgroundColor
            });
        }

        return {
            restrict: 'E',
            scope: {
                offerId: '@',
                learnMoreUrl: '@',
                image: '@',
                logo: '@',
                title: '@',
                keyMessage: '@',
                videoId: '@',
                restrictedAge: '@',
                videoDescription: '@',
                textColor: '@',
                backgroundColor: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/home/views/takeover.html'),
            controller: originStoreHomeTakeoverCtrl,
            link: originStoreHomeTakeoverLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreHomeTakeover
     * @restrict E
     * @element ANY
     * @scope
     * @description Static promo module
     * @param {string} offer-id The offer ID
     * @param {url} learn-more-url Optional override for the "Learn More" button
     * @param {ImageUrl} image The background image for this module.
     * @param {ImageUrl} logo The game logo
     * @param {ColorsEnumeration} text-color The text color scheme: dark or light
     * @param {ColorsEnumeration} background-color The background color (rgba)
     * @param {LocalizedString} title The game title override
     * @param {Video} video-id The youtube video ID of the video for this promo (Optional).
     * @param {Number} restricted-age The Restricted Age of the youtube video for this promo (Optional).
     * @param {LocalizedString} video-description The text for the video cta
     * @param {LocalizedString} key-message The key message override
     * @param {LocalizedString} learn-more-text 'Learn More' button label, defaults to the component dictionary
     * @param {LocalizedString} continue-to-store-text 'Continue to Store', defaults to the component dictionary
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-home-takeover
     *             image="http://docs.x.origin.com/origin-x-design-prototype/images/store/template_heroic_.png"
     *             logo="http://docs.x.origin.com/origin-x-design-prototype/images/store/template_heroic_logo.png"
     *             title="Dragon Age: Inquisition"
     *             key-message="Immerse Yourself in the Ultimate Star Wars™ Experience"
     *             offer-id="OFB-EAST:1000032"
     *             text-color="light"
     *             background-color="rgba(30, 38, 44, 1)">
     *     </origin-store-home-takeover>
     * </example>
     */
    angular.module('origin-components')
        .controller('originStoreHomeTakeoverCtrl', originStoreHomeTakeoverCtrl)
        .directive('originStoreHomeTakeover', originStoreHomeTakeover);
}());
