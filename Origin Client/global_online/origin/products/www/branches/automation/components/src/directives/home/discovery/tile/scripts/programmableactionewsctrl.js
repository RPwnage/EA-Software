/**
 * @file home/discovery/tile/programmableactionnewsctrl.js
 */
(function() {
    'use strict';

    function OriginHomeDiscoveryTileProgrammableActionNewsCtrl($scope) {

        function onCtaClicked(event, format) {
            var source = Origin.client.isEmbeddedBrowser() ? 'client' : 'web';
            format = format ? format : 'live_tile';

            event.stopPropagation();
            Origin.telemetry.sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'message', source, 'success', {
                'type': 'in-game',
                'service': 'originx',
                'format': format,
                'client_state': 'foreground',
                'msg_id': $scope.ctid ? $scope.ctid : $scope.image,
                'status': 'click',
                'content': $scope.description,
                'destination_name': _.get($scope, ['$parent', 'titleStr']) || ''
            });
        }


        $scope.supportTextVisible = _.get($scope, ['descriptiontitle', 'length'], 0) || _.get($scope, ['description', 'length'], 0);

        // We listen for an event that is emitted by the transcluded cta directive,
        // in order to send telemetry that includes the ctid, which identifies the tile.
        $scope.$on('cta:clicked', onCtaClicked);
    }

    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileProgrammableActionNewsCtrl', OriginHomeDiscoveryTileProgrammableActionNewsCtrl);

}());