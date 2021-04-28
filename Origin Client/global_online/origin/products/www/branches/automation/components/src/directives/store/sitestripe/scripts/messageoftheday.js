
/**
 * @file /store/sitestripe/scripts/Messageoftheday.js
 */ 
(function(){
    'use strict';

    var FontColorEnumeration = {
        "light" : "#FFFFFF",
        "dark" : "#1E262C"
    };

    var ImagePositionEnumeration = {
        "left": "left",
        "center": "center"
    };

    var CONTEXT_NAMESPACE = 'origin-store-sitestripe-messageoftheday',
        IMAGE_CLASS_CENTER = 'origin-store-sitestripe-messageoftheday-image-center',
        IMAGE_CLASS_LEFT = 'origin-store-sitestripe-messageoftheday-image-left';

    function originStoreSitestripeMessageofthedayCtrl($scope, NavigationFactory){
        $scope.goTo = function(event, href){
            event.preventDefault();
            NavigationFactory.openLink(href);
        };
        $scope.hoverClass = $scope.fontcolor === FontColorEnumeration.dark ? 'close-transition-dark' : 'close-transition-light';

        $scope.imagePositionClass = $scope.imagePosition === ImagePositionEnumeration.center ? IMAGE_CLASS_CENTER: IMAGE_CLASS_LEFT;
    }

    function originStoreSitestripeMessageoftheday(ComponentsConfigFactory, SiteStripeFactory) {

        return {
            restrict: 'E',
            replace: true,
            scope: {
                uniqueSitestripeId: '@',
                titleStr: '@',
                subtitle: '@',
                image: '@',
                logo: '@',
                href: '@',
                edgecolor: '@',
                fontcolor: '@',
                ctid: '@',
                imagePosition: '@',
                showOnPage: '@',
                hideOnPage: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/sitestripe/views/messageoftheday.html'),
            controller: originStoreSitestripeMessageofthedayCtrl,
            link: SiteStripeFactory.siteStripeLinkFn(CONTEXT_NAMESPACE)
        };
    }
    
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSitestripeMessageoftheday
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title-str The title for this site stripe.
     * @param {LocalizedString} subtitle The title for this site stripe.
     * @param {ImageUrl|OCD} image The background image for this site stripe.
     * @param {ImageUrl|OCD} logo The logo image for this site stripe.
     * @param {string} href The URL for this site stripe to link to.
     * @param {string} edgecolor The background color for this site stripe (Hex: #000000).
     * @param {FontColorEnumeration} fontcolor The font color of the site stripe, light or dark.
     * @param {string} unique-sitestripe-id a unique id to identify this site stripe from others. Must be specified to enable site stripe dismissal.
     * @param {string} ctid Campaign Targeting ID
     * @param {ImagePositionEnumeration} image-position left or center. Default to left
     * @param {string} show-on-page Urls to show this sitestripe on (comma separated)
     * @param {string} hide-on-page Urls to hide this sitestripe on (comma separated)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-sitestripe-wrapper
     *             last-modified="June 5, 2015 06:00:00 AM PDT">
     *             <origin-store-sitestripe-Messageoftheday
     *                 title-str="some title"
     *                 subtitle="some subtitle"
     *                 image="//somedomain/someimage.jpg"
     *                 logo="//somedomain.com/someimage.jpg"
     *                 href="//somedomain.com"
     *                 edgecolor="#000000">
     *             </origin-store-sitestripe-Messageoftheday>
     *         </origin-store-sitestripe-wrapper>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreSitestripeMessageoftheday', originStoreSitestripeMessageoftheday);
}());
