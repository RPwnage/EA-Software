;(function( $, undefined ) {

    "use strict";

    $.fn.consent = function( options, parameters ) {

        var settings = {
            isCurrentUser : false,
            hasAchievementsGames : false,
            consentSource : "",
            privacySettings : { pidPrivacySettings : {
                                    shareAchievements : "PENDING"
                                }
                              },
            on : true
        };

        var methods = {
                
            init : function() {

                $.extend( settings, options );

                settings.isCurrentUser = methods.helpers.isCurrentUser();

                if (typeof consentSource === 'undefined') {
                    // variable is undefined do nothing
                } else {
                    settings.consentSource = consentSource;
                }

                entitlementManager.added.connect( function(entitlement){
                    if (entitlement.achievementSet && entitlement.achievementSet !== "") {
                        settings.hasAchievementsGames = true;
                    }
                    methods.helpers.verifyPrivacySettings();
                });

                $.each(entitlementManager.topLevelEntitlements, function(index, entitlement) {
                    if (entitlement.achievementSet && entitlement.achievementSet !== "") {
                        settings.hasAchievementsGames = true;
                        return false;
                    }
                });

                var achievementSharingStateQuery = originSocial.currentUser.achievementSharingState();

                achievementSharingStateQuery.succeeded.connect( 
                    function(privacySettings) {
                        settings.privacySettings = $.parseJSON(privacySettings);
                        methods.helpers.verifyPrivacySettings();
                });

                achievementSharingStateQuery.failed.connect( function() {
                    methods.helpers.verifyPrivacySettings();
                });

                originSocial.currentUser.succeeded.connect(function( privacySettings ){
                    methods.helpers.verifyPrivacySettings();
                });

                $("#consent-to-share .close").on( "click", methods.events.closeMessage );
                $("#consent-to-share .consent a").on( "click", methods.events.consentShare );
                $("#consent-to-share .info a").on( "click", methods.events.showPrivacySettings );

            }, //END init

            events : {

                closeMessage : function( event ) {
                    if(settings.on) {
                        settings.privacySettings.pidPrivacySettings.shareAchievements = "DISMISSED";
                        var newStateString = JSON.stringify(settings.privacySettings.pidPrivacySettings);
                        originSocial.currentUser.setAchievementSharingState(newStateString);
                        telemetryClient.sendAchievementSharing( 'dismiss', settings.consentSource );
                    }
                    $("#consent-to-share-wrapper").addClass("hidden");
                },

                consentShare : function( event ) {
                    settings.privacySettings.pidPrivacySettings.shareAchievements = "EVERYONE";
                    var newStateString = JSON.stringify(settings.privacySettings.pidPrivacySettings);

                    $("#consent-to-share-wrapper div.consent").addClass("hidden");
                    $("#consent-to-share-wrapper div.info").removeClass("hidden");
                    originSocial.currentUser.setAchievementSharingState(newStateString);
                    telemetryClient.sendAchievementSharing( 'show', settings.consentSource );
                    settings.on = false;
                },

                showPrivacySettings : function( event ) {
                    clientNavigation.showMyAccountPath("Privacy");
                }
            },

            helpers : {
                isCurrentUser : function() {
                    return $.getQueryString('userid') === originSocial.currentUser.id
                           || $.getQueryString('userid') == "";
                },

                verifyPrivacySettings : function() {

                    if (settings.isCurrentUser && settings.hasAchievementsGames){
                        if ( !$('#consent-to-share-wrapper .consent').hasClass('hidden') ){
                            if( settings.privacySettings.pidPrivacySettings.shareAchievements == "PENDING" ) {
                                $("#consent-to-share-wrapper").removeClass("hidden");
                            } else {
                                $("#consent-to-share-wrapper").addClass("hidden");
                            }
                        }
                    }
                }
            }

        };// END: methods

        // initialization
        
        var publicMethods = [ "init" ];
        
        if ( typeof options === 'object' || ! options ) {
            methods.init(); // initializes plugin

        } else if ( $.inArray( options, publicMethods ) ) {
            // call specific methods with arguments
            
        };

    };  // END: $.consentToShare = function() {


})( jQuery ); // END: (function( $ ){


$(document).ready(function() {
    $.fn.consent({});
});