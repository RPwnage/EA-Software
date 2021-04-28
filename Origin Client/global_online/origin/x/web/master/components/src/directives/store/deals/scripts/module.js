/**
 * @file store/deals/scripts/module.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-deals';

    function originStoreDealsModule(ComponentsConfigFactory, UtilFactory) {

        function originStoreDealsModuleLink(scope) {
            scope.learnMoreText = UtilFactory.getLocalizedStr(scope.learnMoreText, CONTEXT_NAMESPACE, 'learnmore');
        }

        return {
            restrict: 'E',
            scope: {
                image: '@',
                header: '@',
                title: '@',
                text: '@',
                learnMoreUrl: '@',
                learnMoreText: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/deals/views/module.html'),
            link: originStoreDealsModuleLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreDealsModule
     * @restrict E
     * @element ANY
     * @scope
     * @description Store deals module
     * @param {ImageUrl} image The background image for this module.
     * @param {LocalizedString} header header text
     * @param {LocalizedString} title promo title
     * @param {LocalizedString} text promo text
     * @param {LocalizedString} learn-more-text the "Learn More" button caption
     * @param {url} learn-more-url Optional override for the "Learn More" button
     *
     * @example
     * <example module="origin-components">
     *     <origin-store-deals-module
     *         image="http://docs.x.origin.com/origin-x-design-prototype/images/ggg.png"
     *         header="Great Game Guarantee"
     *         title="We Stand By Our Games. If you don't love it, return it."
     *         text="If you're not completly satisfied with your EA full games digital download (PC or Mac) purchased on Origin, you can get a full refund."
     *         learn-more-text="Learn more"
     *         learnMoreUrl="greatgameguarantee">
     *     </origin-store-deals-module>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreDealsModule', originStoreDealsModule);
}());
