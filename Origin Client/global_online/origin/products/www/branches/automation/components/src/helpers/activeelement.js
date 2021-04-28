/**
 * Helper functions for active element (focused)
 *
 * This factory provides access to common functions when interacting
 * with the DOM through the keyboard.
 *
 *
 * @file helpers/activeelement.js
 */
(function() {
    'use strict';

    function ActiveElementHelper() {

        /**
         * Determine if the active element is a menu or menu item
         * @return {Boolean}
         */
        function isActiveElementMenuOrMenuItem() {
            var active = document.activeElement;

            if(active && active.attributes && active.attributes.keytarget) {
                return (active.attributes.keytarget.value === 'contextmenuitem') || (active.attributes.keytarget.value === 'contextmenu');
            }
        }

        /**
         * Gets the targets available for keyboard navigation
         * @param  {boolean} contextmenuFilter do we want only context menu items
         * @return {element[]} array of targets
         */
        function getVisibleTargets(contextmenuFilter) {

            var keyItemTargets = contextmenuFilter ? angular.element('[keytarget="contextmenuitem"]:visible') : angular.element('[keytarget]:visible');

            return keyItemTargets.filter(function() {
                return angular.element(this).css('max-height') !== '0px' &&
                    angular.element(this).css('opacity') !== '0' &&
                    angular.element(this).css('visibility') !== 'hidden' &&
                    angular.element(this).css('pointer-events') !== 'none';
            });

        }

        /**
         * gets the next element
         * @param  {number} focused     the current index
         * @param  {boolean} contextmenuFilter show context menut items only
         * @return {element[]}           array of elements
         */
        function getNext(focused, contextmenuFilter) {
            var next,
                keyboardTargets = getVisibleTargets(contextmenuFilter),
                index = keyboardTargets.index(focused);

            if (index < (keyboardTargets.length - 1)) {
                next = keyboardTargets.get(index + 1);
            } else if (index === keyboardTargets.length - 1) {
                next = keyboardTargets.eq(0);
            }

            return next;
        }

        /**
         * gets the previous element
         * @param  {number} focused     the current index
         * @param  {boolean} contextmenuFilter show context menut items only
         * @return {element[]}           array of elements
         */
        function getPrev(focused, contextmenuFilter) {
            var prev,
                keyboardTargets = getVisibleTargets(contextmenuFilter),
                index = keyboardTargets.index(focused);

            if (index > 0) {
                prev = keyboardTargets.get(index - 1);
            } else {
                prev = keyboardTargets.last();
            }
            return prev;
        }

        return {
            isActiveElementMenuOrMenuItem: isActiveElementMenuOrMenuItem,
            getNext: getNext,
            getPrev: getPrev
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ActiveElementHelper
     * @description
     *
     * Dom active Element Helper
     */
    angular.module('origin-components')
        .factory('ActiveElementHelper', ActiveElementHelper);

}());
