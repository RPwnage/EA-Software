describe "Sonar Client", ->
    client = null
    handler = (e) -> ('test')
    
    beforeEach ->
        client = new Sonar({token: 'test'})
        client.bind 'placeholder1', -> null
        client.bind 'placeholder2', -> null
        @addMatchers toContain: (expected) ->
            for found in this.actual
                return true if expected is found
            false
    
    it "can bind an event to a handler", ->
        client.bind 'audio', handler
        expect(client.handlers['audio']).toContain handler
        
    it "can unbind a specific event handler", ->
        client.bind 'audio', handler
        client.unbind 'audio', handler
        expect(client.handlers['audio']).not.toContain handler
        
#    it "can unbind all handlers for specific event, ->
#        asd
        
    it "can unbind all handlers", ->
        client.unbind()
        expect(client.handlers['placeholder1']).not.toBeDefined()
        expect(client.handlers['placeholder2']).not.toBeDefined()
    
    it "can trigger an event to a handler", ->
        data = 'Testing data'
        event = null
        client.bind 'audio', (e) -> event = e
        client.trigger 'audio', data
        expect(data).toEqual(event.data)

    it "can trigger an event to a wildcard handler", ->
        data = 'Testing data'
        c = 0
        client.bind '*', (e) -> c++
        client.trigger 'audio', data
        client.trigger 'testing', data
        client.trigger 'next', data
        expect(c).toEqual(3)
        
    it "has browser plugin installed", ->
        expect(client._isPluginInstalled()).toBeTruthy()

    # check for version
    # check for embed

console.log 'sonar-spec.coffee declared'