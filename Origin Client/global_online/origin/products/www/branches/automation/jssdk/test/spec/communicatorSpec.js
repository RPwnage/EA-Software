
describe('Origin Communicator', function() {

    it('test simple on and fire', function() {
        var events = new Origin.utils.Communicator(),
            counter = 0;

        function callback() {
            counter++;
        }

        events.on('customevent', callback);
        events.fire('customevent');

        expect(counter).toEqual(1);
    });

    it('test simple on and fire twice', function() {
        var events = new Origin.utils.Communicator(),
            counter = 0;

        function callback() {
            counter++;
        }

        // callback should fire twice
        events.on('customevent', callback);
        events.fire('customevent');
        events.fire('customevent');

        expect(counter).toEqual(2);
    });


    it('test simple off', function() {
        var events = new Origin.utils.Communicator(),
            counter = 0;

        function callback() {
            counter++;
        }

        // should not fire
        events.on('customevent', callback);
        events.off('customevent', callback);
        events.fire('customevent');

        expect(counter).toEqual(0);
    });

    it('test removing an event twice', function() {
        var events = new Origin.utils.Communicator(),
            counter = 0;

        function callback() {
            counter++;
        }

        // shouldn't throw an error
        events.on('customevent', callback);
        events.off('customevent', callback);
        events.off('customevent', callback);
        events.fire('customevent');

        expect(counter).toEqual(0);
    });

    it('test removing an event and firing after', function() {
        var events = new Origin.utils.Communicator(),
            counter = 0;

        function callback() {
            counter++;
        }

        // should only fire once
        events.on('customevent', callback);
        events.fire('customevent');
        events.off('customevent', callback);
        events.fire('customevent');

        expect(counter).toEqual(1);
    });

    it('test once should only fire once', function() {
        var events = new Origin.utils.Communicator(),
            counter = 0;

        function callback() {
            counter++;
        }

        // should only fire once
        events.once('customevent', callback);
        events.fire('customevent');
        events.fire('customevent');

        expect(counter).toEqual(1);
    });

    it('test using detach before event fires', function() {
        var events = new Origin.utils.Communicator(),
            counter = 0,
            handle;

        function callback() {
            counter++;
        }

        handle = events.on('customevent', callback);
        handle.detach();
        events.fire('customevent');

        expect(counter).toEqual(0);
    });

    it('test using detach after event fires', function() {
        var events = new Origin.utils.Communicator(),
            counter = 0,
            handle;

        function callback() {
            counter++;
        }

        handle = events.on('customevent', callback);
        events.fire('customevent');
        handle.detach();
        events.fire('customevent');

        expect(counter).toEqual(1);
    });

    it('test detaching the handle twice', function() {
        var events = new Origin.utils.Communicator(),
            counter = 0,
            handle;

        function callback() {
            counter++;
        }

        handle = events.on('customevent', callback);
        events.fire('customevent');

        handle.detach();
        handle.detach();
        events.fire('customevent');

        expect(counter).toEqual(1);
    });

    it('test reattaching', function () {
        var events = new Origin.utils.Communicator(),
            counter = 0,
            handle;

        function callback() {
            counter++;
        }

        handle = events.on('customevent', callback);
        events.fire('customevent');
        handle.detach();
        events.fire('customevent');
        handle.attach();
        events.fire('customevent');

        expect(counter).toEqual(2);
    });

    it('test reattaching twice', function() {
        var events = new Origin.utils.Communicator(),
            counter = 0,
            handle;

        function callback() {
            counter++;
        }

        handle = events.on('customevent', callback);
        events.fire('customevent');
        handle.detach();
        events.fire('customevent');
        handle.attach();
        handle.attach();
        events.fire('customevent');

        expect(counter).toEqual(2);
    });

});