/**
 * @file gamepropertiesgeneral.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-gamepropertiesgeneral';

    /**
     * controller function
     * @method OriginDialogContentGamepropertiesgeneralCtrl
     */
    function OriginDialogContentGamepropertiesgeneralCtrl($scope, UrlHelper, UtilFactory, NavigationFactory, GamesDataFactory, ObserverFactory, GamePropertiesFactory, moment) {

        $scope.gameManualLoc = UtilFactory.getLocalizedStr($scope.gamemanualstr, CONTEXT_NAMESPACE, 'gamemanualstr');
        $scope.epilepsyWarningLoc = UtilFactory.getLocalizedStr($scope.epilepsywarningstr, CONTEXT_NAMESPACE, 'epilepsywarningstr');
        $scope.cdKeyLoc = UtilFactory.getLocalizedStr($scope.cdkeystr, CONTEXT_NAMESPACE, 'cdkeystr');
        $scope.dateAddedLoc = UtilFactory.getLocalizedStr($scope.dateaddedstr, CONTEXT_NAMESPACE, 'dateaddedstr');
        $scope.commandLineLoc = UtilFactory.getLocalizedStr($scope.commandlinestr, CONTEXT_NAMESPACE, 'commandlinestr');
        $scope.gameNameLoc = UtilFactory.getLocalizedStr($scope.gamenamestr, CONTEXT_NAMESPACE, 'gamenamestr');
        $scope.gameLocationLoc = UtilFactory.getLocalizedStr($scope.gamelocationstr, CONTEXT_NAMESPACE, 'gamelocationstr');

        var enableOIGGameLoc = UtilFactory.getLocalizedStr($scope.enableoigstr, CONTEXT_NAMESPACE, 'enableoigstr');

        /**
         * save all general game properties
         * @method saveGamePropertiesGeneral
         */
        function saveGamePropertiesGeneral() {

            GamePropertiesFactory.saveGamePropertiesGeneral({
                enableOig: $scope.general.enableOig,
                gameTitle: $scope.general.gameTitle,
                commandLineText: $scope.general.commandLineText
            });
        }

        $scope.$on('origin-dialog-content-gameproperties:save', saveGamePropertiesGeneral);

        /**
         * open an external browser to the URL for the epilepsy warning
         * @method openEpilepsyWarning
         */
        $scope.openEpilepsyWarning = function($event) {
            $event.preventDefault();
            NavigationFactory.externalUrl(UrlHelper.getEpilepsyWarningUrl(Origin.locale.locale()));
        };
        
        $scope.getEpilepsyWarningAbsoluteUrl = function(){
            return NavigationFactory.getAbsoluteUrl(UrlHelper.getEpilepsyWarningUrl(Origin.locale.locale()));
        };

        /**
         * open an external browser to the URL for the game manuals
         * @method openEpilepsyWarning
         */
        $scope.openGameManual = function($event) {
            $event.preventDefault();
            NavigationFactory.externalUrl($scope.general.gameManualUrl);
        };

        $scope.getGameManualAbsoluteUrl = function(){
            if ($scope.general && $scope.general.gameManualUrl){
                return NavigationFactory.getAbsoluteUrl($scope.general.gameManualUrl);
            }
        };

        /**
         * format a given date
         * @return {string} formatted date     
         * @method formatDate
         */
        $scope.formatDate = function(d) {
            return moment(d).format('LL');
        };

        $scope.$watch('general.gameTitle', function() {
            if (!$scope.enableOIGGameLoc && !!$scope.general.gameTitle) {
                $scope.enableOIGGameLoc = enableOIGGameLoc.replace('%game%', $scope.general.gameTitle);
            }
        });

        ObserverFactory.create(GamePropertiesFactory.getClientPropertiesObservable())
            .attachToScope($scope, 'client');        

        ObserverFactory.create(GamePropertiesFactory.getGamePropertiesObservable())
            .attachToScope($scope, 'general');
    }

    /**
     * directive function
     * @return {Object} directive definition object     
     * @method originDialogContentGamepropertiesgeneral
     */
    function originDialogContentGamepropertiesgeneral(ComponentsConfigFactory) {

        var directiveDefinitionObj = {
            restrict: 'E',
            scope: {
                gamemanualstr: '@gamemanualstr',
                epilepsywarningstr: '@epilepsywarningstr',
                cdkeystr: '@cdkeystr',
                commandlinestr: '@commandlinestr',
                gamenamestr: '@gamenamestr',
                dateaddedstr: '@dateaddedstr',
                gamelocationstr: '@gamelocationstr',
                offerId: '@offerid'
            },
            controller: 'OriginDialogContentGamepropertiesgeneralCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/gamepropertiesgeneral.html')
        };

        return directiveDefinitionObj;
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentGamepropertiesgeneral
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} gamemanualstr the game manual link text
     * @param {LocalizedString} epilepsywarningstr the epilepsy warning link text
     * @param {LocalizedString} cdkeystr the CD Key label   
     * @param {LocalizedString} commandlinestr Command Line label
     * @param {LocalizedString} gamenamestr Game Name label
     * @param {LocalizedString} enableoigstr the Enable OIG label
     * @param {LocalizedString} gamelocationstr the Enable OIG label
     * @param {LocalizedString} dateaddedstr Added to Library
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-gamepropertiesgeneral gamelocationstr="Game Location" gamenamestr="Name" commandlinestr="Command Line:" enableoigstr="Enable Origin in game for %game%" gamemanualstr="Game Manual" epilepsywarningstr="Epilepsy Warning" cdkeystr="CD Key:"></origin-dialog-content-gamepropertiesgeneral>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginDialogContentGamepropertiesgeneralCtrl', OriginDialogContentGamepropertiesgeneralCtrl)
        .directive('originDialogContentGamepropertiesgeneral', originDialogContentGamepropertiesgeneral);
}());