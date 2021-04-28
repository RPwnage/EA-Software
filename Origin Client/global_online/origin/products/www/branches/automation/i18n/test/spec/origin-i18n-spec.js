describe('Origin i18n module', function() {
    var LocDictionaryFactory,
        LocFactory,
        TEST_DATA = {
            'test': 'abcd',
            'playingnow': '%friends% are playing now',
            'pluraltest': '{0} No friends are playing now | {1} You have %friends% friend playing now',
            'pluralrangetest': '{0} No friends are playing now | {1} You have %friends% friend playing now | [2,10] You have %friends% friends playing now',
            'negativetest': '{-1} You owe me a dollar | {0} You owe me nothing | {1} I owe you a dollar',
            'exclusiverangetestleft': '{0} No friends are playing now | ]1,9] you have %friends% playing now',
            'exclusiverangetestright': '{0} No friends are playing now | [2,10[ you have %friends% playing now',
            'exclusiverangetestleftright': '{0} No friends are playing now | ]1,10[ you have %friends% playing now',
            'exclusiverangetestinfleft': ']-Inf,0] You owe me money | [1,10] you have money',
            'exclusiverangetestinfright': '{0} You have no money | [1,+Inf[ you have %dollars% dollars',
            'mygames': {
                'test': 'efgh',
                'playingnow': 'You have %friends% playing right now'
            }
        };

    beforeEach(function() {
        angular.mock.module('origin-i18n-default-dictionary', function($provide) {
            // overwrite the default dictionary to avoid any problems
            $provide.constant('DEFAULT_DICTIONARY', {});
        });
        angular.mock.module('origin-i18n');
        inject(function(_LocDictionaryFactory_, _LocFactory_) {
            LocDictionaryFactory = _LocDictionaryFactory_;
            LocFactory = _LocFactory_;
        });
    });

    // Checking exported factories. This should be restricted to only the factories that other systems/components depend upon
    it('should have LocFactory', function() {
        expect(LocDictionaryFactory).toBeDefined();
    });

    describe('Getting strings from the dictionary', function() {
        beforeEach(function() {
            LocDictionaryFactory.initDefaults();
            LocDictionaryFactory.addDictionary({
                'data': TEST_DATA,
                'source': 'test',
                'prefix': ''
            });
        });

        it('should get simple translation', function() {
            expect(LocFactory.trans('test')).toBe('abcd');
        });

        it('should get namespaced translation', function() {
            expect(LocFactory.trans('test', {}, 'mygames')).toBe('efgh');
        });

        it('should get simple substitution', function() {
            expect(LocFactory.trans('playingnow', {'%friends%': 12})).toBe('12 are playing now');
        });

        it('should get namespaced substitution', function() {
            expect(LocFactory.trans('playingnow', {'%friends%': 13}, 'mygames')).toBe('You have 13 playing right now');
        });
    });

    describe('Handling dictionary lookup misses', function() {
        beforeEach(function() {
            LocDictionaryFactory.initDefaults();
            LocDictionaryFactory.addDictionary({
                'data': TEST_DATA,
                'source': 'test',
                'prefix': ''
            });
        });

        it('should mark a miss', function() {
            expect(LocFactory.trans('testmiss')).toBe('misstestmiss');
        });

        it('should mark a namespaced miss', function() {
            expect(LocFactory.trans('testmiss', {}, 'mygames')).toBe('misstestmiss');
        });

        it('should mark a namespaced miss with a miss: ', function() {
            expect(LocFactory.trans('testmiss', {}, 'mygamesmiss')).toBe('misstestmiss');
        });
    });

    describe('Pluralization', function() {
        beforeEach(function() {
            LocDictionaryFactory.initDefaults();
            LocDictionaryFactory.addDictionary({
                'data': TEST_DATA,
                'source': 'test',
                'prefix': ''
            });
        });

        it('Testing pluralization with numeric match', function() {
            expect(LocFactory.transChoice('pluraltest', 0, {'%friends%': 0})).toBe('No friends are playing now');
        });

        it('Testing pluralization with numeric match and substitution', function() {
            expect(LocFactory.transChoice('pluraltest', 1, {'%friends%': 1})).toBe('You have 1 friend playing now');
        });

        it('Testing pluralization miss', function() {
            expect(LocFactory.transChoice('pluraltestmiss', 0, {'%friends%': 1})).toBe('misspluraltestmiss');
        });


        //'negativetest': '{-1} You owe me a dollar | {0} You owe me nothing | {1} I owe you a dollar',
        it('Testing negatives with -1', function() {
            expect(LocFactory.transChoice('negativetest', -1)).toBe('You owe me a dollar');
        });

        it('Testing negatives with 0', function() {
            expect(LocFactory.transChoice('negativetest', 0)).toBe('You owe me nothing');
        });

        it('Testing negatives with 1', function() {
            expect(LocFactory.transChoice('negativetest', 1)).toBe('I owe you a dollar');
        });


        it('Testing pluralization range with numeric match with no substitution', function() {
            expect(LocFactory.transChoice('pluralrangetest', 0, {'%friends%': 0})).toBe('No friends are playing now');
        });

        it('Testing pluralization range with second numeric match with substitution', function() {
            expect(LocFactory.transChoice('pluralrangetest', 1, {'%friends%': 1})).toBe('You have 1 friend playing now');
        });

        it('Testing pluralization range with lower bound with substitution', function() {
            expect(LocFactory.transChoice('pluralrangetest', 2, {'%friends%': 2})).toBe('You have 2 friends playing now');
        });

        it('Testing pluralization range with upper bound with substitution', function() {
            expect(LocFactory.transChoice('pluralrangetest', 10, {'%friends%': 10})).toBe('You have 10 friends playing now');
        });

        it('Testing pluralization range with number in range with substitution', function() {
            expect(LocFactory.transChoice('pluralrangetest', 5, {'%friends%': 5})).toBe('You have 5 friends playing now');
        });

        it('Testing pluralization range with number out of range', function() {
            expect(LocFactory.transChoice('pluralrangetest', 11, {'%friends%': 11})).toBe('misspluralrangetest');
        });


        it('Testing exclusive range test left with number match', function() {
            expect(LocFactory.transChoice('exclusiverangetestleft', 0, {'%friends%': 0})).toBe('No friends are playing now');
        });

        it('Testing exlusive range test left with number out of range (lower)', function() {
            expect(LocFactory.transChoice('exclusiverangetestleft', 1, {'%friends%': 1})).toBe('missexclusiverangetestleft');
        });

        it('Testing exlusive range test left with number at lower range', function() {
            expect(LocFactory.transChoice('exclusiverangetestleft', 2, {'%friends%': 2})).toBe('you have 2 playing now');
        });

        it('Testing exlusive range test left with number in range', function() {
            expect(LocFactory.transChoice('exclusiverangetestleft', 5, {'%friends%': 5})).toBe('you have 5 playing now');
        });

        it('Testing exlusive range test left with number at higher range', function() {
            expect(LocFactory.transChoice('exclusiverangetestleft', 9, {'%friends%': 9})).toBe('you have 9 playing now');
        });

        it('Testing exlusive range test left with number out of range', function() {
            expect(LocFactory.transChoice('exclusiverangetestleft', 10, {'%friends%': 10})).toBe('missexclusiverangetestleft');
        });


        it('Testing exclusive range test right with number match', function() {
            expect(LocFactory.transChoice('exclusiverangetestright', 0, {'%friends%': 0})).toBe('No friends are playing now');
        });

        it('Testing exlusive range test right with number out of range (lower)', function() {
            expect(LocFactory.transChoice('exclusiverangetestright', 1, {'%friends%': 1})).toBe('missexclusiverangetestright');
        });

        it('Testing exlusive range test right with number at lower range', function() {
            expect(LocFactory.transChoice('exclusiverangetestright', 2, {'%friends%': 2})).toBe('you have 2 playing now');
        });

        it('Testing exlusive range test right with number in range', function() {
            expect(LocFactory.transChoice('exclusiverangetestright', 5, {'%friends%': 5})).toBe('you have 5 playing now');
        });

        it('Testing exlusive range test right with number at higher range', function() {
            expect(LocFactory.transChoice('exclusiverangetestright', 9, {'%friends%': 9})).toBe('you have 9 playing now');
        });

        it('Testing exlusive range test right with number out of range', function() {
            expect(LocFactory.transChoice('exclusiverangetestright', 10, {'%friends%': 10})).toBe('missexclusiverangetestright');
        });


        it('Testing exclusive range test leftright with number match', function() {
            expect(LocFactory.transChoice('exclusiverangetestleftright', 0, {'%friends%': 0})).toBe('No friends are playing now');
        });

        it('Testing exlusive range test leftright with number out of range (lower)', function() {
            expect(LocFactory.transChoice('exclusiverangetestleftright', 1, {'%friends%': 1})).toBe('missexclusiverangetestleftright');
        });

        it('Testing exlusive range test leftright with number at lower range', function() {
            expect(LocFactory.transChoice('exclusiverangetestleftright', 2, {'%friends%': 2})).toBe('you have 2 playing now');
        });

        it('Testing exlusive range test leftright with number in range', function() {
            expect(LocFactory.transChoice('exclusiverangetestleftright', 5, {'%friends%': 5})).toBe('you have 5 playing now');
        });

        it('Testing exlusive range test leftright with number at higher range', function() {
            expect(LocFactory.transChoice('exclusiverangetestleftright', 9, {'%friends%': 9})).toBe('you have 9 playing now');
        });

        it('Testing exlusive range test leftright with number out of range', function() {
            expect(LocFactory.transChoice('exclusiverangetestleftright', 10, {'%friends%': 10})).toBe('missexclusiverangetestleftright');
        });


        it('Testing exclusive range test infinity lower bound in inf range', function() {
            expect(LocFactory.transChoice('exclusiverangetestinfleft', -5)).toBe('You owe me money');
        });

        it('Testing exclusive range test infinity lower bound at upper bound of inf range', function() {
            expect(LocFactory.transChoice('exclusiverangetestinfleft', 0)).toBe('You owe me money');
        });

        it('Testing exclusive range test infinity lower bound at lower bound in closed range', function() {
            expect(LocFactory.transChoice('exclusiverangetestinfleft', 1)).toBe('you have money');
        });

        it('Testing exclusive range test infinity lower bound at upper bound in closed range', function() {
            expect(LocFactory.transChoice('exclusiverangetestinfleft', 10)).toBe('you have money');
        });

        it('Testing exclusive range test infinity lower bound out of range', function() {
            expect(LocFactory.transChoice('exclusiverangetestinfleft', 11)).toBe('missexclusiverangetestinfleft');
        });


        it('Testing exclusive range test infinity upper bound out of range', function() {
            expect(LocFactory.transChoice('exclusiverangetestinfright', -1, {'%dollars%': -1})).toBe('missexclusiverangetestinfright');
        });

        it('Testing exclusive range test infinity upper bound numeric match', function() {
            expect(LocFactory.transChoice('exclusiverangetestinfright', 0, {'%dollars%': 0})).toBe('You have no money');
        });

        it('Testing exclusive range test infinity upper bound at lower bound', function() {
            expect(LocFactory.transChoice('exclusiverangetestinfright', 1, {'%dollars%': 1})).toBe('you have 1 dollars');
        });

        it('Testing exclusive range test infinity upper bound in range', function() {
            expect(LocFactory.transChoice('exclusiverangetestinfright', 100, {'%dollars%': 100})).toBe('you have 100 dollars');
        });

        it('Testing cases where a decimal number is used in pluralization - must use a string serialization if a particular precision is required', function() {
            var numDollars = '1.60';
            expect(LocFactory.transChoice('exclusiverangetestinfright', numDollars, {'%dollars%': numDollars})).toBe('you have 1.60 dollars');
        });

        it('Testing cases where a mangled number is sent to the parser and replacer', function() {
            expect(LocFactory.transChoice('exclusiverangetestinfright', undefined, {'%dollars%': undefined})).toBe('You have no money');
        });

        it('Testing cases where a mangled number is sent to the replacer only', function() {
            expect(LocFactory.transChoice('exclusiverangetestinfright', 100, {'%dollars%': undefined})).toBe('you have undefined dollars');
        });

        it('Testing cases where a mangled number is sent to the parser only', function() {
            expect(LocFactory.transChoice('exclusiverangetestinfright', undefined, {'%dollars%': 100})).toBe('You have no money');
        });

        it('will not double escape characters used in substitution', function() {
            expect(LocFactory.substitute('I&apos;m a happy, cheerful doe&copy; I cost %dollars%', {'%dollars%': '$0.00'})).toBe('I&apos;m a happy, cheerful doe&copy; I cost $0.00');
        });
    });
});
