/**
 * @file game/gameproperties.js
 */
(function() {
    'use strict';

    function GamePropertiesFactory(ObservableFactory, DialogFactory, GamesClientFactory, GamesCatalogFactory, GamesNonOriginFactory, GamesEntitlementFactory, AuthFactory) {
        var GAMEPROPERTIES_DIALOG_ID = 'origin-dialog-content-gameproperties-id',
            GAMEPROPERTIES_DIRECTIVE_NAME = 'origin-dialog-content-gameproperties',
            initialGameProperties = {
                gameTitle: '',
                cloudSavesEnabled: false,
                cloudSavesSupported: false,
                cdKey: '',
                grantDate: null,
                gameManualUrl: '',
                enableOig: true,
                commandLineText: '',
                selectedLaunchOption: '',
                multiLaunchOptions: [],
                locales: [],
                selectedLocale: { id: -1 },
                usagePercent: 0,
                usageDisplay: '',
                lastBackupValid: true,
                installed: false,
                isNOG: false,
                nogExecutablePath: ''
            },
            gamePropertiesObservable = ObservableFactory.observe(initialGameProperties),
            clientPropertiesObservable = ObservableFactory.observe({
                isOnline: false
            }), 
            SETTING_OIGDISABLEDGAMES = 'OigDisabledGames',
            SETTING_GAMECOMMANDLINEARGUMENTS = 'GameCommandLineArguments',
            SETTING_MULTILAUNCHPRODUCTIDSANDTITLES = 'MultiLaunchProductIdsAndTitles',
            SETTING_ENABLECLOUDSAVES = 'cloud_saves_enabled',
            SEPARATOR = '#//',
            UNDERSCORE = '_',
            DISABLED_SETTING = 't',
            oigDisabledSettingsData = '',
            multilaunchSettingsData = '',
            commandLineSettingsData = '',
            currentGameLanguageId = -1,
            currentCloudSavesEnabled = false,
            currentGameTitle = '',
            currentOfferId = '';

        /**
         * given an setting string, determine if setting is for current offer id
         * @param {string} settingStr
         * @return {boolean} setting is for current game
         * @method isSettingForThisOfferId
         */
        function isSettingForThisOfferId(settingStr) {
            return (settingStr.indexOf(currentOfferId) === 0);
        }


        /**
         * update the client setting for the current offer id
         * @param {string} settingId the ID of the setting to update
         * @param {string} settingData the existing data of the setting
         * @param {string} newSetting the new setting for the current offer id
         * @method updateClientSettingForCurrentOfferId
         */
        function updateClientSettingForCurrentOfferId(settingId, settingData, newSetting) {
            if (!_.isUndefined(settingData)) {
                var allGames = settingData.split(SEPARATOR);
                var gameIndex = _.findIndex(allGames, isSettingForThisOfferId);

                if (gameIndex >= 0) {
                    allGames.splice(gameIndex, 1);
                }

                if (!!newSetting) {
                    allGames.push(newSetting);                    
                }

                Origin.client.settings.writeSetting(settingId, allGames.join(SEPARATOR));
            }
        }

        /**
         * read the client setting for the current offer id
         * @param {string} settingData the existing data of the setting
         * @method getClientSettingForCurrentOfferId
         */
        function getClientSettingForCurrentOfferId(settingData) {
            var result;
            if (!_.isUndefined(settingData)) {
                var allGames = settingData.split(SEPARATOR);
                var gameIndex = _.findIndex(allGames, isSettingForThisOfferId);
                if (gameIndex >= 0) {
                    result = allGames[gameIndex].slice(currentOfferId.length+1);
                }

            }
            return result;
        }

        /**
         * save the command line argument , if changed
         * @method saveCommandLineArgument
         */
        function saveCommandLineArgument(data) {
            var newSetting;
            var currentCommandLineText = getClientSettingForCurrentOfferId(commandLineSettingsData);
            if (currentCommandLineText !== data.commandLineText) {

                if (data.commandLineText.length) {
                    newSetting = currentOfferId + UNDERSCORE + data.commandLineText;
                }
                updateClientSettingForCurrentOfferId(SETTING_GAMECOMMANDLINEARGUMENTS, commandLineSettingsData, newSetting);
            }
        }

        /**
         * save OIG enabled setting
         * @method saveOigEnabled
         */
        function saveOigEnabled(data) {
            var newSetting;
            var currentEnableOig = (getClientSettingForCurrentOfferId(oigDisabledSettingsData) !== DISABLED_SETTING);
            if (currentEnableOig !== data.enableOig) {

                if (!data.enableOig) {
                    newSetting = currentOfferId + UNDERSCORE + DISABLED_SETTING;
                }
                updateClientSettingForCurrentOfferId(SETTING_OIGDISABLEDGAMES, oigDisabledSettingsData, newSetting);
            }
        }

        /**
         * save the game title (NOGs only)
         * @method saveGameTitle
         */
        function saveGameTitle(data) {
            if (currentGameTitle !== data.gameTitle && data.gameTitle.length) {
                Origin.client.games.updateNOGGameTitle(currentOfferId, data.gameTitle);                    

            }
        }

        /**
         * save all general game properties
         * @method saveGamePropertiesGeneral
         */
        function saveGamePropertiesGeneral(general) {
            saveOigEnabled(general);
            if (gamePropertiesObservable.data.isNOG) {
                saveGameTitle(general);
                saveCommandLineArgument(general);
            }
        }

        /**
         * save all cloud saves game properties
         * @method saveGamePropertiesCloudSaves
         */
        function saveGamePropertiesCloudSaves(data) {
            if (currentCloudSavesEnabled !== data.cloudSavesEnabled) {
                Origin.client.settings.writeSetting(SETTING_ENABLECLOUDSAVES, data.cloudSavesEnabled);
            }
        }

        /**
         * save the multi launch option if changed
         * @method saveMultiLaunchOption
         */
        function saveMultiLaunchOption(data) {

            var newSetting;
            var currentMultiLaunchOption = getClientSettingForCurrentOfferId(multilaunchSettingsData);
            if (gamePropertiesObservable.data.multiLaunchOptions.length &&
                currentMultiLaunchOption !== data.selectedLaunchOption) {

                if (data.selectedLaunchOption.length) {
                    newSetting = currentOfferId + UNDERSCORE + data.selectedLaunchOption;
                }

                Origin.client.games.updateMultiLaunchOptions(currentOfferId, data.selectedLaunchOption).then(function(result){
                    if (result.success !== "done"){ // couldn't find function updateMultiLaunchOptions on client
                        // fallback to older title-based setting
                        updateClientSettingForCurrentOfferId(SETTING_MULTILAUNCHPRODUCTIDSANDTITLES, multilaunchSettingsData, newSetting); 
                    }
                });
            }
        }

        /**
         * save the game language
         * @method saveGameLanguage
         */
        function saveGameLanguage(data) {
            if (!!data.selectedLocale && currentGameLanguageId !== data.selectedLocale.id) {
                Origin.client.games.updateInstalledLocale(currentOfferId, data.selectedLocale.id);                    
            }
        }

        /**
         * save all advanced game properties
         * @method saveGamePropertiesAdvanced
         */
        function saveGamePropertiesAdvanced(advanced) {
            saveMultiLaunchOption(advanced);
            saveCommandLineArgument(advanced);
            saveGameLanguage(advanced);
        }


        /**
          * updates cloud save usage data
          * @method updateCloudScopeVars
          */
        function updateCloudData() {
            var game = GamesClientFactory.getGame(currentOfferId),
                cloudSavesUpdateNeeded = game && game.cloudSaves;
            if (cloudSavesUpdateNeeded) {
                    angular.merge(gamePropertiesObservable.data, {
                        usagePercent : (game.cloudSaves.currentUsage / game.cloudSaves.maximumUsage) * 100 + '%',
                        usageDisplay : game.cloudSaves.usageDisplay,
                        lastBackupValid : game.cloudSaves.lastBackupValid,
                        lastBackupDate: game.cloudSaves.lastBackupDate
                    });
            }
            return cloudSavesUpdateNeeded;
        }

        /**
         * given catalogInfo, parse out displayName and gameManualURL
         * @param {Object} catalogInfo for this offer id
         * @method parseCatalogInfo
         */
        function parseCatalogInfo(catalogInfo) {
            if (Object.keys(catalogInfo).length) {
                var catalogItem = catalogInfo[currentOfferId];
                gamePropertiesObservable.data.gameTitle = catalogItem.i18n.displayName;
                gamePropertiesObservable.data.gameManualUrl = catalogItem.i18n.gameManualURL;                
            }
            return catalogInfo;
        }


        /**
         * given the game object, parse out the cloud saves and installed data
         * @param {Object} game object from getClientGamePromise
         * @method parseGame
         */
        function parseGame(game) {
            gamePropertiesObservable.data.cloudSavesSupported = angular.isDefined(game.cloudSaves);
            gamePropertiesObservable.data.installed = game.installed;
        }

        /**
         * given the catalog data check if this is a NOG, and if so, pull the NOG catalog data
         * @param {Object} catalogInfo
         * @method checkForNonOriginGameTitle
         */
        function checkForNonOriginGameTitle(catalogInfo) {
            if (!Object.keys(catalogInfo).length) {
                var nogData = GamesNonOriginFactory.getCatalogInfo([currentOfferId]);

                if (Object.keys(nogData).length) {
                    gamePropertiesObservable.data.isNOG = true;
                    gamePropertiesObservable.data.gameTitle = nogData[currentOfferId].i18n.displayName;
                } 
            }                    
        }

        /**
         * given the setting data for SETTING_OIGDISABLEDGAMES, parse out the setting for this game
         * @param {string} settingsData
         * @method parseOigDisabledSetting
         */
        function parseOigDisabledSetting(settingsData) {
            oigDisabledSettingsData = settingsData;

            gamePropertiesObservable.data.enableOig = (getClientSettingForCurrentOfferId(settingsData) !== DISABLED_SETTING);
        }

        /**
         * given the setting data for SETTING_GAMECOMMANDLINEARGUMENTS, parse out the setting for this game
         * @param {string} settingsData
         * @method parseCommandLineArgumentsSetting
         */                
        function parseCommandLineArgumentsSetting(settingsData) {
            commandLineSettingsData = settingsData;
            gamePropertiesObservable.data.commandLineText = getClientSettingForCurrentOfferId(settingsData);
        }


        /**
         * given the setting data for SETTING_MULTILAUNCHPRODUCTIDSANDTITLES, parse out the setting for this game
         * @param {string} settingsData
         * @method parseMultiLaunchSetting
         */                
        function parseMultiLaunchSetting(settingsData) {
            multilaunchSettingsData = settingsData;
        }

        /**
         * given the options data from the client
         * @param {Object} options
         * @method parseMultiLaunchOptions
         */                
        function parseMultiLaunchOptions(options) {
            if (!!options && !!options.multilaunch && options.multilaunch.length > 1) {

                _.set(gamePropertiesObservable, ['data', 'multiLaunchOptions'], options.multilaunch);

                // grab selection from options.selection (from client) as we will use the unique id setting if available first
                if (!!options.selection){
                    _.set(gamePropertiesObservable, ['data', 'selectedLaunchOption'], options.selection);
                }
                else{   // if the client does not support this yet, fallback to the old method of grabbing it from the title setting data
                    _.set(gamePropertiesObservable, ['data', 'selectedLaunchOption'], getClientSettingForCurrentOfferId(multilaunchSettingsData));
                }
                if (!gamePropertiesObservable.data.selectedLaunchOption) { 
                    // if selected launch option is not set, use the given default or default to first option
                    _.set(gamePropertiesObservable, ['data', 'selectedLaunchOption'], options.default ? options.default : options.multilaunch[0]);
                }
            }
        }

        /**
         * given the options data from the client,
         * @param {Object} options
         * @method parseMultiLaunchOptions
         */                
        function parseAvailableLocales(list) {
            gamePropertiesObservable.data.locales = list.locales;
            gamePropertiesObservable.data.selectedLocale = list.locales[list.currentindex];
            if (list.locales.length) {
                currentGameLanguageId = gamePropertiesObservable.data.selectedLocale.id;
            }
        }

        /**
          * reads the value of SETTING_ENABLECLOUDSAVES setting and attaches to scope variable
          * @method getCloudSaveEnabledSetting
          */
        function getCloudSaveEnabledSetting(enabled) {
            currentCloudSavesEnabled = enabled;
            gamePropertiesObservable.data.cloudSavesEnabled = enabled;
        }

        /**
          * reads the value of NOG executable path
          * @method getNogExecutablePath
          */
        function parseNogExecutablePath(nogData) {
            gamePropertiesObservable.data.nogExecutablePath = nogData.executePath;
                                                                  
        }

        function parseEntitlementData(offerId) {
            var entitlement = GamesEntitlementFactory.getEntitlement(offerId);
            if (!!entitlement) {                
                gamePropertiesObservable.data.cdKey = entitlement.cdKey;
                gamePropertiesObservable.data.grantDate = entitlement.grantDate;                
            }
        }

        /**
         * open the game properties dialog and request all required data
         * @param {String} offerId offer id of the game
         * @method openGameProperties
         */
        function openGameProperties(offerId) { 
            // Set the current offer Id
            currentOfferId = offerId;

            // Initialize the observable data
            gamePropertiesObservable.data =  {
                gameTitle: '',
                cloudSavesEnabled: false,
                cloudSavesSupported: false,
                cdKey: '',
                grantDate: null,
                gameManualUrl: '',
                enableOig: true,
                commandLineText: '',
                selectedLaunchOption: '',
                multiLaunchOptions: [],
                locales: [],
                selectedLocale: { id: -1 },
                usagePercent: 0,
                usageDisplay: '',
                lastBackupValid: true,
                installed: false,
                isNOG: false,
                nogExecutablePath: ''
            };

            Promise.all([
                GamesClientFactory.getGamePromise(offerId)
                    .then(parseGame),
                GamesCatalogFactory.getCatalogInfo([offerId])
                    .then(parseCatalogInfo)
                    .then(checkForNonOriginGameTitle),
                Origin.client.settings.readSetting(SETTING_OIGDISABLEDGAMES)
                    .then(parseOigDisabledSetting),
                Origin.client.settings.readSetting(SETTING_ENABLECLOUDSAVES)
                    .then(getCloudSaveEnabledSetting),
                Origin.client.settings.readSetting(SETTING_MULTILAUNCHPRODUCTIDSANDTITLES)
                    .then(parseMultiLaunchSetting)       
                    .then(Origin.client.games.getMultiLaunchOptions(offerId)
                        .then(parseMultiLaunchOptions)),
                Origin.client.games.getAvailableLocales(offerId)
                    .then(parseAvailableLocales),
                Origin.client.games.getNOGExecutablePath(offerId)
                    .then(parseNogExecutablePath),
                Origin.client.settings.readSetting(SETTING_GAMECOMMANDLINEARGUMENTS)
                    .then(parseCommandLineArgumentsSetting),
                Origin.client.games.pollCurrentCloudUsage(offerId),
                updateCloudData(),
                parseEntitlementData(offerId)
                ]).then(function() {
                    gamePropertiesObservable.commit();  

                    // Open the modal dialog
                    DialogFactory.openDirective({
                        id: GAMEPROPERTIES_DIALOG_ID,
                        name: GAMEPROPERTIES_DIRECTIVE_NAME,
                        size: 'large',
                        data: {
                            offerId: offerId,
                        }
                    });

                });
        }

        /**
          * called when client online state changes
          * @method onClientOnlineStateChanged
          */
        function onClientOnlineStateChanged(onlineObj) {
            clientPropertiesObservable.data.isOnline = (!!onlineObj && onlineObj.isOnline);
            clientPropertiesObservable.commit();
        }

        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
        onClientOnlineStateChanged({isOnline: AuthFactory.isClientOnline()});

        return {
            openGameProperties: openGameProperties, 

            saveGamePropertiesGeneral: saveGamePropertiesGeneral,
            saveGamePropertiesCloudSaves: saveGamePropertiesCloudSaves,
            saveGamePropertiesAdvanced: saveGamePropertiesAdvanced,

            updateCloudData: updateCloudData,

            closeDialog: function() {
                DialogFactory.close(GAMEPROPERTIES_DIALOG_ID);
            },

            getGamePropertiesObservable: function() {
                return gamePropertiesObservable;
            },

            getClientPropertiesObservable: function() {
                return clientPropertiesObservable;
            }
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamePropertiesFactorySingleton(ObservableFactory, DialogFactory, GamesClientFactory, GamesCatalogFactory, GamesNonOriginFactory, GamesEntitlementFactory, AuthFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamePropertiesFactory', GamePropertiesFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamePropertiesFactory
     
     * @description
     *
     * GamePropertiesFactory
     */
    angular.module('origin-components')
        .factory('GamePropertiesFactory', GamePropertiesFactorySingleton);
}());
