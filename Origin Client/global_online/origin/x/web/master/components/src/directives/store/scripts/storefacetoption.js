/**
 * @file filter/filteroption.js
 */
(function() {
    'use strict';
    /* jshint ignore:start */
    var FacetIdEnumeration = {

            "Shooter": "shooter",
            "Action": "action",
            "Adventure": "adventure",
            "Arcade": "arcade",
            "Casual": "casual",
            "Family": "family",
            "Fighting": "fighting",
            "Mmo": "mmo",
            "Online": "online",
            "Puzzle": "puzzle",
            "Racing": "racing",
            "Role-playing": "role-playing",
            "Simulation": "simulation",
            "Sports": "sports",
            "Strategy": "strategy",
            "On sale": "on-sale",
            "Under $5": "pricebargains",
            "Under $10": "pricetier1",
            "$10 - $20": "pricetier2",
            "$20 - $30": "pricetier3",
            "$30 - $40": "pricetier4",
            "$50 - $60": "pricetier5",
            "Above $60": "pricetier6",

            "Base Games": "basegame",
            "Bundles": "bundles",
            "Currency": "currency",
            "DLC": "dlc",
            "Extra Content": "extra-content",
            "Expansion Packs": "expansion",
            "Free to Play": "freegames",
            "Online Subscriptions": "onlinesubscription",



            "Mutiplayer": "multiplayer",
            "Single Player": "singleplayer",

            "The Sims Studio": "the-sims-studio",
            "Epic Games": "-epic-games",
            "INC.": "-inc.",
            "38 Studios": "38-studios",
            "4A Games": "4a-games",
            "5th Cell": "5th-cell",
            "Big Huge Games38 Studios": "big-huge-games38-studios",
            "Bioware": "bioware",
            "Bright Future": "bright-future",
            "Bright Light": "bright-light",
            "Bullfrog Productions": "bullfrog-productions",
            "Cateia Games": "cateia-games",
            "CD Projekt RED": "cd-projekt-red",
            "Codemasters Birmingham": "codemasters-birmingham",
            "Coldwood Interactive": "coldwood-interactive",
            "Core Design": "core-design",
            "Criterion Games": "criterion-games",
            "Crystal Dynamics": "crystal-dynamics",
            "Crytek": "crytek",
            "Cyanide": "cyanide",
            "Danger Close Games": "danger-close-games",
            "Dice": "dice",
            "Dontnod Entertainment": "dontnod-entertainment",
            "EA Black Box": "ea-black-box",
            "EA Canada": "ea-canada",
            "EA Hasbro": "ea-hasbro",
            "EA Phenomic": "ea-phenomic",
            "EA Salt Lake": "ea-salt-lake",
            "Eala": "eala",
            "Ensemble Studios": "ensemble-studios",
            "Eugen Systems": "eugen-systems",
            "Firaxis Games": "firaxis-games",
            "Firefly Studios": "firefly-studios",
            "Frontier Developments": "frontier-developments",
            "Funcom": "funcom",
            "Ghost Games": "ghost-games",
            "Giants Software": "giants-software",
            "Haeimont Games": "haeimont-games",
            "Inxile Entertainment": "inxile-entertainment",
            "King Art": "king-art",
            "Kuju Entertainment": "kuju-entertainment",
            "Maxis": "maxis",
            "Monolith Productions": "monolith-productions",
            "Monte Sristo": "monte-cristo",
            "Mythic Entertainment": "mythic-entertainment",
            "Ninja Theory": "ninja-theory",
            "Nixxes Software": "nixxes-software",
            "Obsidian Entertainment": "obsidian-entertainment",
            "Origin Systems": "origin-systems",
            "Pandemic Studios": "pandemic-studios",
            "Paradox Development Studio": "paradox-development-studio",
            "People Can Fly": "people-can-fly",
            "Piranha Bytes": "piranha-bytes",
            "Platinumgames inc.": "platinumgames-inc.",
            "Popcap Games": "popcap-games",
            "Realmforge Studios": "realmforge-studios",
            "Rebellion Oxford": "rebellion-oxford",
            "Remedy Entertainment": "remedy-entertainment",
            "Respawn Entertainment": "respawn-entertainment",
            "Rocksteady Studios": "rocksteady-studios",
            "Slightly Mad Studios": "slightly-mad-studios",
            "Snowblind Studios": "snowblind-studios",
            "Spicy Horse": "spicy-horse",
            "Spiders Studios": "spiders-studios",
            "Starbreeze Studios": "starbreeze-studios",
            "Techland": "techland",
            "Telltale Games": "telltale-games",
            "The Creative Assembly": "the-creative-assembly",
            "Tilted Mill Entertainment": "tilted-mill-entertainment",
            "Trapdoor": "trapdoor",
            "Traveller's Tales": "traveller's-tales",
            "TT Games": "tt-games",
            "Uber Entertainment": "uber-entertainment",
            "Ubisoft Montpellier": "ubisoft-montpellier",
            "Ubisoft Montreal": "ubisoft-montreal",
            "Ubisoft Shanghai": "ubisoft-shanghai",
            "Victory Games": "victory-games",
            "Vigil Games": "vigil-games",
            "Visceral Games": "visceral-games",
            "Volition": "volition",
            "Westwood Studios": "westwood-studios",
            "BioWare": "bioware",
            "Codemasters": "codemasters-birmingham",
            "CCP Games": "ccp-games",
            "Codemasters Southam": "codemasters-southam",
            "Danger Close And DICE": "danger-close-and-dice",
            "Danger Close Games And DICE": "danger-close-games-and-dice",
            "DICE": "dice",
            "EA Tiburon": "ea-tiburon",
            "EALA": "eala",
            "InXile Entertainment": "inxile-entertainment",
            "Monte Cristo": "monte-cristo",
            "N Fusion Interactive": "n-fusion-interactive",
            "People Can Fly,Epic Games": "people-can-fly,epic-games",
            "PlatinumGames Inc.": "platinumgames-inc.",
            "PopCap Games": "popcap-games",
            "Relic Entertainment": "relic-entertainment",
            "Slightly-Mad Studios": "slightly-mad-studios",
            "Travellers Tales": "travellers-tales",
            "Trion Worlds": "trion-worlds",
            "United Front Games": "united-front-games",
            "Visceral": "visceral",
            "Volition Inc.": "volition-inc.",

            "PEGI 3": "pegi3",
            "PEGI 7": "pegi7",
            "PEGI 12": "pegi12",
            "PEGI 16": "pegi16",
            "PEGI 16 Provisional": "pegi16Provisional",
            "PEGI 18": "pegi18",
            "PEGI 18 Provisional": "pegi18Provisional",
            "Mature" : "mature",
            "Teen" : "teen",
            "Everyone" : "everyone",
            "Everyone 10+" : "everyone-10+",
            "Rating pending" : "rating-pending",

            "Battlefield": "battlefield",
            "Need for Speed": "nfs",
            "ULTIMA": "ultima",
            "The Sims": "sims",
            "FIFA": "fifa",
            "Dead Space": "deadspace",
            "SymCity": "simcity",
            "Dragon Age": "dragonage",
            "Assassin's Creed": "assassin's-creed",
            "Crysis": "crysis",
            "Mass Effect": "masseffect",
            "Command & Conquer": "commandandconquer",
            "MysteryPI": "mysterypi",
            "PVZ": "pvz",
            "Titanfall": "titanfall",
            "AmazingAdventures": "amazingadventures",
            "TombRaider": "tombraider",
            "WingCommander": "wingcommander",
            "Koa": "koa",
            "MOH": "moh",
            "Mirror's edge": "mirrorsedge",
            "NFS": "nfs",
            "Sims": "sims",
            "Command and Conquer": "commandandconquer",
            "None": "none",
            "Battle forge": "battleforge",
            "Sim City": "simcity",
            "Spore": "spore",
            "Madden": "madden",
            "Monopoly": "monopoly",
            "NFL Head Coach": "nflheadcoach",
            "NHL": "nhl",
            "Army Of Two": "armyoftwo",
            "Bejeweled": "bejeweled",
            "Burnout": "burnout",
            "Fight Night": "fightnight",
            "Skate": "skate",
            "SSX": "ssx",
            "Saboteur": "saboteur",
            "Tiger": "tiger",
            "War": "war",
            "Rail Simulator": "railsimulator",
            "Batman": "batman",
            "Harry Potter": "harrypotter",
            "LOTR": "lotr",
            "Mercenaries": "mercenaries",
            "FIFA Street": "fifastreet",
            "NBA": "nba",
            "Hell Gate": "hellgate",
            "Little Stpet Shop": "littlestpetshop",
            "Cricket": "cricket",
            "EA Sports Active": "EA SPORTS Active",
            "Escape": "escape",
            "Star Wars": "star-wars",
            "Wing Commander": "wingcommander",
            "Peggle": "peggle",
            "Syndicate": "syndicate",
            "POGO": "pogo",
            "Amazing Adventures": "amazingadventures",
            "DAOC": "daoc",
            "Feeding frenzy": "feedingfrenzy",
            "Insaniquarium": "insaniquarium",
            "Secret World": "secretworld",
            "Mystery PI": "mysterypi",
            "Vacation Quest": "vacationquest",
            "Defiance": "defiance",
            "Far Cry": "far-cry",
            "Alchemy": "alchemy",
            "Alice": "alice",
            "Land so flore": "landsoflore",
            "Star flight": "starflight",
            "Theme Hospital": "themehospital",
            "Big Money": "bigmoney",
            "Bookworm": "bookworm",
            "Dunge On Keeper": "dungeonkeeper",
            "Dynomite": "dynomite",
            "Magic carpet": "magiccarpet",
            "Populous": "populous",
            "Sidmeiers": "sidmeiers",
            "The Gamee of Life": "thegameeoflife",
            "Warp": "warp",
            "Zuma Srevenge": "zumasrevenge",
            "Tomb Raider": "tombraider",
             "Witcher": "witcher",
            "Zubo": "zubo",
            "Eve": "eve",
            "The Book Of Unwritten Tales": "thebookofunwrittentales",
            "Nrave": "nrave"
        },
        CategoryEnumeration = {
            "Sort": "sort",
            "Genre": "genre",
            "Price": "price",
            "Franchise": "franchise",
            "Game Type": "gameType",
            "Platform": "platform",
            "Availability": "availability",
            "Players": "players",
            "Rating": "rating",
            "Developer": "developer"
        };
        /* jshint ignore:end */

    function originStoreFacetOption(ComponentsConfigFactory, StoreSearchHelperFactory, StoreSearchFactory) {
        function originStoreFacetOptionLink(scope) {
             scope.count = 0;
             scope.countVisible = true;

           /**
             * Determines whether the facet is selected or not.
             */

           scope.$watch(StoreSearchFactory.isModelReady, function (newValue) {
               if (newValue) {
                   scope.isActive = StoreSearchHelperFactory.isFilterActive(scope.categoryId, scope.facetId);
                   var filter = StoreSearchHelperFactory.getFilter(scope.categoryId, scope.facetId);
                   if (filter) {
                       scope.count = filter.count;
                   }
               }
           });


            scope.enabled = function () {
                return scope.isActive || scope.count > 0;
            };

            /**
             * Toggles active/inactive state. Doesn't affect disabled items
             * @return {void}
             */
            scope.toggle = function () {
                if (scope.enabled()) {
                    scope.isActive = !scope.isActive;
                    if (scope.categoryId) {
                        StoreSearchHelperFactory.applyFilter(scope.categoryId, scope.facetId);
                    }
                }

            };
        }

        return {
            restrict: 'E',
            scope: {
                label: '@',
                facetId: '@',
                categoryId: '@'
            },
            link: originStoreFacetOptionLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/storefacetoption.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:OriginStoreFacetOption
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} label of the facet option
     * @param {FacetIdEnumeration} facet-id Type of this facet. Must be an enum
     * @param {CategoryIdEnumeration} category-id Type of this facet group. Must be an enum
     * @description
     *
     * Filter panel item that can be toggled on and off.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-facet-category label="Genre" category-id="genre">
     *           <origin-store-facet-option label="Action" facet-id="action"></origin-store-facet-option>
     *         </origin-store-facet-category>
     *     </file>
     * </example>
     *
     */

        // directive declaration
    angular.module('origin-components')
        .directive('originStoreFacetOption', originStoreFacetOption);
}());

