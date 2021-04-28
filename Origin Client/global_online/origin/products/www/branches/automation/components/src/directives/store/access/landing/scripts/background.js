
/** 
 * @file store/access/landing/scripts/background.js
 */ 
(function(){
    'use strict';

    function originStoreAccessLandingBackground() {
        function originStoreAccessLandingBackgroundLink(scope, elem, attrs, ctrl) {
            var images = {
                background: _.get(scope, 'background', ''),
                foreground: _.get(scope, 'foreground', '')
            };

            if(images.background && images.foreground) {
                ctrl.registerBackground(images);
            }
        }
        return {
            restrict: 'E',
            require: '^originStoreAccessLandingHero',
            scope: {
                background: '@',
                foreground: '@'
            },
            link: originStoreAccessLandingBackgroundLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessLandingBackground
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {ImageUrl} background the background image
     * @param {ImageUrl} foreground the foreground image
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-landing-background />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessLandingBackground', originStoreAccessLandingBackground);
}()); 
