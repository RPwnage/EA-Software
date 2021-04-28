/**
 * @file /policyitem.js
 */
(function () {
    'use strict';

    function originPolicyitem(ComponentsConfigFactory) {
        function originPolicyitemLink(scope, $element, attrs, ctrl){
            /** @type policyItem */
            var policy = {
                date: scope.date,
                id: scope.id,
                title: scope.title
            };

            ctrl.registerPolicyItem(policy);

            function getSelectedPolicy() {
                return ctrl.getSelectedPolicy();
            }

            scope.$watch(getSelectedPolicy, function(selectedPolicy){
                scope.selectedPolicy = selectedPolicy;
            });
        }

        return {
            restrict: 'E',
            scope: {
                date: '@',
                title: '@',
                id: '@',
                content: '@'
            },
            require: '^originPolicy',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/views/policyitem.html'),
            link: originPolicyitemLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originPolicyitem
     * @restrict E
     * @element ANY
     * @scope
     * @param {DateTime} date - date the policy was drafted (UTC time)
     * @param {LocalizedString} title Title of the policy
     * @param {String} id The id that identifies this policy
     * @param {LocalizedText} content - policy content
     *
     * @description A policy item directive. Requires parent directive origin-policy.
     *
     * @example
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *        <origin-policy>
     *          <origin-policyitem date="2016-02-01"
     *                             content="test"
     *                             title="Fifa Fanatics"
     *                             id="fifafanatics-2016-02-01"></origin-policyitem>
     *        </origin-policy>
     *     </file>
     * </example>
     */

    angular.module('origin-components')
        .directive('originPolicyitem', originPolicyitem);
}());
