/**
 * Scope Helper Methods
 * @file scope.js
 */
(function() {
    'use strict';

    function ScopeHelper($rootScope, ComponentsLogFactory) {
        /**
         * This method hardens the AngularJs rootScope::$digest call, which throws exceptions
         * if called while a digest is already in progress. It also handles cases where the
         * scope is not actively defined. This method uses private angular variables and should
         * be tested when upgrading AngularJs to new versions.
         * @param  {Object} scope the local AngularJsscope
         */
        function digestIfDigestNotInProgress(scope) {
            if(!scope || !_.isFunction(scope.$digest)) {
                return;
            }

            if (!$rootScope.$$phase && !scope.$$phase) {
                try {
                    scope.$digest();
                } catch (err) {
                    ComponentsLogFactory.log(err);
                }
            }
        }

        return {
            digestIfDigestNotInProgress: digestIfDigestNotInProgress
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ScopeHelper

     * @description
     *
     * Scope Helper Methods to digest asynchronously
     */
    angular.module('origin-components')
        .factory('ScopeHelper', ScopeHelper);
}());