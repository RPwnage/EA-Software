/**
* @jquery.profile.user.js
* Script used to retrieve and display information about a person's profile
* It is broken into a number of parts. 
*
* 1. Profile User: inital loading for a users profile information.
* 2. Profile Friends: Loading users contacts, then loading each friends OP.
* 3. Profile Games: Loading users games, then using the games information to determine
*    which games has achievements to build out the achievements widget.
*    The boxart for each game is then loaded.
* 4. Profile Achievements: load and fill out the achievements that was 
*    scaffolding that was build in the games section.
*
* This is used only in the index.html
*
* TODO: Generalize a function that determine if the show/hide button for
* each module.
*
*/

;(function ($, undefined) {

    'use strict';

    $.fn.userProfile = function (options, parameters) {

        var settings = {
            userId : 0,
            profileUser : null,
            achievementPortfolio : null,
            achievementTemplate : null,
            containerAchievementAchieved : null,
            containerAchievementPoints : null
        };

        var subscriptionState = {
            "BOTH" : true,
            "FROM" : true,
            "NONE" : false,
            "TO" : false
        };

        var $profileInfo,
            isFriend = false,
            isCurrentUser = true,
            achievementSetsMap = {},
            $body = $("body");

        // bindings
        var methods = {

            init : function () {

                $.extend(settings, options);
                // Get the Profile User
                settings.userId =  $.getQueryString('id');
                $profileInfo = $('#profile-info');

                // Disable edit profile button if we are in OIG.
                // originClient\dev\common\source\UIScope.h
                if (window.helper && window.helper.context() === 1) {
                    $('#btn-edit-profile').addClass('disabled');
                }                

                // Offline mode
                methods.helpers.checkOnlineState();
                onlineStatus.onlineStateChanged.connect(function (state) {
                    methods.helpers.checkOnlineState();
                });

                // check if current user or remote user
                methods.helpers.isCurrentUser();
                if ( isCurrentUser ) {
                    $body.addClass('profile-user');
                    settings.profileUser = originSocial.currentUser;

                    // if current user show Learn More button
                    $('#achievements-summary .btn-achievements-overview')
                        .show().on('click', methods.events.goToOriginHelp);

                    // Edit Profile Button Event
                    $('#btn-edit-profile').on('click', methods.events.loadEditProfile);

                } else {
                    methods.helpers.loadRemoteUserOptions();
                }
                
                // Current User Visibility
                originSocial.currentUser.visibilityChanged.connect(function () {
                    var container = isCurrentUser ? $profileInfo : $("#" + originSocial.currentUser.id);
                    methods.helpers.updateStatus(container, originSocial.currentUser);
                });

                originSocial.connection.changed.connect(function() {
                    methods.helpers.connectionChanged ();
                });

                methods.helpers.loadBasicProfile();
                methods.helpers.loadExtendedProfile();

                // Commonly used dom objects
                settings.containerAchievementAchieved = $('#achievements-achieved .achievements-total');
                settings.containerAchievementPoints = $('#achievements-earned-points .achievements-total'); 

                // Contacts Section
                $('.album-content-fold span').on('click', methods.events.toggleFoldClick);               

            }, //END init

            events : {

                //Accept / Ignore friend request
                acceptFriendRequest : function (event) {
                    event.preventDefault();
                    if (!$('#btn-chat').attr("disabled")) {
                        settings.profileUser.acceptSubscriptionRequest();
                        $body.removeClass('profile-stranger profile-friend-pending-request')
                                          .addClass('profile-friend'); 
                    } 
                },

                blockUser : function (event) {
                    event.preventDefault();
                    //since we don't have the ability to disable links, use the state of the button
                    //to determine if request to chat server is disabled
                    if (!$('#btn-chat').attr("disabled")) {
                        settings.profileUser.block();
                    }
                },                

                goToAchievements : function (event) {
                    clientNavigation.showAchievements();
                },

                goToOriginHelp : function (event) {
                    event.preventDefault();
                    clientNavigation.showOriginHelp();
                },

                ignoreFriendRequest : function (event) {
                    event.preventDefault();
                    if (!$('#btn-chat').attr("disabled")) {
                        settings.profileUser.rejectSubscriptionRequest();
                        $body.removeClass('profile-friend-pending-request');
                    }
                },

                linkProfileClick : function (event) {
                    event.preventDefault();
                    if ( isCurrentUser ) {
                        clientNavigation.showMyProfile("MY_PROFILE");
                    } else if (isFriend) {
                        clientNavigation.showMyProfile("FRIENDS_PROFILE");
                    } else {
                        clientNavigation.showMyProfile("NON_FRIENDS_PROFILE");
                    }                    
                },

                loadEditProfile : function (event) {
                    clientNavigation.showMyAccount();
                },
               
                reportUser : function (event) {
                    event.preventDefault();
                    //since we don't have the ability to disable links, use the state of the button
                    //to determine if request to chat server is disabled
                    if (!$('#btn-chat').attr("disabled")) {
                        settings.profileUser.reportTosViolation();
                    }
                },

                removeFriend : function (event) {
                    event.preventDefault();
                    //since we don't have the ability to disable links, use the state of the button
                    //to determine if request to chat server is disabled
                    if (!$('#btn-chat').attr("disabled")) {
                        originSocial.roster.removeContact(event.data.friend);
                    }
                },

                sendFriendRequest : function (event) {
                    event.preventDefault();
                    if (!$(this).attr("disabled")) {
                        event.data.friend.requestSubscription();
                        $body.addClass('profile-friend-requested');
                    }
                },

                startConversation: function (event) {
                    if (!$(this).attr("disabled")) {
                        settings.profileUser.startConversation();
                    }
                },

                toggleFoldClick : function (event) {
                    var $album = $(this).closest('.album');

                    if ($album.hasClass('show-list')){
                        $('#content').stop().scrollTo($album, 500, function () {
                            $album
                                .toggleClass('show-list')
                                .toggleClass('hide-list');
                        });
                    } else {
                        $album
                            .toggleClass('show-list')
                            .toggleClass('hide-list');
                    }

                },               

                unblockUser : function (event) {
                    event.preventDefault();
                    //since we don't have the ability to disable links, use the state of the button
                    //to determine if request to chat server is disabled
                    if (!$('#btn-chat').attr("disabled")) {
                        settings.profileUser.unblock();
                    }
                },

                unfriendBlockUser : function (event) {
                    event.preventDefault();
                    //since we don't have the ability to disable links, use the state of the button
                    //to determine if request to chat server is disabled
                    if (!$('#btn-chat').attr("disabled")) {
                        settings.profileUser.removeAndBlock();
                    }
                }

            }, // END events

            helpers : {
                
                checkOnlineState : function () {
                    // needs to run through check here.
                    if (!onlineStatus.onlineState) {
                        $body.addClass('offline-mode');
                    } else {
                        $body.removeClass('offline-mode');
                    }
                },

                connectionChanged : function () {
                    if (originSocial.connection.established && originSocial.roster.hasLoaded)
                    {
                        //enable the buttons again
                        $(".origin-button-social").removeAttr("disabled");
                    }
                    else
                    {
                        if (onlineStatus.onlineState)
                        {
                            $(".origin-button-social").attr("disabled", "disabled");
                        }
                    }
                },

                isCurrentUser : function () {
                    isCurrentUser = settings.userId === originSocial.currentUser.id;                    
                },

                // Sets profile avatar, presence, display name;
                loadBasicProfile : function () {

                    // Presence
                    methods.helpers.updateStatus($profileInfo, settings.profileUser);
                    settings.profileUser.presenceChanged.connect(function () {
                        methods.helpers.updateStatus($profileInfo, settings.profileUser);
                    });

                    // Avatar
                    if (settings.profileUser.largeAvatarUrl) {
                        $('img.profile-avatar', $profileInfo).attr({ src : settings.profileUser.largeAvatarUrl });
                    } else {
                        settings.profileUser.requestLargeAvatar();
                    }

                    settings.profileUser.largeAvatarChanged.connect(function () {
                        $('img.profile-avatar', $profileInfo).attr({ src : settings.profileUser.largeAvatarUrl });
                    });

                    // Nickname
                    if (settings.profileUser.nickname){
                        $('.display-name').text(settings.profileUser.nickname);
                        methods.helpers.stringSubstitution();
                    }
                    // Real Name
                    if (settings.profileUser.realName) {
                        methods.helpers.setRealName();
                    } else {
                        settings.profileUser.requestRealName();
                    }

                    settings.profileUser.nameChanged.connect(function () {
                        if (settings.profileUser.realName) {
                            methods.helpers.setRealName();
                        }
                        
                        if (settings.profileUser.nickname) {
                            $(".display-name").text(settings.profileUser.nickname);
                        }

                        methods.helpers.stringSubstitution();
                    });

                    // Event Bindings

                    // Menu Navigation
                    $('.main-nav .link-profile').on('click', methods.events.linkProfileClick);

                    // Achievement Section
                    $('.link-achievements, .user-total-points').on('click', methods.events.goToAchievements);
                },

                loadExtendedProfile : function () {

                    // Create Friends
                    $.fn.friendsList( { 
                        profileUser : settings.profileUser, 
                        isFriend : isFriend
                    });

                    // Create Game
                    $.fn.gamesList( { 
                        profileUser : settings.profileUser,
                        isCurrentUser : isCurrentUser
                    });

                    // Create Achievements
                    $.fn.achievementsList( { 
                        profileUser : settings.profileUser,
                        isCurrentUser : isCurrentUser,
                        userId : settings.userId
                    });
                },

                // Sets remote user menu options:
                // block/unblock, unfriend, report, etc. 
                loadRemoteUserOptions : function () {
                    
                    settings.profileUser = originSocial.getUserById(settings.userId);

                    // check if friend (being re-evaluted).
                    isFriend = subscriptionState[settings.profileUser.subscriptionState.direction];

                    // pending friend request
                    settings.profileUser.subscriptionState.pendingContactApproval ?
                        $body.addClass('profile-friend-request-sent') : false;

                    // Accept / Ignore friend request
                    settings.profileUser.subscriptionState.pendingCurrentUserApproval ?
                        $body.addClass('profile-friend-pending-request') : false;

                    // if friend and not blocked                    
                    isFriend ? $body.addClass('profile-friend') : $body.addClass('profile-stranger');

                    // if user is blocked
                    settings.profileUser.blocked ? $body.addClass('profile-blocked') : false;

                    // Set friend's name to the section titles
                    if (settings.profileUser.nickname){
                        $('.none-user-name').text(settings.profileUser.nickname);
                        methods.helpers.stringSubstitution();
                    }

                    // watcher for friend status change
                    settings.profileUser.subscriptionStateChanged.connect(function(){

                        if( subscriptionState[settings.profileUser.subscriptionState.direction] ) {
                            $body.removeClass().addClass('profile-friend');
                        } else {
                            $body.removeClass().addClass('profile-stranger');
                            
                            // TODO Create a method
                            //Accept / Ignore friend request                            
                            settings.profileUser.subscriptionState.pendingContactApproval ?
                                $body.addClass('profile-friend-request-sent') : false;
                            
                            settings.profileUser.subscriptionState.pendingCurrentUserApproval ?
                                $body.addClass('profile-friend-pending-request') : false;
                        }
                    });

                    // TODO: Update the list of friends when added or removed
                    // avoid trigger this event before accept the friend request
                    // would be better to have a list of friends and a list of pending Approval
                    /*
                    settings.profileUser.addedToRoster.connect(function(){
                        // Current behavior this signal is fired when the user have a new friend Request
                        // adding that request to the list of current friends (roster)
                        // with the difference that the property subscriptionState.pendingContactApproval = false
                        // or subscriptionState.pendingCurrentUserApproval = false
                    });
                    */
                    
                    //Remove contact from the friend list
                    settings.profileUser.removedFromRoster.connect(function(){
                        var id = "#" + settings.profileUser.id;
                        $("#" + settings.profileUser.id).remove();
                    });
                    
                    // watches for blocked status change
                    settings.profileUser.blockChanged.connect( function() {
                        if( settings.profileUser.blocked ) {
                            $body.addClass('profile-blocked');
                        } else {
                            $body.removeClass('profile-blocked');
                        }
                    });

                    // Event Bindings

                    // Chat Button
                    $('#btn-chat').on('click', methods.events.startConversation);

                    // Unblock User
                    $('#unblock-user, #btn-unblock', $profileInfo).on('click', methods.events.unblockUser);

                    // Add / Remove Friend
                    $('#btn-add-user', $profileInfo).on('click', { "friend" : settings.profileUser }, methods.events.sendFriendRequest);
                    $('#remove-user', $profileInfo).on('click', { "friend" : settings.profileUser }, methods.events.removeFriend);

                    // Remove friend and Block
                    $('#unfriend-block-user', $profileInfo).on('click', methods.events.unfriendBlockUser);

                    // Accept / Ignore friend request
                    $("#stranger-pending-request .accept-request", $profileInfo).on('click', methods.events.acceptFriendRequest);
                    $("#stranger-pending-request .ignore-request", $profileInfo).on('click', methods.events.ignoreFriendRequest);

                    // Report User
                    $('#report-user').on('click', methods.events.reportUser);

                    //check and see if we're disconnected from the chat server, if so, disable buttons
                    if (!originSocial.connection.established) {
                        //disable the buttons
                        $(".origin-button-social").attr("disabled", "disabled");
                    }
					
					// reload block list, in case it has changed
					settings.profileUser.refreshBlockList();
                },

                setRealName : function() {
                    var firstName = settings.profileUser.realName.firstName ? $.trim(settings.profileUser.realName.firstName) : "",
                        lastName =  settings.profileUser.realName.lastName ? $.trim(settings.profileUser.realName.lastName) : "";
                    if (firstName.length > 0 || lastName.length > 0) {
                        $('#real-name').text(firstName + " " + lastName);
                    } else {
                        settings.profileUser.requestRealName();
                    }
                },

                stringSubstitution : function () {

                    // %s
                    $('#friends-list .album-header-title-someone').text($.formatC($('#friends-list .album-header-title-someone-text:first').text(), settings.profileUser.nickname));

                    $('#friends-list .private-message').text($.formatC($('#friends-list .private-message-text:first').text(), settings.profileUser.nickname));
                    $('#games-list .private-message').text($.formatC($('#games-list .private-message-text:first').text(), settings.profileUser.nickname));


                    // {0}
                    $('#achievements-list .album-header-title-someone').text($.formatPython($('#achievements-list .album-header-title-someone-text:first').text(), settings.profileUser.nickname));
                    $('#games-list .album-header-title-someone').text($.formatPython($('#games-list .album-header-title-someone-text:first').text(), settings.profileUser.nickname));

                    // %1
                    $('#achievements-list .private-message').text($.formatVB($('#achievements-list .private-message-text:first').text(), settings.profileUser.nickname));
                },

                updateStatus : function (container, contact) {
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

                    var statusClass = "";

                    // TODO: status structure can be simplified so that class addition and remove can be done more cleanly.

                    if (contact.visibility === 'INVISIBLE') {
                        statusClass = profileStatusStates.INVISIBLE;                        
                        contact.requestedAvailability = "UNAVAILABLE";
                    } else if (contact.playingGame) {
                        
                        if (contact.playingGame.broadcastUrl) {
                            statusClass = profileStatusStates.BROADCAST;
                            $('.status-broadcast-text', container).text(
                                $.formatVB( $('.status-broadcast-name-text', container).text(), 
                                            contact.playingGame.gameTitle));
                        } else {
                            statusClass = profileStatusStates.IN_GAME;
                            $('.status-in-game-text', container).text(
                                $.formatVB( $('.status-in-game-name-text', container).text(),
                                            contact.playingGame.gameTitle));
                        }

                    } else if (!contact.availability) {
                        statusClass = profileStatusStates.UNKNOWN;
                    } else {
                        statusClass = profileStatusStates[contact.availability];
                    }                    
                    
                    // Remove the current 'status-*' class of the contact container
                    if(container.attr("class")) {
                        var exp = new RegExp("status(.*)",'g');
                        exp = new Array(container.attr("class").match(exp));
                        container.removeClass(exp.toString());                        
                    }
                    container.addClass(statusClass);
                }

            } // END helpers
        };// END: methods

        // initialization

        var publicMethods = ["init"];

        if (typeof options === 'object' || !options) {

            methods.init(); // initializes plugin

        } else if ($.inArray(options, publicMethods)) {
            // call specific methods with arguments

        }

    };  // END: $.profile = function () {

})(jQuery); // END: (function ($) {


$(document).ready(function () {
    'use strict';
    $.fn.userProfile({});
});
