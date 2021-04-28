/**
 * @file store/pdp/sections/scripts/main.js
 */
(function(){
    'use strict';

    function OriginStorePdpSectionsMainCtrl($scope, ProductFactory, ObjectHelperFactory, PdpUrlFactory) {
        var takeHead = ObjectHelperFactory.takeHead,
            allEditionsSelector = PdpUrlFactory.getCurrentGameSelector(),
            currentEditionSelector = PdpUrlFactory.getCurrentEditionSelector();

        function setCurrentOfferId(offerId) {
            $scope.selectedEditionId = offerId;
            $scope.highlightedEditionId = offerId;
        }

        $scope.setHighlightedEdition = function (offerId) {
            $scope.highlightedEditionId = offerId;
        };

        $scope.goToPdp = PdpUrlFactory.goToPdp;

        ProductFactory
            .findOfferIds(currentEditionSelector)
            .then(takeHead)
            .then(setCurrentOfferId);

        ProductFactory
            .filterBy(allEditionsSelector)
            .attachToScope($scope, 'editions');
    }

    function originStorePdpSectionsMain(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginStorePdpSectionsMainCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/sections/views/main.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSectionsMain
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
     *     <origin-store-pdp-sections-main />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpSectionsMainCtrl', OriginStorePdpSectionsMainCtrl)
        .directive('originStorePdpSectionsMain', originStorePdpSectionsMain);
}());
