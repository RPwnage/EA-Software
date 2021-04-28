/**
 * @file common/ocd-helper.js
 */
(function(){

    'use strict';

    var DEFAULT_TARGETING_WRAPPER_CLASS = 'origin-cms-targeting-wrapper';

    function OcdHelperFactory(ObjectHelperFactory, CQTargetingFactory, UtilFactory) {
        var getProperty = ObjectHelperFactory.getProperty,
            filterBy = ObjectHelperFactory.filterBy,
            buildTag = UtilFactory.buildTag;

        /**
         * Recurse through an OCD formatted sling document and build a dom tree
         * @param {Object} data an OCD response node
         * @param {Object} currentNode the current document context
         * @return {Object} HTMLElement
         */
        function slingDataToDom(data, currentNode) {
            var items;

            if(!currentNode) {
                currentNode = document.createDocumentFragment();
            }

            _.forEach(data, function(crxNode, key) {
                currentNode = currentNode.appendChild(buildTag(key, _.omit(crxNode, 'items')));
                items = _.get(crxNode, 'items');
                if(_.isArray(items)) {
                    _.forEach(items, function(item) {
                        slingDataToDom(item, currentNode);
                    });
                }
            });

            return currentNode;
        }

        /**
         * returns true if the ocd directive object is not targeted or meets the targeting requirements
         * @param  {object}  value ocdDirectiveObj
         * @return {Boolean}       returns true if is not targeted or meets the targeting requirements
         */
        function hasTargetingMatch(ocdDirectiveObj) {
            var hasCQTargetingItem = ObjectHelperFactory.getProperty('origin-cq-targeting-item')(ocdDirectiveObj);
            return !angular.isDefined(hasCQTargetingItem) || CQTargetingFactory.targetingItemMatches(ocdDirectiveObj);
        }

        /**
         * Given an OCD resource, find matching directives by directive name
         * @param {string} directiveName the directive name to match
         * @param {Array} subPath the optional subpath to filter in case find is being executed on a child directive
         * @return {Function}
         */
        function findDirectives(directiveName, subPath) {
            /**
             * Return a lambda to eventually process the collection
             * @param {Object[]} data the OCD collection to process
             * @return {Object[]} the filtered collection
             */
            return function (data) {
                var path = ['gamehub','components','items'],
                    components;

                if (_.isArray(subPath)) {
                    path = subPath;
                }

                components = getProperty(path)(data);

                return filterBy(function(component) {
                    if(!_.isObject(component)) {
                        return false;
                    }

                    return component.hasOwnProperty(directiveName) && hasTargetingMatch(component[directiveName]);
                })(components);
            };
        }

        /**
         * Given a parent and child directive, create a new collection of all children
         * related to the parent preserving the order they were merchandised
         * @param {string} parentDirective the parent directive name eg. origin-game-tile
         * @param {string} childDirective  the immediate child directive eg. origin-game-violator-programmed
         * @param {Array} subPath the optional subpath to filter in case find is being executed on a child directive
         * @return {Function}
         */
        function coalesceChildDirectives(parentDirective, childDirective, subPath) {
            /**
             * Return a lambda to eventually process the collection
             * @param {Object[]} data the OCD collection to process
             * @return {Object[]} the filtered collection
             */
            return function (data) {
                var parentDirectives = findDirectives(parentDirective, subPath)(data),
                    childDirectives = [];

                for(var i = 0; i < parentDirectives.length; i++) {
                    childDirectives = childDirectives.concat(findDirectives(childDirective, [i, parentDirective, 'items'])(parentDirectives));
                }

                return childDirectives;
            };
        }

        /**
         * Flatten directive collections into an array excluding the directive name
         * @param {string} directiveName the directive name to match
         * @return {Function}
         */
        function flattenDirectives(directiveName) {
            /**
             * Map the directive returning the list of attributes
             * @param  {Object} item the item to analyze
             * @return {Object} the attribute list
             */
            function mapDirectives(item) {
                if(item && item.hasOwnProperty(directiveName)) {
                    return item[directiveName];
                }
            }

            /**
             * Filter out records that are invalid for attribute collections
             * @param  {mixed} item the item to analyze
             * @return {Boolean} true if valid
             */
            function isValid(item) {
                if(_.isObject(item)) {
                    return true;
                }

                return false;
            }

            return function(data) {
                if(_.isArray(data)) {
                    return data.map(mapDirectives).filter(isValid);
                }

                return [];
            };
        }

        /**
         * Wrap a tag or document fragment that is to be $compiled by Angular to ensure targeting can bind to a valid parent
         * Essentially it pads your directive(s) like this "<div class="origin-cms-targeting-wrapper"> + your Tag(s) here + </div>"
         * @param {Object} documentFragment a valid document fragment or single tag HTMLNode
         * @param {String} wrapperClass an optional wrapper class
         * @return {Object} HTMLNode
         */
        function wrapDocumentForCompile(documentFragment, wrapperClass) {
            var doc = document.createDocumentFragment(),
                targetingContainer;

            try {
                targetingContainer = doc.appendChild(buildTag('div', {
                    class: wrapperClass || DEFAULT_TARGETING_WRAPPER_CLASS
                }));

                targetingContainer.appendChild(documentFragment);

                return targetingContainer;
            } catch(err) {
                return doc;
            }
        }

        return {
            buildTag: buildTag,
            slingDataToDom: slingDataToDom,
            findDirectives: findDirectives,
            coalesceChildDirectives: coalesceChildDirectives,
            flattenDirectives: flattenDirectives,
            wrapDocumentForCompile: wrapDocumentForCompile
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.OcdHelperFactory
     * @description
     *
     * OCD Response handler functions
     */
    angular.module('origin-components')
        .factory('OcdHelperFactory', OcdHelperFactory);
}());