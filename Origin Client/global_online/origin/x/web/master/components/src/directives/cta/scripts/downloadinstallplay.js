/**
 * @file cta/downloadinstallplay.js
 */

(function() {
    'use strict';

    var CLS_ICON_DOWNLOAD = 'otkicon-download';
    /**
     * A list of button type options
     * @readonly
     * @enum {string}
     */
    var ButtonTypeEnumeration = {
        "primary": "primary",
        "secondary": "secondary",
        "transparent": "transparent"
    };

    function OriginCtaDownloadinstallplayCtrl($scope, GameCTAFactory, GamesDataFactory) {

        var clickAction = null;

        function handleButtonState(buttonState) {
            if (buttonState) {
                $scope.btnText = buttonState.phaseDisplay ? buttonState.phaseDisplay : buttonState.label;
                clickAction = buttonState.clickAction;
                if (!clickAction) {
                    $scope.disabled = true;
                } else {
                    $scope.disabled = buttonState.disabled;
                }
                $scope.$digest();
            }
        }

        function updateButtonState() {
            var offerid = $scope.offerid;
            if ($scope.mastertitleid && !$scope.offerid) {
                var ent = GamesDataFactory.getBaseGameEntitlementByMasterTitleId($scope.mastertitleid);
                if (ent) {
                    offerid = ent.offerId;
                }
            }

            GameCTAFactory.getButtonState(offerid, $scope.ismoredetailsdisabled)
                .then(handleButtonState);
        }

        function onDestroy() {
            GamesDataFactory.events.off('games:releaseStateUpdate:' + $scope.offerid, updateButtonState);
            GamesDataFactory.events.off('games:update:' + $scope.offerId, updateButtonState);
            GamesDataFactory.events.off('games:downloadqueueupdate:' + $scope.offerid, updateButtonState);
            GamesDataFactory.events.off('games:progressupdate:' + $scope.offerid, updateButtonState);
            GamesDataFactory.events.off('games:initialClientStateReceived', updateButtonState);
        }

        this.init = function() {
            $scope.type = $scope.type || ButtonTypeEnumeration.primary;
            $scope.iconClass = CLS_ICON_DOWNLOAD;
            $scope.disabled = false;

            $scope.onBtnClick = function() {
                if (clickAction) {
                    clickAction($scope.offerid);
                }
            };

            //set the intial button state
            updateButtonState();

            GamesDataFactory.events.on('games:releaseStateUpdate:' + $scope.offerid, updateButtonState);
            GamesDataFactory.events.on('games:update:' + $scope.offerid, updateButtonState);
            GamesDataFactory.events.on('games:downloadqueueupdate:' + $scope.offerid, updateButtonState);
            GamesDataFactory.events.on('games:progressupdate:' + $scope.offerid, updateButtonState);
            GamesDataFactory.events.once('games:initialClientStateReceived', updateButtonState);
        };

        // when the element is destroyed make sure that we cleanup
        $scope.$on('$destroy', onDestroy);
    }

    function originCtaDownloadinstallplay(ComponentsConfigFactory) {

        function originCtaDownloadinstallplayLink(scope, element, attrs, ctrl) {
            ctrl.init();
        }

        return {
            restrict: 'E',
            scope: {
                offerid: '@',
                type: '@',
                mastertitleid: '@href',
                ismoredetailsdisabled: '@'
            },
            controller: 'OriginCtaDownloadinstallplayCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html'),
            link: originCtaDownloadinstallplayLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaDownloadinstallplay
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offerid of the game you want to interact with
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @param {LocalizedString} retrievinggamestate retrieving game state message
     * @param {LocalizedString} downloading downloading state message
     * @param {LocalizedString} installing installing state message
     * @param {LocalizedString} playing playing state message
     * @param {LocalizedString} learnmorecta learn more call to action
     * @param {LocalizedString} addtoqueuecta add to queue call to action
     * @param {LocalizedString} removefromqueuecta remove from queue call to action
     * @param {LocalizedString} downloadcta download call to action
     * @param {LocalizedString} installcta install call to action
     * @param {LocalizedString} preloadcta preload call to action
     * @param {LocalizedString} copyfromdisccta copy from disc/dvd call to action
     * @param {LocalizedString} playcta play (game) call to action
     * @param {LocalizedString} starttrialcta start trial call to action
     * @param {LocalizedString} moredetailscta more details... call to action
     * @param {LocalizedString} getitwithorigincta get it with origin call to action
     * @description
     *
     * Download/Install/Play call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-downloadinstallplay offerid="OFB-EAST:109549060" type="primary"></origin-cta-downloadinstallplay>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaDownloadinstallplayCtrl', OriginCtaDownloadinstallplayCtrl)
        .directive('originCtaDownloadinstallplay', originCtaDownloadinstallplay);
}());
