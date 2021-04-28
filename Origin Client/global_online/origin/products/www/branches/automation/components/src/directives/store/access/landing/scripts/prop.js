
/** 
 * @file store/access/landing/scripts/prop.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-landing-prop';
    var PARALLAX_ELEMENT = '.origin-store-access-landing-prop-background-image';

    function OriginStoreAccessLandingPropCtrl($scope, UtilFactory) {
        $scope.strings = {
            header: UtilFactory.getLocalizedStr($scope.header, CONTEXT_NAMESPACE, 'header'),
            caption: UtilFactory.getLocalizedStr($scope.caption, CONTEXT_NAMESPACE, 'caption')
        };
    }
    function originStoreAccessLandingProp($timeout, AnimateFactory, ComponentsConfigFactory, CSSUtilFactory, ScreenFactory) {
        function originStoreAccessLandingPropLink(scope, element) {
            var paralaxImage = element.find(PARALLAX_ELEMENT);

            function updatePosition() {
                if (paralaxImage.length && !ScreenFactory.isSmall()) {
                    var top = paralaxImage.offset().top - 0,
                        parallaxPosition = ((window.pageYOffset - top) / 8).toFixed(2),
                        transition = 'translate3d(0,' + parallaxPosition + 'px, 0)';
                    paralaxImage.css(CSSUtilFactory.addVendorPrefixes('transform', transition));
                }
            }

            function clearParallax() {
                if (ScreenFactory.isSmall() && paralaxImage.length) {
                    paralaxImage.removeAttr('style');
                }
            }

            AnimateFactory.addScrollEventHandler(scope, updatePosition);
            AnimateFactory.addResizeEventHandler(scope, clearParallax, 100);

            $timeout(updatePosition, 0, false);
        }
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                header: '@',
                background: '@',
                foreground: '@',
                logo: '@',
                caption: '@'
            },
            controller: OriginStoreAccessLandingPropCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/landing/views/prop.html'),
            link: originStoreAccessLandingPropLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessLandingProp
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} header the main title
     * @param {ImageUrl} background the background image
     * @param {ImageUrl} foreground the foreground character image
     * @param {ImageUrl} logo the logo in the bottom left
     * @param {LocalizedString} caption the text below the logo
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-landing-prop />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessLandingProp', originStoreAccessLandingProp);
}()); 
