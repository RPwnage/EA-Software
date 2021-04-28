describe('Origin User API', function() {
    it('Origin.user.getAccessToken', function(){
        expect(Origin.user.accessToken()).toEqual('QVQwOjEuMDozLjA6MTk6UUVGd042cDZsakgwVmc4aVZlWkVjTGd0MG5XQWtVcmkwM1U6OTU1MDI6bWxsYzE');
    });

    it('Origin.user.originId', function(){
        expect(Origin.user.originId()).toEqual('jssdkmockupdata');
    });

    it('Origin.user.personalId', function(){
        expect(Origin.user.personaId()).toEqual(1000101795502);
    });

    it('Origin.user.getUnderAge', function(){
        expect(Origin.user.underAge()).toEqual(false);
    });

    it('Origin.user.getUnderAge', function(){
        expect(Origin.user.dob()).toEqual('1980-01-01');
    });

    it('Origin.user.getUserStatus', function(){
        expect(Origin.user.userStatus()).toEqual('ACTIVE');
    });

    it('Origin.user.email()', function(){
        expect(Origin.user.email()).toEqual('eatest.jssdkmockupdata@eadtest.ea.com');
    });

    it('Origin.user.emailStatus()', function(){
        expect(Origin.user.emailStatus()).toEqual('UNKNOWN');
    });

    it('Origin.user.globalEmailSignup()', function(){
        expect(Origin.user.globalEmailSignup()).toEqual(false);
    });

    it('Origin.user.tfaSignup()', function(){
        expect(Origin.user.tfaSignup()).toEqual(false);
    });

    it('Origin.user.registrationDate()', function(){
        expect(Origin.user.registrationDate()).toEqual('2014-08-19T22:10Z');
    });
});
