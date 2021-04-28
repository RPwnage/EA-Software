describe('Origin Utils API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.utils.getProperty(obj, chain)', function() {
		
		var null_object = null;
		
		var test_object = {
			attribute_1: {
				inner_attribute: "value",
			},
			attribute_2: "value_2",
		}
		
		//1
		var attributes = [];
		var expected_result = null;
		var result = Origin.utils.getProperty(null_object, attributes);
		expect(result).toEqual(expected_result);
		
		//2
		attributes = [];
		expected_result = test_object;
		result = Origin.utils.getProperty(test_object, attributes);
		expect(result).toEqual(expected_result);
		
		//3
		attributes = ["attribute_2"];
		expected_result = test_object.attribute_2;
		result = Origin.utils.getProperty(test_object, attributes);
		expect(result).toEqual(expected_result);
		
		//4
		attributes = ["attribute_1", "inner_attribute"];
		expected_result = test_object.attribute_1.inner_attribute;
		result = Origin.utils.getProperty(test_object, attributes);
		expect(result).toEqual(expected_result);
		
		//5
		attributes = ["incorrect_attribute"];
		expected_result = null;
		result = Origin.utils.getProperty(test_object, attributes);
		expect(result).toEqual(expected_result);
		
		//6
		attributes = ["attribute_1", "incorrect_attribute"];
		expected_result = null;
		result = Origin.utils.getProperty(test_object, attributes);
		expect(result).toEqual(expected_result);
		
    });
	
	it('Origin.utils.isChainDefined(obj, chain)', function() {
		
		var test_object = {
			attribute_1: {
				inner_attribute: "value",
			},
			attribute_2: "value_2",
		}
		
		//1
		var attributes = [];
		var expected_result = true;
		var result = Origin.utils.isChainDefined(null, attributes);
		expect(result).toEqual(expected_result);
		
		//2
		var attributes = [];
		var result = Origin.utils.isChainDefined(test_object, attributes);
		expect(result).toBeTruthy('null attribute should exist');
		
		//3
		attributes = ["attribute_2"];
		result = Origin.utils.isChainDefined(test_object, attributes);
		expect(result).toBeTruthy('"attribute_2" attribute chain should exist');
		
		//4
		attributes = ["attribute_1", "inner_attribute"];
		result = Origin.utils.isChainDefined(test_object, attributes);
		expect(result).toBeTruthy('"attribute_1.inner_attribute" attribute chain should exist');
		
		//5
		attributes = ["incorrect_attribute"];
		result = Origin.utils.isChainDefined(test_object, attributes);
		expect(result).toBeFalsy('"incorrect_attribute" attribute chain should not exist');
		
		//6
		attributes = ["attribute_1", "inner_attribute", "incorrect_attribute"];
		result = Origin.utils.isChainDefined(test_object, attributes);
		expect(result).toBeFalsy('"attribute_1.inner_attribute.incorrect_attribute" attribute chain should not exist');
		
    });
	
	it('Origin.utils.mix(obj1, obj2)', function() {
		
		var null_object = null;
		
		var test_object_1 = {
			attribute_1: {
				inner_attribute: "value_1",
			},
			attribute_2: "value_2",
			attribute_3: "value_3",
		}
		
		var test_object_2 = {
			attribute_1: {
				inner_attribute: "new_value_1",
			},
			attribute_2: "new_value_2",
			attribute_4: "new_value_4",
		}
		
		//1
		expected_mix_result = test_object_1;
		Origin.utils.mix(test_object_1, null_object);
		expect(test_object_1).toEqual(expected_mix_result);
		
		//2
		expected_mix_result = {
			attribute_1: {
				inner_attribute: "new_value_1",
			},
			attribute_2: "new_value_2",
			attribute_3: "value_3",
			attribute_4: "new_value_4",
		}
		Origin.utils.mix(test_object_1, test_object_2);
		expect(test_object_1).toEqual(expected_mix_result);

    });
	
	it('Origin.utils.os()', function() {
		
		var osName = ''
		if (window.navigator.appVersion.indexOf('Win') !== -1) {
            osName = 'PCWIN';
        } else if (window.navigator.appVersion.indexOf('Mac') !== -1) {
            osName = 'MAC';
        } else if (window.navigator.appVersion.indexOf('Linux') !== -1) {
            osName = 'Linux';
        }
		
		result = Origin.utils.os();
		expect(result).toEqual(osName);

    });
	
	it('Origin.utils.replaceTemplatedValuesInConfig()', function() {

		var configObject_no_overrides = {
			'hostname':{
				'base':'origin.com',
				'basedata':'https://{version}{env}{cmsstage}api1.{base}',
				'baseapi':'https://{version}{env}{cmsstage}api2.{base}'
			},
			'urls':{
				'abaseGameEntitlementsTEST':'{basedata}/ecommerce2/basegames/{userId}?machine_hash=1',
				'abaseapiGameEntitlementsTEST':'{baseapi}/ecommerce2/basegames/{userId}?machine_hash=1'
			}
		}
		
		var expected_configObject_no_overrides_result = {
			'hostname':{
				'base':'origin.com',
				'basedata':'https://api1.origin.com',
				'baseapi':'https://api2.origin.com'
			},
			'urls':{
				'abaseGameEntitlementsTEST':'https://api1.origin.com/ecommerce2/basegames/{userId}?machine_hash=1',
				'abaseapiGameEntitlementsTEST':'https://api2.origin.com/ecommerce2/basegames/{userId}?machine_hash=1'
			}
		}
		
		var configObject_with_overrides = {
			'overrides':{
				'version': 'version',
				'env': 'env',
				'cmsstage': 'cmsstage'
			},
			'hostname':{
				'base':'origin.com',
				'basedata':'https://{version}{env}{cmsstage}api1.{base}',
				'baseapi':'https://{version}{env}{cmsstage}api2.{base}'
			},
			'urls':{
				'abaseGameEntitlementsTEST':'{basedata}/ecommerce2/basegames/{userId}?machine_hash=1',
				'abaseapiGameEntitlementsTEST':'{baseapi}/ecommerce2/basegames/{userId}?machine_hash=1'
			}
		}
		
		var expected_configObject_with_overrides_result = {
			'overrides':{
				'version': 'version',
				'env': 'env',
				'cmsstage': 'cmsstage'
			},
			'hostname':{
				'base':'origin.com',
				'basedata':'https://version.env.cmsstage.api1.origin.com',
				'baseapi':'https://version.env.cmsstage.api2.origin.com'
			},
			'urls':{
				'abaseGameEntitlementsTEST':'https://version.env.cmsstage.api1.origin.com/ecommerce2/basegames/{userId}?machine_hash=1',
				'abaseapiGameEntitlementsTEST':'https://version.env.cmsstage.api2.origin.com/ecommerce2/basegames/{userId}?machine_hash=1'
			}
		}
		
		//1
		Origin.utils.replaceTemplatedValuesInConfig(configObject_no_overrides);
		expect(configObject_no_overrides).toEqual(expected_configObject_no_overrides_result);
		
		//2
		Origin.utils.replaceTemplatedValuesInConfig(configObject_with_overrides);
		expect(configObject_with_overrides).toEqual(expected_configObject_with_overrides_result);

    });

});
