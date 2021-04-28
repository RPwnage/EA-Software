/**
 * @file home/getstarted/bucket.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-get-started-bucket',
        SIDEBAR_WIDTH_PX = 280, // width of navigation in pixels
        BUCKET_BREAKPOINT_WIDTH_PX = 768; // breakpoint between 2 & 4 tiles shown

    var GetStartedTileEnumeration = {
        "getStartedAchievementVisibility": "getStartedAchievementVisibility",
        "getStartedCustomAvatar": "getStartedCustomAvatar",
        "getStartedOriginEmail": "getStartedOriginEmail",
        "getStartedProfileVisibility": "getStartedProfileVisibility",
        "getStartedRealName": "getStartedRealName",
        "getStartedTwofactorAuth": "getStartedTwofactorAuth",
        "getStartedVerifyEmail": "getStartedVerifyEmail"
    };

    function OriginHomeGetStartedBucketCtrl($scope, $window, GetStartedFactory, ScopeHelper, UtilFactory, ComponentsConfigFactory) {
        var settingsChangedHandle,
            tileAddedHandle;

        $scope.bucket = [];

        function getNumberOfColumnsForScreenSize() {
            var screenSize = $window.innerWidth - SIDEBAR_WIDTH_PX;
            if (screenSize <= BUCKET_BREAKPOINT_WIDTH_PX) {
                return 2;
            } else {
                return 4;
            }
        }
        $scope.limit = getNumberOfColumnsForScreenSize();

        function createFirstHyperlink(string) {
            return '<a class="origin-tile-support-text-link otka getstarted-a" ng-click="onPrivacyClicked()" href="javascript:void(0)">' + string + '</a>';
        }

        function createSecondHyperlink(string) {
            return '<a class="origin-tile-support-text-link otka getstarted-a" ng-click="onContactClicked()" href="javascript:void(0)">' + string + '</a>';
        }

        function verifyEmailAddress() {
            return GetStartedFactory.sendEmailVerification();
        }

        function optinToOriginEmails() {
            return GetStartedFactory.sendOptinEmailSetting();
        }

        var localizer = UtilFactory.getLocalizedStrGenerator(CONTEXT_NAMESPACE);

        $scope.titleLoc = localizer($scope.titleStr, 'title-str');
        $scope.subtitleLoc = localizer($scope.subtitleStr, 'subtitle-str');
        $scope.reminderDaysLoc = localizer($scope.reminderDays, 'reminder-days');

        // View More button
        $scope.viewmoreLoc = localizer($scope.viewMoreStr, 'viewmorestr');
        // Customize Avatar tile
        $scope.customizeavatartitleLoc = localizer($scope.customizeavatartitleStr, 'customizeavatartitlestr');
        $scope.customizeavatarmessageLoc = localizer($scope.customizeavatarmessageStr, 'customizeavatarmessagestr');
        $scope.customizeavatarbuttontextLoc = localizer($scope.customizeavatarbuttontextStr, 'customizeavatarbuttontextstr');
        $scope.customizeavatarlinktextLoc = localizer($scope.customizeavatarlinktextStr, 'customizeavatarlinktextstr');
        // Achievement visibility tile
        $scope.achievementvisibilityTitleLoc = localizer($scope.achievementvisibilitytitleStr, 'achievementvisibilitytitlestr');
        $scope.achievementvisibilitymessageLoc = localizer($scope.achievementvisibilitymessageStr, 'achievementvisibilitymessagestr');
        $scope.achievementvisibilitybuttontextLoc = localizer($scope.achievementvisibilitybuttontextStr, 'achievementvisibilitybuttontextstr');
        $scope.achievementvisibilitylinktextLoc = localizer($scope.achievementvisibilitylinktextStr, 'achievementvisibilitylinktextstr');
        // Profile visibility tile
        $scope.profilevisibilitytitleLoc = localizer($scope.profilevisibilitytitleStr, 'profilevisibilitytitlestr');
        $scope.profilevisibilitymessageLoc = localizer($scope.profilevisibilitymessageStr, 'profilevisibilitymessagestr');
        $scope.profilevisibilitybuttontextLoc = localizer($scope.profilevisibilitybuttontextStr, 'profilevisibilitybuttontextstr');
        $scope.profilevisibilitylinktextLoc = localizer($scope.profilevisibilitylinktextStr, 'profilevisibilitylinktextstr');
        // Sign up for Origin emails tile
        $scope.originemailstitleLoc = localizer($scope.originemailstitleStr, 'originemailstitlestr');
        $scope.originemailsmessageprivacylinktextstrLoc = localizer($scope.originemalsmessageprivacylinktextStr, 'originemailsmessageprivacylinktextstr');
        $scope.originemailsmessageprivacylinkhrefLoc = localizer($scope.originemailsmessageprivacylinkHref, 'originemailsmessageprivacylinkhref');
        $scope.originemailsmessagecontactlinkLoc = localizer($scope.originEmailsMessageContactLinkStr, 'originemailsmessagecontactlinktextstr');
        $scope.originemailsmessageLoc = localizer($scope.originemailsmessageStr, 'originemailsmessagestr', {
                '%privacycookiepolicy%' : createFirstHyperlink($scope.originemailsmessageprivacylinktextstrLoc),
                '%contactea%' : createSecondHyperlink($scope.originemailsmessagecontactlinkLoc)
            });
        $scope.originemailsconfirmationLoc = localizer($scope.originemailsconfirmationStr, 'originemailsconfirmationstr', {'%email%' : Origin.user.email()});
        $scope.originemailsbuttontextLoc = localizer($scope.originemailsbuttontextStr, 'originemailsbuttontextstr');
        $scope.originemailslinktextLoc = localizer($scope.originemailslinktextStr, 'originemailslinktextstr');
        $scope.contacteadialogtitleLoc = localizer($scope.contacteadialogtitleStr, 'contacteadialogtitlestr');
        $scope.contacteadialogmessageLoc = localizer($scope.contacteadialogmessageStr, 'contacteadialogmessagestr');
        $scope.contacteadialogbuttontextLoc = localizer($scope.contacteadialogbuttontextStr, 'contacteadialogbuttontextstr');
        // Set Real name tile
        $scope.realnametitleLoc = localizer($scope.realnametitleStr, 'realnametitlestr');
        $scope.realnamemessageLoc = localizer($scope.realnamemessageStr, 'realnamemessagestr');
        $scope.realnamebuttontextLoc = localizer($scope.realnamebuttontextStr, 'realnamebuttontextstr');
        $scope.realnamelinktextLoc = localizer($scope.realnamelinktextStr, 'realnamelinktextstr');
        // Verify Email time
        $scope.verifyemailtitleLoc = localizer($scope.verifyemailtitleStr, 'verifyemailtitlestr');
        $scope.verifyemailmessageLoc = localizer($scope.verifyemailmessageStr, 'verifyemailmessagestr');
        $scope.verifyemailconfirmationLoc = localizer($scope.verifyemailconfirmationStr, 'verifyemailconfirmationstr', {'%email%' : Origin.user.email()});
        $scope.verifyemailbuttontextLoc = localizer($scope.verifyemailbuttontextStr, 'verifyemailbuttontextstr');
        $scope.verifyemaillinktextLoc = localizer($scope.verifyemaillinktextStr, 'verifyemaillinktextstr');
        // Two-factor auth tile
        $scope.twofactorauthtitleLoc = localizer($scope.twofactorauthtitleStr, 'twofactorauthtitlestr');
        $scope.twofactorauthmessageLoc = localizer($scope.twofactorauthmessageStr, 'twofactorauthmessagestr');
        $scope.twofactorauthbuttontextLoc = localizer($scope.twofactorauthbuttontextStr, 'twofactorauthbuttontextstr');
        $scope.twofactorauthlinktextLoc = localizer($scope.twofactorauthlinktextStr, 'twofactorauthlinktextstr');

        var getStartedTiles = {
            'getStartedCustomAvatar': {
                image: ComponentsConfigFactory.getImagePath('getstarted\\completeProfile_customizeAvatar.png'),
                title: $scope.customizeavatartitleLoc,
                msg: $scope.customizeavatarmessageLoc,
                buttonText: $scope.customizeavatarbuttontextLoc,
                linkText: $scope.customizeavatarlinktextLoc,
                externalLink: 'true',
                href: 'eaAccounts', //aboutMe
                name: 'getStartedCustomAvatar' // Tealium uses this attribute
            },
            'getStartedAchievementVisibility': {
                image: ComponentsConfigFactory.getImagePath('getstarted\\completeProfile_achievementVisibility.png'),
                title: $scope.achievementvisibilityTitleLoc,
                msg: $scope.achievementvisibilitymessageLoc,
                buttonText: $scope.achievementvisibilitybuttontextLoc,
                linkText: $scope.achievementvisibilitylinktextLoc,
                externalLink: 'true',
                href: 'accountPrivacySettings', //PrivacySettings
                name: 'getStartedAchievementVisibility' // Tealium uses this attribute
            },
            'getStartedProfileVisibility': {
                image: ComponentsConfigFactory.getImagePath('getstarted\\completeProfile_profileVisibility.png'),
                title: $scope.profilevisibilitytitleLoc,
                msg: $scope.profilevisibilitymessageLoc,
                buttonText: $scope.profilevisibilitybuttontextLoc,
                linkText: $scope.profilevisibilitylinktextLoc,
                externalLink: 'true',
                href: 'accountPrivacySettings', //PrivacySettings
                name: 'getStartedProfileVisibility' // Tealium uses this attribute
            },
            'getStartedOriginEmail': {
                image: ComponentsConfigFactory.getImagePath('getstarted\\completeProfile_optin.png'),
                title: $scope.originemailstitleLoc,
                msg: $scope.originemailsmessageLoc,
                msgHref: $scope.originemailsmessageprivacylinkhrefLoc,
                buttonText: $scope.originemailsbuttontextLoc,
                linkText: $scope.originemailslinktextLoc,
                confirmationStr: $scope.originemailsconfirmationLoc,
                externalLink: 'false',
                tileAction: optinToOriginEmails,
                name: 'getStartedOriginEmail' // Tealium uses this attribute
            },
            'getStartedRealName': {
                image: ComponentsConfigFactory.getImagePath('getstarted\\completeProfile_realName.png'),
                title: $scope.realnametitleLoc,
                msg: $scope.realnamemessageLoc,
                buttonText: $scope.realnamebuttontextLoc,
                linkText: $scope.realnamelinktextLoc,
                externalLink: 'true',
                href: 'eaAccounts', //aboutMe
                name: 'getStartedRealName' // Tealium uses this attribute
            },
            'getStartedVerifyEmail': {
                image: ComponentsConfigFactory.getImagePath('getstarted\\completeProfile_emailVerification.png'),
                title: $scope.verifyemailtitleLoc,
                msg: $scope.verifyemailmessageLoc,
                buttonText: $scope.verifyemailbuttontextLoc,
                linkText: $scope.verifyemaillinktextLoc,
                confirmationStr: $scope.verifyemailconfirmationLoc,
                externalLink: 'false',
                tileAction: verifyEmailAddress,
                name: 'getStartedVerifyEmail' // Tealium uses this attribute
            },
            'getStartedTwofactorAuth': {
                image: ComponentsConfigFactory.getImagePath('getstarted\\completeProfile_twoFactorAuthentication.png'),
                title: $scope.twofactorauthtitleLoc,
                msg: $scope.twofactorauthmessageLoc,
                buttonText: $scope.twofactorauthbuttontextLoc,
                linkText: $scope.twofactorauthlinktextLoc,
                externalLink: 'true',
                href: 'accountSecuritySettings', //Security
                name: 'getStartedTwofactorAuth' // Tealium uses this attribute
            }
        };

        /**
         * Check if a tile was already added to the bucket
         * @param {String} tileName
         * @returns {boolean}
         */
        function isTileAlreadyAdded(tileName) {
            return !_.isUndefined(_.find($scope.bucket, {'name': tileName}));
        }

        /**
         * Sort bucket after a new tile was added
         */
        function sortBucket() {
            $scope.bucket = GetStartedFactory.shuffleAndSortBucket($scope.bucket);
            $scope.$digest();
        }
        var sortBucketThrottled = _.throttle(sortBucket, 200, {'leading': false}); // Throttled version so we don't re-sort more than necessary

        function addTileToBucket(tile) {
            if (GetStartedFactory.checkSetting(tile.name) === true && !isTileAlreadyAdded(tile.name)) {
                $scope.bucket.push(tile);
                sortBucketThrottled();

                var tealiumPayload = {
                    type: 'getStarted',
                    tile: tile
                };
                Origin.telemetry.events.fire('origin-tealium-discovery-tile-loaded', tealiumPayload);
            }
            updateVisibility(); // update whether or not a tile was added, in case none will be shown
        }

        $scope.showMoreTiles = function () {
            $scope.limit = 999999;
        };

        /**
         * Show bucket of tiles once settings data has been loaded
         * @param {Boolean} success Whether getData call succeeded or not
         */
        function showBucket(success) {
            if (success) {
                $scope.isVisible = true;
                $scope.$digest();
            }
        }

        /**
         * Set visible number of tiles based on how many should be shown
         */
        function updateVisibility() {
            if ($scope.bucket.length === 0) {
                $scope.isVisible = false;
            }
            ScopeHelper.digestIfDigestNotInProgress($scope);
        }

        /**
         * Updates bucket when a given tile's action CTA is clicked on (thus dismissing it)
         * @param {String} name
         */
        function handleSettingChanged(name) {
            _.remove($scope.bucket, function(tile) { return tile.name === name; });
            updateVisibility();
        }

        /**
         * Handle tile-population event
         * @param {Object} tileConfig object containing CYA tile name and priority level
         * @param {String} tileConfig.name Name of respective CYA tile type, e.g. "getStartedRealName"
         * @param {Number} [tileConfig.priority] Priority level for this tile.
         */
        function handleTileAdded(tileConfig) {
            var tile = _.get(getStartedTiles, GetStartedTileEnumeration[tileConfig.name]);
            if (tile) {
                tile.priority = parseInt(_.get(tileConfig, 'priority', 0)) || 0;
                addTileToBucket(tile);
            }
        }

        $scope.$on('$destroy', function() {
            if(settingsChangedHandle) {
                settingsChangedHandle.detach();
            }
            if (tileAddedHandle) {
                tileAddedHandle.detach();
            }
        });

        settingsChangedHandle = GetStartedFactory.events.on("getstarted:settingChanged", handleSettingChanged);
        tileAddedHandle = GetStartedFactory.events.on("getstarted:tileAdded", handleTileAdded);

        GetStartedFactory.setRemindMeLater($scope.reminderDaysLoc);
        GetStartedFactory.getData().then(showBucket);
    }

    function originHomeGetStartedBucket(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                subtitleStr: '@',
                reminderDays: '@',
                viewMoreStr: '@viewmorestr',
                customizeavatartitleStr: '@customizeavatartitlestr',
                customizeavatarmessageStr: '@customizeavatarmessagestr',
                customizeavatarbuttontextStr: '@customizeavatarbuttontextstr',
                customizeavatarlinktextStr: '@customizeavatarlinktextstr',
                //
                achievementvisibilitytitleStr: '@achievementvisibilitytitlestr',
                achievementvisibilitymessageStr: '@achievementvisibilitymessagestr',
                achievementvisibilitybuttontextStr: '@achievementvisibilitybuttontextstr',
                achievementvisibilitylinktextStr: '@achievementvisibilitylinktextstr',
                //
                profilevisibilitytitleStr: '@profilevisibilitytitlestr',
                profilevisibilitymessageStr: '@profilevisibilitymessagestr',
                profilevisibilitybuttontextStr: '@profilevisibilitybuttontextstr',
                profilevisibilitylinktextStr: '@profilevisibilitylinktextstr',
                //
                originemailstitleStr: '@originemailstitlestr',
                originemailsmessageStr: '@originemailsmessagestr',
                originemalsmessageprivacylinktextStr: '@originemailsmessageprivacylinktextstr',
                originemailsmessageprivacylinkHref: '@originemailsmessageprivacylinktextstr',
                originEmailsMessageContactLinkStr: '@originemailsmessagecontactlinktextstr',
                originemailsmessagecontactlinkLoc: '@originemailsmessagecontactlinktextstr',
                originemalsconfirmationStr: '@originemalsconfirmationstr',
                originemailsbuttontextStr: '@originemailsbuttontextstr',
                originemailslinktextStr: '@originemailslinktextstr',
                //
                realnametitleStr: '@realnametitlestr',
                realnamemessageStr: '@realnamemessagestr',
                realnamebuttontextStr: '@realnamebuttontextstr',
                realnamelinktextStr: '@realnamelinktextstr',
                //
                verifyemailtitleStr: '@verifyemailtitlestr',
                verifyemailmessageStr: '@verifyemailmessagestr',
                verifyemailconfirmationStr: '@verifyemailconfirmationstr',
                verifyemailbuttontextStr: '@verifyemailbuttontextstr',
                verifyemaillinktextStr: '@verifyemaillinktextstr',
                //
                twofactorauthtitleStr: '@twofactorauthtitlestr',
                twofactorauthmessageStr: '@twofactorauthmessagestr',
                twofactorauthbuttontextStr: '@twofactorauthbuttontextstr',
                twofactorauthlinktextStr: '@twofactorauthlinktextstr'
            },
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/getstarted/views/bucket.html'),
            controller: 'OriginHomeGetStartedBucketCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeGetStartedBucket
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str the tile for this area
     * @param {LocalizedString} subtitle-str the area subtitle
     * @param {Number} reminder-days The number of days we want to wait before reminding the user

     * @param {LocalizedString} customizeavatartitlestr "Customize your avater"
     * @param {LocalizedString} customizeavatarmessagestr "Add a custom avatar or choose from a library of avatars inspired by your favorite Origin games."
     * @param {LocalizedString} customizeavatarbuttontextstr "Customize avatar"
     * @param {LocalizedString} customizeavatarlinktextstr "Remind me later"

     * @param {LocalizedString} achievementvisibilitytitlestr "Achievement visibility"
     * @param {LocalizedString} achievementvisibilitymessagestr "You are awesome, so show off your hard work and let everyone see what achievements you have earned."
     * @param {LocalizedString} achievementvisibilitybuttontextstr "Show achievements visibility"
     * @param {LocalizedString} achievementvisibilitylinktextstr "Remind me later"

     * @param {LocalizedString} profilevisibilitytitlestr "Profile Visibility"
     * @param {LocalizedString} profilevisibilitymessagestr "Show profile visibility"
     * @param {LocalizedString} profilevisibilitybuttontextstr "Be in control and choose who can view your profile."
     * @param {LocalizedString} profilevisibilitylinktextstr "Remind me later"

     * @param {LocalizedString} originemailstitlestr "Sign up for Origin emails"
     * @param {LocalizedString} originemailsmessagestr "Sign up for emails about Origin and EA's products, news, events,a nd promotions consistent with %link%"
     * @param {LocalizedString} originemailsmessagelinktextstr "EA's Privacy and Cookie Policy"
     * @param {LocalizedString} originemailsmessagelinkhref "http://www.ea.com/privacy-policy"
     * @param {LocalizedString} originemailsbuttontextstr "Join Game"
     * @param {LocalizedString} originemailslinktextstr "Remind me later"
     * @param {LocalizedString} originemailsmessageprivacylinktextstr "EA's Privacy and Cookie Policy"
     * @param {LocalizedString} originemailsmessagecontactlinktextstr "Contact EA"
     * @param {LocalizedString} originemailsmessageprivacylinkhref "http://www.ea.com/privacy-policy"


     * @param {LocalizedString} realnametitlestr "Playing"
     * @param {LocalizedString} realnamemessagestr "Broadcasting"
     * @param {LocalizedString} realnamebuttontextstr "Join Game"
     * @param {LocalizedString} realnamelinktextstr "Remind me later"

     * @param {LocalizedString} verifyemailtitlestr "Playing"
     * @param {LocalizedString} verifyemailmessagestr "Broadcasting"
     * @param {LocalizedString} verifyemailconfirmationstr "A verification email has been sent to %email%"
     * @param {LocalizedString} verifyemailbuttontextstr "Join Game"
     * @param {LocalizedString} verifyemaillinktextstr "Remind me later"

     * @param {LocalizedString} twofactorauthtitlestr "Playing"
     * @param {LocalizedString} twofactorauthmessagestr "Broadcasting"
     * @param {LocalizedString} twofactorauthbuttontextstr "Join Game"
     * @param {LocalizedString} twofactorauthlinktextstr "Remind me later"
     *
     *
     * @param {LocalizedString} contacteadialogbuttontextstr *Update in Defaults
     * @param {LocalizedString} viewmorestr *Update in Defaults
     * @param {LocalizedString} contacteadialogmessagestr *Update in Defaults
     * @param {LocalizedString} originemailsconfirmationstr *Update in Defaults
     * @param {LocalizedString} contacteadialogtitlestr *Update in Defaults
     *
     * @description
     *
     * getting started buckets
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-get-started-bucket>
     *         </origin-home-get-started-bucket>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeGetStartedBucketCtrl', OriginHomeGetStartedBucketCtrl)
        .directive('originHomeGetStartedBucket', originHomeGetStartedBucket);

}());
