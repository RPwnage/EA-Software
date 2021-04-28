/**
 * file gamelibrary/ogdcontentpanel.js
 */
(function() {

    'use strict';

    function OriginGamelibraryOgdContentPanelCtrl() {

    }

    function originGamelibraryOgdContentPanel(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                title: '@title',
                friendlyName: '@friendlyname'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdcontentpanel.html'),
            controller: OriginGamelibraryOgdContentPanelCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdContentPanel
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title the title of the panel
     * @param {string} friendlyname The friendly name for the deep link 
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryOgdContentPanelCtrl', OriginGamelibraryOgdContentPanelCtrl)
        .directive('originGamelibraryOgdContentPanel', originGamelibraryOgdContentPanel);
})();