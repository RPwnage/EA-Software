/**
 * @file filter/filtersearchfield.js
 */
(function() {
    'use strict';

    function OriginFilterSearchfieldCtrl($scope) {
        /**
         * Clears text in the search field
         * @return {void}
         */
        $scope.dismiss = function () {
            $scope.modelObject[$scope.modelFieldName] = '';
        };

        /**
         * Returns true if the search string is not empty, false otherwise
         * @return {boolean}
         */
        $scope.isActive = function () {
            var searchText = $scope.modelObject[$scope.modelFieldName];

            return (typeof searchText === 'string') && searchText.length > 0;
        };

        $scope.$watch('modelObject[modelFieldName]', function (newValue, oldValue) {
            if (newValue !== oldValue) {
                $scope.onChange();
            }
        });
    }

    function originFilterSearchfield(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginFilterSearchfieldCtrl',
            scope: {
                placeholder: '@',
                modelFieldName: '@',
                modelObject: '=',
                onChange: '&'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('filter/views/filtersearchfield.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originFilterSearchfield
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} placeholder text to display when the search string is empty
     * @param {object} model-object parent scope object that will hold the filter's state.
     * @param {string} model-field-name property of the model object to bind the filter string to.
     *        Note: we must use a property of an object attached to the scope, not a property of the scope itself,
     *        otherwise Angular would not be able to maintain 2-way binding.
     * @param {function} on-change function to call when the filter string has changed
     * @description
     *
     * Input field for filtering by a text string. This directive is a partial not directly merchandised.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-filter-searchfield placeholder="Search by name" model-object="filters" model-field-name="searchByName" on-change="updateList()"></origin-filter-searchfield>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .controller('OriginFilterSearchfieldCtrl', OriginFilterSearchfieldCtrl)
        .directive('originFilterSearchfield', originFilterSearchfield);
}());