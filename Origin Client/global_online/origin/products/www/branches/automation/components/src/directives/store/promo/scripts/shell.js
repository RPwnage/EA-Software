
/** 
 * @file store/promo/scripts/shell.js
 */ 
(function(){
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */
    
    function originStorePromoShell(ComponentsConfigFactory, NavigationFactory) {
        function originStorePromoShellLink (scope, element) {
            scope.images = [];
            if (scope.image){
                scope.images.push(scope.image);
            }
            scope.onBtnClick = function(e, url) {
                var href = url || scope.href;

                if(href){
                    NavigationFactory.openLink(href);
                }
            };

            var carouselElement = element.parents('.origin-store-carousel-hero');
            
            if (carouselElement.length > 0) {
                carouselElement.addClass('origin-store-promo-carousel');
            }
        }

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                backgroundColor: '@',
                badgeImage: '@',
                image: '@',
                videoid: '@',
                matureContent: '@',
                href: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/shell.html'),
            link: originStorePromoShellLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoShell
     * @restrict E
     * @element ANY
     *
     * @param {ImageUrl} image The background image for this module.
     * @param {ImageUrl} badge-image optional badge image
     * @param {Url} href Url that this module links to
     * @param {Video} videoid The youtube video ID of the video for this promo (Optional).
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {string} background-color Hex value of the background color
     *
     * @description Contsructs the basic scaffolding for a promo item, including the background image 
     * and a placeholder for the banner. Transcludes a banner. Used to build out a promo item.
     *
     * @example
     * <file name="index.html">
     *   <origin-store-promo-shell ocd-path="" start-color="" end-color="" image="" videoid="" href="">
     *       <origin-store-promo-banner-static></origin-store-promo-banner-static>
     *   </origin-store-promo-shell>
     *   </file>
     */
    angular.module('origin-components')
        .directive('originStorePromoShell', originStorePromoShell);
}()); 
