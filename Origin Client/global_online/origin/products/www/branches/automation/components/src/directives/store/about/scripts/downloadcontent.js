/**
 * @file store/about/scripts/section-content.js
 */

(function () {
    'use strict';

    /**
     * Sets the layout of this module.
     * @readonly
     * @enum {string}
     */
    /* jshint ignore:start */
    var AlignmentEnumeration = {
        "center": "center",
        "left": "left"
    };

    var TextColorEnumeration = {
        "dark": "dark",
        "light": "light"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-download-section-content';

    function originStoreDownloadSectionContent(ComponentsConfigFactory, BeaconFactory, DirectiveScope) {

        function originStoreDownloadSectionContentLink(scope) {

            scope.isCompatible = null;


            function handleResponse(isCompatible) {
                scope.isCompatible = isCompatible;
            }

            function checkCompatibility() {
                BeaconFactory
                    .isInstallable()
                    .then(handleResponse);
            }

            DirectiveScope.populateScope(scope, CONTEXT_NAMESPACE)
                .then(checkCompatibility);

        }


        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                alignment: '@',
                compatibleTitleStr: '@',
                compatibleSubtitle: '@',
                incompatibleTitleStr: '@',
                incompatibleSubtitle: '@',
                textColor: '@'
            },
            link: originStoreDownloadSectionContentLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/downloadcontent.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreDownloadSectionContent
     * @restrict E
     * @element ANY
     * @scope
     * @param {AlignmentEnumeration} alignment Set heading alignment
     * @param {LocalizedString} compatible-title-str Text content for section's title when user is on a compatible platform
     * @param {LocalizedString} compatible-subtitle Text content for section's subtitle/description when user is on a compatible platform
     * @param {LocalizedString} incompatible-title-str Text content for section's title
     * @param {LocalizedString} incompatible-subtitle Text content for section's subtitle/description
     * @param {TextColorEnumeration} text-color Set color for heading text (defaults to black)
     *
     * @description Wrapper for the heading and content of each about section.
     *              Every section needs this directive. Transcludes the content. Comes
     *              with the heading built in.
     *
     * @example
     * <origin-store-about-section>
     *     <origin-store-download-section-content></origin-store-download-section-content>
     * </origin-store-about-section>
     */
    angular.module('origin-components')
        .directive('originStoreDownloadSectionContent', originStoreDownloadSectionContent);
}());
