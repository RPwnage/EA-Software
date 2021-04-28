/**
 * @file gamepropertiescloudsaves.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-gamepropertiescloudsaves';

    function OriginDialogContentGamepropertiescloudsavesCtrl($scope, GamesDataFactory, UtilFactory) {

        this.init = function() {
            $scope.cloud = {};
            $scope.cloud.cloudsavestitlestr = UtilFactory.getLocalizedStr($scope.cloud.cloudsavestitlestr, CONTEXT_NAMESPACE, 'cloud-saves-title');
            $scope.cloud.cloudsavesdescstr = UtilFactory.getLocalizedStr($scope.cloud.cloudsavesdescstr, CONTEXT_NAMESPACE, 'cloud-saves-desc');
            $scope.cloud.enablecloudsavesstr = UtilFactory.getLocalizedStr($scope.cloud.enablecloudsavesstr, CONTEXT_NAMESPACE, 'enable-cloud-saves');
            $scope.cloud.offlinehintstr = UtilFactory.getLocalizedStr($scope.cloud.offlinehintstr, CONTEXT_NAMESPACE, 'offline-hint');
            $scope.cloud.restorelastsavetitlestr = UtilFactory.getLocalizedStr($scope.cloud.restorelastsavetitlestr, CONTEXT_NAMESPACE, 'restore-last-save-title');
            $scope.cloud.restorelastsavedescstr = UtilFactory.getLocalizedStr($scope.cloud.restorelastsavedescstr, CONTEXT_NAMESPACE, 'restore-last-save-desc');
            $scope.cloud.restorelastsavestr = UtilFactory.getLocalizedStr($scope.cloud.restorelastsavestr, CONTEXT_NAMESPACE, 'restore-last-save');

            function updateCloudScopeVars() {
                var game = GamesDataFactory.getClientGame($scope.offerId),
                    cloudSavesUpdateNeeded = game && game.cloudSaves;
                if (cloudSavesUpdateNeeded) {
                        $scope.cloud.usagePercent = (game.cloudSaves.currentUsage / game.cloudSaves.maximumUsage) * 100 + '%';
                        $scope.cloud.usageDisplay = game.cloudSaves.usageDisplay;
                        $scope.cloud.lastBackupValid = game.cloudSaves.lastBackupValid;
                }
                return cloudSavesUpdateNeeded;
            }

            function onClientGameUpdated() {
                if(updateCloudScopeVars()) {
                    $scope.$digest();
                }
            }

            function onDestroy() {
                GamesDataFactory.events.off('games:update:' + $scope.offerId, onClientGameUpdated);
            }

            GamesDataFactory.events.on('games:update:' + $scope.offerId, onClientGameUpdated);
            $scope.$on('$destroy', onDestroy);

            Origin.client.settings.readSetting("cloud_saves_enabled").then(function(val) {
                $scope.cloud.enabled = val;
                $scope.$digest();
            });

            $scope.cloud.restoreSave = function() {
                Origin.client.games.restoreLastCloudBackup($scope.offerId);
            };

            Origin.client.games.pollCurrentCloudUsage($scope.offerId);
            updateCloudScopeVars();
        };
    }

    function originDialogContentGamepropertiescloudsaves(ComponentsConfigFactory) {

        function originDialogContentGamepropertiescloudsavesLink(scope, element, attrs, ctrl) {
            ctrl.init();
        }

        // Don't declare a scope here--we're re-using the parent directive's scope
        return {
            restrict: 'E',
            controller: 'OriginDialogContentGamepropertiescloudsavesCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/gamepropertiescloudsaves.html'),
            link: originDialogContentGamepropertiescloudsavesLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentGamepropertiescloudsaves
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} cloudsavestitlestr the title of the cloud saves section
     * @param {string} cloudsavesdescstr the text explaining how cloud saves work
     * @param {string} enablecloudsavesstr the enable cloud saves string; used next to checkbox
     * @param {string} offlinehintstr explains to the user why cloud save settings can't be modified offline
     * @param {string} restorelastsavetitlestr the title of the restore section
     * @param {string} restorelastsavedescstr the text describing what the restore button does
     * @param {string} restorelastsavestr the text on the restore button
     * @description
     *
     * Settings section
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-footer titlestr="Looking for more?" descriptionstr="The more you play games and the more you add friends, the better your recommendations will become."></origin-home-footer>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginDialogContentGamepropertiescloudsavesCtrl', OriginDialogContentGamepropertiescloudsavesCtrl)
        .directive('originDialogContentGamepropertiescloudsaves', originDialogContentGamepropertiescloudsaves);
}());