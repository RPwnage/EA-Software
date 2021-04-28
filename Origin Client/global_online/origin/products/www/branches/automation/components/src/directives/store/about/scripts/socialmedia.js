/**
 * @file store/about/scripts/socialmedia.js
 */
(function(){
    'use strict';

    /* Directive Declaration */
    function OriginStoreAboutSocialmedia(ComponentsConfigFactory){
        return {
            restrict: 'E',
            scope:{},
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/socialmedia.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutSocialmedia
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * A module on the About page that contains links to the various Origin Insider social pages
     *
     * @example
     * <origin-store-socialmedia></origin-store-socialmedia>
     */
    angular.module('origin-components')
        .directive('originStoreAboutSocialmedia', OriginStoreAboutSocialmedia);
}());
