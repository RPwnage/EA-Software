/**
 * @file store/scripts/accordionlist.js
 */
(function(){
    'use strict';

    /* Directive Declaration */
    function originStoreAccordionlist(ComponentsConfigFactory){
        return {
            restrict: 'E',
            scope: {
            },
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/accordionlist.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccordionlist
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Generic accordion list container
     *
     * @example
     * <origin-store-accordionlist>
     *     <origin-store-accordionlist-item topic=""
     *                                      content="">
     *     </origin-store-accordionlist-item>
     * </origin-store-accordionlist>
     */
    angular.module('origin-components')
        .directive('originStoreAccordionlist', originStoreAccordionlist);
}());
