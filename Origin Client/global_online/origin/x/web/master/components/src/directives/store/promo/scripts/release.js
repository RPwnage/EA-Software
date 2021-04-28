
/** 
 * @file store/promo/scripts/release.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-promo',
        DATE_FORMAT = 'MM/DD/YYYY';

    function originStorePromoRelease(ComponentsConfigFactory, UtilFactory, moment) {
        function originStorePromoReleaseLink(scope) {
            if (scope.startDate || scope.endDate) {
                var sDate = moment(scope.startDate).format(DATE_FORMAT),
                    eDate = moment(scope.endDate).format(DATE_FORMAT);
                scope.aText = UtilFactory.getLocalizedStr(scope.availableText, CONTEXT_NAMESPACE, 'releaseString', {'%startDate%': sDate, '%endDate%': eDate});
            }
        }

        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/release.html'),
            link: originStorePromoReleaseLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoRelease
     * @restrict E
     * @element ANY
     * @description Configures a localized release date. The available-text attribute is optional, but if not given, the current scope must have both a start-date and end-date on it. 
     * If available-text is given then the specified start-date/end-date must be given.
     * For the available-text string, the dates will subbed in the place of %startDate% and %endDate%. EG: "Available %startDate% - %endDate% would yeild "Available 6/10/2015 - 6/15/2015"
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-promo-release></origin-store-promo-release>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePromoRelease', originStorePromoRelease);
}()); 
