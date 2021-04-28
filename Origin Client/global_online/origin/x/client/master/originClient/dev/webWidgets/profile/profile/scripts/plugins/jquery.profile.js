
/**
* @jquery.profile.js
* Script used to retrieve and display information about a person's profile
* It can be broken into a number of parts. 
*
* 1. Inital loading for a users profile information.
* 2. Loading users contacts, then loading each friends OP.
* 3. Loading users games, then using the games information to determine
*    which games has achievements to build out the achievements widget.
*    The boxart for each game is then loaded.
* 4. Load achievements, and fill out the achievements that was 
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

    $.fn.profile = function (options, parameters) {

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
            achievementSetsMap = {},
            $body = $("body"),
            userContactsQueryOperation = null;

        // bindings
        var methods = {

            init : function () {

                $.extend(settings, options);
                // Get the Profile User
                settings.userId =  $.getQueryString('id');

                $profileInfo = $('#profile-info');

                // Disable edit profile button if we are in OIG.
                // originClient\dev\common\source\UIScope.h
                if (window.helper && window.helper.context() === 1)
                {
                    $('#btn-edit-profile').addClass('disabled');
                }

                // Offline mode
                methods.helpers.checkOnlineState();
                onlineStatus.onlineStateChanged.connect(function (state) {
                    methods.helpers.checkOnlineState();
                });

                // Current User Visibility
                originSocial.currentUser.visibilityChanged.connect(function () {
                    var container = methods.helpers.isCurrentUser() ? $profileInfo : $("#" + originSocial.currentUser.id);
                    methods.helpers.updateStatus(container, originSocial.currentUser);
                });

                originSocial.connection.changed.connect(function() {
                    methods.helpers.connectionChanged ();
                });

                // Commonly used dom objects
                settings.containerAchievementAchieved = $('#achievements-achieved .achievements-total');
                settings.containerAchievementPoints = $('#achievements-earned-points .achievements-total');

                // check if current user or remote user
                if (!methods.helpers.isCurrentUser()) {

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

                    // watches for friend status change
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
                    /*                    
                    settings.profileUser.removedFromRoster.connect(function(){
                        var id = "#" + settings.profileUser.id;
                        $("#" + settings.profileUser.id).remove();
                    });
                    */
                    
                    // watches for blocked status change
                    settings.profileUser.blockChanged.connect( function() {
                        if( settings.profileUser.blocked ) {
                            $body.addClass('profile-blocked');
                        } else {
                            $body.removeClass('profile-blocked');
                        }
                    });

                } else {

                    $body.addClass('profile-user');
                    settings.profileUser = originSocial.currentUser;

                    // if current user show Learn More button
                    $('#achievements-summary .btn-achievements-overview')
                        .show().on('click', methods.events.goToOriginHelp);
                }

                methods.helpers.loadBasicProfile();
                methods.helpers.loadExtendedProfile();

                // Event Bindings

                // Menu Navigation
                $('.main-nav .link-profile').on('click', methods.events.linkProfileClick);

                // Chat Button
                $('#btn-chat').on('click', methods.events.startConversation);

                // Edit Profile Button
                $('#btn-edit-profile').on('click', methods.events.loadEditProfile);

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

                // Contacts Section
                $('.album-content-fold span').on('click', methods.events.toggleFoldClick);

                // Achievement Section
                $('.link-achievements, .user-total-points').on('click', methods.events.goToAchievements);
                $body.on('click', '.achievement-game-item', methods.events.achievementCardClick);

                // Game Section
                $("body").on( "click", "#games-common .games-item", methods.events.gameCardClick );
                $("body").on( "click", "#games-other .games-item", methods.events.showStorePage );

                // Report User
                $('#report-user').on('click', methods.events.reportUser);

                //check and see if we're disconnected from the chat server, if so, disable buttons
                if (!originSocial.connection.established)
                {
                    //disable the buttons
                    $(".origin-button-social").attr("disabled", "disabled");
                }

            }, //END init

            events : {
                blockUser : function (event) {
                    event.preventDefault();
                    //since we don't have the ability to disable links, use the state of the button
                    //to determine if request to chat server is disabled
                    if (!$('#btn-chat').attr("disabled"))
                    {
                        settings.profileUser.block();
                    }
                },

                unblockUser : function (event) {
                    event.preventDefault();
                    //since we don't have the ability to disable links, use the state of the button
                    //to determine if request to chat server is disabled
                    if (!$('#btn-chat').attr("disabled"))
                    {
                        settings.profileUser.unblock();
                    }
                },

                unfriendBlockUser : function (event) {
                    event.preventDefault();
                    //since we don't have the ability to disable links, use the state of the button
                    //to determine if request to chat server is disabled
                    if (!$('#btn-chat').attr("disabled"))
                    {
                        settings.profileUser.block();
                    }
                },

                linkProfileClick : function (event) {
                    event.preventDefault();
                    if (methods.helpers.isCurrentUser()) {
                        clientNavigation.showMyProfile("MY_PROFILE");
                    } else if (isFriend) {
                        clientNavigation.showMyProfile("FRIENDS_PROFILE");
                    } else {
                        clientNavigation.showMyProfile("NON_FRIENDS_PROFILE");
                    }
                    
                },

                gameCardClick : function (event) {
                    event.preventDefault();
                    clientNavigation.showGameDetails($(this).data('content-id'));
                },
                
                reportUser : function (event) {
                    event.preventDefault();
                    //since we don't have the ability to disable links, use the state of the button
                    //to determine if request to chat server is disabled
                    if (!$('#btn-chat').attr("disabled"))
                    {
                        settings.profileUser.reportTosViolation();
                    }
                },
                
                showStorePage : function( event ) {
                    event.preventDefault();
                    // content-id is actually the product id. TODO: rename to product-id
                    clientNavigation.showStoreProductPage($(this).data("content-id"));
                },

                achievementCardClick : function (event) {
                    event.preventDefault();
                    clientNavigation.showAchievementSetDetails($(this).data('achievement-set-id'), settings.userId, $(this).find('.achievements-game-title').text());
                },

                loadEditProfile : function (event) {
                    clientNavigation.showMyAccount();
                },

                goToOriginHelp : function (event) {
                    event.preventDefault();
                    clientNavigation.showOriginHelp();
                },

                goToAchievements : function (event) {
                    clientNavigation.showAchievements();
                },

                goToAchievementSetDetails : function (event) {
                    var id = this.id;
                    clientNavigation.showAchievementSetDetails(id.substr(id.indexOf('-') + 1));
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

                removeFriend : function (event) {
                    event.preventDefault();
                    //since we don't have the ability to disable links, use the state of the button
                    //to determine if request to chat server is disabled
                    if (!$('#btn-chat').attr("disabled"))
                    {
                        originSocial.roster.removeContact(event.data.friend);
                    }
                },

                sendFriendRequest : function (event) {
                    event.preventDefault();
                    if (!$(this).attr("disabled"))
                    {
                        event.data.friend.requestSubscription();
                        $body.addClass('profile-friend-requested');
                    }
                },

                startConversation: function (event) {
                    if (!$(this).attr("disabled"))
                    {
                        settings.profileUser.startConversation();
                    }
                },

                showProfile : function (event) {
                    if (methods.helpers.isCurrentUser()) {
                        event.data.friend.showProfile("MY_PROFILE");				
                    } else if (isFriend) {
                        event.data.friend.showProfile("FRIENDS_PROFILE");
                    } else {
                        event.data.friend.showProfile("NON_FRIENDS_PROFILE");
                    }
                },

                //Accept / Ignore friend request
                acceptFriendRequest : function (event) {
                    event.preventDefault();
                    if (!$('#btn-chat').attr("disabled"))
                    {
                        settings.profileUser.acceptSubscriptionRequest();
                        $body.removeClass('profile-stranger profile-friend-pending-request')
                                        .addClass('profile-friend'); 
                    }
                },

                ignoreFriendRequest : function (event) {
                    event.preventDefault();
                    if (!$('#btn-chat').attr("disabled"))
                    {
                        settings.profileUser.rejectSubscriptionRequest();
                        $body.removeClass('profile-friend-pending-request');
                    }
                }

            }, // END events

            helpers : {

                isCurrentUser : function () {
                    return settings.userId === originSocial.currentUser.id;
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

                },

                loadExtendedProfile : function () {

                    // Create Friends
                    userContactsQueryOperation = settings.profileUser.queryContacts();

                    if (userContactsQueryOperation) {
                        userContactsQueryOperation.succeeded.connect(function (contactsList) {
                            $('#friends-list').removeClass('album-loading');
                            methods.helpers.getFriendsList(contactsList);
                        });

                        userContactsQueryOperation.queryError.connect(function (httpStatusCode) {
                            if (httpStatusCode === 403) {
                                $('#friends-list').addClass('private');
                            }
                        });

                        userContactsQueryOperation.failed.connect(function () {
                            $('#friends-list').removeClass('album-loading');
                            $('#friends-list').addClass('error');
                        });

                        userContactsQueryOperation.getUserFriends();
                    }

                    // Create Game
                    var userProductsQueryOperation = settings.profileUser.queryOwnedProducts();

                    if (userProductsQueryOperation) {

                        userProductsQueryOperation.succeeded.connect(function (products) {
                            $('#games-list').removeClass('album-loading');
                            methods.helpers.getGames(products);
                        });

                        userProductsQueryOperation.failed.connect(function () {
                            $('#games-list').removeClass('album-loading').addClass('error');
                        });

                        userProductsQueryOperation.queryError.connect(function (httpStatusCode) {
                            if (httpStatusCode === 403) {
                                $('#games-list, #achievements-list').addClass('private');
                            }
                        });
                        userProductsQueryOperation.getUserProducts();
                    }                    

                    // Create Achievements
                    settings.achievementPortfolio = settings.profileUser.updateAchievements();

                    if (settings.achievementPortfolio) {
                        settings.achievementPortfolio.availableUpdated.connect(function () {
                            methods.helpers.getAchievements();
                        });

                        settings.achievementPortfolio.achievementSetAdded.connect(function (achievementSet) {
                            methods.helpers.getAchievements();
                        });

                        settings.achievementPortfolio.achievementGranted.connect(function () {
                            methods.helpers.getAchievements();
                        });

                        settings.achievementPortfolio.xpChanged.connect(function (xpValue) {
                            methods.helpers.getAchievements();
                        });

                        methods.helpers.getAchievements();
                    }

                }, // loadExtendedProfile();

                setRealName : function() {
                    var firstName = settings.profileUser.realName.firstName ? $.trim(settings.profileUser.realName.firstName) : "",
                        lastName =  settings.profileUser.realName.lastName ? $.trim(settings.profileUser.realName.lastName) : "";
                    if (firstName.length > 0 || lastName.length > 0) {
                        $('#real-name').text(firstName + " " + lastName);
                    } else {
                        settings.profileUser.requestRealName();
                    }
                },

                getGameArt : function (index, imageUrls, productId, gamesList) {

                    if (index >= imageUrls.length) {
                      //settings.gamesBoxArt[ productId ] = '';
                        var $currentSelectedCard = $('.games-item', gamesList).filter(function () {  return $(this).data('content-id') === productId; });
                        $('.game-art', $currentSelectedCard).attr('src', '/profile/images/default-boxart.jpg');
                        return;
                    }
                    var img = new Image();
                    img.src = imageUrls[index];
                    img.onload = function () {
                        // if game found find achievement and game product to fill
                        // TODO: make cleaner
                        var $selectedCard = $('.games-item', gamesList).filter(function () {  return $(this).data('content-id') === productId; });
                        $('.game-art', $selectedCard).attr('src', imageUrls[index]);
                        return;
                    };
                    img.onerror = function () {
                        methods.helpers.getGameArt(index + 1, imageUrls, productId, gamesList);
                    };

                },

                getAchievementArt : function (achievementSetId) {

                    var $achievementList = $('#achievements-list');
                    var achievementImageSrc = "http://static.cdn.ea.com/ebisu/u/f/achievements/" + achievementSetId + "/images/en_US/profile_" + $.md5(achievementSetId + "profileAchievement") + ".jpg";

                    var img = new Image();
                    img.src = achievementImageSrc;
                    img.onload = function () {
                        var $currentSelectedAchievement = $('.achievement-game-item', $achievementList).filter(function () {  return $(this).data('achievement-set-id') === achievementSetId; });
                        $('.achievements-game-art', $currentSelectedAchievement).css('background-image', 'url(' + achievementImageSrc + ')');
                        return;
                    };

                },

                sortListAsc : function (list, param) {
                    return list.sort(function (a, b) {
                        if (a[param].toLowerCase() < b[param].toLowerCase()) { return -1; }
                        if (a[param].toLowerCase() > b[param].toLowerCase()) { return 1; }
                        return 0;
                    });
                },

                setContactXpPoints : function (xpValue, availability, container) {
                    if (xpValue > 0 && availability) {
                        container.text(xpValue).removeClass('hidden');
                    } else {
                        container.addClass('hidden');
                    }
                },

                getFriendsList : function (contactsList) {

                    var $otherFriends = $('#friends-other'),
                        $commonFriends = $('#friends-common'),
                        $friendInfoTemplate = $('.friend-info').detach(),
                        $friendInfo,
                        sortedList,
                        rosterMap = {},
                        commonFriendCount = 0;


                    sortedList = methods.helpers.sortListAsc(contactsList, 'nickname');
                    if (sortedList.length === 0) {
                        $('#friends-list').hide();
                        return;
                    }
                    
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

                        $friendInfo.attr({ id : contact.id });//, href : "index.html?id=" + contact.id });

                        // Friend nickname
                        $('.friend-name', $friendInfo).text(contact.nickname);

                        // friend avatar
                        $('.friend-avatar', $friendInfo).attr({ src: contact.avatarUrl });

                        var portfolio = contact.updateAchievements();

                        if (portfolio) {

                            $friendInfo.attr('persona-id', portfolio.personaId);

                            methods.helpers.setContactXpPoints(portfolio.earnedXp, portfolio.available, $('.friend-points', $friendInfo));

                            portfolio.xpChanged.connect(function (xpValue) {
                                var $contact = $('.friend-info[persona-id="' + portfolio.personaId + '"]');
                                methods.helpers.setContactXpPoints(xpValue, portfolio.available, $('.friend-points', $contact));
                            });

                            portfolio.updatePoints();
                        }

                        // Friend Status
                        methods.helpers.updateStatus( $friendInfo, contact );

                        if ( rosterMap[contact.id] ) {
                            $commonFriends.append($friendInfo);
                            commonFriendCount++;
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

                        $friendInfo.on( "click", { "friend" : contact }, methods.events.showProfile );
                    });

                    // TODO: Too much is happening in the next couple of sections. This needs to be simplfied
                    // controls number of friends to show
                    var commonFriendsCount = $("#friends-common .friend-info").length;
                    if (commonFriendsCount === 0) {
                        $(".friends-container").addClass("album-content-common-0");
                    } else if (commonFriendsCount >= 9) {
                        $(".friends-container").addClass("album-content-common-3");
                    } else {
                        $(".friends-container").addClass("album-content-common-" + Math.ceil(commonFriendsCount / 3));
                    }

                    $(".friends-container .list-items").each(function (index, friendList) {
                        if ($(".album-content-item", friendList).length === 0) {
                            $(friendList).hide();
                        }
                    });

                    //TODO: needs to remove hidden class
                    if ((Math.ceil($('.album-content-item', $otherFriends).length / 3)
                        + Math.ceil($('.album-content-item', $commonFriends).length / 3)) > 4) {
                        $('#friends-list').addClass('hide-list');
                    }

                    $('#friends-common, #friends-other').each(function (index, section) {
                        $('.items-count-number', section).text($('.friend-info', section).length);
                    });

                    //$('#showfriendrate .total-rate, #friends-total').html(contactsList.length);
                    $('#friends-total').text(contactsList.length);
                    $('#friends-common .items-count-number').text(commonFriendCount);
                    $('#friends-other .items-count-number').text(contactsList.length - commonFriendCount);
                    //$('#showfriendrate .rate').html($('.friends-container .friend-info:visible').length);

                    $('#friends-showing .section-rates-showing-all').text($.formatVB($('#friends-showing .section-rates-showing-all').text(), contactsList.length, contactsList.length));
                    $('#friends-showing .section-rates-showing-part').text($.formatVB($('#friends-showing .section-rates-showing-part').text(), $.unique($('.friends-container .album-content-item:visible')).length, contactsList.length));

                },

                getGames : function (products) {

                    var $gamesList = $('#games-list'),
                        $gameItemTemplate = $('.games-item', $gamesList).detach(),
                        $gamesCommon = $('#games-common'),
                        $gamesOther = $('#games-other'),
                        gamesMap = {},
                        commonGamesCount = 0,
                        uncommonGamesCount = 0,
                        $gameItem,
                        sortedProducts;

                    if (products.length === 0) {
                        $('#games-list').hide();
                        return;
                    }


                    // more efficient to create a key map of current entitlements
                    $.each(entitlementManager.topLevelEntitlements, function (index, entitlement) {
                        gamesMap[entitlement.id] = entitlement.id;
                    });

                    sortedProducts = methods.helpers.sortListAsc(products, 'title');

                    $.each(sortedProducts, function (index, product) {

                        // creates game card
                        $gameItem = $gameItemTemplate.clone();
                        $gameItem.data('content-id', product.productId);
                        $('.game-title', $gameItem).text(product.title);

                        if (gamesMap[product.productId]) {
                            $gamesCommon.append($gameItem);
                            commonGamesCount++;
                        } else if (!methods.helpers.isCurrentUser()) {
                            // add to strangers
                            uncommonGamesCount++;
                            $gamesOther.append($gameItem);
                        }

                    });

                    // TODO: Need to factor this out to a function. This is used in friends and achievements with different
                    // mod values
                    commonGamesCount = $('#games-common .games-item').length;
                    if (commonGamesCount === 0) {
                        $('.games-container').addClass('album-content-common-0');
                    } else if (commonGamesCount >= 9) {
                        $('.games-container').addClass('album-content-common-3');
                    } else {
                        $('.games-container').addClass('album-content-common-' + Math.ceil(commonGamesCount / 5));
                    }


                    $('.games-container .list-items').each(function (index, gameList) {
                        if ($('.album-content-item', gameList).length === 0) {
                            $(gameList).hide();
                        }
                    });

                    //TODO: needs to remove hidden class
                    if ((Math.ceil($('.album-content-item', $gamesOther).length / 5)
                        + Math.ceil($('.album-content-item', $gamesCommon).length / 5)) > 4) {
                        $('#games-list').addClass('hide-list');
                    }


                    // Get boxart
                    $.each(products, function (index, product) {
                        methods.helpers.getGameArt(0, product.boxartUrls, product.productId, $gamesList);
                    });

                    $('#showGamerate .rate, #games-total').text(uncommonGamesCount + commonGamesCount);
                    $('#games-common .items-count-number').text(commonGamesCount);
                    $('#games-other .items-count-number').text(uncommonGamesCount);

                    $('#games-showing .section-rates-showing-all').text($.formatVB($('#games-showing .section-rates-showing-all').text(), uncommonGamesCount + commonGamesCount, uncommonGamesCount + commonGamesCount));
                    $('#games-showing .section-rates-showing-part').text($.formatVB($('#games-showing .section-rates-showing-part').text(), $.unique($('.games-container .album-content-item:visible')).length, uncommonGamesCount + commonGamesCount));

                },

                getAchievements : function() {
                    if ( !settings.achievementPortfolio ) {
                        return false;
                    }

                    if (!settings.achievementPortfolio.available) {
                        $('#achievements-list').removeClass('album-loading').show().addClass('private');
                        return true;
                    }

                    var $achievementList = $('#achievement-common'),
                        $achievementItem;
                        
                    if ( !settings.achievementTemplate ) {
                        settings.achievementTemplate = $('.achievement-game-item').detach();
                    }

                    // create achievement card if achievement exists
                    if (settings.achievementPortfolio.achievementSets.length == 0) {
                        $('#achievements-list').hide();
                    } else {
                        $('#achievements-list').removeClass('album-loading').show();
                        $('#achievements-list .album-content-item').remove();
                        $('.achievement-common').show();

                        $.each(settings.achievementPortfolio.achievementSets, function (index, achievementSet ) {
                            if (achievementSet.id.indexOf('META_') < 0) {
                                $achievementItem = settings.achievementTemplate.clone();
                                $achievementItem.data('achievement-set-id', achievementSet.id);
                                $('.current-game-xp', $achievementItem).text(achievementSet.earnedXp);
                                $('.current-total-xp', $achievementItem).text(achievementSet.totalXp);
                                $('.achievements-game-title', $achievementItem).text(achievementSet.displayName);
                                $('.achievements-game-achieved', $achievementItem).text(achievementSet.achieved);
                                $('.achievements-total-achieved', $achievementItem).text(achievementSet.total);
                                $achievementList.append($achievementItem);
                                
                                methods.helpers.getAchievementArt( achievementSet.id);

                            }
                        });

                        settings.containerAchievementAchieved.text(settings.achievementPortfolio.achieved);
                        settings.containerAchievementPoints.text(settings.achievementPortfolio.earnedXp);

                        // TODO: Need to factor this out to a function. This is used in friends and achievements with different
                        // mod values
                        var commonAchievementsCount = $('#achievement-common .achievement-game-item').length;
                        if (commonAchievementsCount === 0) {
                            $('.achievements-container').addClass('album-content-common-0');
                        } else if (commonAchievementsCount >= 3) {
                            $('.achievements-container').addClass('album-content-common-1');
                        } else {
                            $('.achievements-container').addClass('album-content-common-' + Math.ceil(commonAchievementsCount / 5));
                        }

                        $('.achievements-container .list-items').each(function (index, achievementList) {
                            if ($('.album-content-item', achievementList).length === 0) {
                                $(achievementList).hide();
                            }
                        });

                        if ((Math.ceil($('#achievement-common .album-content-item').length / 3)
                            + Math.ceil($('#achievement-other .album-content-item').length / 3)) > 1) {
                            $('#achievements-list').addClass('hide-list');
                        }

                        $('#achievements-showing .section-rates-showing-all').text($.formatVB($('#achievements-showing .section-rates-showing-all').text(), commonAchievementsCount, commonAchievementsCount));
                        $('#achievements-showing .section-rates-showing-part').text($.formatVB($('#achievements-showing .section-rates-showing-part').text(), $.unique($('.achievements-container .album-content-item:visible')).length, commonAchievementsCount));

                        if (methods.helpers.isCurrentUser() && settings.achievementPortfolio.earnedXp > 0) {
                            $('.user-op-points').show().find('.user-total-points').text(settings.achievementPortfolio.earnedXp);
                        }
                    }
                },

                checkOnlineState : function () {
                    // needs to run through check here.
                    if (!onlineStatus.onlineState) {
                        $body.addClass('offline-mode');
                    } else {
                        $body.removeClass('offline-mode');
                    }
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
    $.fn.profile({});
});
