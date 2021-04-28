/**
 * @file /store/sitestripe/scripts/Info.js
 */
(function(){
    'use strict';

    var DEFAULT_EDGE_COLOR = '#269DCF';

    /* jshint ignore:start */
    var FontColorEnumeration = {
        "light" : "#FFFFFF",
        "dark" : "#1E262C"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-sitestripe-info';

    function OriginStoreSitestripeInfoCtrl($scope) {
        $scope.edgecolor = DEFAULT_EDGE_COLOR;
        $scope.info = true;
    }

    function originStoreSitestripeInfo(ComponentsConfigFactory, SiteStripeFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                uniqueSitestripeId: '@',
                titleStr: '@',
                fontcolor: '@',
                ctid: '@',
                showOnPage: '@',
                hideOnPage: '@'
            },
            controller: OriginStoreSitestripeInfoCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/sitestripe/views/messageoftheday.html'),
            link: SiteStripeFactory.siteStripeLinkFn(CONTEXT_NAMESPACE)
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSitestripeInfo
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {string} unique-sitestripe-id a unique id to identify this site stripe from others. Must be specified to enable site stripe dismissal.
     * @param {LocalizedString} title-str The title for this site stripe.
     * @param {FontColorEnumeration} fontcolor The font color of the site stripe, light or dark.
     * @param {string} ctid Campaign Targeting ID
     * @param {string} show-on-page Urls to show this sitestripe on (comma separated)
     * @param {string} hide-on-page Urls to hide this sitestripe on (comma separated)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-sitestripe-wrapper
     *             last-modified="June 5, 2015 06:00:00 AM PDT">
     *             <origin-store-sitestripe-info
     *                 title-str="some title">
     *             </origin-store-sitestripe-info>
     *         </origin-store-sitestripe-wrapper>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreSitestripeInfo', originStoreSitestripeInfo);
}());