;(function( $, undefined ) {

    "use strict";

    $.fn.achievements = function( options, parameters  ) {

        var settings = {
            achievementTemplate : null,
            releasedTemplate : null,
            achievementArray : null,
            achievementLastUpdate : null,
            isInIgo : false,
            myAchievementSets : [],
            releasedSets : []

        };

        // bindings
        var methods = {

            init : function() {

                $.extend( settings, options );

                if (window.achievementManager == null) { return false; }

                if ( $.getQueryString("inigo") == "true" ){
                    settings.isInIgo = true;
                };

                $("div.wrapper").removeClass("game-loading");

                // check if banner has been dismissed before
                var bannerKey = methods.helpers.generateBannerKey( originUser.originId );
                if ( bannerKey && bannerKey != "" ){
                    var bannerValue = localStorage.getItem( bannerKey.toLowerCase() );
                    if ( bannerValue && bannerValue.length > 0 ){
                        $(".msg-banner").hide();
                    }
                }

                // check online state and set up watcher for online state change
                methods.helpers.initialLoadSetup();

                // event bindings for the page
                $(".link-profile").on( "click", methods.events.clickProfile );
                $(".msg-banner header .btn-close").on( "click", methods.events.clickCloseBanner );
                $("section.other-games .item").on("click", methods.events.clickGameAd );
                $("body").on("click", ".link-achievements-game", methods.events.clickGame );

                $(".offline-gos-mode .button-refresh").on("click", function(){
                    $.each( achievementManager.achievementSets, function( index, achievementSet ) {
                        achievementManager.updateAchievementSet( achievementSet.id )
                    });
                });

                // draw once more when entitements have been full loaded
                // lots of reasons to redraw with both entitlements and achievementSets are removed and added.
                entitlementManager.refreshCompleted.connect( function() {
                    methods.helpers.drawAchievements();
                });

                entitlementManager.added.connect( function( entitlement ) {
                    methods.helpers.drawAchievements();
                });

                entitlementManager.removed.connect( function( entitlement ) {
                    methods.helpers.drawAchievements();
                });

                achievementManager.achievementSetAdded.connect( function( achievementSet ) {
                    methods.helpers.drawAchievements();
                });
                
                achievementManager.achievementSetRemoved.connect( function( achievementSet ) {
                    methods.helpers.drawAchievements();
                });
                
                achievementManager.achievementGranted.connect( function( achievementSet, achievement ) {
                    methods.helpers.drawAchievements();
                });

                achievementManager.xpChanged.connect( function( xpValue ) {
                    methods.helpers.drawAchievements();
                });

                achievementManager.rpChanged.connect( function( rpValue ) {
                    methods.helpers.drawAchievements();
                });

                // if there's an update signal check to see if any achievment set failed update.
                // if any failed show GOS offline page
                // if none failed redraw page.

                $.each( achievementManager.achievementSets, function( index, achievementSet ){

                    achievementSet.updated.connect( function( ) {
                        var achievementSetIsUpdated = methods.helpers.isUpdated();

                        if ( achievementSetIsUpdated ){
                            $("body").removeClass( "offline-gos-mode" );
                            methods.helpers.drawAchievements();
                        } else {
                            $("body").addClass( "offline-gos-mode" );
                        }

                    });
                });

                onlineStatus.onlineStateChanged.connect( function(state){
                    methods.helpers.viewSelection();
                });

                originPageVisibility.visibilityChanged.connect( function() {
                    telemetryClient.sendAchievementWidgetPageView( 'Portal', '', settings.isInIgo );    
                });

            }, //END init

            events : {

                clickProfile : function ( event ){

                    event.preventDefault();
                    clientNavigation.showMyProfile("ACHIEVEMENTS");

                },

                clickCloseBanner : function ( event ){

                    var bannerKey = methods.helpers.generateBannerKey( originUser.originId );
                    if ( bannerKey != "" ){
                        localStorage.setItem( bannerKey.toLowerCase(), 'true' );
                    }

                    $(this).parents('.msg-banner').slideUp();

                },

                clickGameAd : function( event ){

                    var masterId = $(this).data("master-id");
                    if ( masterId ){
                        telemetryClient.sendAchievementWidgetPurchaseClick( masterId );
                        clientNavigation.showStoreMasterTitlePage( masterId );
                    } else {
                        clientNavigation.showStoreHome();
                    }

                },

                clickGame : function( event ){
                    event.preventDefault();
                    document.location.href = "achievement-details.html?id=" + $(this).attr("data-game-id");
                }

            }, // END events

            helpers : {

                initialLoadSetup : function (){

                    telemetryClient.sendAchievementWidgetPageView( 'Portal', '', settings.isInIgo );
                    methods.helpers.viewSelection();
                },
                
                getAchievementArt : function (achievementSetId){

                    var img = new Image(),
                        bannerLocation =  "http://static.cdn.ea.com/ebisu/u/f/achievements/" + achievementSetId + "/images/en_US/overview_" + $.md5(achievementSetId + "overviewAchievement") + ".jpg";
                    img.onload = function ( evt ){
                        var $currentSelectedAchievement = $('.play-games .item').filter(function () {  return $(this).data('achievement-set-id') === achievementSetId; });
                        $currentSelectedAchievement.find("img").attr( "src", bannerLocation );
                    }
                    img.src = bannerLocation;
                },

                viewSelection : function(){

                    var lastUpdatedAchievement = methods.helpers.isUpdated();

                    // needs to run through check here. 
                     if ( !onlineStatus.onlineState ) {
                        $("body").addClass( "offline-mode" ).removeClass( "offline-gos-mode" );
                    } else if ( onlineStatus.onlineState && !lastUpdatedAchievement && achievementManager.achievementSets.length > 0 ){
                        $("body").removeClass( "offline-mode" ).addClass( "offline-gos-mode" );
                        $(".offline-gos-mode .timestamp").text( dateFormat.formatLongDateTime( settings.achievementLastUpdate ) );
                    } else {
                        $("body").removeClass( "offline-mode" ).removeClass( "offline-gos-mode" );
                        $("div.wrapper").removeClass("game-loading");
                        methods.helpers.drawAchievements();
                    }

                },

                isUpdated : function (){

                    var achievementSetIsUpdated = false;
                    $.each( achievementManager.achievementSets, function( index, achievementSet ) {
                        settings.achievementLastUpdate = achievementSet.lastUpdated;
                        if( achievementSet.isUpdated ) {
                            achievementSetIsUpdated = true;
                            return false;
                        }
                    });

                    return achievementSetIsUpdated;
                },

                drawOwnedGames : function(){

                    // find game container, clone it, and add customization to it, add it to the list
                    var $gameItems = $("#achievements-games"),
                        countAchievements = 0,
                        countOriginPoints = 0,
                        totalAchievements = 0,
                        totalOriginPoints = 0,
                        entitlementTotal = 0;

                    // show my games section
                    $("section.play-games").toggle(true);

                    if ( settings.achievementTemplate == null ){
                        settings.achievementTemplate = $("#item-template").removeAttr("id").detach();
                    }

                    $(".item", $gameItems).remove();

                    $.each( settings.myAchievementSets, function( index, achievementSet ) {

                        if ( achievementSet.id.toLowerCase().indexOf("meta_") != 0
                             && achievementSet.id.toLowerCase() != "origin"
                             && achievementSet.achievements.length > 0 ) {

                            var $gameItem = settings.achievementTemplate.clone();
                            
                            $gameItem.show();
                            $gameItem.find( ".game-title" ).text( achievementSet.displayName );
                            $gameItem.find( ".link-achievements-game" ).attr( "data-game-id", encodeURI(achievementSet.id)  );
                            $gameItem.find( ".game-origin-points" ).text( achievementSet.earnedXp );
                            $gameItem.find( ".game-origin-points-total" ).text( achievementSet.totalXp );
                            $gameItem.find( ".game-achievements-points" ).text( achievementSet.achieved );
                            $gameItem.find( ".game-achievements-total" ).text( achievementSet.total );
                            $gameItem.data("achievement-set-id", achievementSet.id);

                            $.each( entitlementManager.topLevelEntitlements, function( index, entitlement ) {
                                if ( entitlement.achievementSet &&
                                     entitlement.achievementSet.indexOf( achievementSet.id ) < 0 ) {
                                    $gameItem.addClass("game-revoked");
                                    return false;
                                }
                            });

                            $gameItems.append( $gameItem );
                            
                            methods.helpers.getAchievementArt(achievementSet.id);

                            totalAchievements = totalAchievements + achievementSet.total;
                            totalOriginPoints = totalOriginPoints + achievementSet.totalXp;
                            countAchievements = countAchievements + achievementSet.achieved;
                            countOriginPoints = countOriginPoints + achievementSet.earnedXp;
                        }

                    });

                    // Achievement Summary
                    $("#achievements-completed").text( countAchievements );
                    $("#achievements-total").text( totalAchievements );
                    $("#origin-points-earned").text( countOriginPoints );
                    $("#origin-points-total").text( totalOriginPoints );                    

                    // show or hide purchase text
                    $("#no-achievements").toggle( achievementManager.xp == 0 );
                },

                drawOtherGames : function(releasedSets) {

                    var $gamesList = $("#released-games");

                    // show other games section
                    $("section.other-games").toggle(true);
                    
                    if ( settings.releasedTemplate == null ){
                        settings.releasedTemplate = $("#released-template").removeAttr("id").detach();
                    }

                    $(".item", $gamesList).remove();

                    $.each(releasedSets, function(index, releasedSet){

                        if( releasedSet.achievementSetId.toLowerCase().indexOf("meta_") != 0 && 
                            releasedSet.achievementSetId.toLowerCase() != "origin") {

                            var $gameItem = settings.releasedTemplate.clone(),
                                location = "http://static.cdn.ea.com/ebisu/u/f/achievements/" 
                                            + releasedSet.achievementSetId + "/images/en_US/ad_" 
                                            + $.md5(releasedSet.achievementSetId + "overviewAchievement") + ".jpg",
                                img = $('.game-banner', $gameItem);

                            // set entitlements images at the Other Games section
                            img.css("background-image", "url(" + location + ")");

                            // set data to the Other Games items
                            $gameItem.attr("data-achievement-set-id", releasedSet.achievementSetId);
                            $gameItem.attr("data-master-id", releasedSet.masterTitleId);
                            $(".game-title > span", $gameItem).text(releasedSet.displayName);

                            $gamesList.append($gameItem);
                        }
                    });
                },

                unownedGames : function() {

                    var achvGamesId = [],
                        unownedGamesList = [];

                    // List of ids of all the achievements games
                    $.each(settings.releasedSets, function(i, game){ achvGamesId.push(game.achievementSetId); });
                    // Get the list of ids of the unowned games
                    $.each(settings.myAchievementSets, function(i, game){ 
                        if($.inArray(game.id, achvGamesId) !== -1){
                            achvGamesId.splice($.inArray(game.id, achvGamesId), 1); 
                        }
                    });

                    // Get list of objects of the unowned games
                    $.grep(settings.releasedSets, function(game) {
                        if($.inArray(game.achievementSetId, achvGamesId) !== -1)
                            unownedGamesList.push(game);
                    });

                    methods.helpers.drawOtherGames(unownedGamesList);
                },

                setInterval : function () {
                    var intents = 0,
                        interval = setInterval(function() {
                        if(!methods.helpers.getAchievementSets()) {
                            $("div.game-listing").removeClass("error-loading");
                            clearInterval(interval);
                            methods.helpers.drawAchievements();
                        } else if(++intents === 5) {
                            $("div.game-listing").addClass("error-loading");
                            clearInterval(interval);
                        }
                    }, 60000);
                },

                getAchievementSets : function() {
                    // Listing Achievement Games
                    settings.myAchievementSets = achievementManager.achievementSets?
                                                 achievementManager.achievementSets.sort( methods.helpers.sortByName ) : [],
                    settings.releasedSets = achievementManager.achievementSetReleaseInfo? 
                                            achievementManager.achievementSetReleaseInfo.sort( methods.helpers.sortByName ) : [];
                    return (settings.myAchievementSets.length == 0 && settings.releasedSets.length == 0);
                },

                drawAchievements : function(){
                    
                    if(methods.helpers.getAchievementSets()){
                        // Hide my games section
                        $("section.play-games").toggle(false);

                        // Hide unowned games
                        $("section.other-games").toggle(false);
                        $("div.game-listing").addClass("error-loading");

                        // Set time interval
                        methods.helpers.setInterval();

                        return false;                    
                    } 

                    $("div.game-listing").removeClass("error-loading");

                    if(settings.myAchievementSets.length == 0) {
                        // Hide my games section
                        $("section.play-games").toggle(false);

                        // Set unowned games
                        methods.helpers.drawOtherGames(settings.releasedSets);                        

                    } else if( settings.myAchievementSets.length >= settings.releasedSets.length ||
                               settings.releasedSets.length == 0) {
                        // Set owned games
                        methods.helpers.drawOwnedGames();
                        
                        // Hide other games section
                        $("section.other-games").toggle(false);

                    } else {
                        // Set owned games
                        methods.helpers.drawOwnedGames();

                        // Set unowned games
                        methods.helpers.unownedGames();
                    }
                },

                generateBannerKey : function( originId ){
                    var originUserId = originId.replace(/[^a-zA-Z0-9]/g, "");
                    if ( originUserId.length > 0 ){
                        return "achievementBanner" + originUserId;
                    } else {
                        return "";
                    }
                },

                sortByName : function ( achievementSet1, achievementSet2) {
                    if ( achievementSet1.displayName.toLowerCase() > achievementSet2.displayName.toLowerCase() ) return 1;
                    if ( achievementSet1.displayName.toLowerCase() < achievementSet2.displayName.toLowerCase() ) return -1;
                    return 0;
                }

            } // END helpers


        };// END: methods

 
        // initialization
        
        var publicMethods = [ "init" ];     
        
        if ( typeof options === 'object' || ! options ) {

            methods.init(); // initializes plugin

        } else if ( $.inArray( options, publicMethods ) ) {
            // call specific methods with arguments
            
        };



    };  // END: $.achievements = function() {
        


})( jQuery ); // END: (function( $ ){


$(document).ready(function() {
    $.fn.achievements({});
});
