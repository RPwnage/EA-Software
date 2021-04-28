
/** 
 * @file /store/sitestripe/scripts/Messageoftheday.js
 */ 
(function(){
    'use strict';
    
    /**
    * The directive
    */
    function originStoreSitestripeMessageoftheday(ComponentsConfigFactory) {
        /**
        * The directive link
        */
        function originStoreSitestripeMessageofthedayLink(scope, elem, attrs, ctrl) {
            scope.siteStripeDismiss = ctrl.siteStripeDismiss;
        }
        return {
            require: '^originStoreSitestripeWrapper',
            restrict: 'E',
            replace: true,
            scope: {
                title: '@',
                subtitle: '@',
                image: '@',
                logo: '@',
                href: '@',
                edgecolor: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/sitestripe/views/messageoftheday.html'),
            link: originStoreSitestripeMessageofthedayLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSitestripeMessageoftheday
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title The title for this site stripe.
     * @param {LocalizedString} subtitle The title for this site stripe.
     * @param {ImageUrl|OCD} image The background image for this site stripe.
     * @param {ImageUrl|OCD} logo The logo image for this site stripe.
     * @param {string} href The URL for this site stripe to link to.
     * @param {string} edgecolor The background color for this site stripe (Hex: #000000).
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-sitestripe-wrapper
     *             last-modified="June 5, 2015 06:00:00 AM PDT">
     *             <origin-store-sitestripe-Messageoftheday
     *                 title="some title"
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
