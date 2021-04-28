/**
 * Jasmine functional test
 */

'use strict';

describe('UrlTriggerFactory', function() {

    var $location, AppCommFactory, DialogFactory, FeatureConfig, UrlTriggerFactory;

    beforeEach(function () {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide) {
            $provide.factory('AppCommFactory', function() {
                return {
                    'events': {
                        'fire': function(){},
                        'on': function(){}
                    }
                };
            });

            $provide.factory('FeatureConfig', function(){
                return {
                    isGiftingEnabled: function(){}
                }
            });

            $provide.factory('DialogFactory', function(){
                return {
                    open: function(){}
                }
            });

            $provide.factory('$location', function(){
                return {
                    search: function(){}
                }
            });
        });
    });

    beforeEach(inject(function(_$location_, _AppCommFactory_, _DialogFactory_, _FeatureConfig_, _UrlTriggerFactory_){
        $location = _$location_;
        AppCommFactory = _AppCommFactory_;
        DialogFactory = _DialogFactory_;
        FeatureConfig = _FeatureConfig_;
        UrlTriggerFactory = _UrlTriggerFactory_;
    }));

    describe('startActions', function() {

        it('will open the dialog when the respective query param key is present', function() {
            spyOn(DialogFactory, 'open');
            spyOn(FeatureConfig, 'isGiftingEnabled').and.returnValue(true);
            spyOn($location, 'search').and.returnValue({'giftingintro': true});

            UrlTriggerFactory.startActions();
            expect($location.search.calls.count()).toBe(2); // once to find the key, second time to remove it afterward
            expect(DialogFactory.open).toHaveBeenCalled();
        });

        it('will NOT open the dialog when the respective query param key is not present', function() {
            spyOn(DialogFactory, 'open');
            spyOn(FeatureConfig, 'isGiftingEnabled').and.returnValue(true);
            spyOn($location, 'search').and.returnValue({});

            UrlTriggerFactory.startActions();
            expect($location.search.calls.count()).toBe(1);  // was not called a 2nd time to remove the key :)
            expect(DialogFactory.open).not.toHaveBeenCalled();
        });

        it('will open the dialog when the page navigates and query param is present', function() {
            spyOn(DialogFactory, 'open');
            spyOn(FeatureConfig, 'isGiftingEnabled').and.returnValue(true);

            UrlTriggerFactory.startActions();  // fire first time to start AppCommFactory listener
            expect(DialogFactory.open).not.toHaveBeenCalled();

            spyOn($location, 'search').and.returnValue({'giftingintro': true});
            spyOn(AppCommFactory.events, 'fire').and.callFake(UrlTriggerFactory.startActions);  // mock event fire behaviour
            AppCommFactory.events.fire('uiRouter:stateChangeSuccess');
            expect($location.search.calls.count()).toBe(2); // once to find the key, second time to remove it afterward
            expect(DialogFactory.open).toHaveBeenCalled();
        });

        it('will NOT open the dialog when the page navigates and query param is not present', function() {
            spyOn(DialogFactory, 'open');
            spyOn(FeatureConfig, 'isGiftingEnabled').and.returnValue(true);

            UrlTriggerFactory.startActions();  // fire first time to start AppCommFactory listener
            expect(DialogFactory.open).not.toHaveBeenCalled();

            spyOn($location, 'search').and.returnValue({});
            spyOn(AppCommFactory.events, 'fire').and.callFake(UrlTriggerFactory.startActions);  // mock event fire behaviour
            AppCommFactory.events.fire('uiRouter:stateChangeSuccess');
            expect($location.search.calls.count()).toBe(1); // only once since key was not found & won't be removed
            expect(DialogFactory.open).not.toHaveBeenCalled();
        });

    });

});