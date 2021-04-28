/**
 * @file common/ocd-helper.js
 */
(function(){

    'use strict';

    function OcdHelperFactory(ObjectHelperFactory) {
        var forEach = ObjectHelperFactory.forEach;

        /**
         * Build a dom document
         * @param  {string} elementName       The element Name
         * @param  {Object} elementAttributes An attribute list
         * @return {Object} HTMLElement
         */
        function buildTag(elementName, elementAttributes) {
            var tagElement = document.createElement(elementName);

            forEach(function(value, key) {
                if (key !== 'items') {
                    tagElement.setAttribute(key, value);
                }
            }, elementAttributes);

            return tagElement;
        }

        /**
         * Recurse through an OCD formatted sling document and build a dom tree
         * @param {Object} data an OCD response node
         * @param {Object} currentNode the current document context
         * @return {Object} HTMLElement
         */
        function slingDataToDom(data, currentNode) {
            var index, crxNode;

            for(index in data) {
                if(!currentNode) {
                    currentNode = document.createDocumentFragment();
                }

                crxNode = data[index];
                currentNode = currentNode.appendChild(buildTag(index, crxNode));
                if(crxNode.items && crxNode.items.length > 0) {
                    for (var i = 0; i < crxNode.items.length; i++) {
                        slingDataToDom(crxNode.items[i], currentNode);
                    }
                }
            }

            return currentNode;
        }

        return {
            buildTag: buildTag,
            slingDataToDom: slingDataToDom
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function OcdHelperFactorySingleton(ObjectHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('OcdHelperFactory', OcdHelperFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.OcdHelperFactory
     * @description
     *
     * OCD Response handler functions
     */
    angular.module('origin-components')
        .factory('OcdHelperFactory', OcdHelperFactorySingleton);
}());