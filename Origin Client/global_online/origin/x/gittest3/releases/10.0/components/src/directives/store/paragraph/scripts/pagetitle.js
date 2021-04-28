
/** 
 * @file store/paragraph/scripts/pagetitle.js
 */ 
(function(){
    'use strict';

    /**
    * The directive
    */
    function originStoreParagraphPagetitle() {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                description: '@',
                showSocialMedia: '@showsocialmedia'
            },
            template:   '<h1 class="origin-store-paragraph-pagetitle otktitle-page">' +
                            '<span>{{ description }}</span>' +
                            '<origin-socialmedia ng-if="showSocialMedia===\'true\'"></origin-socialmedia>' +
                        '</h1>'
        };
    }

    /**
     * Show or Hide The social media icons
     * @readonly
     * @enum {string}
     */
    /* jshint ignore:start */
    var ShowSocialMediaEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreParagraphPagetitle
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} description The text for this paragraph.
     * @param {ShowSocialMediaEnumeration} showsocialmedia - If social media icons should be shown.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-paragraph-pagetitle 
     *     		description="Some text."
     *          showsocialmedia="true">
     *     </origin-store-paragraph-pagetitle>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreParagraphPagetitle', originStoreParagraphPagetitle);
}());