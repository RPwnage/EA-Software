/**
 * @file gamelibrary/ogdtabbaritem.js
 */
(function(){

    'use strict';

    /**
     * BooleanEnumeration
     * @enum {string}
     */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    function OriginGamelibraryOgdTabBarItemCtrl($scope) {
        $scope.getActiveClass = function() {
            return ($scope.active === BooleanEnumeration.true) ? 'otkpill-active' : '';
        };
    }

    function originGamelibraryOgdTabBarItem(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                title: '@title',
                friendlyname: '@friendlyname',
                offerId: '@offerid',
                active: '@active'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdtabbaritem.html'),
            controller: OriginGamelibraryOgdTabBarItemCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdTabBarItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title the tab title
     * @param {string} friendlyname the friendly permalink
     * @param {OfferId} offerid the offer id
     * @param {BooleanEnumeration} active Sets the tab to the active state
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryOgdTabBarItemCtrl', OriginGamelibraryOgdTabBarItemCtrl)
        .directive('originGamelibraryOgdTabBarItem', originGamelibraryOgdTabBarItem);
})();