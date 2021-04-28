describe('Origin windows - externalUrl: Browser', function() {
    beforeEach(function() {
        spyOn(window, 'open').and.callFake(function() {
            return true;
        });
    });

    it('Will open a new window using window.open in the browser context', function() {
        Origin.windows.externalUrl('http://www.origin.com/foo/?bar=baz');
        expect(window.open).toHaveBeenCalled();
        expect(window.open).toHaveBeenCalledWith('http://www.origin.com/foo/?bar=baz');
    });
});

describe('Origin windows - externalUrl: Client with a fully qualified URL', function() {
    beforeEach(function() {
        spyOn(Origin.client, 'isEmbeddedBrowser').and.callFake(function(){return true;});
        spyOn(Origin.client.desktopServices, 'asyncOpenUrl').and.callFake(function(){return true;});
    });

    it('Will launch an external browser in the Origin client context for a fully qualified URL', function() {
        Origin.windows.externalUrl('http://www.origin.com/foo/?bar=baz');
        expect(Origin.client.desktopServices.asyncOpenUrl).toHaveBeenCalled();
        expect(Origin.client.desktopServices.asyncOpenUrl).toHaveBeenCalledWith('http://www.origin.com/foo/?bar=baz');
    });
});
describe('Origin windows - externalUrl: RelativeUrl', function() {
    beforeEach(function() {
        spyOn(Origin.client, 'isEmbeddedBrowser').and.callFake(function(){return true;});
        spyOn(Origin.client.desktopServices, 'asyncOpenUrl').and.callFake(function(){return true;});
    });

    it('Will launch an external browser in the Origin client with the full FQDN added in the case of a relative path', function() {
        Origin.windows.externalUrl('/foo/?bar=baz');
        expect(Origin.client.desktopServices.asyncOpenUrl).toHaveBeenCalled();
        expect(Origin.client.desktopServices.asyncOpenUrl).toHaveBeenCalledWith('file:///foo/?bar=baz');
    });
});