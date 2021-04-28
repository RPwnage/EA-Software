/**
 * @file store/pdp/feature/scripts/accoladevideo.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    /* jshint ignore:end */

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureAccoladeVideo
     * @restrict E
     * @element ANY
     * @scope
     * @description
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
     *
     * Large PDP feature image with parallax effect
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-feature-accolade-video></origin-store-pdp-feature-accolade-video>
     *     </file>
     * </example>
     */
}());
