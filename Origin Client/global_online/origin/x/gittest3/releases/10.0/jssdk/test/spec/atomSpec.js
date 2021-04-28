describe('Origin Atom API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.atom.atomUserInfoByUserIds()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.atom.atomUserInfoByUserIds('1000120382991,12306939085').then(function(result) {

            var expected = [{
                userId : '1000120382991',
                personaId : '1000060182991',
                EAID : 'ithasbeenthere'
            }, {
                userId : '12306939085',
                personaId : '408187599',
                EAID : 'itsnotthere',
                firstName : null,
                lastName : null
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

    it('Origin.atom.atomGameUsage()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.atom.atomGameUsage('182555', '71654').then(function(result) {

            var expected = {
                gameId : '182555',
                total : '0',
                MultiplayerId : '71654',
                lastSession : '0',
                lastSessionEndTimeStamp : '0'
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

});
