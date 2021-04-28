/**
 * Scope helper functions for Angular JS 1.x series
 */

'use strict';

describe('ScopeHelper - convenient handlers for scope functions', function() {
    var scopeHelper, componentsLogFactory;

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide){
            $provide.factory('ComponentsLogFactory', function() {
                return {
                    log: function(){}
                };
            });
        });
    });

    beforeEach(inject(function (_ScopeHelper_, _ComponentsLogFactory_) {
        scopeHelper = _ScopeHelper_;
        componentsLogFactory = _ComponentsLogFactory_;
    }));

    describe('digestIfDigestNotInProgress', function() {
        var mockScope;

        beforeEach(function() {
            mockScope = {
                $on: function() {},
                $off: function() {},
                $$phase: false,
                $digest: function() {}
            };
        });

        it('will digest if a digest is not already in progress', function() {
            spyOn(mockScope, '$digest').and.callThrough();

            scopeHelper.digestIfDigestNotInProgress(mockScope);

            expect(mockScope.$digest).toHaveBeenCalled();
        });

        it('will return in case the scope object is missing', function() {
            spyOn(mockScope, '$digest').and.callThrough();

            scopeHelper.digestIfDigestNotInProgress(undefined);

            expect(mockScope.$digest).not.toHaveBeenCalled();
        });

        it('will return in case the scope object is malformed', function() {
            spyOn(mockScope, '$digest').and.callThrough();

            scopeHelper.digestIfDigestNotInProgress({foo:'moo'});

            expect(mockScope.$digest).not.toHaveBeenCalled();
        });

        it('will handle recursion exceptions and other throws in the digest func', function() {
            function throwFunc() {
                throw 'an error';
            }

            spyOn(componentsLogFactory, 'log').and.callThrough();
            spyOn(mockScope, '$digest').and.callFake(throwFunc);

            scopeHelper.digestIfDigestNotInProgress(mockScope);

            expect(mockScope.$digest).toHaveBeenCalled();
            expect(componentsLogFactory.log).toHaveBeenCalledWith('an error');
        });

        it('Will not digest if the process is underway', function() {
            spyOn(mockScope, '$digest').and.callThrough();
            mockScope.$$phase = true;

            scopeHelper.digestIfDigestNotInProgress(mockScope);

            expect(mockScope.$digest).not.toHaveBeenCalled();
        });
    });
});