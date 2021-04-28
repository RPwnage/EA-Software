/**
 * @file store/paragraph/scripts/pagetitle.js
 */
(function () {
    'use strict';

    /**
     * Show or Hide The social media icons
     * @readonly
     * @enum {string}
     */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var CONTEXT_NAMESPACE = 'origin-store-paragraph-pagetitle';

    function originStoreParagraphPagetitle(ComponentsConfigFactory, DirectiveScope) {
        function originStoreParagraphPagetitleLink(scope) {

            function postPopulate() {
                scope.showSocialMedia = scope.showSocialMedia === BooleanEnumeration.true;
            }

            DirectiveScope.populateScope(scope, CONTEXT_NAMESPACE).then(postPopulate);
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                description: '@',
                showSocialMedia: '@showsocialmedia',
                facebookShareTitle: '@',
                facebookShareDescription: '@',
                facebookShareImage: '@'
            },
            link: originStoreParagraphPagetitleLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/paragraph/views/pagetitle.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreParagraphPagetitle
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {LocalizedString} description The text for this paragraph.
     * @param {BooleanEnumeration} showsocialmedia - If social media icons should be shown.
     * @param {LocalizedString} facebook-share-title Facebook share title
     * @param {LocalizedString} facebook-share-description Facebook share description
     * @param {ImageUrl} facebook-share-image Facebook share image
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-paragraph-pagetitle
     *            description="Some text."
     *          showsocialmedia="true">
     *     </origin-store-paragraph-pagetitle>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreParagraphPagetitle', originStoreParagraphPagetitle);
}());
