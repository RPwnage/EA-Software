/**
 * @file store/pdp/scripts/featurevideo-wrapper.js
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

    var TextSlideEnumeration = {
        "true": true,
        "false": false
    };

    /**
     * list text alignment types
     * @readonly
     * @enum {string}
     */
    var TextAlignEnumeration = {
        "Left": "left",
        "Center": "center",
        "Right": "right"
    };

    /**
     * A list of text colors
     * @readonly
     * @enum {string}
     */
    var TextColorsEnumeration = {
        "dark": "dark",
        "light": "light"
    };

    /**
     * A list of button type options
     * @readonly
     * @enum {string}
     */
    var ButtonTypeEnumeration = {
        "none": "",
        "primary": "primary",
        "secondary": "secondary",
        "transparent": "transparent"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-pdp-feature-video-wrapper';

    function originStorePdpFeatureVideoWrapper(ComponentsConfigFactory, DirectiveScope) {
        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                anchorName: '@',
                showOnNav: '@',
                headerTitle: '@',
                videoId: '@',
                videoSmallScreenImageurl: '@',
                videoHeader: '@',
                videoText: '@',
                videoTextAlign: '@',
                videoTextColor: '@',
                videoTextSlide: '@',
                videoCtaType: '@',
                videoCtaDescription: '@',
                videoCtaHref: '@',
                matureContent: '@',
                videoImage: '@'
            },
            link: DirectiveScope.populateScopeLinkFn(CONTEXT_NAMESPACE),
            controller: 'originHeroWrapperCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/featurevideo-wrapper.html')
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureVideoWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description wrapper to hide/show header/nav item when no data is available
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @param {LocalizedString} header-title title of this section
     * @param {Video} video-id youtube id of video
     * @param {ImageUrl} video-small-screen-imageurl image to replace background video on small devices (600px or less)
     * @param {TextColorsEnumeration} video-text-color text color: light or dark
     * @param {TextAlignEnumeration} video-text-align text alignment
     * @param {BooleanEnumeration} video-text-slide boolean to determine if text slides in or not
     * @param {LocalizedString} video-header (optional) header text
     * @param {LocalizedString} video-text (optional) main paragraph
     * @param {ButtonTypeEnumeration} video-cta-type type of CTA button
     * @param {LocalizedString} video-cta-description CTA description text
     * @param {Url} video-cta-href URL of CTA link
     * @param {BooleanEnumeration} mature-content if the video has mature content
     * @param {ImageUrl} video-image will be displayed if video has mature content and user is under age
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-feature-video-wrapper></origin-store-pdp-feature-video-wrapper>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureVideoWrapper', originStorePdpFeatureVideoWrapper);
}());

