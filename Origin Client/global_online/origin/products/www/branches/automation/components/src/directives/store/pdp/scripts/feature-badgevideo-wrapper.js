/**
 * @file store/pdp/scripts/feature-badgevideo-wrapper.js
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

    var CONTEXT_NAMESPACE = 'origin-store-pdp-feature-badge-video-wrapper';

    function originStorePdpFeatureBadgeVideoWrapper(ComponentsConfigFactory, DirectiveScope) {

        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                anchorName: '@',
                showOnNav: '@',
                headerTitle: '@',
                badgeImage: '@',
                quoteStr: '@',
                videoId: '@',
                smallScreenImageurl: '@',
                matureContent: '@',
                videoImage: '@'
            },
            link: DirectiveScope.populateScopeLinkFn(CONTEXT_NAMESPACE),
            controller: 'originHeroWrapperCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/feature-badgevideo-wrapper.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureBadgeVideoWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description wrapper to hide/show header/nav item when no data is available
     *
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {String} anchor-name the hash anchor name to link to
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @param {LocalizedString} header-title title of this section
     * @param {LocalizedString} quote-str Promo text
     * @param {ImageUrl} badge-image badge image
     * @param {ImageUrl} video-image will be displayed if video has mature content and user is under age
     * @param {Video} video-id youtube id of video
     * @param {ImageUrl} small-screen-imageurl image to replace background video on small devices (600px or less)
     * @param {BooleanEnumeration} mature-content if video has mature content
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-feature-badge-video-wrapper
     *             quote-str="Awesome game"
     *             badge-image="some image.png"
     *             video-id="abcdef"
     *         </origin-store-pdp-feature-accolade-video-wrapper>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureBadgeVideoWrapper', originStorePdpFeatureBadgeVideoWrapper);
}());