/**
 * @file store/pdp/sections/description.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-hero';

    function OriginStorePdpSectionsDescriptionCtrl($scope, $state, ProductFactory, UtilFactory) {
        $scope.strings = {
            readMoreText: UtilFactory.getLocalizedStr($scope.readMoreText, CONTEXT_NAMESPACE, 'readmore')
        };

        // @todo: replace with the tab navigation event when PDP nav is ready
        $scope.goToInfo = function () {
            $state.go('app.store.wrapper.pdp.info', {
                offerid: $scope.highlightedEditionId
            });
        };

        $scope.model = {};
        $scope.$watch('highlightedEditionId', function (newValue) {
            if (newValue) {
                ProductFactory.get(newValue).attachToScope($scope, 'model');
            }
        });
    }

    function originStorePdpSectionsDescription(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginStorePdpSectionsDescriptionCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/sections/views/description.html')

        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSectionsDescription
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {string=}
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-sections-description offer-id="{{ model.offerId }}"></origin-store-pdp-sections-description>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpSectionsDescriptionCtrl', OriginStorePdpSectionsDescriptionCtrl)
        .directive('originStorePdpSectionsDescription', originStorePdpSectionsDescription);
}());
