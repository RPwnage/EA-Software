/**
 * @file store/pdp/sections/packart.js
 */
(function(){
    'use strict';

    function OriginStorePdpSectionsPackartCtrl($scope, ComponentsConfigFactory, ProductFactory) {
        $scope.model = {};
        $scope.$watch('highlightedEditionId', function (newValue) {
            if (newValue) {
                ProductFactory.get(newValue).attachToScope($scope, 'model');
            }
        });

        $scope.packArtDefault = ComponentsConfigFactory.getImagePath('packart-placeholder.jpg');
    }

    function originStorePdpSectionsPackart(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginStorePdpSectionsPackartCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/sections/views/packart.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSectionsPackart
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
     *         <origin-store-pdp-sections-packart offer-id="{{ model.offerId }}"></origin-store-pdp-sections-packart>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpSectionsPackartCtrl', OriginStorePdpSectionsPackartCtrl)
        .directive('originStorePdpSectionsPackart', originStorePdpSectionsPackart);
}());
