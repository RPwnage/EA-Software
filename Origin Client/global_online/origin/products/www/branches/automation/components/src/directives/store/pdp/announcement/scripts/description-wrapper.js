/**
 * @file store/pdp/announcement/scripts/description-wrapper.js
 */
(function () {
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

    var CONTEXT_NAMESPACE = 'origin-store-announcement-description-wrapper';

    function originStoreAnnouncementDescriptionWrapper(ComponentsConfigFactory, DirectiveScope) {
        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                anchorName: '@',
                showOnNav: '@',
                headerTitle: '@',
                longDescription: '@'
            },
            link: DirectiveScope.populateScopeLinkFn(CONTEXT_NAMESPACE),
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/announcement/views/description-wrapper.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAnnouncementDescriptionWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description wrapper to hide/show header/nav item when no data is available
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @param {LocalizedString} header-title title of this section
     * @param {LocalizedText} long-description game description html
     *
     * @example
     *  <origin-store-announcement-description-wrapper>
     *  </origin-store-announcement-description-wrapper>
     */
    angular.module('origin-components')
        .directive('originStoreAnnouncementDescriptionWrapper', originStoreAnnouncementDescriptionWrapper);

}());