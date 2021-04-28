;(function($){

"use strict";
var groupGuid = $.getQueryString("groupGuid"), 
    thisGroup = originSocial.getGroupByGuid( groupGuid ),
    groupMembersModule = angular.module('GroupMembersModule', [ ]),
    handleConnectionChange,
    groupName = thisGroup.name;
    
// Object for representing dummy data
var DummyUser = function(data)
{
    this.dummy = true;
    this.id = data.id;
    this.nickname = ( (data.nickname && data.nickname.length>0) ? data.nickname : tr('ebisu_client_loading') );
    this.availability = data.availability;
    this.playingGame = data.playingGame;
};
    
groupMembersModule.controller( "GroupMembersCtrl", ["$scope", "$filter", "$timeout", function($scope, $filter, $timeout) {      
    var ADMIN_MAX_LIMIT = 20,
        MEMBERS_MAX_LIMIT = 99,
        ADMIN_INITIAL_LIMIT = 20,
        MEMBERS_INITIAL_LIMIT = 20,
        updateDisplayCount, onMemberLoadFinished, onRefresh, hideSpinner, createDummyArray,
        loadMoreMembers, activateViewport, activateData,
        checkLazyLoadSpinner, scrollThrottleFn, handleAnyUserChange;

    $scope.adminsMaxLimit = ADMIN_MAX_LIMIT;
    $scope.membersMaxLimit = MEMBERS_MAX_LIMIT;

    $scope.adminLimit = ADMIN_INITIAL_LIMIT;
    $scope.membersLimit = MEMBERS_INITIAL_LIMIT;
    
    $scope.membersArray = [];
    $scope.ownerAdminsArray = [];
    
    $scope.dummyArray = [];
    $scope.allMembersArray = [];

    $scope.groupMembersWindow = window.groupMembersWindow;

    $scope.getPresence = Origin.utilities.presenceStringForUser;

    $scope.getUserRoleString = function(user) {
        var role = thisGroup.getRemoteUserRole(user), roleString = "";
        if (role === "superuser") {
            roleString = tr('ebisu_client_owner');
        } else if (role === "admin") {
            roleString = tr('ebisu_client_admin');
        } else if (role === "member") {
            roleString = ""; // "Member" is not currently displayed in the UI
        }
        return roleString;
    };
    $scope.hasRealName = Origin.utilities.hasRealName;    
    $scope.getRealName = Origin.utilities.getRealName;     
    
    hideSpinner = function() {
        $('.group-members-window-spinner-container').hide();       
    }    
    
    // Increase the maximum display limits
    loadMoreMembers = function() {
        if (($scope.adminsMaxLimit < $scope.ownerAdminsCount) || ($scope.membersMaxLimit < $scope.membersCount)) {
            $scope.adminsMaxLimit = Math.min($scope.adminsMaxLimit+ADMIN_MAX_LIMIT, $scope.ownerAdminsCount);
            $scope.membersMaxLimit = Math.min($scope.membersMaxLimit+MEMBERS_MAX_LIMIT, $scope.membersCount);
            updateDisplayCount();
            $scope.membersArray = $scope.dummyArray.slice(0, $scope.membersLimit);                   
        } 
    };
    
    checkLazyLoadSpinner = function() {    
        // turn off animations, save CPU
        $timeout( function() { $("#members-lazy-loading-spinner").removeClass("animated"); }, 1000 );
        // If no more members to load, then hide the lazy load spinner
        $("#members-lazy-loading-spinner").toggle($scope.membersLimit !== $scope.allMembersArray.length);
    }
    
    activateData = function(start, end) {
        var size = end - start, membersViewport = [], temp = [];    
        
        // fill up array with dummy data
        temp = $scope.dummyArray.slice(0, $scope.membersLimit);
        
        // "bind" live data to viewport
        temp.splice(start, size, $scope.allMembersArray.slice(start, start+size));
        
        // flatten array
        membersViewport = membersViewport.concat.apply(membersViewport, temp);
        
        // apply to current scope
        $scope.membersArray = membersViewport;

        // hit scope.apply
        $timeout( function() { $scope.$apply(); }, 0 );
    };

    activateViewport = function() {
        var first_visible = null, previous = null, last_visible;
                
        $('.group-member-marker').each(function() {
            if (!first_visible) {
                if ( Origin.utilities.isElementAboveViewport(this) ) {                    
                    first_visible = ( !!previous ? previous : $(this).attr("data-index") );
                }
            } else {
                last_visible = $(this).attr("data-index");
                if ( Origin.utilities.isElementBelowViewport(this) ) {                                                        
                    return false;
                }                
            }
            previous = $(this).attr("data-index");
        });
        
        if (typeof last_visible === "undefined") last_visible = previous;
        if (last_visible === "max") last_visible = $scope.membersLimit;
        
        activateData(parseInt(first_visible), parseInt(last_visible)+1);
    };

    // Update the display limits based on the counts
    updateDisplayCount = function() {
        $scope.ownerAdminsCount = $scope.ownerAdminsArray.length;
        $scope.membersCount = $scope.allMembersArray.length;        
        
        if ($scope.ownerAdminsCount >= $scope.adminsMaxLimit) {
            $scope.adminLimit = $scope.adminsMaxLimit;
        } else {
            $scope.adminLimit = $scope.ownerAdminsCount;
        }
                    
        if ($scope.membersCount >= $scope.membersMaxLimit) {
            $scope.membersLimit = $scope.membersMaxLimit;
        } else {
            $scope.membersLimit = $scope.membersCount;
        }
    };
                
    createDummyArray = function() {
        var owners = $filter('orderBy')(thisGroup.owners, 'nickname'),
            admins = $filter('orderBy')(thisGroup.admins, 'nickname');
        $scope.ownerAdminsArray = owners.concat(admins);        
        
        $scope.allMembersArray = $filter('orderBy')(thisGroup.members, 'nickname');
        
        $scope.dummyArray = [];
        $.each($scope.allMembersArray, function(i, el) {
            $scope.dummyArray.push(new DummyUser(originSocial.getUserById(el.id)));
        });            
    }

    // Handle disconenctions from the XMPP server
    handleConnectionChange = function () {           
        if (!originSocial.connection.established) {            
            $(".offline-warning").show();
            $(".group-members-header").hide();
            $("#group-members-contact-list").hide();
        } // else no requirement to re-connect
    };
    
    originSocial.connection.changed.connect(handleConnectionChange);
    if (!originSocial.connection.established) {
        handleConnectionChange();
    }
    
    onMemberLoadFinished = function() {
        createDummyArray();        
        updateDisplayCount();
        $scope.membersArray = $scope.dummyArray.slice(0, $scope.membersLimit);                           
        hideSpinner();
        activateViewport();   
        checkLazyLoadSpinner();
    }

    onRefresh = function() {        
        // connect to membersLoadFinished signal    
        thisGroup.membersLoadFinished.connect(onMemberLoadFinished);    

        // call for member list query
        window.groupMembersWindow.callQueryGroupMembers(groupGuid);
    }
    
    // connect to refresh signal
    window.groupMembersWindow.refresh.connect(onRefresh);

    // connect to "anychange" signal
    handleAnyUserChange = function() {
        createDummyArray();        
        updateDisplayCount();
        $scope.membersArray = $scope.dummyArray.slice(0, $scope.membersLimit);                           
        activateViewport();   
    };
    
    thisGroup.anyChange.connect(Origin.utilities.throttle(handleAnyUserChange)); 
    
    originSocial.roster.contactAdded.connect(Origin.utilities.throttle(handleAnyUserChange));
    originSocial.roster.contactRemoved.connect(Origin.utilities.throttle(handleAnyUserChange));

            
    
    // If this group members are already loaded, then populate the bound arrays
    if (thisGroup.members.length > 0) {
        $timeout( function() {
            createDummyArray();        
            updateDisplayCount();
            $scope.membersArray = $scope.dummyArray.slice(0, $scope.membersLimit);                   
            hideSpinner();
            activateViewport();  
            checkLazyLoadSpinner();                
        }, 0 );
    };

    //trigger the refresh
    onRefresh();      
    
    $(document).ready(function()
    {                
        scrollThrottleFn = Origin.utilities.reverse_throttle(function(evt) {   
            if(Origin.utilities.isElementInViewport($("#members-lazy-loading-spinner")[0])) {
                loadMoreMembers();
                activateViewport();
                checkLazyLoadSpinner(); 
            }}, 300);             // 300 milliseconds reverse throttle

        // Set the dialog title
        $(".group-members-header").text(tr('ebisu_client_members_title').replace('%1', groupName)); 
        
        // Set up reverse-throttle binding on the scroll event. Activate viewport, and add members if at bottom
        $('#group-members-content').bind('scroll',function(evt) {            
            // Animate spinner if it is visible
            if(Origin.utilities.isElementInViewport($("#members-lazy-loading-spinner")[0])) {
                $("#members-lazy-loading-spinner").addClass("animated");
                scrollThrottleFn.apply();
            }
        }); 

        // Helper to add events on all users    
        var onMemberEvent = function(eventName, callback, selector)
        {
            $('section#group-members-content').on(eventName, selector, function(evt)
            {
                var $contact = $(this).closest("div.group-members-contact"),
                    user = Origin.views.remoteUserForElement($contact), role;

                if (user) {                
                    role = thisGroup.getRemoteUserRole(user);
                    
                    // Grab the event
                    evt.stopImmediatePropagation();
                    evt.preventDefault();

                    // Call our inner callback
                    callback.call(this, user, role);
                }
            });    
        };

        onMemberEvent('contextmenu', function (user, role)
        {
            $("div.group-members-contact.showing-menu").removeClass("showing-menu");
            Origin.GroupMembersContextMenuManager.popup(user, groupGuid, role, thisGroup.role);
            $(this).addClass('showing-menu');
            return false;
        }, "div.group-members-contact");
        
        onMemberEvent('click', function (user, role)
        {
            $("div.group-members-contact.showing-menu").removeClass("showing-menu");
            Origin.GroupMembersContextMenuManager.popup(user, groupGuid, role, thisGroup.role);
            $(this).addClass('showing-menu');
            return false;
        }, "button.otk-hover-context-menu-button");
 
        onMemberEvent('mouseenter', function(user, role) { 
            if ($(this).children(".otk-otk-hover-context-menu-button").length === 0) {
                $(this).append("<button class='otk-hover-context-menu-button'></button>");
            }
        }, "div.otk-contact-mini");
        
        onMemberEvent('mouseleave', function(user, role) {
            $(this).find(".otk-hover-context-menu-button").remove();
        }, "div.otk-contact-mini");        
    });    
}]);

groupMembersModule.directive('groupMember', function() {
    return {
      restrict: 'E',
      scope: {
        member: "=",
        index: "@"
      },
      templateUrl: 'group-member-directive.html',
      link: function($scope, element, attrs) {
            $scope.getPresence = Origin.utilities.presenceStringForUser;
      }      
    };
});
  
}(jQuery));


