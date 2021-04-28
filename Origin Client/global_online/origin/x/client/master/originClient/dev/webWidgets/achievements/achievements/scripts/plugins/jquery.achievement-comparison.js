;(function( $, undefined ) {

    "use strict";

    $.fn.achievementComparison = function( options, parameters ) {

        var settings = {
            cmpFriend : null
            , currentUser : null
            , userId : null
            , achievementPortfolio : null
            , currentUserAchSet : null
            , cmpFriendAchSet : null
            , lastScroll : 0
            , currentIndex : -1
            , headersAndTops : []
            , headersHeight : 0
            , tableTemplate : null
            , expansionTemplate : null
            , bodyWrapper : $("body > .outer-wrapper > .wrapper")
        };

    	// bindings
        var methods = {

            init : function() {
            	
            	$.extend( settings, options );
            	
            	$(".album-navigation > .selected").removeClass('selected');
                $(".album-navigation #tab-comparison").addClass('selected');

                settings.userId = $.getQueryString("userid");
                settings.cmpFriend = originSocial.getUserById(settings.userId);
                settings.currentUser = originSocial.currentUser;

                // Users Achievements Sets
                settings.cmpFriendAchSet = methods.helpers.getUserAchievementSet( settings.cmpFriend );
                settings.currentUserAchSet = methods.helpers.getUserAchievementSet();
                
                // Load the page
                methods.helpers.initialPageLoad();

                // Make the comparison between users achievements lists
                methods.helpers.compareAchievementsList();

                // Floating Header
                settings.headersHeight = parseInt( $("#cmp-main-header").height() );
                methods.helpers.setHeadersAndTops();
                settings.bodyWrapper.on("scroll", methods.events.showTopHeader);
                
            }, // END: init

            helpers : {

                getUserAchievementSet : function( friend ) {
                    // Returns the achievement set of the friend or the main user.
                    // through the achievement set id of the comparison game
                    var achievementSetId = $.getQueryString("id");

                    if (friend)
                        return friend.updateAchievements().getAchievementSet(achievementSetId);
                    else
                        return achievementManager.getAchievementSet( achievementSetId );
                },

                initialPageLoad : function(){

                    // Users Data
                    methods.helpers.setPlayerInfo( settings.cmpFriend, $('.user-wrapper.cmp-friend'), settings.cmpFriendAchSet );
                    methods.helpers.setPlayerInfo( settings.currentUser, $('.user-wrapper.current-user'), settings.currentUserAchSet );

                    // Nickname
                    $(".cmp-friend .user-name").text( settings.cmpFriend.nickname );
                    $(".current-user .user-name").text( settings.currentUser.nickname );

                    // Game Title
                    $(".cmp-table th .cmp-game-name").text($.getQueryString("gameTitle"));
                },                

                setPlayerInfo : function( player, $container, achievementSet ) {
                    // Avatar
                    if ( player.largeAvatarUrl ){
                        $("img.avatar", $container).attr({ src : player.largeAvatarUrl });
                    } else {
                        player.requestLargeAvatar();
                    }

                    player.largeAvatarChanged.connect( function(){
                         $("img.avatar", $container).attr({ src : player.largeAvatarUrl });
                    });

                    // Presence
                    methods.helpers.updateStatus( $container, player );

                    player.presenceChanged.connect( function() {
                        methods.helpers.updateStatus( $container, player );
                    });

                    // Visibility
                    originSocial.currentUser.visibilityChanged.connect(function(){
                        methods.helpers.updateStatus( $container, player );
                    });

                    // Set General Achievement Info
                    achievementSet ? 
                        methods.helpers.setAchievementInfo( achievementSet, $container ) : false;

                    // Go to Users profile
                    $container.on( "click", { "friend" : player }, methods.events.showProfile );
            	},

                setAchievementInfo : function(achievementSet, $container) {

                    // Total Points
                    $(".user-data .user-points", $container).text( achievementSet.earnedXp );

                    // Update Achievements Comparison
                    achievementSet.updated.connect( function(){
                        $(".user-data .user-points", $container).text( achievementSet.earnedXp );

                        $("#album-comparison .cmp-list").html("");
                        methods.helpers.compareAchievementsList();
                        methods.helpers.resetHeadersAndTops();
                    });
                },

            	updateStatus : function( $container, player ) {
                    var profileStatusStates = {
                        "AWAY": "status-away", // #DC453E
                        "CHAT" : "status-unknown", // none
                        "DND" : "status-unknown", // none
                        "ONLINE" : "status-online", // #87C95B
                        "UNAVAILABLE": "status-offline", // #B3B3B3
                        "XA" : "status-unknown", // none
                        "INVISIBLE" : "status-invisible",

                        "IN_GAME" : "status-in-game", // #02A0E2
                        "JOINABLE" : "status-in-game", // #02A0E2
                        "BROADCAST": "status-broadcast", //#6441A5

                        "UNKNOWN" : "status-unknown" // none
                    };

                    var statusClass ="";

                    // TODO: status structure can be simplified so that class addition and remove can be done more cleanly.

                    if ( player.playingGame ){
                        statusClass = player.playingGame.broadcastUrl ?
                            profileStatusStates.BROADCAST :
                            profileStatusStates.IN_GAME;

                    } else if ( !player.availability ){
                        statusClass = profileStatusStates.UNKNOWN;
                    } else {
                        statusClass = profileStatusStates[player.availability];
                    }
                    
                    if ( player.visibility === "INVISIBLE") {
                        statusClass = profileStatusStates.INVISIBLE;
                        player.requestedAvailability = "UNAVAILABLE";
                    }
                    $('div:first-child', $container).removeClass().addClass( statusClass );
                },

                // Compare based on the friend list
                compareAchievementsList : function() {
                    
                    settings.tableTemplate = settings.tableTemplate ? 
                                                settings.tableTemplate : 
                                                $(".cmp-table").detach();

                    settings.expansionTemplate = settings.expansionTemplate ?
                                                    settings.expansionTemplate :
                                                    $(".cmp-list .cmp-header").detach();
                    
                    var friendExpansionsList = settings.cmpFriendAchSet.expansions,
                        userExpansionsList = settings.currentUserAchSet ?
                                                settings.currentUserAchSet.expansions : false;

                    $.each( friendExpansionsList, function( index, expansion ){

                        var $expansion = settings.expansionTemplate.clone(),
                            $table = settings.tableTemplate.clone(),
                            $rowTemplate = $("tr.cmp-game-info", $table).detach(),
                            userExpansion, sortedList;

                        if( userExpansionsList ) {
                            userExpansion = settings.currentUserAchSet.getExpansion(expansion.id);
                            sortedList = methods.helpers.getSortedList(userExpansionsList[index].achievements, expansion.achievements);
                            $(".total-current-user", $expansion).text(userExpansion.earnedXp);
                        } else {
                            sortedList = methods.helpers.getSortedList(expansion.achievements, expansion.achievements);
                        }
                        
                        if( friendExpansionsList.length === 1 ) {
                            $("#cmp-main-header .cmp-first-column").addClass("name");
                            $("#cmp-main-header .cmp-game-name").text(expansion.displayName);
                            $(".cmp-list .cmp-header").addClass("hidden");
                        } else {                            
                            $(".cmp-game-name", $expansion).text(expansion.displayName);
                            $(".cmp-points", $expansion).text(expansion.totalXp);
                            $(".total-friend", $expansion).text(expansion.earnedXp);
                            $(".cmp-list").append($expansion);
                        }

                        $.each( sortedList, function(index, userAchievement) {
                            var $row = $rowTemplate.clone(), friendAchievement;

                            $(".item-icon", $row).attr({ src : userAchievement.getImageUrl(208) });
                            $('.item-content h4', $row).text( userAchievement.name );
                            $('.item-content p', $row).text( userAchievement.description );
                            $('.cmp-second-column .ach-points', $row).text(userAchievement.xp);

                            if( userExpansionsList ) {
                                friendAchievement = settings.cmpFriendAchSet.getAchievement(userAchievement.id);

                                if( userAchievement.achieved ) {
                                    $('.cmp-third-column > span', $row).addClass('check');
                                    $(".item-icon", $row).removeClass("item-disabled");
                                }
                            } else {
                                friendAchievement = userAchievement;
                            } 

                            friendAchievement.achieved ? 
                                $('.cmp-fourth-column > span', $row).addClass('check') : false;

                            $table.append($row);
                        });

                        $(".cmp-list").append($table);
                    });
                },

                getSortedList : function( userList, friendList ) {
                    var bothList = [], justUserList = [],
                        justFriendList = [], anyList = [], finalList = [];

                    userList.sort(methods.helpers.sortById);
                    friendList.sort(methods.helpers.sortById);

                    $.each(userList, function( index, userAchv ){
                        var friendAchv = friendList[index];
                        if( userAchv.achieved && friendAchv.achieved ) bothList.push(userAchv);
                        if( userAchv.achieved && !friendAchv.achieved ) justUserList.push(userAchv);
                        if( !userAchv.achieved && friendAchv.achieved ) justFriendList.push(userAchv);
                        if( !userAchv.achieved && !friendAchv.achieved ) anyList.push(userAchv);
                    });

                    bothList.sort(methods.helpers.sortByPoints);
                    justUserList.sort(methods.helpers.sortByPoints);
                    justFriendList.sort(methods.helpers.sortByPoints);
                    anyList.sort(methods.helpers.sortByPoints);

                    finalList = bothList.concat(justUserList, justFriendList, anyList);

                    return finalList;
                },

                sortByPoints : function( achievement1, achievement2) {

                    var achievementXp1 = achievement1.xp != "--" ? parseInt( achievement1.xp ) : -1,
                        achievementXp2 = achievement2.xp != "--" ? parseInt( achievement2.xp ) : -1;

                    if ( achievementXp1 > achievementXp2 ) return -1;
                    if ( achievementXp1 < achievementXp2 ) return 1;
                    if ( achievementXp1 == achievementXp2 )
                    {
                        if ( achievement1.name.toLowerCase() > achievement2.name.toLowerCase() ) { return 1; }
                        if ( achievement1.name.toLowerCase() < achievement2.name.toLowerCase() ) { return -1; }
                    }
                    return 0;
                },

                sortById : function (achievement1, achievement2) {
                    if ( achievement1.id.toLowerCase() > achievement2.id.toLowerCase() ) return 1;
                    if ( achievement1.id.toLowerCase() < achievement2.id.toLowerCase() ) return -1;
                    return 0;
                },

                resetHeadersAndTops : function() {
                    var currentScroll = settings.bodyWrapper.scrollTop() + settings.headersHeight;

                    $.each( $(".cmp-header"), function( index, item ) {
                        settings.headersAndTops[index].header = $(item);
                    });

                    if( currentScroll >= settings.headersAndTops[settings.currentIndex].top && settings.currentIndex >= 0 ) {
                        settings.headersAndTops[settings.currentIndex].header.addClass('sticky');
                    }
                },

                setHeadersAndTops : function() {
                    settings.currentIndex = 0;
                    $.each( $(".cmp-header"), function( index, item ) {
                        settings.headersAndTops.push({
                            header: $(item),
                            top: $(item).offset().top - settings.headersHeight
                        });
                    });
                },

                setHeader : function( currentScroll ) {
                    for(var i = 0; i < settings.headersAndTops.length; i++) {
                        if( currentScroll >= settings.headersAndTops[i].top ) {
                            settings.headersAndTops[settings.currentIndex].header.removeClass('sticky');
                            settings.headersAndTops[i].header.addClass('sticky');
                            settings.currentIndex = i; 
                        } else {
                            settings.headersAndTops[i].header.removeClass('sticky');
                        }
                        settings.lastScroll = currentScroll;
                    }
                }
            }, // END: helpers

            events : {
                showProfile : function( event ) {
                    event.data.friend.showProfile("ACHIEVEMENTS");
                },

                showTopHeader : function( event ){
                    var currentScroll = $(this).scrollTop() + settings.headersHeight;

                    clearTimeout( settings.currentTimeoutId );
                    settings.currentTimeoutId = setTimeout(function(){
                        methods.helpers.setHeader( currentScroll );
                    }, 10);
                }
            } // END: events

        }; // END: methods

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
    $.fn.achievementComparison({});
});
