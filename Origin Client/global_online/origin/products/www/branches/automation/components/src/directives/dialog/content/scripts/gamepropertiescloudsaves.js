/**
 * @file gamepropertiescloudsaves.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-gamepropertiescloudsaves';

    /**
     * controller function
     * @method OriginDialogContentGamepropertiescloudsavesCtrl
     */
    function OriginDialogContentGamepropertiescloudsavesCtrl($scope, $compile, GamesClientFactory, UtilFactory, EventsHelperFactory, GamePropertiesFactory, ObserverFactory) {
        var handles = [];

        $scope.cloudSavesTitleLoc = UtilFactory.getLocalizedStr($scope.cloudsavestitlestr, CONTEXT_NAMESPACE, 'cloudsavestitlestr');
        $scope.cloudSavesDescLoc = UtilFactory.getLocalizedStr($scope.cloudsavesdescstr, CONTEXT_NAMESPACE, 'cloudsavesdescstr');
        $scope.enableCloudSavesLoc = UtilFactory.getLocalizedStr($scope.enablecloudsavesstr, CONTEXT_NAMESPACE, 'enablecloudsavesstr');
        $scope.restoreLastSaveTitleLoc = UtilFactory.getLocalizedStr($scope.restorelastsavetitlestr, CONTEXT_NAMESPACE, 'restorelastsavetitlestr');
        $scope.restoreLastSaveDescLoc = UtilFactory.getLocalizedStr($scope.restorelastsavedescstr, CONTEXT_NAMESPACE, 'restorelastsavedescstr');
        $scope.restoreLastSaveLoc = UtilFactory.getLocalizedStr($scope.restorelastsavestr, CONTEXT_NAMESPACE, 'restorelastsavestr');        
        $scope.goOnlineLoc = UtilFactory.getLocalizedStr($scope.goonlinestr, CONTEXT_NAMESPACE, 'goonlinestr');
        $scope.offlineWarningLoc = UtilFactory.getLocalizedStr($scope.offlinewarningstr, CONTEXT_NAMESPACE, 'offlinewarningstr');
        $scope.restoreDateLoc = UtilFactory.getLocalizedStr($scope.restoredatestr, CONTEXT_NAMESPACE, 'restoredatestr');

        /**
          * called when save button is pressed. Saves all cloud save settings
          * @method saveGamePropertiesCloudSaves
          */
        function saveGamePropertiesCloudSaves() {
            if (!$scope.cloud.isNOG) {
                GamePropertiesFactory.saveGamePropertiesCloudSaves({
                    cloudSavesEnabled: $scope.cloud.cloudSavesEnabled
                });
            }
        }

        $scope.$on('origin-dialog-content-gameproperties:save', saveGamePropertiesCloudSaves);

        $scope.restoreSave = function() {
            Origin.client.games.restoreLastCloudBackup($scope.offerId);
        };

        ObserverFactory.create(GamePropertiesFactory.getGamePropertiesObservable())
            .attachToScope($scope, 'cloud');

        ObserverFactory.create(GamePropertiesFactory.getClientPropertiesObservable())
            .attachToScope($scope, 'client');
        
        /**
          * called when $scope is destroyed, release event handlers
          * @method onDestroy
          */
        function onDestroy() {
            EventsHelperFactory.detachAll(handles);
        }

        handles[handles.length] = GamesClientFactory.events.on('GamesClientFactory:updateCloudUsage:' + $scope.offerId, GamePropertiesFactory.updateCloudData());

        $scope.$on('$destroy', onDestroy);

    }

    /**
      * directive function
      * @return {Object} directive definition object
      * @method originDialogContentGamepropertiescloudsaves
      */
    function originDialogContentGamepropertiescloudsaves(ComponentsConfigFactory) {

        /**
          * link function
          * @method originDialogContentGamepropertiescloudsavesLink
          */
        function originDialogContentGamepropertiescloudsavesLink(scope, elem) {
            var goOnlineLoc = '<a class="otka" href="javascript:void(0)" onClick="Origin.client.onlineStatus.goOnline()">' + scope.goOnlineLoc + '</a>';
            var offlineWarningLoc = scope.offlineWarningLoc.replace('%go-online-link%', goOnlineLoc);

            elem.find('.origin-gameproperties-cloudsaves-warning-text').append(offlineWarningLoc);
        }

        var directiveDefinitionObject = {
            restrict: 'E',
            scope: {
                cloudsavestitlestr: '@cloudsavestitlestr',
                cloudsavesdescstr: '@cloudsavesdescstr',
                enablecloudsavesstr: '@enablecloudsavesstr',
                offlinewarningstr: '@offlinewarningstr',
                restorelastsavetitlestr: '@restorelastsavetitlestr',
                restorelastsavedescstr: '@restorelastsavedescstr',
                restorelastsavestr: '@restorelastsavestr',
                restoredatestr: '@restoredatestr',
                goonlinestr: '@goonlinestr',
                offerId: '@offerid'
            },
            link: originDialogContentGamepropertiescloudsavesLink,
            controller: 'OriginDialogContentGamepropertiescloudsavesCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/gamepropertiescloudsaves.html')
        };
        return directiveDefinitionObject;
    }



    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentGamepropertiescloudsaves
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} cloudsavestitlestr the title of the cloud saves section
     * @param {LocalizedString} cloudsavesdescstr the text explaining how cloud saves work
     * @param {LocalizedString} enablecloudsavesstr the enable cloud saves string; used next to checkbox
     * @param {LocalizedString} restorelastsavetitlestr the title of the restore section
     * @param {LocalizedString} restorelastsavedescstr the text describing what the restore button does
     * @param {LocalizedString} restorelastsavestr the text on the restore button
     * @param {LocalizedString} goonlinestr the text on the Go Online hyperlink
     * @param {LocalizedString} offlinewarningstr You're offline, %go-online-link% to modify these options.
     * @param {LocalizedString} restoredatestr Will restore save data from: %date%
     * @description
     *
     * Settings section
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-gamepropertiescloudsaves cloudsavestitlestr="Cloud Save" cloudsavesdescstr="Cloud save description" enablecloudsavesstr="Enable cloud saves" restorelastsavetitlestr="Restore Last Save" restorelastsavedescstr="Restore last save description" restorelastsavestr="Restore Now" goonlinestr="Go Online"></origin-dialog-content-gamepropertiescloudsaves>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginDialogContentGamepropertiescloudsavesCtrl', OriginDialogContentGamepropertiescloudsavesCtrl)
        .directive('originDialogContentGamepropertiescloudsaves', originDialogContentGamepropertiescloudsaves);
}());