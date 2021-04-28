;(function( $, undefined ) {

    "use strict";

    $.fn.achievementDetails = function( options, parameters  ) {

        var settings = {
            buttonTop : null,
            navigationHeaderHeight : 0,
            achievementTemplate : null,
            albumSetTemplate : null,
            expandedState : null,

            achievementSetId : null,
            selectedAchievementSet : null,
            gameTitle : null,
            selectedEntitlement : null,
            currentAchievementSet : null,
            achievementLastUpdate : null,
            isInIgo : false,
            isCurrentUser : true,
            userId : null,
            productId : null
        };

        // bindings
        var methods = {

            init : function() {

                $.extend( settings, options );

                if ( window.achievementManager == null ) { return false; }

                if ( $.getQueryString("inigo") == "true" ){
                    settings.isInIgo = true;
                };

                $("div.wrapper").removeClass("game-loading");

                // setup for the 'back to top' button
                settings.navigationHeaderHeight = parseInt( $(".nav-wrapper").outerHeight() );
                settings.buttonTop = $(".btn-back");

                settings.achievementSetId = $.getQueryString("id");
                settings.userId = $.getQueryString("userid") ? $.getQueryString("userid") : originSocial.currentUser.id;
                
                settings.gameTitle = $.getQueryString("gameTitle");
                
                // find entitlement so we can initiate purchasing for DLC's

                $.each( entitlementManager.topLevelEntitlements, function( index, entitlement ){
                    if ( entitlement.achievementSet &&  entitlement.achievementSet.indexOf( settings.achievementSetId ) > -1 ){
                        settings.selectedEntitlement = entitlement;
                        return false;
                    }
                });
                if ( settings.userId != null && settings.userId != "" && settings.userId != originSocial.currentUser.id )
                {
                    settings.isCurrentUser = false;
                    // check if userid is part of user rooster
                    if ( !originSocial.getRosterContactById( settings.userId ) ){ return; }
                    var achievementPortfolio = originSocial.getRosterContactById( settings.userId ).updateAchievements();

                    // go through portfolio to see if game achievement set exist, 
                    //if not request it and wait for it to be loaded before drawing achievements

                    var achievementSetExists = false;
                    $.each( achievementPortfolio.achievementSets, function( index, achievementSet ){
                        if ( achievementSet.id == settings.achievementSetId ){
                           achievementSetExists = true;
                           settings.currentAchievementSet = achievementSet;
                        }
                    });

                    if ( achievementSetExists ){
                        methods.helpers.initialLoadSetup();

                    } else {
                        // add watcher for achievement added signal so we know when to render results
                        achievementPortfolio.achievementSetAdded.connect( function( achievementSet ){

                            if ( achievementSet.id == settings.achievementSetId ){
                               settings.currentAchievementSet = achievementSet;
                               methods.helpers.initialLoadSetup();
                            }
                        });

                        achievementPortfolio.updateAchievementSet( settings.achievementSetId );

                    }

                    // set the someone's name at the title bar
                    $('body').addClass("achievements-friend-album comparison-view");
                    methods.helpers.stringSubstitution( settings.userId );

                    // Load Comparison View
                    $("#album-comparison").html("").load("achievement-comparison.html");

                    $("#tab-comparison").on("click", function() {
                        methods.events.telemetryComparisonView();
                        $(".album-navigation > .selected").removeClass('selected');
                        $(".album-navigation > #tab-comparison").addClass('selected');
                        $("body").addClass('comparison-view');
                    });

                    $("#tab-achievements").on("click", function() {
                        methods.events.telemetryAchievementDetailsView();
                        $(".album-navigation > .selected").removeClass('selected');
                        $(".album-navigation > #tab-achievements").addClass('selected');
                        $("body").removeClass('comparison-view');
                    });

                } else {                    
                    settings.currentAchievementSet = achievementManager
                                                        .getAchievementSet( settings.achievementSetId );
                    if ( settings.currentAchievementSet != null )
                    {
                        methods.helpers.initialLoadSetup();
                    }
                }

                // event bindings for the page
                $(".link-profile").on( "click", methods.events.clickProfile );
                $("#dropdown-sort").on( "change", methods.events.changeSort );
                $("body > .outer-wrapper > .wrapper").on("scroll", methods.events.showTopButton );
                
                $(document).on( "click", ".button-buy-now", methods.events.clickBuyNow );
                $(document).on( "click", ".button-info", methods.events.clickInfo );
                $(document).on( "click", ".button-more-or-less", methods.events.clickMoreOrLess );

                settings.buttonTop.on("click", methods.events.clickTopButton );

                onlineStatus.onlineStateChanged.connect( function(state){
                    methods.helpers.viewSelection();
                });

                $.each( achievementManager.achievementSets, function( index, achievementSet ){

                    // if there's an update signal check to see if any achievment set failed update.
                    // if any failed show GOS offline page
                    // if none failed redraw page.

                    achievementSet.updated.connect( function( ) {
                        methods.helpers.viewSelection();
                    });
                });

                $(".offline-gos-mode .button-refresh").on("click", function(){
                    $.each( achievementManager.achievementSets, function( index, achievementSet ) {
                        achievementManager.updateAchievementSet( achievementSet.id )
                    });
                });

            }, //END init

            events : { 

                changeSort : function( event ){

                    methods.helpers.drawAchievements();

                },

                showTopButton : function( event ){

                    settings.buttonTop.toggle( settings.navigationHeaderHeight > $('.container:first').offset().top );

                },

                clickTopButton : function( event ){
                   $('.wrapper:first').scrollTo({top:'0px', left:'0px'}, 500);
                },

                clickProfile : function ( event ){

                    event.preventDefault();
                    clientNavigation.showMyProfile("ACHIEVEMENTS");

                },

                clickMoreOrLess : function ( event ){
                    var index = $(this).attr("button-index");
                    var state = settings.expandedState[index];

                    if ( state == "collapsed" ){ settings.expandedState[index] = "expanded"; }
                    if ( state == "expanded" ) { settings.expandedState[index] = "collapsed"; }


                    $(this)
                        .toggleClass("button-more")
                        .toggleClass("button-less")
                        .parent()
                            .toggleClass("expandable");

                },

                clickBuyNow : function ( event ){
                    var expansionId = $(this).data( "expansion-id" );

                    $(settings.selectedEntitlement.expansions).each( function( expansionIndex, expansion ){
                        if ( expansion.expansionId == expansionId ){
                            telemetryClient.sendAchievementWidgetPurchaseClick( expansion.productId );
                            expansion.purchase();
                        }
                    });

                    $(settings.selectedEntitlement.addons).each( function( addonsIndex, addon ){
                        if ( addon.expansionId == expansionId ){
                            telemetryClient.sendAchievementWidgetPurchaseClick( addon.productId );
                            addon.purchase();
                        }
                    });

                },

                clickInfo : function ( event ){
                    if ( settings.selectedEntitlement && settings.selectedEntitlement.hasPDLCStore ){
                        telemetryClient.sendAchievementWidgetPageView( "ExpansionStore", settings.selectedEntitlement.id, settings.isInIgo );
                        // Changed productId to masterId as the solution of the bug:
                        // https://developer.origin.com/support/browse/EBIBUGS-25173
                        clientNavigation.showPDLCStore( settings.selectedEntitlement.masterTitleId );                        
                    } else {
                        clientNavigation.showStoreHome();
                    }
                },
                
                telemetryComparisonView : function (){

                    if ( settings.selectedEntitlement ){
                        telemetryClient.sendAchievementComparisonView( settings.selectedEntitlement.productId, true );
                    } else {
                        var userProductsQueryOperation = originSocial.getUserById( settings.userId ).queryOwnedProducts();

                        userProductsQueryOperation.succeeded.connect( function( products ){

                            $.each( products, function ( index, product ){
                                if ( product.achievementSetId.indexOf( settings.achievementSetId ) > -1 ){
                                    settings.productId = product.productId;

                                }
                            });

                            telemetryClient.sendAchievementComparisonView( settings.productId, false );
                        });
                        userProductsQueryOperation.getUserProducts();
                    }
                },
                
                telemetryAchievementDetailsView : function(){

                    if ( settings.selectedEntitlement ){
                        telemetryClient.sendAchievementWidgetPageView( "Game", settings.selectedEntitlement.productId, settings.isInIgo );
                    } else {
                        var userProductsQueryOperation = originSocial.getUserById( settings.userId ).queryOwnedProducts();

                        userProductsQueryOperation.succeeded.connect( function( products ){

                            $.each( products, function ( index, product ){
                                if ( product.achievementSetId.indexOf( settings.achievementSetId ) > -1 ){
                                    settings.productId = product.productId;
                                }
                            });

                            telemetryClient.sendAchievementWidgetPageView( "Game", settings.productId, settings.isInIgo );
                        });
                        userProductsQueryOperation.getUserProducts();
                    }

                }

            }, // END events

            helpers : {

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


                initialLoadSetup : function (){

                    // Telemetry tracking
                    if ( settings.isCurrentUser ){                        
                        methods.events.telemetryAchievementDetailsView();
                    } else {
                        methods.events.telemetryComparisonView();
                    }

                    settings.sortedAchievements = settings.currentAchievementSet.achievements;
                    settings.sortedAchievements.sort( methods.helpers.sortByCompletionDate );
                    
                    // set up array to maintain expanded and unexpanded states.
                    settings.expandedState = new Array( settings.currentAchievementSet.expansions.length );
                    $.each( settings.currentAchievementSet.expansions, function( index, expansion ) {
                        if ( expansion.achievements.length > 8  && settings.currentAchievementSet.expansions.length > 1 )
                        {
                            settings.expandedState[index] = 'collapsed'
                        }
                    });

                    // needs to run through check here.
                    methods.helpers.viewSelection();

                    // if an achievement was granted, redraw achievements to show it.
                    $.each( settings.sortedAchievements, function( index, achievement ){
                        achievement.achievementGranted.connect( function() {
                            methods.helpers.drawAchievements();
                        });
                    });

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
                        methods.helpers.drawAchievements();
                    }

                },

                sortByName : function (achievement1, achievement2) {
                    if ( achievement1.name.toLowerCase() > achievement2.name.toLowerCase() ) return 1;
                    if ( achievement1.name.toLowerCase() < achievement2.name.toLowerCase() ) return -1;
                    return 0;
                },

                sortByOriginPoints : function (achievement1, achievement2) {

                    var achievementXp1 = achievement1.xp != "--" ? parseInt( achievement1.xp ) : -1;
                    var achievementXp2 = achievement2.xp != "--" ? parseInt( achievement2.xp ) : -1;

                    if ( achievementXp1 > achievementXp2 ) return -1;
                    if ( achievementXp1 < achievementXp2 ) return 1;
                    if ( achievementXp1 == achievementXp2 )
                    {
                        if ( achievement1.name.toLowerCase() > achievement2.name.toLowerCase() ) { return 1; }
                        if ( achievement1.name.toLowerCase() < achievement2.name.toLowerCase() ) { return -1; }
                        return 0;
                    }
                },

                sortByCompletionDate : function (achievement1, achievement2) {

                  // organized by not hidden and hidden
                  // achieved and not achieved
                  // complete date

                    if ( achievement1.xp == "--" && achievement2.xp != "--" ) 
                    {
                        return 1;
                    } else if ( achievement1.xp != "--" && achievement2.xp == "--" )
                    {
                        return -1;
                    } else if ( achievement1.xp == "--" && achievement2.xp == "--" ) 
                    {
                        return 0;
                    } else if ( achievement1.xp != "--" && achievement2.xp != "--" ) 
                    {

                        if ( achievement1.achieved && !achievement2.achieved )
                        {
                            return -1;
                        } else if ( !achievement1.achieved && achievement2.achieved )
                        {
                            return 1;
                        } else if ( !achievement1.achieved && !achievement2.achieved ) 
                        {

                            if ( achievement1.name.toLowerCase() > achievement2.name.toLowerCase() ) return 1;
                            if ( achievement1.name.toLowerCase() < achievement2.name.toLowerCase() ) return -1;
                            return 0;

                        } else if ( achievement1.achieved && achievement2.achieved ) 
                        {

                            if ( achievement1.updateTime < achievement2.updateTime ) return 1;
                            if ( achievement1.updateTime > achievement2.updateTime ) return -1;
                            return 0;

                        }

                    }

                },


                drawAchievements : function(){

                    if ( settings.gameTitle != null && settings.gameTitle != "" ){
                        $("#game-title").text( settings.gameTitle );
                    } else {
                        $("#game-title").text( settings.currentAchievementSet.displayName );
                    }

                    if ( settings.achievementTemplate == null ){
                        settings.achievementTemplate = $("#item-template").removeClass("item-wrapper").detach();
                    }

                    if ( settings.albumSetTemplate == null ){
                        settings.albumSetTemplate = $("#album-set").removeAttr("id").detach();
                    }


                    var $albumCompletion = $(".album-completion");

                    $albumCompletion.find( ".game-achievements" ).text( settings.currentAchievementSet.achieved );
                    $albumCompletion.find( ".game-achievements-total" ).text( settings.currentAchievementSet.total );
                    $albumCompletion.find( ".game-origin-points" ).text( settings.currentAchievementSet.earnedXp );
                    $albumCompletion.find( ".game-origin-points-total" ).text( settings.currentAchievementSet.totalXp );

                    $("#album #album-content > .row").remove();

                    $.each( settings.currentAchievementSet.expansions, function( index, expansion )
                    {

                        var $albumSet = settings.albumSetTemplate.clone();

                        var $achievements = $albumSet.find(".album-items");

                        $albumSet.find( ".expansion-name h2" ).text( expansion.displayName );
                        $albumSet.find( ".game-achievements" ).text( expansion.achieved );
                        $albumSet.find( ".game-achievements-total" ).text( expansion.total );
                        $albumSet.find( ".game-origin-points" ).text( expansion.earnedXp );
                        $albumSet.find( ".game-origin-points-total" ).text( expansion.totalXp );


                        var potentialText = $albumSet.find( ".pts-potential-includes" ).text();
                        potentialText = potentialText.replace( '%1', expansion.total );
                        potentialText = potentialText.replace( '%2', expansion.totalXp );
                        $albumSet.find( ".pts-potential-includes" ).text( potentialText );

                        $albumSet.find(".album-stats")
                            .toggleClass( "owned", expansion.owned )
                            .toggleClass( "unowned", !expansion.owned );

                        if ( settings.selectedEntitlement ){

                            var showCommerce = false;

                            $(settings.selectedEntitlement.expansions).each( function( expansionIndex, entitlemementExpansion ){
                                if ( entitlemementExpansion.expansionId ==  expansion.id && entitlemementExpansion.purchasable ){
                                    showCommerce = true;
                                }
                            });

                            $(settings.selectedEntitlement.addons).each( function( addonsIndex, entitlementAddon ){
                                if ( entitlementAddon.expansionId ==  expansion.id && entitlementAddon.purchasable ){
                                    showCommerce = true;
                                }
                            });

                            if ( !showCommerce )
                            {
                                $albumSet.find( ".button-buy-now" ).hide();
                            } else {
                                $albumSet.find( ".button-buy-now" ).data( 'expansion-id', expansion.id );
                            }

                            /*  Special case just for Battlefield 3 Limited Edition.
                            If they own it, Back to Karkland is included. */
                            if ( settings.selectedEntitlement.productId == 'DR:224766400' && expansion.id == '71191' )
                            {
                                $albumSet.find(".album-stats")
                                    .addClass( "owned" )
                                    .removeClass( "unowned" );
                            }

                        } else {
                            $albumSet.find( ".button-buy-now" ).hide();
                        }


                        if ( settings.expandedState[index] == 'collapsed' )
                        {
                            $achievements
                                .addClass("expandable")
                                    .find(".button-more-or-less")
                                    .addClass("button-more")
                                    .removeClass("button-less")
                                    .attr("button-index",  index)
                                    .show();
                        } else if ( settings.expandedState[index] == 'expanded' ){
                            $achievements
                                .removeClass("expandable")
                                    .find(".button-more-or-less")
                                    .addClass("button-less")
                                    .removeClass("button-more")
                                    .attr("button-index",  index)
                                    .show();
                        }

                        var dropDownValue = $("#dropdown-sort").val();
                        var achievementsClone = expansion.achievements;

                        if ( dropDownValue == "name" ){
                            achievementsClone.sort( methods.helpers.sortByName );
                        } else if ( dropDownValue == "origin-points" ){
                            achievementsClone.sort( methods.helpers.sortByOriginPoints );
                        } else if ( dropDownValue == "completion-date" ){
                            achievementsClone.sort( methods.helpers.sortByCompletionDate );
                        }

                        $.each( achievementsClone, function( index, achievement )
                        {
                            var $achievementItem = settings.achievementTemplate.clone(),
                                $progressBarWrapper = $achievementItem.find(".progress-bar-wrapper"),
                                $progressSummary = $achievementItem.find(".progress-summary");
                            $achievementItem.attr( "id", "achievement_" + achievement.id );
                            $achievementItem.attr( "class", "item-wrapper" );

                            $achievementItem.find(".item-name").text( achievement.name );
                            $achievementItem.find(".item-total-number" ).text( achievement.xp );

                            // display howto when the achievement is hidden
                            if ( achievement.xp != "--" )
                            {
                                $achievementItem.find(".info").text( achievement.description );
                            } else
                            {
                                $achievementItem.find(".info").text( achievement.howto );
                            }


                            if ( achievement.achieved  )
                            {
                                $achievementItem.find(".item-date").text( dateFormat.formatLongDate( achievement.updateTime ) );
                                $progressBarWrapper.addClass('complete-bar');
                                $progressSummary.hide();
                            }
                            else
                            {
                                $achievementItem.find(".item-content").addClass("item-disabled");
                                $achievementItem.find(".item-date").html("");

                                if( achievement.percent == 0)
                                {
                                    $progressBarWrapper.hide();
                                    $progressSummary.hide();
                                }
                                else
                                {
                                    $achievementItem.find(".progress-bar .status-bar" )
                                        .css({ width: parseInt(achievement.percent * 100) + '%'});
                                    $(".total", $progressSummary).text( parseInt(achievement.percent * 100) );
                                }
                            }

                            var img = new Image();
                            img.onerror = function ( evt ){ /* do nothing default to background */ }
                            img.onload = function ( evt ){ $achievementItem.find(".item-icon").attr({ src : achievement.getImageUrl(208) }); }

                            // sizes are 40, 208, 416
                            img.src = achievement.getImageUrl(208);

                            $achievements.append( $achievementItem );
                        });

                        $("#album-content").append( $albumSet );

                    });

                },

                stringSubstitution : function( userId ) {
                    // String Substitution{0}
                    var anotherUser = originSocial.getUserById(userId);
                    if ( anotherUser ) {
                        $('.achievements-title-someone').text( 
                            $.formatPython( $('.achievements-title-someone:first').text(),
                                    anotherUser.nickname ) );
                    }
                }


            } // END helpers


        };// END: methods

 
        // initialization

        var publicMethods = [ "init" ];     
        
        if ( typeof options === 'object' || ! options ) 
        {
            methods.init(); // initializes plugin
        }
        else if ( $.inArray( options, publicMethods ) )
        {
            // call specific methods with parameters
        };

    };  // END: $.achievementDetails = function() {



})( jQuery ); // END: (function( $ ){


$(document).ready(function() {
    $.fn.achievementDetails({});
});
