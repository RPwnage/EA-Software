describe('Origin Avatar API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.avatar.avatarInfoByUserIds()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.avatar.avatarInfoByUserIds('1000120382991;12306939085', 'AVATAR_SZ_SMALL').then(function(result) {

            var expected = [{
                userId : '1000120382991',
                avatar : {
                    avatarId : '1',
                    orderNumber : '1',
                    isRecent : 'false',
                    link : 'https://stage.download.dm.origin.com/integration/avatar/int/1/1/40x40.JPEG',
                    typeId : '3',
                    typeName : 'premium',
                    statusId : '2',
                    statusName : 'approved',
                    galleryId : '1',
                    galleryName : 'default'
                }
            }, {
                userId : '12306939085',
                avatar : {
                    avatarId : '11456',
                    isRecent : 'false',
                    link : 'https://stage.download.dm.origin.com/integration/avatar/int/userAvatar/11456/40x40.JPEG'
                }
            }];

            for (var i = 0; i < expected.length; i++) {
                expect(result).toContain(expected[i]);
            }

            expect(result.length).toEqual(expected.length, 'returned result should have ' + expected.length + ' entries');
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

});
