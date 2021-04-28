/**
 * @file cta/icon.js
 */

(function() {
    'use strict';

    /**
     * A list of button type options
     * @readonly
     * @enum {string}
     */
    var ButtonTypeEnumeration = {
        "secondary": "secondary",
        "transparent": "transparent",
        "round": "round"
    };

    /**
     * A list of icons that can be applied. This should be a growing list and match the icons from directives/scripts/icon.js
     * @enum {string}
     */
    var IconEnumeration = {
        "language": "language"
    };

    function OriginCtaIconCtrl($scope, $state) {

        $scope.type = $scope.type || ButtonTypeEnumeration.primary;
        $scope.icon = IconEnumeration[$scope.icon];

        $scope.onBtnClick = function() {
            $state.go($scope.href);
        };

    }

    function originCtaIcon(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                href: '@',
                btnText: '@description',
                type: '@',
                icon: '@'
            },
            controller: 'OriginCtaIconCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/icon.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaIcon
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} description the button text
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @param {Url} href the internal origin url you want to load
     * @param {IconEnumeration} icon can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @description
     *
     * Load internal url call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-icon  description="Take me to internal page" type="secondary"></origin-cta-icon>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaIconCtrl', OriginCtaIconCtrl)
        .directive('originCtaIcon', originCtaIcon);
}());