/**
 * Jasmine functional test
 */

'use strict';

describe('OGD Content Tile', function() {
    var $controller, $rootScope, $compile, AppCommFactory;

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide) {
            $provide.factory('AppCommFactory', function() {
                return {
                    'events': {
                        'on': function(){},
                        'fire': function(){}
                    }
                };
            });
        });
    });

    beforeEach(inject(function(_$controller_, _$rootScope_, _$compile_, _AppCommFactory_) {
        $controller = _$controller_;
        $rootScope = _$rootScope_;
        $compile = _$compile_;
        AppCommFactory = _AppCommFactory_;
    }));

    describe('$scope', function() {
        var $scope, controller;

        beforeEach(function() {
            $scope = $rootScope.$new();
            spyOn(AppCommFactory.events, 'fire').and.callThrough();
            controller = $controller('OriginGamelibraryOgdContentCtrl', {
                $scope: $scope
            });
        });

        it('Will redirect the user to te first tab if the URL targets an invalid link', function() {
            $scope.validateRoute('garfield-the-cat', 'OFB-EAST:1234');
            expect(AppCommFactory.events.fire).toHaveBeenCalled();
            expect(AppCommFactory.events.fire).toHaveBeenCalledWith(
                'uiRouter:go', 'app.game_gamelibrary.ogd.topic', {'offerid': 'OFB-EAST:1234', 'topic': 'overview'}
            );
        });

        it('Will test a tab state based on the current topic to support choosing the active tab', function() {
            $scope.topic = 'overview';
            expect($scope.isActiveTab('overview')).toEqual('true');
            expect($scope.isActiveTab('notoverview')).toEqual('false');
        });

        it('Creates expected dom for a single tab', function() {
            $scope.offerId = 'DR:225064100';
            var tabs = $scope.generateContentTabs([
                {
                    'origin-gamelibrary-ogd-content-panel': {
                        title: 'Overview',
                        friendlyName: 'overview',
                        items: []
                    }
                }
            ]);
            expect(tabs).not.toContain('origin-gamelibrary-ogd-tab-bar');
            expect(tabs).not.toContain('origin-gamelibrary-ogd-tab-bar-item');
            expect(tabs).not.toContain($scope.offerId);
        });

        it('Creates expected dom for multiple tabs', function() {
            $scope.offerId = 'DR:225064100';
            var tabs = $scope.generateContentTabs([
                {
                    'origin-gamelibrary-ogd-content-panel': {
                        title: 'Overview',
                        friendlyName: 'overview',
                        items: []
                    }
                }, {
                    'origin-gamelibrary-ogd-content-panel': {
                        title: 'DLC',
                        friendlyName: 'battlefield-downloadable-content',
                        items: []
                    }
                }
            ]);
            expect(tabs).toContain('origin-gamelibrary-ogd-tab-bar');
            expect(tabs).toContain('origin-gamelibrary-ogd-tab-bar-item');
            expect(tabs).toContain($scope.offerId);
        });

        it('Generates a stylesheet from OCD', function() {
            $scope.offerId = 'OFB-EAST:41420';
            expect($scope.generateStylesheet({tabtextcolor: '#24789A'})).toContain('color: #24789A');
            expect($scope.generateStylesheet({tabactiveselectioncolor: '#24789B'})).toContain('color: #24789B');
            expect($scope.generateStylesheet({tabactivetextcolor: '#24789C'})).toContain('color: #24789C');
            expect($scope.generateStylesheet({tabhorizontalrulecolor: '#24789D'})).toContain('1px solid #24789D');
        });

        it('Can compile the linking functions', function() {
            var element = $compile('<origin-gamelibrary-ogd-content offerId="DR:225064100"></origin-gamelibrary-ogd-content>')($scope);
            $scope.$digest();
            expect(element.html()).toContain('tabs');
            expect(element.html()).toContain('content');
        });
    });
});
