/**
 * @file gameproperties.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-gameproperties';

    function OriginDialogContentGamepropertiesCtrl($scope, GamesDataFactory, UtilFactory, DialogFactory, AuthFactory) {

        this.init = function() {
            $scope.generalstr = UtilFactory.getLocalizedStr($scope.generalstr, CONTEXT_NAMESPACE, 'tab-general');
            $scope.cloudsavesstr = UtilFactory.getLocalizedStr($scope.cloudsavesstr, CONTEXT_NAMESPACE, 'tab-cloudsaves');
            $scope.advancedstr = UtilFactory.getLocalizedStr($scope.advancedstr, CONTEXT_NAMESPACE, 'tab-advanced');
            $scope.savestr = UtilFactory.getLocalizedStr($scope.savestr, CONTEXT_NAMESPACE, 'save');
            $scope.cancelstr = UtilFactory.getLocalizedStr($scope.cancelstr, CONTEXT_NAMESPACE, 'cancel');

            $scope.save = function() {
                // TODO: Save changes from all tabs
                if ($scope.cloud) {
                    Origin.client.settings.writeSetting("cloud_saves_enabled", $scope.cloud.enabled);
                }
                DialogFactory.close('origin-dialog-content-gameproperties-id');
            };

            $scope.cancel = function() {
                DialogFactory.close('origin-dialog-content-gameproperties-id');
            };

            GamesDataFactory.getCatalogInfo([$scope.offerId]).then(function(catalogInfo) {
                $scope.titlestr = UtilFactory.getLocalizedStr($scope.titlestr, CONTEXT_NAMESPACE, 'title', { '%game%': catalogInfo[$scope.offerId].i18n.displayName });
                $scope.$digest();
            });

            var game = GamesDataFactory.getClientGame($scope.offerId);
            $scope.cloudSavesSupported = (typeof(game.cloudSaves) !== 'undefined');

            function onClientOnlineStateChanged(onlineObj) {
                $scope.offline = (onlineObj && !onlineObj.isOnline);
                $scope.$digest();
            }
            $scope.offline = !AuthFactory.isClientOnline();

            function onDestroy() {
                AuthFactory.events.off('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
            }

            AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
            $scope.$on('$destroy', onDestroy);
        };

        this.isSet = function(checkTab) {
            return this.tab === checkTab;
        };

        this.setTab = function(setTab) {
            this.tab = setTab;

            var pill = $('.origin-dialog-content-gameproperties-nav li')[this.tab];
            if (pill) {
                $('.origin-dialog-content-gameproperties-nav .otknav-pills-bar').css({ 'width': $(pill).width() + 'px', 'transform': 'translate3d(' + $(pill).position().left + 'px, 0px, 0px)' });
            }
        };
    }

    function originDialogContentGameproperties(ComponentsConfigFactory, $timeout) {

        function originDialogContentGamepropertiesLink(scope, element, attrs, ctrl) {
            ctrl.init();

            angular.element(document).ready(function() {
                $timeout(function() {
                    ctrl.setTab(0);
                }, 0);
            });
        }

        return {
            restrict: 'E',
            scope: {
                titlestr: '@titlestr',
                savestr: '@savestr',
                cancelstr: '@cancelstr',
                offerId: '@offerid'
            },
            controller: 'OriginDialogContentGamepropertiesCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/gameproperties.html'),
            link: originDialogContentGamepropertiesLink,
            controllerAs: 'tab'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentGameproperties
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} titlestr the title string
     * @param {string} savestr the save str
     * @param {string} cancelstr the cancel str
     * @param {string} offerid the offer id
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
        .controller('OriginDialogContentGamepropertiesCtrl', OriginDialogContentGamepropertiesCtrl)
        .directive('originDialogContentGameproperties', originDialogContentGameproperties);
}());