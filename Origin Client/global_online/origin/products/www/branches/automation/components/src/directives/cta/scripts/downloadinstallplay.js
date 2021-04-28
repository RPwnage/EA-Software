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

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    function OriginCtaDownloadinstallplayCtrl($scope, GameCTAFactory, GamesDataFactory, EventsHelperFactory, ComponentsLogFactory, AuthFactory, NavigationFactory) {

        var clickAction = null,
            eventHandles;

        $scope.type = $scope.type || ButtonTypeEnumeration.primary;
        $scope.iconClass = CLS_ICON_DOWNLOAD;
        $scope.disabled = false;

        function onClientOnlineStateChanged() {
            updateButtonState();
        }

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
            GameCTAFactory.getButtonState($scope.offerid, $scope.ismoredetailsdisabled)
                .then(handleButtonState);
        }

        function onDestroy() {
            EventsHelperFactory.detachAll(eventHandles);
        }

        $scope.onBtnClick = function(evt) {
            // Emit an event that is picked up by OriginHomeDiscoveryTileProgrammableCtrl,
            // in order to record cta-click telemetry for the tile.
            $scope.$emit('cta:clicked');

            if (clickAction) {
                if ($scope.isgift === BooleanEnumeration.true) {
                    NavigationFactory.goToState('app.game_gamelibrary');
                }

                clickAction($scope.offerid);
            }
            evt.stopPropagation();
            evt.preventDefault();
        };

        $scope.onQuickDownloadClick = function(evt) {
            Origin.telemetry.sendMarketingEvent('Game Library', 'Quick Download Game', $scope.offerid);
            $scope.onBtnClick(evt);
        };

        eventHandles = [
            GamesDataFactory.events.on('games:releaseStateUpdate:' + $scope.offerid, updateButtonState),
            GamesDataFactory.events.on('games:update:' + $scope.offerid, updateButtonState),
            GamesDataFactory.events.on('games:downloadqueueupdate:' + $scope.offerid, updateButtonState),
            GamesDataFactory.events.on('games:progressupdate:' + $scope.offerid, updateButtonState),
            GamesDataFactory.events.once('games:initialClientStateReceived', updateButtonState),
            AuthFactory.events.on('myauth:clientonlinestatechanged', onClientOnlineStateChanged)
        ];

        // when the element is destroyed make sure that we cleanup
        $scope.$on('$destroy', onDestroy);

        function handleError(error) {
            ComponentsLogFactory.error('[origin-cta-downloadinstallplay]', error);
        }

        function setOfferId(offerId) {
            $scope.offerid = offerId;
        }

        GamesDataFactory.lookUpOfferIdIfNeeded($scope.ocdPath, $scope.offerid)
            .then(setOfferId)
            .then(updateButtonState)
            .catch(handleError);
    }

    function originCtaDownloadinstallplay(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerid: '@',
                type: '@',
                ocdPath: '@',
                ismoredetailsdisabled: '@',
                isgift: '@'
            },
            controller: 'OriginCtaDownloadinstallplayCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaDownloadinstallplay
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offerid of the game you want to interact with
     * @param {OcdPath} ocd-path - ocd path of the offer to download/install/play (ignored if offer id entered)
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
     * @param {LocalizedString} playonorigincta Play on Origin call to action
     * @param {LocalizedString} detailscta details cta
     *
     * @param {LocalizedString} primarypreordercta primary preorder cta. Set up once as default. E.G., "Pre-order"
     * @param {LocalizedString} primarybuynowcta primary buy now cta text. Set up once as default. E.G., "Buy Now"
     * @param {LocalizedString} primaryrestorecta primary restore cta. Set up once as default. E.G., "Restore"
     * @param {LocalizedString} primaryplaycta primary play cta. Set up once as default. E.G., "Play"
     * @param {LocalizedString} primaryupdatecta primary update cta. Set up once as default. E.G., "Update"
     * @param {LocalizedString} primarydownloadcta primary download cta. Set up once as default. E.G., "Download"
     * @param {LocalizedString} primarydownloadnowcta primary download now cta text. Set up once as default. E.G., "Download Now"
     * @param {LocalizedString} primarypreloadcta primary preload cta. Set up once as default. E.G., "Preload"
     * @param {LocalizedString} primarypreloadnowcta primary preload now cta text. Set up once as default. E.G.,  "Preload Now"
     * @param {LocalizedString} primaryaddtodownloadscta primary add to downloads cta text. Set up once as default. E.G., "Add to Queue"
     * @param {LocalizedString} primarypausedownloadcta primary pause download cta text. Set up once as default. E.G., "Pause Download"
     * @param {LocalizedString} primaryresumedownloadcta primary resume download cta text. Set up once as default. E.G., "Resume Download"
     * @param {LocalizedString} primarypausepreloadcta primary pause preload cta text. Set up once as default. E.G., "Pause Preload"
     * @param {LocalizedString} primaryresumepreloadcta primary resume preload cta text. Set up once as default. E.G., "Resume Preload"
     * @param {LocalizedString} primarypauseupdatecta pause update cta primary text. Set up once as default. E.G., "Pause Update"
     * @param {LocalizedString} primaryresumeupdatecta primary resume update cta text. Set up once as default. E.G., "Resume Update"
     * @param {LocalizedString} primarypauserepaircta primary pause repair cta text. Set up once as default. E.G., "Pause Repair"
     * @param {LocalizedString} primaryresumerepaircta primary resume repair cta text. Set up once as default. E.G., "Resume Repair"
     * @param {LocalizedString} primaryinstallcta primary instlal cta. Set up once as default. E.G., "Install"
     * @param {LocalizedString} primarypauseinstallcta primary pause install cta text. Set up once as default. E.G., "Pause Install"
     * @param {LocalizedString} primaryresumeinstallcta primary resume install cta text. Set up once as default. E.G., "Resume Install"
     * @param {LocalizedString} primaryuninstallcta primary uninstall cta. Set up once as default. E.G., "Uninstall"
     * @param {LocalizedString} viewspecialoffercta view special offer cta text. Set up once as default. E.G., "View Special Offer"
     * @param {LocalizedString} viewinstorecta view in store cta text. Set up once as default. E.G., "View in Store"
     * @param {LocalizedString} incompatibleplatformtitle incompatible platform title. Set up once as default. E.G., "Incompatible Platform"
     * @param {LocalizedString} incompatibleplatformdesc incompatible platform description. Set up once as default. E.g., "Visit your Game Library from your PC to start playing."
     * @param {LocalizedString} continuebrowsing continue browsing text. Set up once as default. E.G., "Continue Browsing"
     *
     * @param {BooleanEnumeration} isgift Whether the offer in question is a gift
     *
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
