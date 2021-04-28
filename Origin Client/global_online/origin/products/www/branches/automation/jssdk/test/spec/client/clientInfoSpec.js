describe('Origin Client Info API', function() {

    it('Origin.client.info.version()', function() {
        
        var expected = '10.0.0.16892';
        var result = Origin.client.info.version();
        
        expect(result).toEqual(expected, 'Origin.client.info.version() did not return the expected value');
    });
    
    it('Origin.client.info.versionNumber()', function() {
        
        var expected = 100016892;
        var result = Origin.client.info.versionNumber();
        
        expect(result).toEqual(expected, 'Origin.client.info.versionNumber() did not return the expected value');
    });
    
    it('Origin.client.info.isBeta()', function() {
        
        var expected = true;
        var result = Origin.client.info.isBeta();
        
        expect(result).toEqual(expected, 'Origin.client.info.isBeta() did not return the expected value');
    });
});

