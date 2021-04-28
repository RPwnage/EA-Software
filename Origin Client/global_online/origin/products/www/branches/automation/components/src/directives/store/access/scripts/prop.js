
/** 
 * @file store/access/scripts/prop.js
 */ 
(function(){
    'use strict';

    var CONTENT_ITEM_CLASS = 'origin-store-access-prop-content-hidden',
        PARALLAX_IMG_SELECTOR = '.origin-store-access-prop-background img',
        PARALLAX_EFFECT = 8; // How strong the parallax effect is. Inverse relation ie. smaller number is larger effect.

    /**
     * Show or Hide the info bubble
     * @readonly
     * @enum {string}
     */
    /* jshint ignore:start */
    var alignmentEnumeration = {
        "First module below hero": "top",
        "Lower module, text on the left": "left",
        "Lower module, text on the right": "right"
    };
    /* jshint ignore:end */

    function originStoreAccessProp(ComponentsConfigFactory, CSSUtilFactory, AnimateFactory, ScreenFactory) {
        function originStoreAccessPropLink(scope, element) {
            var parallaxImage = element.find(PARALLAX_IMG_SELECTOR),
                contentElement = element.find('.' + CONTENT_ITEM_CLASS);

            scope.leftSlice = ComponentsConfigFactory.getImagePath('store/left-corner.png');
            scope.rightSlice = ComponentsConfigFactory.getImagePath('store/right-corner.png');

            /**
             * Renders parallax effect
             * @return {void}
             */
            function updatePosition() {
                if (parallaxImage.length && !ScreenFactory.isSmall()) {
                    var top = element.offset().top - 0,
                        parallaxPosition = ((window.pageYOffset - top) / PARALLAX_EFFECT).toFixed(2),
                        transition = 'translate3d(0,' + parallaxPosition + 'px, 0)';
                    parallaxImage.css(CSSUtilFactory.addVendorPrefixes('transform', transition));
                }

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

            function clearParallax() {
                if (ScreenFactory.isSmall() && parallaxImage.length) {
                    parallaxImage.removeAttr('style');
                }
            }

            AnimateFactory.addScrollEventHandler(scope, updatePosition);
            AnimateFactory.addResizeEventHandler(scope, clearParallax, 100);

            updatePosition();
        }
        
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                titleStr: '@',
                background: '@',
                image: '@',
                alignment: '@',
                legal: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/views/prop.html'),
            link: originStoreAccessPropLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessProp
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {ImageUrl} background The background image
     * @param {ImageUrl} image The forground character image
     * @param {LocalizedString} title-str The title for this hero module
     * @param {LocalizedString} legal The legal text for this module
     * @param {alignmentEnumeration} alignment Choose how this module will be aligned
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-prop
     *          title-str="some title"
     *          legal="some text"
     *          image="https://someimage.com/pic.jpg"
     *          background="https://someimage.com/background.jpg"
     *          alignment="left">
     *     </origin-store-access-prop>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessProp', originStoreAccessProp);
}()); 
