
/** 
 * @file store/pdp/scripts/description.js
 */ 
(function(){
    'use strict';

    /**
    * The directive
    */
    function originStorePdpDescription(ComponentsConfigFactory, GamesDataFactory, $stateParams) {
        /**
        * The directive link
        */
        function originStorePdpDescriptionLink(scope) {
            // TODO: will need to fetch catalog info based on offer path, rather than offerid
            if($stateParams.offerid) {
                GamesDataFactory.getCatalogInfo([$stateParams.offerid])
                    .then(function(data){
                        if (data){
                            scope.$evalAsync(function() {
                                // TODO: replace with catalog data
                                scope.description = '<strong>'+data[$stateParams.offerid].displayName+'</strong><br><br><strong>Experience Epic Battles on a Galactic Scale</strong><br><br>Feel the ominous thud of an AT-AT stomping down on the frozen tundra of Hoth. Zip through the lush forests of Endor on an Imperial speeder bike while dodging incoming blaster fire. Engage in intense dogfights as squadrons of X-wings and TIE fighters fill the skies. Immerse yourself in the epic Star Wars™ battles you’ve always dreamed of and create new heroic moments of your own in Star Wars™ Battlefront™. <br><br><strong>Pre-order Bonus: Get 1 week early access to the Battle of Jakku with 2 additional maps!</strong><br><br>• Learn what happened before the events of Star Wars™: The Force Awakens™. <br><br>• 2 maps set on the all-new planet of Jakku. <br><br>• 7 days early access for those who pre-order Star Wars™ Battlefront™, beginning on December 1, 2015. <br><br>• Available as a free download to all players on December 8th, 2015. <br><br><strong>Get five exciting in-game items with the Star Wars™ Battlefront™ Deluxe Edition.</strong><br><br>• DL-44 Blaster, as made famous by Han Solo (instant access) <br><br>• Ion Grenade (instant access) <br><br>• Ion Torpedo (instant access) <br><br>• Ion Shock (exclusive emote) <br><br>• Victory (exclusive emote) <br><br><strong>Game Features:</strong><br><br>• Engage in multiplayer battles on a galactic scale. Fight for the Rebellion or Empire in a wide variety of multiplayer matches that support up to 40 players, and seamlessly swap between first-person and third-person viewpoints to experience the action however you wish. Fight alongside your friends online, or in exciting challenges inspired by the films, available for solo or co-op play. <br><br>• Master the battlefield with iconic Star Wars™ characters. Play as some of the most memorable characters in the Star Wars™ universe, including Darth Vader and Boba Fett, and encounter a variety of beloved characters from the original trilogy such as C-3PO and R2-D2. <br><br>• Pilot incredible Star Wars™ vehicles. Hop in the cockpit of X-wings, TIE fighters, and even the Millennium Falcon for some edge-of-your-seat aerial combat. Then take the battle to the ground: Pilot a diverse set of ground-based vehicles including nimble speeder bikes, massive AT-ATs, and more. <br><br>• Immerse yourself in an authentic Star Wars™ experience. Each battle is brought to life by the original sound effects from the films and amazing digital replicas of authentic Star Wars™ movie models. Visit classic planets from the original Star Wars™ trilogy, detailed with an unprecedented amount of realism that will transport you to a galaxy far, far away. </p>';
                            });
                        }
                    })
                    .catch(function(error) {
                        Origin.log.exception(error, 'origin-store-pdp-description - getCatalogInfo');
                    });
            }
        }
        
        return {
            restrict: 'E',
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/description.html'),
            link: originStorePdpDescriptionLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpDescription
     * @restrict E
     * @element ANY
     * @scope
     * @description PDP description block, retrieved from catalog
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-description></origin-store-pdp-description>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpDescription', originStorePdpDescription);
}()); 
