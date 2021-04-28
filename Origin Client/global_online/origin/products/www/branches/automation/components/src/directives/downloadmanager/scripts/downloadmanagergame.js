/**
 * @file downloadmanager/downloadmanagergame.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-download-manager-game',
        ACTION_INSTALL = 'Install',
        ACTION_UPDATE = 'Update',
        ACTION_REPAIR = 'Repair',
        MAX_PERCENT = 100,
        FLAG_DEFAULT_OFFSET = 15,
        MIN_FLAG_LEFT = 3,
        MAX_FLAG_LEFT = 72;

    /**
    * The Controller
    */
   function originDownloadManagerGameCtrl($scope, moment, GamesDataFactory, OcdHelperFactory, ObjectHelperFactory, DownloadManagerFactory, UtilFactory, ComponentsLogFactory, AuthFactory, EventsHelperFactory) {

        var observableParticipant,
            handles,
            downloadActions = {
            'pause': Origin.client.games.pauseDownload,
            'resume': Origin.client.games.resumeDownload,
            'cancel': Origin.client.games.cancelDownload,
            'pauseInstall': Origin.client.games.pauseInstall,
            'resumeInstall': Origin.client.games.resumeInstall,
            'cancelInstall': Origin.client.games.cancelInstall,
            'pauseUpdate': Origin.client.games.pauseUpdate,
            'resumeUpdate': Origin.client.games.resumeUpdate,
            'cancelUpdate': Origin.client.games.cancelUpdate,
            'pauseRepair': Origin.client.games.pauseRepair,
            'resumeRepair': Origin.client.games.resumeRepair,
            'cancelRepair': Origin.client.games.cancelRepair,
            'play': Origin.client.games.play,
            'pushToTop': Origin.client.games.moveToTopOfQueue,
            'remove': Origin.client.contentOperationQueue.remove
        };

        $scope.calculatingLoc = UtilFactory.getLocalizedStr($scope.calculatingStr, CONTEXT_NAMESPACE, 'calculatingstr');
        $scope.playableLoc = UtilFactory.getLocalizedStr($scope.playablestr, CONTEXT_NAMESPACE, 'playablestr');
        $scope.playableAtLoc = UtilFactory.getLocalizedStr($scope.playableAtStr, CONTEXT_NAMESPACE, 'playableatstr');
        $scope.waitingLoc = UtilFactory.getLocalizedStr($scope.waitingStr, CONTEXT_NAMESPACE, 'waitingstr');
        $scope.updatingLoc = UtilFactory.getLocalizedStr($scope.updatingstr, CONTEXT_NAMESPACE, 'updatingstr');
        $scope.repairingLoc = UtilFactory.getLocalizedStr($scope.repairingstr, CONTEXT_NAMESPACE, 'repairingstr');
        $scope.goOnlineLoc = UtilFactory.getLocalizedStr($scope.goonlinestr, CONTEXT_NAMESPACE, 'goonlinestr');

        /**
        * Perfom Game download specific actions - download, cancel , resume
        * @method onIconClick
        */
        $scope.onIconClick = function(event, action) {
            event.stopPropagation();
            if(action){
                _.get(downloadActions, action).call(this, $scope.offerId);
            }
        };

        /**
        * Set the catalog data for title and packart of the current offer id
        * @param {object} ocdData
        * @param {object} catalogData
        * @method setGameDetails
        */
        function setGameDetails(ocdData, catalogData) {
            $scope.title = _.get(catalogData, ['i18n', 'displayName']);
            $scope.packart = _.get(catalogData, ['i18n', 'packArtLarge']);
            $scope.backgroundImage = _.get(ocdData, 'download-background-image');
            $scope.$digest();
        }

        /**
        * Get readable value from bytes for network speed and size
        * @param {string} bytes
        * @param {string} key
        * @return {string} [translated formatted byte]
        * @method getFormattedValue
        */
        function getFormattedBytes(bytes, key) {
            var size = UtilFactory.formatBytes(bytes);
            return UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, key , {'%size%' : size}, bytes);
        }

        /**
        * Get readable value from time remaining
        * @param seconds
        * @param key
        * @return {string} [translated formatted time]
        * @method getFormattedTime
        */
        function getFormattedTime(seconds, key) {
            var timeRemaining = moment.duration(seconds, 'seconds')._data;
            return UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, key , {
                '%hours%' : timeRemaining.hours,
                '%minutes%' : timeRemaining.minutes,
                '%seconds%' : timeRemaining.seconds
            });
        }

        /**
        * Reset the playable flag
        * @param progressInfo
        * @method setPlayableFlag
        */
        function resetPlayableFlag(){
            $scope.shouldLightFlag = false;
            $scope.arrowPosition = 0;
            $scope.flagPosition = 0;
        }

        /**
        * Set the playable flag if the game is playbale before full download
        * @param progressInfo
        * @method setPlayableFlag
        */
        function setPlayableFlag(progressInfo) {
            $scope.shouldLightFlag = progressInfo.shouldLightFlag;
            $scope.arrowPosition = (progressInfo.playableAt * 100).toFixed(0);
            $scope.flagPosition = ($scope.arrowPosition - FLAG_DEFAULT_OFFSET);

            if($scope.flagPosition < MIN_FLAG_LEFT) {
                $scope.flagPosition = MIN_FLAG_LEFT;
            } else if($scope.flagPosition > MAX_FLAG_LEFT) {
                $scope.flagPosition = MAX_FLAG_LEFT;
            }

            $scope.playable = ($scope.shouldLightFlag) ? $scope.playableLoc : $scope.playableAtLoc.replace('%percent%', $scope.arrowPosition+'%');
        }

        /**
        * Update the action states based on the progress status
        * @param {string} action
        * @method updateIconActionStates
        */
        function updateIconActionStates(action) {
            var axn = (action)? action : "";
            $scope.states = {
                'pause': 'pause'+axn,
                'resume': 'resume'+axn,
                'cancel': 'cancel'+axn
            };
        }

        /**
        * Set icon actions and current status if updating or repairing
        * @param {object} progress
        * @method setIconActionStates
        */
        function setIconActionStates(progress) {
            $scope.currentStatus = "";

            if(progress.installing) {
                updateIconActionStates(ACTION_INSTALL);
            }
            else if(progress.updating) {
                $scope.currentStatus = $scope.updatingLoc;
                updateIconActionStates(ACTION_UPDATE);
            }
            else if(progress.repairing) {
                $scope.currentStatus = $scope.repairingLoc;
                updateIconActionStates(ACTION_REPAIR);
            }
            else {
                updateIconActionStates();
            }
        }

        /**
        * Get Game Progress data from observer
        * @param data
        * @method getGameData
        */
        function getGameData(data) {
            var progressInfo;

            if(data && data.progressInfo) {
                progressInfo = data.progressInfo;
                $scope.progress = data;
                $scope.percent = Math.floor(progressInfo.progress * MAX_PERCENT);
                $scope.phase = progressInfo.phase;
                $scope.totalFileSize = progressInfo.totalFileSize;

                // Needed this to solve ugly c++ model issue...
                setIconActionStates(data);

                if($scope.status === 'active') {
                    $scope.progressState = UtilFactory.getDownloadState(progressInfo);
                    $scope.size = getFormattedBytes($scope.totalFileSize, 'sizestr');
                    $scope.downloaded = getFormattedBytes(progressInfo.bytesDownloaded, 'sizestr');
                    $scope.timeRemaining = getFormattedTime(progressInfo.secondsRemaining, 'timestr');

                    if($scope.phase === "RUNNING") {
                        $scope.phaseDisplay = (progressInfo.secondsRemaining > 0) ? $scope.timeRemaining : $scope.calculatingLoc;
                        $scope.speed = getFormattedBytes(progressInfo.bytesPerSecond, 'speedstr');
                    } else {
                        $scope.phaseDisplay = progressInfo.phaseDisplay;
                    }

                    // for launch we have to treat paused and download-in-queue as the same - c++ bug
                    $scope.resume = ($scope.phase === "PAUSED" || $scope.phase === "ENQUEUED")? true : false;

                    if(progressInfo.playableAt) {
                        setPlayableFlag(progressInfo);
                    } else {
                        resetPlayableFlag(); // need to reset for mini download manager usecase
                    }

                    $scope.pausable = (data.playing)? false : data.pausable;
                    $scope.resumable = (data.playing)? false : data.resumable;
                }
            }
        }

        /**
        * Get Game OCD and Catalog data
        * @method getGameDetails
        */
        function getGameDetails() {
            Promise.all([
                GamesDataFactory.getOcdByOfferId($scope.offerId)
                    .then(OcdHelperFactory.findDirectives('origin-game-defaults'))
                    .then(OcdHelperFactory.flattenDirectives('origin-game-defaults'))
                    .then(_.head),
                GamesDataFactory.getCatalogInfo([$scope.offerId])
                    .then(ObjectHelperFactory.getProperty($scope.offerId))
                ])
                .then(_.spread(setGameDetails))
                .catch(function(error) {
                    ComponentsLogFactory.error('[download-manager-game GamesDataFactory.getCatalogInfo] error', error);
                });
        }

        // Creates a new observable which will serve data to both mini and large download manager
        $scope.$watch('offerId', function (newValue) {
            if (newValue) {
                $scope.offerId = newValue;
                getGameDetails();
                if(observableParticipant) {
                    observableParticipant.detach();
                }
                observableParticipant = DownloadManagerFactory.getObserver(newValue, $scope, getGameData);
            }
        });

        /**
        * Called to set the online status of client
        * @method setOnlineStatus
        */
        function setOnlineStatus() {
            $scope.isOnline = AuthFactory.isClientOnline();
        }

        /**
        * Update online status based on the clientonlinestatechanged event
        * @param {object} onlineObj
        * @method updateOnlineState
        */
        function updateOnlineState(onlineObj) {
            if (onlineObj) {
                setOnlineStatus();
            }
            $scope.$digest();
        }

        // Determine whether the user is online or offline when they come to the game library
        setOnlineStatus();

        /**
        * Called when $scope is destroyed, release event handlers
        * @method onDestroy
        */
        function onDestroy() {
            EventsHelperFactory.detachAll(handles);
        }

        handles = [
            AuthFactory.events.on('myauth:clientonlinestatechanged', updateOnlineState)
        ];

        $scope.$on('$destroy', onDestroy);

   }

    /**
    * The directive
    */
    function originDownloadManagerGame(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                format: '@format',
                status: '@status',
                parentOfferId: '@parentofferid'
            },
            controller:originDownloadManagerGameCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('downloadmanager/views/downloadmanagergame.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originDownloadManagerGame
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offerid of the game
     * @param {string} format large or small
     * @param {string} status active, inQueue or complete
     * @param {string} parentofferid offerid of the parent if the parent is in download queue.
     * @param {LocalizedString} sizestr "{0} 0 B | ]1,1024] %size% B | ]1024, 1048576] %size% KB | ]1048576, 1073741824] %size% MB | ]1073741824,1099511627776] %size% GB | ]1099511627776,+Inf] %size% TB"
     * @param {LocalizedString} speedstr "{0} 0 B / SEC | ]1,1024] %size% B / SEC | ]1024, 1048576] %size% KB / SEC | ]1048576, 1073741824] %size% MB / SEC | ]1073741824,1099511627776] %size% GB / SEC| ]1099511627776,+Inf] %size% TB / SEC"
     * @param {LocalizedString} timestr "%hours%h %minutes%m %seconds%s"
     * @param {LocalizedString} calculatingstr "Calculating"
     * @param {LocalizedString} waitingstr "Waiting for base game to finish installing"
     * @param {LocalizedString} updatingstr "Updating"
     * @param {LocalizedString} repairingstr "Repairing"
     * @param {LocalizedString} goonlinestr "Go Online to Download"
     * @param {LocalizedString} playablestr "PLAYABLE"
     * @param {LocalizedString} playableatstr "Playable At %percent%"
     * @description
     *
     * Origin Download Manager Game
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-download-manager-game offerid="DR:30303232" format="large" status="inQueue" sizestr="{0} 0 B | ]1,1024] %size% B | ]1024, 1048576] %size% KB | ]1048576, 1073741824] %size% MB | ]1073741824,1099511627776] %size% GB | ]1099511627776,+Inf] %size% TB"></origin-download-manager-game>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originDownloadManagerGameCtrl', originDownloadManagerGameCtrl)
        .directive('originDownloadManagerGame', originDownloadManagerGame);
}());
