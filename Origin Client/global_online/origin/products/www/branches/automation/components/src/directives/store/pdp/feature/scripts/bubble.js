
/**
 * @file store/pdp/feature/scripts/bubble.js
 */
(function(){
    'use strict';

    var GRANDPARENT_CONTEXT_NAMESPACE = 'origin-store-pdp-feature-container-wrapper';
    var PARENT_CONTEXT_NAMESPACE = 'origin-store-pdp-feature-container';
    var CONTEXT_NAMESPACE = 'origin-store-pdp-feature-bubble';

    function originStorePdpFeatureBubble(ComponentsConfigFactory, DirectiveScope) {
        function originStorePdpFeatureBubbleLink(scope, element, attribute, originStorePdpFeatureContainerCtrl) {
            DirectiveScope.populateScope(scope, [GRANDPARENT_CONTEXT_NAMESPACE, PARENT_CONTEXT_NAMESPACE, CONTEXT_NAMESPACE]).then(function() {
                if (scope.image || scope.titleStr || scope.text) {
                    originStorePdpFeatureContainerCtrl.registerItem();
                }
            });
        }

        return {
            restrict: 'E',
            require: '^originStorePdpFeatureContainer',
            replace: true,
            scope: {
                image: '@',
                titleStr: '@',
                text: '@'
            },
            link: originStorePdpFeatureBubbleLink,
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
     * @param {LocalizedString} title-str The title of the module
     * @param {LocalizedString} text The title of the module
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *      <origin-store-pdp-feature-bubble
     *              title-str="Title"
     *              text="Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod"
     *              image="https://qa.preview.cms.origin.com/content/dam/originx/web/app/home/actions/tile_chinarising_long.png">
     *      </origin-store-pdp-feature-bubble>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureBubble', originStorePdpFeatureBubble);
}());
