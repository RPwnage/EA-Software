/**
 * @file store/pdp/scripts/feature-accoladevideo-wrapper.js
 */
(function(){
    'use strict';

    /**
     * BooleanTypeEnumeration list of allowed types
     * @enum {string}
     */
    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-pdp-feature-accolade-video-wrapper';

    function originStorePdpFeatureAccoladeVideoWrapper(ComponentsConfigFactory, DirectiveScope) {

        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                anchorName: '@',
                showOnNav: '@',
                headerTitle: '@',
                quote1Str: '@',
                quote1SourceStr: '@',
                quote2Str: '@',
                quote2SourceStr: '@',
                quote3Str: '@',
                quote3SourceStr: '@',
                videoId: '@',
                smallScreenImageurl: '@',
                matureContent: '@',
                videoImage: '@'
            },
            link: DirectiveScope.populateScopeLinkFn(CONTEXT_NAMESPACE),
            controller: 'originHeroWrapperCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/feature-accoladevideo-wrapper.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureAccoladeVideoWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description wrapper to hide/show header/nav item when no data is available
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {String} anchor-name the hash anchor name to link to
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @param {LocalizedString} header-title title of this section
     * @param {LocalizedString} quote1-str Promo quote 1 - should be wrapped in quotes - use \&#8220; text \&#8221;
     * @param {LocalizedString} quote1-source-str Promo quote 1 source
     * @param {LocalizedString} quote2-str Promo quote 2 - not visible on small screens - should be wrapped in quotes - use  \&#8220; text \&#8221;
     * @param {LocalizedString} quote2-source-str Promo quote 2 source - not visible on small screens
     * @param {LocalizedString} quote3-str Promo quote 3 - not visible on small screens - should be wrapped in quotes - use  \&#8220; text \&#8221;
     * @param {LocalizedString} quote3-source-str Promo quote 3 source - not visible on small screens
     * @param {ImageUrl} video-image will be displayed if video has mature content and user is under age
     * @param {Video} video-id youtube id of video
     * @param {ImageUrl} small-screen-imageurl image to replace background video on small devices (600px or less)
     * @param {BooleanEnumeration} mature-content if video has mature content
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-feature-accolade-video-wrapper
     *             quote1-str="Awesome game"
     *             quote1-source-str="ign"
     *             video-id="abcdef"
     *         </origin-store-pdp-feature-accolade-video-wrapper>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureAccoladeVideoWrapper', originStorePdpFeatureAccoladeVideoWrapper);
}());