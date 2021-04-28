/**
 * @file gameproperties.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-gameproperties';

    /**
     * controller function
     * @method OriginDialogContentGamepropertiesCtrl
     */
    function OriginDialogContentGamepropertiesCtrl($scope, UtilFactory, GamePropertiesFactory, ObserverFactory) {

        $scope.generalLoc = UtilFactory.getLocalizedStr($scope.generalstr, CONTEXT_NAMESPACE, 'generalstr');
        $scope.cloudSavesLoc = UtilFactory.getLocalizedStr($scope.cloudsavesstr, CONTEXT_NAMESPACE, 'cloudsavesstr');
        $scope.advancedLoc = UtilFactory.getLocalizedStr($scope.advancedstr, CONTEXT_NAMESPACE, 'advancedstr');
        $scope.saveLoc = UtilFactory.getLocalizedStr($scope.savestr, CONTEXT_NAMESPACE, 'savestr');
        $scope.cancelLoc = UtilFactory.getLocalizedStr($scope.cancelstr, CONTEXT_NAMESPACE, 'cancelstr');
        
        var titleLoc = UtilFactory.getLocalizedStr($scope.titlestr, CONTEXT_NAMESPACE, 'titlestr');

        $scope.save = function() {
            $scope.$broadcast('origin-dialog-content-gameproperties:save');
            GamePropertiesFactory.closeDialog();
        };

        $scope.cancel = function() {
            GamePropertiesFactory.closeDialog();
        };


        $scope.$watch('gameProperties.gameTitle', function() {
            if (!$scope.dlgTitle && !!$scope.gameProperties.gameTitle) {
                $scope.dlgTitle = titleLoc.replace('%game%', $scope.gameProperties.gameTitle);
                //self.setTab(0);
            }
        });

        ObserverFactory.create(GamePropertiesFactory.getGamePropertiesObservable())
            .attachToScope($scope, 'gameProperties');

        this.isSet = function(checkTab) {
            return this.tab === checkTab;
        };

        this.setTab = function(setTab) {
            this.tab = setTab;

            var pill = $('.origin-gameproperties-nav li')[this.tab];
            if (pill) {
                $('.origin-gameproperties-nav .otknav-pills-bar').css({ 'width': $(pill).width() + 'px', 'transform': 'translate3d(' + $(pill).position().left + 'px, 0px, 0px)' });
            }
        };

        this.setTab(0);
    }

    /**
     * directive function
     * @return {Object} directive definition object
     * @method originDialogContentGameproperties
     */
    function originDialogContentGameproperties(ComponentsConfigFactory) {

        var directiveDefinitionObject = {
            restrict: 'E',
            scope: {
                titlestr: '@titlestr',
                savestr: '@savestr',
                cancelstr: '@cancelstr',
                generalstr: '@generalstr',
                cloudsavesstr: '@cloudsavesstr',
                advancedstr: '@advancedstr',
                offerId: '@offerid'
            },
            controller: 'OriginDialogContentGamepropertiesCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/gameproperties.html'),
            controllerAs: 'tab'
        };

        return directiveDefinitionObject;
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentGameproperties
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} titlestr the title string, i.e. "%game% Properties"
     * @param {LocalizedString} savestr the save button string, i.e. "Save"
     * @param {LocalizedString} cancelstr the cancel button string, i.e. "Cancel"
     * @param {LocalizedString} generalstr the general tab title string, i.e. "General"
     * @param {LocalizedString} cloudsavesstr the cloud save tab title string, i.e. "Cloud Saves"
     * @param {LocalizedString} advancedstr the advanced tab title string, i.e. "Advanced Launch Options"
     * @param {string} offerid the offer id
     * @description
     *
     * Settings section
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-gameproperties offerid="%OFFERID%"></origin-dialog-content-gameproperties>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginDialogContentGamepropertiesCtrl', OriginDialogContentGamepropertiesCtrl)
        .directive('originDialogContentGameproperties', originDialogContentGameproperties);
}());