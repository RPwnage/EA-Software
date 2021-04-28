/**
 * @file store/pdp/sections/offerdetails.js
 */
(function(){
    'use strict';

    function originStorePdpSectionsOfferdetails(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/sections/views/offerdetails.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSectionsOfferdetails
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {OfferId} offer-id the offer id
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-sections-offerdetails offer-id="{{ model.offerId }}"></origin-store-pdp-sections-offerdetails>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpSectionsOfferdetails', originStorePdpSectionsOfferdetails);
}());
