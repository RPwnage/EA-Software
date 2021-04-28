/**
 * @file home/merchandise/area.js
 */
(function() {
    'use strict';

    /**
     * Layout type mapping
     * @enum {string}
     */
    /* jshint ignore:start */
    var LayoutTypeEnumeration = {
        "one": "one",
        "two": "two"
    };
    /* jshint ignore:end */
    function originHomeMerchandiseArea(ComponentsConfigFactory) {

        function originHomeMerchandiseAreaLink(scope, element) {
            var merchandiseTilesContainer = element.find('.origin-home-merchtiles'),
                merchandiseTiles = merchandiseTilesContainer.children();

            //at this point cq targeting has already thrown out all the tiles that don't match
            if (merchandiseTiles.length) {
                //we will only show 1 or two of the promos
                var tilesToShow = ((scope.layouttype === 'one') || (merchandiseTiles.length === 1)) ? 1 : 2;

                //throw out everything after 1 or 2
                merchandiseTiles.slice(tilesToShow).remove();

                //wrap the directive in with the proper css
                merchandiseTiles.wrap('<li class="l-origin-home-merchtile"></li>');

                scope.visible = true;
            }
        }

        return {
            restrict: 'E',
            scope: {
                layouttype: '@'
            },
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/merchandise/views/area.html'),
            link: originHomeMerchandiseAreaLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeMerchandiseArea
     * @restrict E
     * @element ANY
     * @scope
     * @param {LayoutTypeEnumeration} layouttype the layouttype identifier
     * @description
     *
     * merchandise area
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-merchandise-area layouttype="two"></origin-home-merchandise-area>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originHomeMerchandiseArea', originHomeMerchandiseArea);
}());