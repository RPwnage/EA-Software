describe('Origin ID Obfuscate API', function() {
    it('will encode a user id into an obfuscated user id for public sharing', function(done) {
        Origin.idObfuscate.encodeUserId('12302901487')
            .then(function(data) {
                expect(data).toEqual({
                    id: '2vSA3WXkfG4sEJClJKmvMQ--'
                });

                done();
            })
            .catch(function(err) {
                fail(err);
                done(err);
            });
    });

    it('decode a publicly shared obfuscated user id', function(done) {
        Origin.idObfuscate.decodeUserId('2vSA3WXkfG4sEJClJKmvMQ--')
            .then(function(data) {
                expect(data).toEqual({
                    id: '12302901487'
                });

                done();
            })
            .catch(function(err) {
                fail(err);
                done(err);
            });
    });
});