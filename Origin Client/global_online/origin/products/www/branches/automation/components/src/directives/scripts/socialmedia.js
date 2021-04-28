/**
 * @file socialmedia.js
 */
(function() {
    'use strict';
    /* jshint ignore:start */
    var ThemeEnumeration = {
        "light": "light",
        "dark": "dark",
        "lightWithBackground" : "lightWithBackground"
    };
    /* jshint ignore:end */
    
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var CONTEXT_NAMESPACE = 'origin-socialmedia';

    function originSocialmedia(ComponentsConfigFactory, DirectiveScope) {
        function originSocialmediaLink(scope){

            DirectiveScope.populateScope(scope, CONTEXT_NAMESPACE).then(function() {
                scope.isFacebookHidden = (scope.hideFacebook === BooleanEnumeration.true);
            });

        }

        return {
            restrict: 'E',
            scope: {
                theme: '@',
                hideFacebook: '@',
                titleStr: '@',
                description: '@',
                image: '@'
            },
            link: originSocialmediaLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/socialmedia.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialmedia
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {ThemeEnumeration} theme The color theme for the icons (dark = dark icons on light bg, light = light buttons on dark bg)
     * @param {BooleanEnumeration} hide-facebook Hide facebook
     * @param {LocalizedString} title-str Title of page to share
     * @param {LocalizedString} description Description of page to share
     * @param {ImageUrl} image Image to include in share dialog
     *
     * Social Media directive to hold buttons
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-socialmedia theme="dark"></origin-socialmedia>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originSocialmedia', originSocialmedia);
}());
