/**
 * @file home/merchandise/area.js
 */
(function() {
    'use strict';

    /**
     * Layout type mapping
     * @enum {string}
     */
    var LayoutTypeEnumeration = {
        "one": "one",
        "two": "two"
    };

    var CONTEXT_NAMESPACE = 'origin-home-merchandise-area';

    function originHomeMerchandiseArea(UtilFactory, ComponentsConfigFactory, $stateParams) {

        function originHomeMerchandiseAreaLink(scope, element) {
            var merchandiseTilesContainer = element.find('.origin-home-merchtiles'),
                merchandiseTiles = merchandiseTilesContainer.children();

            scope.titleStr = UtilFactory.getLocalizedStr(scope.titleStr, CONTEXT_NAMESPACE, 'title-str');
            scope.subtitleStr = UtilFactory.getLocalizedStr(scope.subtitleStr, CONTEXT_NAMESPACE, 'subtitle-str');

            //if this is triggered from ITO do not show the merchandise
            if($stateParams.source === 'ITO') {
                scope.visible = false;
                return;
            }

            //at this point cq targeting has already thrown out all the tiles that don't match
            if (merchandiseTiles.length) {
                //we will only show 1 or two of the promos
                var tilesToShow = ((scope.layouttype === LayoutTypeEnumeration.one) || (merchandiseTiles.length === 1)) ? 1 : 2;

                //throw out everything after 1 or 2
                merchandiseTiles.slice(tilesToShow).remove();

                scope.visible = true;
            }
        }

        return {
            restrict: 'E',
            scope: {
                layouttype: '@',
                titleStr:'@',
                subtitleStr: '@'
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
     * @param {LocalizedString} title-str the header for the merchandise area
     * @param {LayoutTypeEnumeration} layouttype the layouttype identifier
     * @param {LocalizedString} subtitle-str the subtitle for the merchandise area
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