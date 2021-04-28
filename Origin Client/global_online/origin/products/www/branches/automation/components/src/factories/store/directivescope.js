/**
 *  * @file DirectiveScope.js
 */
(function () {
    'use strict';

    function DirectiveScope(AppCommFactory, ComponentsLogFactory, OcdPathFactory, GamesDataFactory, LocDictionaryFactory, OcdHelperFactory, PdpUrlFactory, UtilFactory, ScopeHelper) {
        var consumedTimestamp;

        function generateTimestamp() {
            return new Date().getTime();
        }

        function resetConsumedTimestampOnHashChanges() {

            consumedTimestamp = generateTimestamp();

            AppCommFactory.events.on('uiRouter:stateChangeSuccess', function () {
                consumedTimestamp = generateTimestamp();
            });
        }

        function getComponentPathWithDepth(componentPath) {
            var componentPathArray = _.isArray(componentPath) ? componentPath : [componentPath];
            return {
                componentPath: componentPathArray,
                depth: componentPathArray.length
            };
        }

        /**
         * Populates scope with any defaults found in dictionary.
         * @param scope
         * @param componentPath
         * @param anyDataFound
         * @returns {boolean}
         */
        function populateWithDefaults(scope, componentPath, anyDataFound) {
            var explodedPath = getComponentPathWithDepth(componentPath),
                defaults = LocDictionaryFactory.getDefaults(explodedPath.componentPath[explodedPath.depth - 1]);

            var foundDefaultData = false;
            if (defaults) {
                var keys = Object.keys(defaults);
                var bindToScope = function (key) {

                    var convertedKey = UtilFactory.htmlToScopeAttribute(key);
                    if (!scope[convertedKey]) {
                        scope[convertedKey] = defaults[key];
                        foundDefaultData = true;
                    }
                };
                _.forEach(keys, bindToScope);
            }
            return Promise.resolve(anyDataFound || foundDefaultData);
        }

        /**
         * Find a node with a given rootPath.
         * @param rootPath
         * @param ocdResponse
         * @returns {Function}
         */
        function findNode(rootPath, ocdResponse) {
            //TODO: recursively call findDirective (returns promise) to reach depth level 2.
            // Refer to populateScopeNestedOcd for more info
            return function() {
                if (rootPath) {
                    return OcdHelperFactory.findDirectives(rootPath)(ocdResponse);
                } else {
                    return ocdResponse;
                }
            };
        }

        /**
         * Similar to populateScope but key difference is there will be multiple directives of same kind
         * Note: Max depth is 3.
         *
         * <parentDirectiveName>
         *     <childDirectiveName></childDirectiveName>
         *     <childDirectiveName></childDirectiveName>
         *     <childDirectiveName>
         *         <grandChildDirectiveName>
         *             this is the max depth this function can handle.
         *         </grandChildDirectiveName>
         *     </childDirectiveName>
         * </parentDirectiveName>
         *
         * @param scope directive scope
         * @param {Object} explodedPath path to find directive
         * @param ocdResponse
         * @returns {*}
         */
        function populateScopeNestedOcd(scope, explodedPath, ocdResponse) {

            var childDirectiveName = explodedPath.componentPath[explodedPath.depth - 1],
                parentDirectiveName = explodedPath.componentPath[explodedPath.depth - 2],
                rootPath,
                rootPathArray;
                if (explodedPath.depth > 2) {
                    rootPath = explodedPath.componentPath[0];
                    rootPathArray = [0, rootPath, 'items']; //xpath to children of the root.
                }

            return Promise.resolve(ocdResponse)
                .then(findNode(rootPath, ocdResponse)) //if depth is > 2, have to traverse to depth 2 before calling coalesceChildDirectives
                .then(OcdHelperFactory.coalesceChildDirectives(parentDirectiveName, childDirectiveName, rootPathArray))
                .then(OcdHelperFactory.flattenDirectives(childDirectiveName))
                .then(_.curry(populateWithOcd)(scope))
                .then(_.curry(populateWithDefaults)(scope, childDirectiveName))
                .catch(handlerError);
        }

        /**
         * populates scope of directive with ocd attributes
         * @param scope directive scope
         * @param componentPath componentPath in ocd response
         * @param ocdResponse response from ocd
         * @returns {*}
         */
        function populateScopeWithOcd(scope, componentPath, ocdResponse) {

            var explodedPath = getComponentPathWithDepth(componentPath);

            if (explodedPath.depth > 1) { //must have at least a parent and child. optionally have subpath
                return populateScopeNestedOcd(scope, explodedPath, ocdResponse);
            }

            return Promise.resolve(ocdResponse)
                .then(OcdHelperFactory.findDirectives(explodedPath.componentPath[0]))
                .then(OcdHelperFactory.flattenDirectives(explodedPath.componentPath[0]))
                .then(_.curry(populateWithOcd)(scope))
                .then(_.curry(populateWithDefaults)(scope, componentPath))
                .catch(handlerError);
        }

        /**
         * Processes ocd objects. converts kabab variables to camelcase and attaches them to scope.
         * Also marks them as consumed (by timestamp) (in case there are multiple of the same within the same node)
         * @param scope
         * @param models
         * @returns {boolean}
         */
        function populateWithOcd(scope, models) {
            var anyDataFound = false;
            if (models && _.isArray(models)) {
                var notConsumedModel = _.find(models, function (model) {
                    return !model.isConsumed || model.isConsumed !== consumedTimestamp;
                });

                if (notConsumedModel) {
                    var keys = Object.keys(notConsumedModel);

                    var bindToScope = function (key) {
                        var convertedKey = UtilFactory.htmlToScopeAttribute(key);
                        if (!scope[convertedKey]) {
                            scope[convertedKey] = notConsumedModel[key];
                        }
                    };

                    _.forEach(keys, bindToScope);
                    anyDataFound = keys.length > 0;
                    notConsumedModel.isConsumed = consumedTimestamp;
                }
            }

            return anyDataFound;
        }

        function handlerError() {
            ComponentsLogFactory.error('Cannot resolve url to ocdPath');
        }


        /**
         * Gets ocd response from ocd path.
         * @returns {Object} ocdResponse
         */
        function getOcdResponseFromUrl() {
            var edition = PdpUrlFactory.getCurrentEditionSelector();

            if (edition.offerId) {
                return GamesDataFactory.getOcdByOfferId(edition.offerId);
            } else if (edition.ocdPath) {
                return GamesDataFactory.getOcdByPath(edition.ocdPath);
            } else {
                return Promise.reject('Cannot resolve path');
            }
        }

        /**
         * Gets ocd response from ocd path.
         * @param ocdPathOrOfferId ocdPath or OfferId
         * @returns {Object} ocdResponse
         */
        function getOcdResponse(ocdPathOrOfferId) {
            if (ocdPathOrOfferId) {
                if (ocdPathOrOfferId.split('/').length > 0) {
                    return GamesDataFactory.getOcdByPath(ocdPathOrOfferId);
                } else {
                    return GamesDataFactory.getOcdByOfferId(ocdPathOrOfferId);
                }
            } else {
                return getOcdResponseFromUrl();
            }
        }

        /**
         * Populates scope of the component with attributes in OCD based on provided componentPath (if ocdPath is provided) as well as with all values in defaults.
         * If no ocdPath is specified, DirectiveScope will try to construct an ocdPath from URL (for PDP, Announcement & Retire pages).
         * If no ocdPath can be constructed, scope will be populated with defaults only.
         *
         * @param {Object} scope angular scope
         * @param {Array|string} componentPath path in OCD to find this component. For nested components pass in an array [parentDirectiveName, childDirectiveName, subpath] otherwise directiveName
         * @param {String} [ocdPath] path to retrieve the OCD response from. If no ocdPath is specified, DirectiveScope will try to construct an ocdPath from URL
         * @returns {Promise}
         */
        function populateScope(scope, componentPath, ocdPath) {
            return getOcdResponse(ocdPath)
                .then(_.curry(populateScopeWithOcd)(scope, componentPath))
                .catch(_.curry(populateWithDefaults)(scope, componentPath));
        }

        /**
         * Populates the scope with data from ocd then calls an optional callback.
         * Assumes ocdPath has resolved to a valid value (either a valid ocd path or empty string because it was not set on template).
         * @param {Object} scope angular scope
         * @param {Array|string} componentPath path in OCD to find this component for nested components [parentDirectiveName, childDirectiveName, subpath] otherwise directiveName
         * @param {String} ocdPath path to retrieve the OCD response from
         * @param {Function|string} bindTo function/modelName to bind the observable to
         */
        function populateScopeWithOcdAndCatalog(scope, componentPath, ocdPath, bindTo) {
            bindTo = bindTo || 'model';

            if (_.isString(bindTo)) {
                scope[bindTo] = scope[bindTo] || {};
            }
            OcdPathFactory.get(ocdPath).attachToScope(scope, bindTo);
            return populateScope(scope, componentPath, ocdPath);
        }

        /**
         * Simple directive link function that populates scope variables from ocd
         * @param componentPath
         * @returns {Function}
         */
        function directiveLink(componentPath) {
            return function (scope) {
                populateScope(scope, componentPath).then(function () {
                    ScopeHelper.digestIfDigestNotInProgress(scope);
                });
            };
        }

        resetConsumedTimestampOnHashChanges();

        return {
            populateScope: populateScope,
            populateScopeWithOcdAndCatalog: populateScopeWithOcdAndCatalog,
            populateScopeLinkFn: directiveLink
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function DirectiveScopeSingleton(AppCommFactory, ComponentsLogFactory, OcdPathFactory, GamesDataFactory, LocDictionaryFactory, OcdHelperFactory, PdpUrlFactory, UtilFactory, ScopeHelper, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('DirectiveScope', DirectiveScope, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.DirectiveScope
     * @description
     *
     * DirectiveScope
     */
    angular.module('origin-components')
        .factory('DirectiveScope', DirectiveScopeSingleton);
}());
