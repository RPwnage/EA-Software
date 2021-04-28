/**
 * @file factories/gamelibrary/gamelibrarytilesize.js
 */
(function() {

    'use strict';

    function GameLibraryTileSizeFactory(ObservableFactory, ObserverFactory) {

        var GAMETILE_SIZES = ['large', 'medium', 'small'],
            GAMETILE_DEFAULT_SIZE = GAMETILE_SIZES[0],
            GAMETILE_SIZE_SETTING = 'GameLibraryTileSize',
            gameTileSizeObservable = ObservableFactory.observe({ tileSize: GAMETILE_DEFAULT_SIZE });

        /**
         * Update the observable object with the grid size setting
         * @param tileSize {String} the game tile size setting
         */
        function setTileSizeObservable(tileSize) {
            gameTileSizeObservable.data.tileSize = tileSize;
            gameTileSizeObservable.commit();
        }

        /**
         * Return the game tile size observer
         * @returns tileSizeObserver {Observer} the game tile size observer
         */
        function getTileSizeObserver() {
            return ObserverFactory.create(gameTileSizeObservable);
        }

        /**
         * Write the game tile size setting to the client
         * @param tileSize {String} the game tile size setting
         */
        function setTileSize(tileSize) {
            if (GAMETILE_SIZES.indexOf(tileSize) >= 0) {
                Origin.client.settings.writeSetting(GAMETILE_SIZE_SETTING, tileSize);
                setTileSizeObservable(tileSize);
            }
        }

        /**
         * Parse the result from the client readSetting call
         * @param tileSize {String} the game tile size setting
         */
        function setGridSizeObservable(tileSize) {
            if (GAMETILE_SIZES.indexOf(tileSize) >= 0) {
                setTileSizeObservable(tileSize);
            } else {
                setTileSizeObservable(GAMETILE_DEFAULT_SIZE);
            }

        }

        /**
         * Handle any error caught in the client readSetting call
         */
        function handleReadGridSizeSettingError() {
            setTileSizeObservable(GAMETILE_DEFAULT_SIZE);
        }

        // Set up the observable by reading the game tile size setting
        if (Origin.client.isEmbeddedBrowser()) {
            Origin.client.settings.readSetting(GAMETILE_SIZE_SETTING)
                .then(setGridSizeObservable)
                .catch(handleReadGridSizeSettingError);
        } else {
            setTileSizeObservable(GAMETILE_DEFAULT_SIZE);
        }

        return {
            getTileSizeObserver: getTileSizeObserver,
            setTileSize: setTileSize
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GameLibraryTileSizeFactory

     * @description
     *
     * Helpers to manage the game library tile sizing
     */
    angular.module('origin-components')
        .factory('GameLibraryTileSizeFactory', GameLibraryTileSizeFactory);
}());
