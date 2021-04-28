
/**
 * @file store/pdp/feature/scripts/bubble.js
 */
(function(){
    'use strict';

    function originStorePdpFeatureBubble(ComponentsConfigFactory) {


        return {
            restrict: 'E',
            replace: true,
            scope: {
                image: '@',
                title: '@',
                text: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/feature/views/bubble.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureBubble
     * @restrict E
     * @element ANY
     * @scope
     *
     * @param {ImageUrl} image The banner image
     * @param {LocalizedString} title The title of the module
     * @param {LocalizedString} text The title of the module
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *      <origin-store-pdp-feature-bubble
     *              title="Title"
     *              text="Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod"
     *              image="https://qa.preview.cms.origin.com/content/dam/originx/web/app/home/actions/tile_chinarising_long.png">
     *      </origin-store-pdp-feature-bubble>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureBubble', originStorePdpFeatureBubble);
}());
