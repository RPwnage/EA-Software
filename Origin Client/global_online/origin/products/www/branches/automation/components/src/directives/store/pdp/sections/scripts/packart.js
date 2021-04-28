/**
 * @file store/pdp/sections/packart.js
 */
(function(){
    'use strict';


    function originStorePdpSectionsPackart(ComponentsConfigFactory) {

        function originStorePdpSectionsPackartLink(scope) {
            scope.packArtDefault = ComponentsConfigFactory.getImagePath('packart-placeholder.jpg');
        }

        return {
            restrict: 'E',
            link: originStorePdpSectionsPackartLink,
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
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-sections-packart></origin-store-pdp-sections-packart>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpSectionsPackart', originStorePdpSectionsPackart);
}());
