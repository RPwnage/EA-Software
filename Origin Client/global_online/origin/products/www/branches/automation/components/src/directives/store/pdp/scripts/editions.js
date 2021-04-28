
/** 
 * @file store/pdp/scripts/editions.js
 */ 
(function(){
    'use strict';
    function originStorePdpEditions(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                vaultUpgradeEdition: '@',
                collectorsEdition: '@',
                completeEdition: '@',
                premiumEdition: '@',
                gameOfTheYearEdition: '@',
                ultimateEdition: '@',
                crysisTrilogy: '@',
                extendedEdition: '@',
                holidayEdition: '@',
                goldEdition: '@',
                deluxeEdition: '@',
                digitalDeluxeEdition: '@',
                maximumEdition: '@',
                specialEdition: '@',
                enhancedEdition: '@',
                insideLimbo: '@',
                limitedEdition: '@',
                arcadeEdition: '@',
                ostComboPack: '@',
                soundtrackEdition: '@',
                survivalEdition: '@',
                gameOfTheYearEditionPc: '@',
                gameOfTheYearEditionMac: '@',
                standardEdition: '@',
                standardEditionPc: '@',
                standardEditionMac: '@',
                legacyEdition: '@',
                playForFree: '@',
                originEdition: '@',
                nonOriginEdition: '@',
                dvdBoxedEdition: '@',
                overlordEdition: '@',
                superDeluxeEdition: '@',
                archonEdition: '@',
                hunterEdition: '@',
                commanderEdition: '@',
                thirdPackageEdition: '@',
                hunterPack: '@',
                recruitPack: '@',
                ultimatePack: '@',
                digitalCollectorsEdition: '@',
                gamePremiumEdition: '@',
                almanacEdition: '@',
                heroEditionCrysis3: '@',
                classicEdition: '@',
                infinityEdition: '@',
                houseOfYorkEdition: '@',
                plusEdition: '@',
                admiralEdition: '@',
                heroEdition: '@',
                royalEdition: '@',
                championEdition: '@',
                deluxePack: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/editions.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpEditions
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} vault-upgrade-edition * Edition translation
     * @param {LocalizedString} collectors-edition * Edition translation
     * @param {LocalizedString} complete-edition * Edition translation
     * @param {LocalizedString} premium-edition * Edition translation
     * @param {LocalizedString} game-of-the-year-edition * Edition translation
     * @param {LocalizedString} ultimate-edition * Edition translation
     * @param {LocalizedString} crysis-trilogy * Edition translation
     * @param {LocalizedString} extended-edition * Edition translation
     * @param {LocalizedString} holiday-edition * Edition translation
     * @param {LocalizedString} gold-edition * Edition translation
     * @param {LocalizedString} deluxe-edition * Edition translation
     * @param {LocalizedString} digital-deluxe-edition * Edition translation
     * @param {LocalizedString} maximum-edition * Edition translation
     * @param {LocalizedString} special-edition * Edition translation
     * @param {LocalizedString} enhanced-edition * Edition translation
     * @param {LocalizedString} inside-limbo * Edition translation
     * @param {LocalizedString} limited-edition * Edition translation
     * @param {LocalizedString} arcade-edition * Edition translation
     * @param {LocalizedString} ost-combo-pack * Edition translation
     * @param {LocalizedString} soundtrack-edition * Edition translation
     * @param {LocalizedString} survival-edition * Edition translation
     * @param {LocalizedString} game-of-the-year-edition-pc * Edition translation
     * @param {LocalizedString} game-of-the-year-edition-mac * Edition translation
     * @param {LocalizedString} standard-edition * Edition translation
     * @param {LocalizedString} standard-edition-pc * Edition translation
     * @param {LocalizedString} standard-edition-mac * Edition translation
     * @param {LocalizedString} legacy-edition * Edition translation
     * @param {LocalizedString} play-for-free * Edition translation
     * @param {LocalizedString} origin-edition * Edition translation
     * @param {LocalizedString} non-origin-edition * Edition translation
     * @param {LocalizedString} dvd-boxed-edition * Edition translation
     * @param {LocalizedString} overlord-edition * Edition translation
     * @param {LocalizedString} super-deluxe-edition * Edition translation
     * @param {LocalizedString} archon-edition * Edition translation
     * @param {LocalizedString} hunter-edition * Edition translation
     * @param {LocalizedString} commander-edition * Edition translation
     * @param {LocalizedString} third-package-edition * Edition translation
     * @param {LocalizedString} hunter-pack * Edition translation
     * @param {LocalizedString} recruit-pack * Edition translation
     * @param {LocalizedString} ultimate-pack * Edition translation
     * @param {LocalizedString} digital-collectors-edition * Edition translation
     * @param {LocalizedString} game-premium-edition * Edition translation
     * @param {LocalizedString} almanac-edition * Edition translation
     * @param {LocalizedString} hero-edition-crysis-3 * Edition translation
     * @param {LocalizedString} classic-edition * Edition translation
     * @param {LocalizedString} infinity-edition * Edition translation
     * @param {LocalizedString} house-of-york-edition * Edition translation
     * @param {LocalizedString} plus-edition * Edition translation
     * @param {LocalizedString} admiral-edition * Edition translation
     * @param {LocalizedString} hero-edition * Edition translation
     * @param {LocalizedString} royal-edition * Edition translation
     * @param {LocalizedString} champion-edition * Edition translation
     * @param {LocalizedString} deluxe-pack * Edition translation
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-editions />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpEditions', originStorePdpEditions);
}()); 
