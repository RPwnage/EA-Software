/**
 * @file store/pdp/scripts/details-wrapper.js
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

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpDetailsWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description wrapper to hide/show header/nav item when no data is available
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @param {LocalizedString} header-title title of this section
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-details-wrapper></origin-store-pdp-details-wrapper>
     * </origin-store-row>
     */
}());
