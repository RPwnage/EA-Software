/**
 * @file store/pdp/feature/scripts/badgevideo.js
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
     * @name origin-components.directives:originStorePdpFeatureBadgeVideo
     * @restrict E
     * @element ANY
     * @scope
     * @description badge video for pdp
     * @param {LocalizedString} quote-str quote
     * @param {ImageUrl} badge-image badge image
     * @param {ImageUrl} video-image will be displayed if video has mature content and user is under age
     * @param {Video} video-id youtube id of video
     * @param {ImageUrl} small-screen-imageurl image to replace background video on small devices (600px or less)
     * @param {BooleanEnumeration} mature-content if video has mature content
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-feature-badge-video></origin-store-pdp-feature-badge-video>
     *     </file>
     * </example>
     */
}());
