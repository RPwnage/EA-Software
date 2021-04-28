describe('Origin Client Content Operation Queue API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.events.CLIENT_OPERATIONQUEUE_ADDEDTOCOMPLETE', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_OPERATIONQUEUE_ADDEDTOCOMPLETE should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_OPERATIONQUEUE_ADDEDTOCOMPLETE, cb);

            expect(result).toEqual(undefined);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_ADDEDTOCOMPLETE, cb);

        window.OriginContentOperationQueueController.addedToComplete();

    });

    it('Origin.events.CLIENT_OPERATIONQUEUE_COMPLETELISTCLEARED', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_OPERATIONQUEUE_COMPLETELISTCLEARED should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_OPERATIONQUEUE_COMPLETELISTCLEARED, cb);

            expect(result).toEqual(undefined);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_COMPLETELISTCLEARED, cb);

        window.OriginContentOperationQueueController.completeListCleared();

    });

    it('Origin.events.CLIENT_OPERATIONQUEUE_ENQUEUED', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_OPERATIONQUEUE_ENQUEUED should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_OPERATIONQUEUE_ENQUEUED, cb);

            expect(result).toEqual(undefined);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_ENQUEUED, cb);

        window.OriginContentOperationQueueController.enqueued();

    });

    it('Origin.events.CLIENT_OPERATIONQUEUE_HEADBUSY', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_OPERATIONQUEUE_HEADBUSY should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_OPERATIONQUEUE_HEADBUSY, cb);

            expect(result).toEqual(undefined);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_HEADBUSY, cb);

        window.OriginContentOperationQueueController.headBusy();

    });

    it('Origin.events.CLIENT_OPERATIONQUEUE_HEADCHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_OPERATIONQUEUE_HEADCHANGED should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_OPERATIONQUEUE_HEADCHANGED, cb);

            expect(result).toEqual(undefined);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_HEADCHANGED, cb);

        window.OriginContentOperationQueueController.headChanged();

    });

    it('Origin.events.CLIENT_OPERATIONQUEUE_REMOVED', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_OPERATIONQUEUE_REMOVED should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_OPERATIONQUEUE_REMOVED, cb);

            expect(result).toEqual(undefined);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_REMOVED, cb);

        window.OriginContentOperationQueueController.removed();

    });

    it('Origin.client.contentOperationQueue.remove()', function() {

        expect(Origin.client.contentOperationQueue.remove('')).toEqual(undefined);

    });

    it('Origin.client.contentOperationQueue.pushToTop()', function() {

        expect(Origin.client.contentOperationQueue.pushToTop('')).toEqual(undefined);

    });

    it('Origin.client.contentOperationQueue.clearCompleteList()', function() {

        expect(Origin.client.contentOperationQueue.clearCompleteList()).toEqual(undefined);

    });

    it('Origin.client.contentOperationQueue.entitlementsCompletedOfferIdList()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.contentOperationQueue.entitlementsCompletedOfferIdList().then(function(result) {

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        }).catch(function(error) {

            expect().toFail(error.message);
            asyncTicker.clear(done);

        });

    });

    it('Origin.client.contentOperationQueue.entitlementsQueued()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.contentOperationQueue.entitlementsQueued().then(function(result) {

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        }).catch(function(error) {

            expect().toFail(error.message);
            asyncTicker.clear(done);

        });

    });

    it('Origin.client.contentOperationQueue.headOfferId()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.contentOperationQueue.headOfferId().then(function(result) {

            expect(result).toEqual(1);
            asyncTicker.clear(done);

        }).catch(function(error) {

            expect().toFail(error.message);
            asyncTicker.clear(done);

        });

    });


    it('Origin.client.contentOperationQueue.index()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.contentOperationQueue.index().then(function(result) {

            expect(result).toEqual(0);
            asyncTicker.clear(done);

        }).catch(function(error) {

            expect().toFail(error.message);
            asyncTicker.clear(done);

        });

    });

    it('Origin.client.contentOperationQueue.isHeadBusy()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.contentOperationQueue.isHeadBusy().then(function(result) {

            expect(result).toEqual(false);
            asyncTicker.clear(done);

        }).catch(function(error) {

            expect().toFail(error.message);
            asyncTicker.clear(done);

        });

    });

    it('Origin.client.contentOperationQueue.isInQueue()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.contentOperationQueue.isInQueue('id').then(function(result) {

            expect(result).toEqual(true);
            asyncTicker.clear(done);

        }).catch(function(error) {

            expect().toFail(error.message);
            asyncTicker.clear(done);

        });

    });

    it('Origin.client.contentOperationQueue.isInQueueOrCompleted()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.contentOperationQueue.isInQueueOrCompleted('id').then(function(result) {

            expect(result).toEqual(true);
            asyncTicker.clear(done);

        }).catch(function(error) {

            expect().toFail(error.message);
            asyncTicker.clear(done);

        });

    });

    it('Origin.client.contentOperationQueue.isParentInQueue()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.contentOperationQueue.isParentInQueue('id').then(function(result) {

            expect(result).toEqual(false);
            asyncTicker.clear(done);

        }).catch(function(error) {

            expect().toFail(error.message);
            asyncTicker.clear(done);

        });

    });

    it('Origin.client.contentOperationQueue.isQueueSkippingEnabled()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.contentOperationQueue.isQueueSkippingEnabled('id').then(function(result) {

            expect(result).toEqual(false);
            asyncTicker.clear(done);

        }).catch(function(error) {

            expect().toFail(error.message);
            asyncTicker.clear(done);

        });

    });

});