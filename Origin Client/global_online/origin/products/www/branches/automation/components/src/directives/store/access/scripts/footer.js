/** 
 * @file store/access/scripts/footer.js
 */ 
(function(){
    'use strict';

    var CONTENT_ITEM_CLASS = 'origin-store-access-footer-content-hidden';

    function originStoreAccessFooter(ComponentsConfigFactory, AnimateFactory, ScreenFactory) {
        function originStoreAccessFooterLink(scope, element) {
            var contentElement = element.find('.' + CONTENT_ITEM_CLASS);

            scope.slice = ComponentsConfigFactory.getImagePath('store/right-corner.png');

            function updateContent() {
                if(ScreenFactory.isXSmall()) {
                    if(contentElement.hasClass(CONTENT_ITEM_CLASS)) {
                        contentElement.removeClass(CONTENT_ITEM_CLASS);
                    }
                } else {
                    if(ScreenFactory.isOnOrAboveScreen(contentElement) && contentElement.hasClass(CONTENT_ITEM_CLASS)) {
                        contentElement.removeClass(CONTENT_ITEM_CLASS);
                    } else if(!ScreenFactory.isOnOrAboveScreen(contentElement) && !contentElement.hasClass(CONTENT_ITEM_CLASS)) {
                        contentElement.addClass(CONTENT_ITEM_CLASS);
                    }
                }
            }

            AnimateFactory.addScrollEventHandler(scope, updateContent);

            updateContent();
        }
        
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                titleStr: '@',
                legal: '@',
                buttonText: '@',
                offerId: '@',
                small: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/views/footer.html'),
            link: originStoreAccessFooterLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessFooter
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title-str The title for this footer module
     * @param {LocalizedString} button-text The button text for this footer module
     * @param {string} offer-id subscription entitlement offerId
     * @param {LocalizedString} small The small text below the button
     * @param {LocalizedString} legal The legal text below this footer module
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *      <origin-store-access-footer
     *          title-str="title"
     *          small="some text"
     *          legal="some legal text"
     *          button-text="some button text"
     *          offer-id="Origin.OFR.50.0001171">
     *      </origin-store-access-footer />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessFooter', originStoreAccessFooter);
}()); 
