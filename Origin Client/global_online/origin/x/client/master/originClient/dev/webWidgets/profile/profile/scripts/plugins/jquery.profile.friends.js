/**
* @jquery.profile.friends.js
* Script used to retrieve and display information about a person's friends
*
* FriendsList: Loads users contacts, then loading each friends OP.
*
* This is called from the userProfile plugin
*/

;(function ($, undefined) {
    'use strict';

    $.fn.friendsList = function(options, paramenters) {

        var settings = {
            profileUser : null,
            isFriend : false,
            friendsList : $('#friends-list')
        };        
        
        var methods = {
            
            init : function() {

                $.extend(settings, options);

                // Create Friends
                var userContactsQueryOperation = settings.profileUser.queryContacts();

                if (userContactsQueryOperation) {
                    methods.helpers.setSignals(userContactsQueryOperation);                    
                } else {
                    settings.friendsList.addClass('error');
                    methods.helpers.setInterval(userContactsQueryOperation);
                }

                // Current User Visibility
                originSocial.currentUser.visibilityChanged.connect(function () {
                    var container = isCurrentUser ? $profileInfo : $("#" + originSocial.currentUser.id);
                    methods.helpers.updateStatus(container, originSocial.currentUser);
                });

            }, // END init

            events : {
                showProfile : function (event) {
                    if (settings.isFriend) {
                        event.data.contact.showProfile("FRIENDS_PROFILE");
                    } else {
                        event.data.contact.showProfile("NON_FRIENDS_PROFILE");
                    }
                }
            }, // END events

            helpers: {
                
                getFriendsList : function (contactsList) {

                    var $otherFriends = $('#friends-other'),
                        $commonFriends = $('#friends-common'),
                        $friendInfoTemplate = $('.friend-info').detach(),
                        $friendInfo,
                        sortedList,
                        rosterMap = {},
                        commonFriendsCount = 0;

                    sortedList = methods.helpers.sortListAsc(contactsList, 'nickname');                    

                    // more efficient to create a key map of current contacts than to constantly iterate over roster.
                    $.each(originSocial.roster.contacts, function (index, userContact) {

                        // Temporal solution until the addToRoster event is fixed
                        if( !userContact.subscriptionState.pendingContactApproval &&
                            !userContact.subscriptionState.pendingCurrentUserApproval ) {
                            rosterMap[userContact.id] = userContact.id;
                        }
                    });

                    $.each(sortedList, function (index, contact) {

                        $friendInfo = $friendInfoTemplate.clone();

                        $friendInfo.attr({ id : contact.id });

                        // Contact nickname
                        $('.friend-name', $friendInfo).text(contact.nickname);

                        // Contact avatar
                        $('.friend-avatar', $friendInfo).attr({ src: contact.avatarUrl });

                        // Contact Portfolio
                        var portfolio = contact.updateAchievements();
                        portfolio ? methods.helpers.setContactPortfolio(portfolio, $friendInfo) : null;
                        
                        // Contact Status
                        methods.helpers.updateStatus( $friendInfo, contact );

                        // Contact type
                        if ( rosterMap[contact.id] ) {
                            $commonFriends.append($friendInfo);
                            commonFriendsCount++;
                        } else {
                            // add to strangers
                            $otherFriends.append($friendInfo);
                        }

                        //Events listeners
                        contact.presenceChanged.connect( function () {
                            var $contact = $("#" + contact.id);
                            methods.helpers.updateStatus( $contact, contact );
                        });

                        contact.avatarChanged.connect( function () {
                            var $contact = $("#" + contact.id);
                            $( ".friend-avatar", $contact ).attr({ src: contact.avatarUrl });
                        });

                        contact.nameChanged.connect( function () {
                            var $contact = $("#" + contact.id);
                            $( ".friend-name", $contact ).text(contact.nickname);
                        });

                        $friendInfo.on( "click", { "contact" : contact }, methods.events.showProfile );
                    });
                    
                    // Controls number of friends to show                    
                    methods.helpers.sectionManager(contactsList, $commonFriends, $otherFriends, commonFriendsCount);   
                },

                // TODO: Make this a general method
                sectionManager : function( contactsList, $commonFriends, $otherFriends, commonFriendsCount ) {

                    if (commonFriendsCount === 0) {
                        $(".friends-container").addClass("album-content-common-0");
                    } else if (commonFriendsCount >= 9) {
                        $(".friends-container").addClass("album-content-common-3");
                    } else {
                        $(".friends-container").addClass("album-content-common-" + Math.ceil(commonFriendsCount / 3));
                    }

                    $(".friends-container .list-items").each(function (index, friendList) {
                        $(".album-content-item", friendList).length === 0 ? $(friendList).hide() : null;
                    });

                    //TODO: needs to remove hidden class
                    var friendsTotal = Math.ceil($('.album-content-item', $otherFriends).length / 3)
                        + Math.ceil($('.album-content-item', $commonFriends).length / 3);
                    friendsTotal > 4 ? settings.friendsList.addClass('hide-list') : null;

                    $('#friends-common, #friends-other').each(function (index, section) {
                        $('.items-count-number', section).text($('.friend-info', section).length);
                    });

                    $('#friends-total').text(contactsList.length);
                    $('.items-count-number', $commonFriends).text(commonFriendsCount);
                    $('.items-count-number', $otherFriends).text(contactsList.length - commonFriendsCount);
                    
                    $('#friends-showing .section-rates-showing-all').text(
                        $.formatVB($('#friends-showing .section-rates-showing-all').text(), contactsList.length, contactsList.length));

                    $('#friends-showing .section-rates-showing-part').text(
                        $.formatVB($('#friends-showing .section-rates-showing-part').text(), 
                            $.unique($('.friends-container .album-content-item:visible')).length, contactsList.length));
                },

                setContactPortfolio : function ( portfolio, $friendInfo ) {
                    $friendInfo.attr('persona-id', portfolio.personaId);

                    methods.helpers.setContactXpPoints(portfolio.earnedXp, portfolio.available, $('.friend-points', $friendInfo));

                    portfolio.xpChanged.connect(function (xpValue) {
                        var $contact = $('.friend-info[persona-id="' + portfolio.personaId + '"]');
                        methods.helpers.setContactXpPoints(xpValue, portfolio.available, $('.friend-points', $contact));
                    });

                    portfolio.updatePoints();
                },

                setContactXpPoints : function (xpValue, availability, container) {
                    if (xpValue > 0 && availability) {
                        container.text(xpValue).removeClass('hidden');
                    } else {
                        container.addClass('hidden');
                    }
                },

                setInterval : function (userContactsQueryOperation) {
                    var intents = 0,
                        interval = setInterval(function() {
                        userContactsQueryOperation = settings.profileUser.queryContacts();
                        if(userContactsQueryOperation) {
                            settings.friendsList.removeClass('error');
                            clearInterval(interval);
                            methods.helpers.setSignals(userContactsQueryOperation);
                        } else if(++intents === 5) {
                            clearInterval(interval);
                        }
                    }, 60000);   
                },

                setSignals : function (userContactsQueryOperation) {
                    userContactsQueryOperation.succeeded.connect(function (contactsList) {
                        if (contactsList.length > 0) {
                            settings.friendsList.removeClass().addClass('album');
                            methods.helpers.getFriendsList(contactsList);
                        } else {
                            settings.friendsList.hide().removeClass().addClass('album');
                        }
                    });

                    userContactsQueryOperation.queryError.connect(function (httpStatusCode) {
                        if (httpStatusCode === 403) {
                            settings.friendsList.addClass('private');
                        }
                    });

                    userContactsQueryOperation.failed.connect(function () {
                        if (httpStatusCode === 403) {
                            settings.friendsList.addClass('private');
                        } else {
                            settings.friendsList.removeClass('private').addClass('error');
                            methods.helpers.setInterval(userContactsQueryOperation);
                        }
                    });

                    userContactsQueryOperation.getUserFriends();
                },

                sortListAsc : function (list, param) {
                    return list.sort(function (a, b) {
                        if (a[param].toLowerCase() < b[param].toLowerCase()) { return -1; }
                        if (a[param].toLowerCase() > b[param].toLowerCase()) { return 1; }
                        return 0;
                    });
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

                    if (contact.playingGame) {
                        
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

                    } else if (contact.visibility === 'INVISIBLE') {
                        statusClass = profileStatusStates.INVISIBLE;                        
                        contact.requestedAvailability = "UNAVAILABLE";
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
            }  // END helpers
        }; // END: methods
        
        // initialization

        var publicMethods = ["init"];

        if (typeof options === 'object' || !options) {

            methods.init(); // initializes plugin

        } else if ($.inArray(options, publicMethods)) {
            // call specific methods with arguments
        }
    };
    
})(jQuery);
