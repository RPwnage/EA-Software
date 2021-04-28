/**
 * @file common/dom-helper.js
 */
(function () {
    'use strict';
    function OriginDomHelper() {
        /**
         *
         * @param parentElement
         * @param element
         * @param attrs
         * @param value
         */
        function addTag(parentElement, element, attrs, text){
            if (parentElement && element) {
                var $parent = angular.element(parentElement),
                    $newTag = angular.element(document.createElement(element));
                if (angular.isObject(attrs) && !_.isEmpty(attrs)) {
                    _.forEach(attrs, function(value, key){
                        attrs[key] = _.escape(attrs[key]);
                    });
                    $newTag.attr(attrs);
                }
                if (text) {
                    $newTag.text(text);
                }
                $parent.append($newTag);
            }
        }

        /**
         *
         * Builds a dom query string that macthes the given arguments
         *
         * @param parent
         * @param element
         * @param attrs
         * @returns {string}
         */
        function getQuerySelector(parent, element, attrs) {
            var stringTemplate = '[%name%="%value%"]',
                selector = [parent, element].join(' ');
            if (!_.isEmpty(attrs)) {
                _.forEach(attrs, function(value, key){
                    key = _.escape(key);
                    value = _.escape(value);
                    selector += stringTemplate.replace('%name%', key).replace('%value%', value);
                });
            }
            return selector;
        }

        /**
         * Removes element that matches the attrs
         *
         * @param element
         * @param attrs
         */
        function removeTag(parent, element, attrs){
            var $element = angular.element(getQuerySelector(parent, element, attrs));
            $element.remove();
        }

        /**
         * Removes the tag and update with new attributes and text
         *
         * @param parent
         * @param element
         * @param attrs
         * @param text
         */
        function updateTag(parent, element, attrs, text){
            var query = getQuerySelector(parent, element, attrs),
                $element = angular.element(query);
            $element.remove();
            addTag(parent, element, attrs, text);
        }

        return {
            addTag: addTag,
            updateTag: updateTag,
            removeTag: removeTag
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function OriginDomHelperSingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('OriginDomHelper', OriginDomHelper, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.OriginDomHelper
     * @description
     *
     * Dedicated factory for dom interaction
     *
     */
    angular.module('origin-components')
        .factory('OriginDomHelper', OriginDomHelperSingleton);

})();
