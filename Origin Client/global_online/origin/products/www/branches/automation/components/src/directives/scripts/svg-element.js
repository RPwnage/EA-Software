/**
 * @file scripts/svg-element.js
 */
(function () {
    'use strict';
    function originSvgElement(ComponentsConfigFactory) {

        var IMAGE_DEFAULTS = {
                x: '0',
                y: '0',
                width: '100%',
                height: '100%'
            };

        function originSvgElementLink(scope, element) {
            var $svgElement = element.find('svg');
            var $imageElement = element.find('image');
            
            function setElementAttributes($element, attributes){
                if ($element && $element.length) {
                    _.forEach(attributes, function (value, attribute) {
                        $element[0].setAttribute(attribute, value);
                    });
                }
            }

            scope.options = _.extend({}, IMAGE_DEFAULTS, scope.imageOptions);
            setElementAttributes($imageElement, scope.options);
            setElementAttributes($svgElement, scope.svgOptions);
        }

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                imageUrl: '=',
                imageOptions: '=',
                svgOptions: '='
            },
            link: originSvgElementLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/svg-element.html')
        };

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSvgElement
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {imageUrl} image-url url of the image to blur
     * @param {Object} image-options (optional) image params
     * @param {Object} svg-options (optional) svg attributes and values
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-svg-element image-url="" svg-options="{}" image-options="{x: '-60%', y: '0', width: '200%', height: '200%'}"/>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originSvgElement', originSvgElement);
}());
