/**
 * @file gamepropertiesadvanced.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-gamepropertiesadvanced';

    /**
     * controller function
     * @method OriginDialogContentGamepropertiesadvancedCtrl
     */
    function OriginDialogContentGamepropertiesadvancedCtrl($scope, UtilFactory, ObserverFactory, GamePropertiesFactory) {
        $scope.whenLaunchingLoc = UtilFactory.getLocalizedStr($scope.whenlaunchingstr, CONTEXT_NAMESPACE, 'whenlaunchingstr');
        $scope.commandLineLoc = UtilFactory.getLocalizedStr($scope.commandlinestr, CONTEXT_NAMESPACE, 'commandlinestr');
        $scope.availableLocalesLoc = UtilFactory.getLocalizedStr($scope.availablelocalesstr, CONTEXT_NAMESPACE, 'availablelocalesstr');
        $scope.goOnlineLoc = UtilFactory.getLocalizedStr($scope.goonlinestr, CONTEXT_NAMESPACE, 'goonlinestr');
        $scope.offlineWarningLoc = UtilFactory.getLocalizedStr($scope.offlinewarningstr, CONTEXT_NAMESPACE, 'offlinewarningstr');

        $scope.multiLaunchOptions = [];

        /**
         * save game properties from the advanced tab
         * @method saveGamePropertiesAdvanced
         */
        function saveGamePropertiesAdvanced() {
            if (!$scope.advanced.isNOG) {
                GamePropertiesFactory.saveGamePropertiesAdvanced({
                    commandLineText: $scope.advanced.commandLineText,
                    selectedLaunchOption: $scope.advanced.selectedLaunchOption,
                    selectedLocale: $scope.advanced.selectedLocale
                });
            }
        }

        $scope.$on('origin-dialog-content-gameproperties:save', saveGamePropertiesAdvanced);

        ObserverFactory.create(GamePropertiesFactory.getGamePropertiesObservable())
            .attachToScope($scope, 'advanced');

        ObserverFactory.create(GamePropertiesFactory.getClientPropertiesObservable())
            .attachToScope($scope, 'client');
    }

    /**
     * directive function
     * @return {Object} directive definition object
     * @method originDialogContentGamepropertiesadvanced
     */
    function originDialogContentGamepropertiesadvanced(ComponentsConfigFactory) {

        /**
          * link function
          * @method originDialogContentGamepropertiesadvancedLink
          */
        function originDialogContentGamepropertiesadvancedLink(scope, elem) {
            var goOnlineLoc = '<a class="otka" href="javascript:void(0)" onClick="Origin.client.onlineStatus.goOnline()">' + scope.goOnlineLoc + '</a>';
            var offlineWarningLoc = scope.offlineWarningLoc.replace('%go-online-link%', goOnlineLoc);

            elem.find('.origin-gameproperties-advanced-warning-text').append(offlineWarningLoc);
        }


        var directiveDefinitionObj = {
            restrict: 'E',
            scope: {
                whenlaunchingstr: '@whenlaunchingstr',
                commandlinestr: '@commandlinestr',
                offlinewarningstr: '@offlinewarningstr',
                goonlinestr: '@goonlinestr',
                offerId: '@offerid'
            },
            controller: 'OriginDialogContentGamepropertiesadvancedCtrl',
            link: originDialogContentGamepropertiesadvancedLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/gamepropertiesadvanced.html')
        };

        return directiveDefinitionObj;
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentGamepropertiesadvanced
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} whenlaunchingstr When launching this game text
     * @param {LocalizedString} commandlinestr Command Line Arguments text
     * @param {LocalizedString} availablelocalesstr Game Languages text
     * @param {LocalizedString} goonlinestr Go online string
     * @param {LocalizedString} offlinewarningstr You're offline, %go-online-link% to modify these options.
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-gamepropertiesadvanced availablelocalesstr="Game Languages" whenlaunchingstr="When launching this game" commandlinestr="Command Line Arrrguments"></origin-dialog-content-gamepropertiesadvanced>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginDialogContentGamepropertiesadvancedCtrl', OriginDialogContentGamepropertiesadvancedCtrl)
        .directive('originDialogContentGamepropertiesadvanced', originDialogContentGamepropertiesadvanced);
}());