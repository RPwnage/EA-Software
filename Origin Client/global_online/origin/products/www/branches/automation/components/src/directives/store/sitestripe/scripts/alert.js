/**
 * @file /store/sitestripe/scripts/Alert.js
 */
(function () {
    'use strict';

    /* jshint ignore:start */
    var FontColorEnumeration = {
        "light" : "#FFFFFF",
        "dark" : "#1E262C"
    };

    /* jshint ignore:end */

    var DEFAULT_EDGE_COLOR = '#C93507';

    var CONTEXT_NAMESPACE = 'origin-store-sitestripe-alert';

    function OriginStoreSitestripeAlertCtrl($scope) {
        $scope.edgecolor = DEFAULT_EDGE_COLOR;
        $scope.alert = true;
    }

    function originStoreSitestripeAlert(ComponentsConfigFactory, SiteStripeFactory) {
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
            controller: OriginStoreSitestripeAlertCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/sitestripe/views/messageoftheday.html'),
            link: SiteStripeFactory.siteStripeLinkFn(CONTEXT_NAMESPACE)
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSitestripeAlert
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
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-sitestripe-wrapper
     *             last-modified="June 5, 2015 06:00:00 AM PDT">
     *             <origin-store-sitestripe-alert
     *                 title-str="some title">
     *             </origin-store-sitestripe-alert>
     *         </origin-store-sitestripe-wrapper>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreSitestripeAlert', originStoreSitestripeAlert);
}());