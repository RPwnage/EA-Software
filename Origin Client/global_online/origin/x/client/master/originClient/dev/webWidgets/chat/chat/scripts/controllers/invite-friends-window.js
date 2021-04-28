;(function($){

"use strict";

var InviteDialogType = [ "Room", "Group", "Game" ], // ENUM InviteDialogType
    dialogType = InviteDialogType[window.friendInviteWindow.dialogType],
    isGameInviteDialog = (dialogType === "Game"),
    isGroupInviteDialog = (dialogType === "Group"),
    isRoomInviteDialog = (dialogType === "Room"),
    groupGuid = $.getQueryString("groupGuid"),
    channelId = $.getQueryString("channelId"),
    thisGroup = originSocial.getGroupByGuid( groupGuid ),
    conversationId = $.getQueryString("conversationId"),
    conversation = originChat.getConversationById(conversationId),
    inviteFriendsModule = angular.module('InviteFriendsModule', [ ]),
    selectedUsers = [],
    isUserInGroup = function(user) {
        return (thisGroup.isUserInGroup(user));
    },
    isUserFullySubscribed = function(user) {
        return (user.subscriptionState.direction === 'BOTH');
    },        
    isUserInGame = function(user) {
        return (user.playingGame);
    },
    isUserOnline = function(user) {
        return ((user.availability === 'ONLINE') || (user.availability === 'AWAY'));
    };
        
inviteFriendsModule.controller("RemoteUserCtrl", ['$scope', '$filter', '$timeout', function ($scope, $filter, $timeout) 
{    
    var DISPLAY_MAX_LIMIT = 20,
        updateArrays, updateDisplayCount, handleAnyUserChange, hideSpinner,
        onMemberLoadFinished, onRefresh, setupDialogElements, handleConnectionChange,
        allNotInGroupArray = [], alreadyInGroupArray = [], onlineNotInGameArray = [], orderByPresence,
        participantIdArray = [], onlineNotInRoomArray = [], alreadyInRoomArray = [], handleConversationEvent;
        
    handleConversationEvent = function(event) {
        if (event.type === "PARTICIPANT_ENTER" || event.type === "PARTICIPANT_EXIT") {
            participantIdArray = [];
            $timeout(function(){
                conversation.participants.forEach(function(user) {
                    participantIdArray.push(user.id);
                });
                
                updateDisplayCount();
                setupDialogElements();
            });            
        }
    }
    
    if (isRoomInviteDialog && !!conversation && !!conversation.participants) {
        conversation.participants.forEach(function(user) {
            participantIdArray.push(user.id);
        });
        
        // Handle all exisiting events
        conversation.events.forEach(handleConversationEvent);

        // Start listening for events
        conversation.eventAdded.connect(handleConversationEvent);        
    }
                
    $scope.contacts = originSocial.roster.contacts;
    
    $scope.onlineInGame = function(user)
    {
        return ( isUserOnline(user) && isUserInGame(user) && isUserFullySubscribed(user));
    }
    $scope.onlineNotInGame = function(user)
    {
        return ( isUserOnline(user) && !isUserInGame(user) && isUserFullySubscribed(user) );
    }
    $scope.onlineNotInGroup = function(user)
    {
        return ( isUserOnline(user) && !isUserInGroup(user) && isUserFullySubscribed(user));
    }
    $scope.inGroup = function(user)
    {
        return ( isUserInGroup(user) && isUserFullySubscribed(user) );
    }
    $scope.offlineNotInGroup = function(user)
    {
        return ( !isUserOnline(user) && !isUserInGroup(user) && isUserFullySubscribed(user));
    }
    $scope.offline = function(user)
    {
        return ( !isUserOnline(user) && isUserFullySubscribed(user) );
    }
    $scope.allNotInGroup = function(user)
    {
        return !isUserInGroup(user) && isUserFullySubscribed(user);
    }
    $scope.onlineNotInRoom = function(user)
    {
        return ( isUserOnline(user) && isUserFullySubscribed(user) && participantIdArray.indexOf(user.id) === -1 );
    }
    $scope.inRoom = function(user)
    {
        return (participantIdArray.indexOf(user.id) >= 0);
    }
    
    orderByPresence = function(user)
    {
        if ( (user.playingGame) || (user.availability === 'ONLINE') || (user.availability === 'AWAY') ) 
            return 0;

        return 1;
    }

    $scope.canInviteUserArray = [];
    $scope.inGameUserArray = [];
    $scope.inGroupUserArray = [];
    $scope.offlineUserArray = [];
    $scope.inRoomUserArray = [];

    updateArrays = function() {
    
        $scope.contacts = originSocial.roster.contacts;
        if (isGroupInviteDialog) 
        {
            allNotInGroupArray = $filter('filter')($scope.contacts, $scope.allNotInGroup);
            $scope.canInviteUserArray = $filter('orderBy')(allNotInGroupArray, [orderByPresence, 'nickname']);
            alreadyInGroupArray = $filter('filter')($scope.contacts, $scope.inGroup);
            $scope.inGroupUserArray = $filter('orderBy')(alreadyInGroupArray, [orderByPresence, 'nickname']);            
        } else if (isGameInviteDialog) 
        {
            onlineNotInGameArray = $filter('filter')($scope.contacts, $scope.onlineNotInGame);
            $scope.canInviteUserArray = $filter('orderBy')(onlineNotInGameArray, 'nickname');
            $scope.inGameUserArray = $filter('filter')($scope.contacts, $scope.onlineInGame);
            $scope.offlineUserArray = $filter('filter')($scope.contacts, $scope.offline);
        } else if (isRoomInviteDialog)
        {
            onlineNotInRoomArray = $filter('filter')($scope.contacts, $scope.onlineNotInRoom);
            $scope.canInviteUserArray = $filter('orderBy')(onlineNotInRoomArray, [orderByPresence, 'nickname']);
            $scope.offlineUserArray = $filter('filter')($scope.contacts, $scope.offline);
            alreadyInRoomArray = $filter('filter')($scope.contacts, $scope.inRoom);
            $scope.inRoomUserArray = $filter('orderBy')(alreadyInRoomArray, [orderByPresence, 'nickname']);
        }        
    }
    
    updateArrays();
                
    $scope.allContacts = [];
          
    $scope.getPresence = Origin.utilities.presenceStringForUser;
    
    $scope.displayMaxLimit = DISPLAY_MAX_LIMIT;

    $scope.canInviteLimit = 0;
    $scope.inGameLimit = 0;
    $scope.inGroupLimit = 0;
    $scope.offlineLimit = 0;
    $scope.inRoomLimit = 0;
    
    $scope.hasRealName = Origin.utilities.hasRealName;
    
    $scope.getRealName = Origin.utilities.getRealName;
    
    $scope.getStatusText = function(contact)
    {
        var subtitleText = "";

        if (!!contact.statusText)
        {
            subtitleText = contact.statusText;
        }
        else 
        {
            if (contact.availability === "ONLINE")
            {
                subtitleText = tr('ebisu_client_presence_online');
            }
            else if (contact.availability === "AWAY")
            {
                subtitleText = tr('ebisu_client_presence_away');
            }
            else if (contact.availability === "UNAVAILABLE")
            {
                subtitleText = tr('ebisu_client_offline');
            }            
        }    
        return subtitleText;    
    }
    
    $scope.click = function(evt, contact) {
        var $item = $(evt.srcElement).closest("div.otk-contact");
        $item.toggleClass("selected");

        if( $item.hasClass("selected") ) {
            selectedUsers.push(contact);
        } else {
            selectedUsers = $.grep(selectedUsers, function(user, index) { 
                if (contact === user) {
                    return false;
                }
                return true;
            });
        }
        
        $("button#inviteButton").toggleClass("disabled", selectedUsers.length===0);
    }    
    
    updateDisplayCount = function() {
        updateArrays();
        
        $scope.canInviteCount = $scope.canInviteUserArray.length;
        $scope.inGameCount = $scope.inGameUserArray.length;
        $scope.inGroupCount = $scope.inGroupUserArray.length;
        $scope.inRoomCount = $scope.inRoomUserArray.length;
        $scope.offlineCount = $scope.offlineUserArray.length;
        
        if ($scope.inGameCount >= $scope.displayMaxLimit) {
            $scope.inGameLimit = $scope.displayMaxLimit;
        } else {
            $scope.inGameLimit = $scope.inGameCount;
            if ($scope.canInviteCount >= $scope.displayMaxLimit) {
                $scope.canInviteLimit = $scope.displayMaxLimit;
            } else {
                $scope.canInviteLimit = $scope.canInviteCount;
                var remaining = $scope.displayMaxLimit - $scope.canInviteCount;
                
                // This is getting unwieldy!! Look into refactoring this TR
                if (isGroupInviteDialog) {
                    if ($scope.inGroupCount >= remaining) {
                        $scope.inGroupLimit = remaining;
                    } else {
                        $scope.inGroupLimit = $scope.inGroupCount;
                        remaining = remaining - $scope.inGroupCount;

                        if ($scope.offlineCount >= remaining) {
                            $scope.offlineLimit = remaining;
                        } else {
                            $scope.offlineLimit = $scope.offlineCount;
                        }                
                    }        
                } else if (isRoomInviteDialog) {
                    if ($scope.inRoomCount >= remaining) {
                        $scope.inRoomLimit = remaining;
                    } else {
                        $scope.inRoomLimit = $scope.inRoomCount;
                        remaining = remaining - $scope.inRoomCount;

                        if ($scope.offlineCount >= remaining) {
                            $scope.offlineLimit = remaining;
                        } else {
                            $scope.offlineLimit = $scope.offlineCount;
                        }                
                    }        
                }
                
            }        
        }
    }
    
    handleAnyUserChange = function() {
        //handle anyUserChange.
        $timeout(function() {
            updateDisplayCount();
            setupDialogElements();
        });
    };        
    originSocial.roster.anyUserChange.connect(Origin.utilities.throttle(handleAnyUserChange));

    originSocial.roster.contactAdded.connect(Origin.utilities.throttle(handleAnyUserChange));
    originSocial.roster.contactRemoved.connect(Origin.utilities.throttle(handleAnyUserChange));
    
    
    // Handle disconenctions from the XMPP server
    handleConnectionChange = function () {           
        if (!originSocial.connection.established) {            
            setupDialogElements();
        } // else no requirement to re-connect
    };
    
    originSocial.connection.changed.connect(handleConnectionChange);
    if (!originSocial.connection.established) {
        handleConnectionChange();
    }
    
        
    hideSpinner = function() {
        $('#invite-friends-remote-users').show();
        $('.invite-friends-window-spinner-container').hide();
    }
    
    onMemberLoadFinished = function() {
		//only need to get member change once in this window, so disconnect.
	    thisGroup.membersLoadFinished.disconnect(onMemberLoadFinished);
        hideSpinner();        
        
        $timeout(function() {
            updateDisplayCount();
            setupDialogElements();
        });
     }
    
    $scope.loadMoreFriends = function() {        
        $timeout(function() {
            $scope.displayMaxLimit = Math.min($scope.displayMaxLimit+DISPLAY_MAX_LIMIT, $scope.contacts.length);
            updateDisplayCount();
            setupDialogElements();
        });
    };
    
    onRefresh = function() {
        if (isGroupInviteDialog) {
            // connect to membersLoadFinished signal    
            thisGroup.membersLoadFinished.connect(onMemberLoadFinished);    

            // call for member list query
            window.friendInviteWindow.callQueryGroupMembers(groupGuid);
        } else if (isGameInviteDialog) {
            hideSpinner();
            updateDisplayCount();
            setupDialogElements();
        } else if (isRoomInviteDialog) {
            hideSpinner();
            updateDisplayCount();
            setupDialogElements();
        
        }
    }
    
    setupDialogElements = function() {
        // Number of users eligible to select to invite
        var canInviteCount = ($scope.canInviteCount + $scope.inGameCount);
        
        // check if offline
        var isConnected = originSocial.connection.established;

        // friends lists
        $("#invite-friends-remote-users").toggle(isConnected);

        if(isGameInviteDialog)
        {
            $("#invite-friends-header").text(tr("ebisu_client_invite_friends_to_play"));
            $("#invite-friends-description").text(tr("ebisu_client_select_friends_to_join_your_game"));
        }
        else if (isGroupInviteDialog)
        {
            $("#invite-friends-header").text(tr("ebisu_client_invite_friends_to_become_members_caps"));
            $("#invite-friends-description").text(tr("ebisu_client_select_friends_to_invite_them_to_become_members"));
        } 
        else if (isRoomInviteDialog) 
        {
            $("#invite-friends-header").text(tr("ebisu_client_invite_friends_to_join_you_in_this_chat_room"));
            $("#invite-friends-description").text(tr("ebisu_client_select_friends_to_invite_them_into_this_chat_room"));
        }

        // show header and description for group invite
        $("#invite-friends-header").toggle(canInviteCount > 0 && isConnected);
        $("#invite-friends-description").toggle(canInviteCount > 0 && isConnected);
        $("#select-friends-subtitle-label").toggle( (isGroupInviteDialog || isRoomInviteDialog) && canInviteCount > 0 && isConnected );
        $("#cancelButton").toggle( isGameInviteDialog || ( (isGroupInviteDialog || isRoomInviteDialog) && canInviteCount > 0) && isConnected);

        // show header and description for game invite
        $("#online-subtitle-label").toggle( isGameInviteDialog && isConnected);

        // all friends are already in group
        $("#all-friends-in-group-header").toggle( (isGroupInviteDialog || isRoomInviteDialog) && canInviteCount === 0 && isConnected);
        $("#all-friends-in-group-description").toggle( (isGroupInviteDialog || isRoomInviteDialog) && canInviteCount === 0 && isConnected);
        $("#addFriendButton").toggle( (isGroupInviteDialog || isRoomInviteDialog) && canInviteCount === 0 && isConnected);
        $("#closeButton").toggle( ((isGroupInviteDialog || isRoomInviteDialog) && canInviteCount === 0) || !isConnected);
        if (!isConnected) {
            $("#closeButton").removeClass("secondary").addClass("primary");
        }
        
        // only show invite button if there are friends to invite
        $("#inviteButton").toggle( canInviteCount > 0 && isConnected);

        // offline warning message
        $("#offline-warning").toggle( !isConnected );    
    }
    
    // connect to refresh signal
    window.friendInviteWindow.refresh.connect(onRefresh);
    onRefresh();          
    
    $(document).ready(function()
    {    
        setupDialogElements();
        
        $("#buttons").on("click", "button#inviteButton:not(.disabled)", function() { 
            if (isGroupInviteDialog) {
                window.friendInviteWindow.inviteUsersToGroup(selectedUsers, groupGuid);            
            } else if (isGameInviteDialog) {
                window.friendInviteWindow.inviteUsersToGame(selectedUsers);            
            } else if (isRoomInviteDialog) {
                window.friendInviteWindow.inviteUsersToRoom(selectedUsers, groupGuid, channelId);
            }
        });

        $("#addFriendButton").on("click", function() { 
            clientNavigation.showFriendSearchDialog();
            window.close();
        });
        
        // scroll check for lazy load / infinite scroll. 
        $('#invite-friends-content').bind('scroll',function (evt) {                
            var $elem = $(evt.currentTarget);            
            if ($elem.scrollTop() + $elem.innerHeight() >= (0.95 * $elem[0].scrollHeight)) {
                $scope.loadMoreFriends();
            }            
        });
    });
    
}]);
  
}(jQuery));


