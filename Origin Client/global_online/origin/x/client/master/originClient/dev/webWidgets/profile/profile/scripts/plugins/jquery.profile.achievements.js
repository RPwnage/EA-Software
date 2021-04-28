/**
* @jquery.profile.achievements.js
* Script used to retrieve and display information about a person's achievements.
*
* AchievementsList: loads and fill out the achievements that was 
* scaffolding and build in the games section.
*
* This is called from the userProfile plugin
*/

;(function ($, undefined) {

	'use strict';

	$.fn.achievementsList = function(options, parameters) {

		var settings = {
			profileUser : null,
            userId : null,
            isCurrentUser : true,
			achievementPortfolio : null,
            achievementsList : null
		};

		// bindings
        var methods = {

        	init : function() {
        		$.extend(settings, options);

                // Create Achievements
                settings.achievementPortfolio = settings.profileUser.updateAchievements();
                
                if (settings.achievementPortfolio) {
                    methods.helpers.setSignals();
                } else {
                    $("#achievements-list").addClass("error");
                    methods.helpers.setInterval();
                }

        	}, // END init

        	events : {
        		
        		goToAchievementSetDetails : function (event) {
                    var id = this.id;
                    clientNavigation.showAchievementSetDetails(id.substr(id.indexOf('-') + 1));
                },

                achievementCardClick : function (event) {
                    event.preventDefault();
                    clientNavigation.showAchievementSetDetails($(this).data('achievement-set-id'), settings.userId, $(this).find('.achievements-game-title').text());
                }
        	}, // END events

        	helpers : {

        		getAchievementArt : function (achievementSetId) {

                    var achievementImageSrc = "http://static.cdn.ea.com/ebisu/u/f/achievements/" 
                        + achievementSetId + "/images/en_US/profile_" + $.md5(achievementSetId + "profileAchievement") + ".jpg";

                    var img = new Image();
                    img.src = achievementImageSrc;
                    img.onload = function () {
                        var $currentSelectedAchievement = $('.achievement-game-item', settings.achievementsList).filter(
                            function () {  return $(this).data('achievement-set-id') === achievementSetId; });

                        $('.achievements-game-art', $currentSelectedAchievement).css('background-image', 'url(' + achievementImageSrc + ')');
                        return;
                    };

                },                

        		getAchievements : function() {                    

                    var $achievementCommonList = $('#achievement-common'),                        
                        $achievementItem;

                    settings.achievementsList = $('#achievements-list');

                    if (!settings.achievementPortfolio.available) {
                        settings.achievementsList.show().addClass('private');
                        return true;
                    }
                        
                    if ( !settings.achievementTemplate ) {
                        settings.achievementTemplate = $('.achievement-game-item').detach();
                    }

                    // create achievement card if achievement exists
                    if (settings.achievementPortfolio.achievementSets.length == 0) {
                        settings.achievementsList.hide().removeClass("album-loading");
                    } else {
                        settings.achievementsList.removeClass('album-loading').show();
                        $('.album-content-item', settings.achievementsList).remove();
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
                                $achievementCommonList.append($achievementItem);
                                
                                methods.helpers.getAchievementArt( achievementSet.id);
                            }
                        });

                        $('#achievements-achieved .achievements-total').text(settings.achievementPortfolio.achieved);
                        $('#achievements-earned-points .achievements-total').text(settings.achievementPortfolio.earnedXp);

                        methods.helpers.sectionManager();

                        $('body').on('click', '.achievement-game-item', methods.events.achievementCardClick);
                    }
                },

                sectionManager : function() {
                    // TODO: Need to factor this out to a function. This is used in friends and achievements with different
                    // mod values
                    var commonAchievementsCount = $('#achievement-common .achievement-game-item').length,
                        $showAllRates = $('#achievements-showing .section-rates-showing-all'),
                        $showPartRates = $('#achievements-showing .section-rates-showing-part');
                    
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
                        settings.achievementsList.addClass('hide-list');
                    }
                    
                    $showAllRates.text($.formatVB($showAllRates.text(), commonAchievementsCount, commonAchievementsCount));
                    $showPartRates.text($.formatVB($showPartRates.text(), $.unique($('.achievements-container .album-content-item:visible')).length, commonAchievementsCount));

                    if (settings.isCurrentUser && settings.achievementPortfolio.earnedXp > 0) {
                        $('.user-op-points').show().find('.user-total-points').text(settings.achievementPortfolio.earnedXp);
                    }
                },

                setInterval : function() {
                    var intents = 0,
                        interval = setInterval(function() {
                            settings.achievementPortfolio = settings.profileUser.updateAchievements();
                            if(settings.achievementPortfolio) {
                                $("#achievements-list").removeClass('error');
                                clearInterval(interval);
                                methods.helpers.setSignals();
                            } else if(++intents === 5 || $("#achievements-list").hasClass('private')) {
                                $("#achievements-list").removeClass('error');
                                clearInterval(interval);
                            }                    
                        }, 60000);
                },

                setSignals : function() {

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

        	} // END helpers            
        }; // END methods

		// initialization

        var publicMethods = [ "init" ];

        if (typeof options === 'object' || !options) {

            methods.init(); // initializes plugin

        } else if ($.inArray(options, publicMethods)) {
            // call specific methods with arguments

        }
	};
})(jQuery);
