/**
 * @file home/discovery/tile/config/cta.js
 */

(function() {
    'use strict';

    /* jshint ignore:start */
    /**
     * Action ID map used in child directive
     * @readonly
     * @enum {string}
     */
    var ActionIdEnumeration = {
        "url-internal": "url-internal",
        "url-external": "url-external",
        "play-video": "play-video",
        "twitch-channel": "twitch-channel",
        "twitch-video": "twitch-video",
        "twitch-clip": "twitch-clip",
        "purchase": "purchase",
        "direct-acquistion": "direct-acquistion",
        "direct-acquistion-vault": "direct-acquistion-vault",
        "pdp": "pdp",
        "download-install-play": "download-install-play"
    };

    /**
     * A list of button type options used in child directive
     * @readonly
     * @enum {string}
     */
    var ButtonTypeEnumeration = {
        "primary": "primary",
        "secondary": "secondary",
        "round": "round"
    };

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    // for video modals, some values must be mapped to properties that makes sense for the respective modal
    // key is action type,  value is the name of the proeprty that scope.href will be mapped to
    var VIDEO_MODAL_PROPERTY_MAP = {
        'play-video': 'video-id',
        'twitch-channel': 'channel',
        'twitch-video': 'video',
        'twitch-clip': 'clip'
    };

    function OriginHomeDiscoveryTileConfigCtaCtrl($scope, $attrs) {
        //look up table to the possible directives
        var actionId = $attrs.actionid;
        var ctaConfigBase = {
                'url-internal': 'origin-cta-loadurlinternal',
                'url-external': 'origin-cta-loadurlexternal',
                'play-video': 'origin-cta-playvideo',
                "twitch-channel": "origin-cta-play-twitch",
                "twitch-video": "origin-cta-play-twitch",
                "twitch-clip": "origin-cta-play-twitch",
                'purchase': 'origin-cta-purchase',
                'direct-acquistion': 'origin-cta-directacquisition',
                'direct-acquistion-vault': 'origin-cta-directacquisition',
                'pdp': 'origin-cta-pdp',
                'download-install-play': 'origin-cta-downloadinstallplay'
            },
            tileButtonConfig = $scope.$parent.childDirectives,
            directiveInfo = {},
            ctaDirectiveName = ctaConfigBase[actionId];

        //create directive info (ocd format compatible)
        directiveInfo[ctaDirectiveName] = {
            href: $attrs.href,
            description: $attrs.description, // Tealium uses this attribute
            type: $attrs.type,
            'ocd-path': _.get($attrs, 'ocdPath', ''), // Tealium uses this attribute
            'secondary-href': _.get($attrs, 'secondaryHref', ''),
            'secondary-href-label': _.get($attrs, 'secondaryHrefLabel', ''),
            'secondary-description': _.get($attrs, 'secondaryDescription', ''),
            'hide-packart': _.get($attrs, 'hidePackart', ''),
            'mature-content': _.get($attrs, 'matureContent', '')
        };

        //add an extra property if its a direct-acquistion-vault type button
        //the direct acquisition button takes an additional property if its a vault type
        //I think determining if an offer is a vault should be encapsulated by the cta directive itself
        //but thats too big of a change at this stage.
        if(actionId === 'direct-acquistion-vault') {
            directiveInfo[ctaDirectiveName]['is-vault'] = true;
        }

        if (VIDEO_MODAL_PROPERTY_MAP[actionId]) {
            var href = _.get(directiveInfo, [ctaDirectiveName, 'videoid'], _.get(directiveInfo, [ctaDirectiveName, 'href']));
            _.set(directiveInfo, [ctaDirectiveName, VIDEO_MODAL_PROPERTY_MAP[actionId]], href);

            // map secondary properties to keys that the video modals are expecting
            _.set(directiveInfo, [ctaDirectiveName, 'content-link'], _.get(directiveInfo, [ctaDirectiveName, 'secondary-href']));
            _.set(directiveInfo, [ctaDirectiveName, 'content-link-btn-text'], _.get(directiveInfo, [ctaDirectiveName, 'secondary-href-label']));
            _.set(directiveInfo, [ctaDirectiveName, 'content-text'], _.get(directiveInfo, [ctaDirectiveName, 'secondary-description']));
        }

        tileButtonConfig.push(directiveInfo);
    }

    function originHomeDiscoveryTileConfigCta() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigCtaCtrl',
            scope: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigCta
     * @restrict E
     * @element ANY
     * @scope
     * @param {ActionIdEnumeration} actionid The type of cta button
     * @param {Url} href The href associated with this cta
     * @param {LocalizedString} description The button text
     * @param {OcdPath} ocd-path (Optional) OCD path associated with this CTA
     * @param {ButtonTypeEnumeration} type The style of the button 'primary', 'secondary' or 'round'
     * @param {Url} secondary-href (Optional) Secondary link
     * @param {LocalizedString} secondary-href-label (Optional) CTA text for secondary link
     * @param {LocalizedString} secondary-description (Optional) Secondary description
     * @param {BooleanEnumeration} hide-packart (Optional) Suppress pack art from showing when OCD path is provided
     * @param {BooleanEnumeration} mature-content Only has effect if the action id is play-video
     * @param {Video} videoid The youtube video ID of the video for this promo (Optional).
     * @description
     *
     * tile config for owned game recently played discovery tile
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigCtaCtrl', OriginHomeDiscoveryTileConfigCtaCtrl)
        .directive('originHomeDiscoveryTileConfigCta', originHomeDiscoveryTileConfigCta);
}());
