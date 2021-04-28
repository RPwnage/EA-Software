/*************************************************************************************************/
/*!
\file inputsanitizertest.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "matchmaker/inputsanitizertest.h"

#include "framework/blaze.h"
#include "test/mock/mocklogger.h"
#include "gamemanager/inputsanitizer.h"
#include "EATDF/tdffactory.h"

namespace Test
{
namespace inputsanitizer 
{

    void InputSanitizerTest::SetUp()
    {
        EA::TDF::TdfFactory::fixupTypes();
    }

    void InputSanitizerTest::TearDown()
    {

    }

    TEST_F(InputSanitizerTest, IntSanitizerTests)
    {
        Blaze::GameManager::SanitizerDefinitionMap sanitizersConfig;

        sanitizersConfig["IntSanitizer"] = sanitizersConfig.allocate_element();
        Blaze::GameManager::SanitizerDefinition& intSanitizer = *sanitizersConfig["IntSanitizer"];
        intSanitizer.setOutAttr("ATTR_OUT_INT");
        intSanitizer.setOutProperty("PROP_OUT_INT");
        {
            intSanitizer.getSanitizer()["IntSanitizer"] = intSanitizer.getSanitizer().allocate_element();
            Blaze::GameManager::TemplateAttributeMapping& mapping = *intSanitizer.getSanitizer()["IntSanitizer"];

            mapping["input"] = mapping.allocate_element();
            mapping["input"]->setAttrName("ATTR_INPUT_INT");
            mapping["input"]->setPropertyName("PROP_INPUT_INT");
            mapping["input"]->getDefault().set(0);

            mapping["default"] = mapping.allocate_element();
            mapping["default"]->setAttrName("ATTR_DEFAULT_INT");
            mapping["default"]->setPropertyName("PROP_DEFAULT_INT");
            mapping["default"]->getDefault().set(0);

            mapping["min"] = mapping.allocate_element();
            mapping["min"]->setAttrName("ATTR_MIN_INT");
            mapping["min"]->setPropertyName("PROP_MIN_INT");
            mapping["min"]->getDefault().set(INT64_MIN);

            mapping["max"] = mapping.allocate_element();
            mapping["max"]->setAttrName("ATTR_MAX_INT");
            mapping["max"]->setPropertyName("PROP_MAX_INT");
            mapping["max"]->getDefault().set(INT64_MAX);

            mapping["failureOp"] = mapping.allocate_element();
            mapping["failureOp"]->setAttrName("ATTR_FAILURE_OP_INT");
            mapping["failureOp"]->setPropertyName("PROP_FAILURE_OP_INT");
            mapping["failureOp"]->getDefault().set(Blaze::GameManager::FAIL_INPUT);
        }

        Blaze::ConfigureValidationErrors validationErrors;
        EXPECT_TRUE(inputSanitizerConfigValidation(sanitizersConfig, validationErrors));


        Blaze::GameManager::SanitizerNameList sanitizersToApply;
        sanitizersToApply.push_back("IntSanitizer");

        const char8_t* failingAttribute = nullptr;
        Blaze::GameManager::TemplateAttributes attributes;
        Blaze::GameManager::PropertyNameMap properties;

        attributes["ATTR_INPUT_INT"] = attributes.allocate_element();
        attributes["ATTR_INPUT_INT"]->set(200);

        properties["PROP_INPUT_INT"] = attributes.allocate_element();
        properties["PROP_INPUT_INT"]->set(100);


        // 100 input [min, max]  (no range limits)
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_TRUE(attributes["ATTR_OUT_INT"]->getValue().asInt64() == 100);
        EXPECT_TRUE(properties["PROP_OUT_INT"]->getValue().asInt64() == 100);


        properties["PROP_MAX_INT"] = properties.allocate_element();
        properties["PROP_MAX_INT"]->set(250);
        properties["PROP_INPUT_INT"]->set(1000);

        // 1000 input [min, 250]  FAIL_INPUT:
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::GAMEMANAGER_ERR_INPUT_SANITIZER_FAILURE);


        attributes["ATTR_FAILURE_OP_INT"] = attributes.allocate_element();
        attributes["ATTR_FAILURE_OP_INT"]->set(Blaze::GameManager::CLAMP_VALUE);
        properties["PROP_INPUT_INT"]->set(1000);

        // 1000 input [min, 250]  CLAMP_VALUE:
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_TRUE(attributes["ATTR_OUT_INT"]->getValue().asInt64() == 250);
        EXPECT_TRUE(properties["PROP_OUT_INT"]->getValue().asInt64() == 250);


        properties["PROP_MIN_INT"] = properties.allocate_element();
        properties["PROP_MIN_INT"]->set(50);
        properties["PROP_DEFAULT_INT"] = properties.allocate_element();
        properties["PROP_DEFAULT_INT"]->set(77);
        attributes["ATTR_FAILURE_OP_INT"]->set(Blaze::GameManager::USE_DEFAULT);
        properties["PROP_INPUT_INT"]->set(10);

        // 10 input [50, 250]  USE_DEFAULT (77):
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_TRUE(attributes["ATTR_OUT_INT"]->getValue().asInt64() == 77);
        EXPECT_TRUE(properties["PROP_OUT_INT"]->getValue().asInt64() == 77);
    }
    TEST_F(InputSanitizerTest, StringSanitizerTests)
    {
        Blaze::GameManager::SanitizerDefinitionMap sanitizersConfig;

        sanitizersConfig["StringSanitizer"] = sanitizersConfig.allocate_element();
        Blaze::GameManager::SanitizerDefinition& stringSanitizer = *sanitizersConfig["StringSanitizer"];
        stringSanitizer.setOutAttr("ATTR_OUT_STR");
        stringSanitizer.setOutProperty("PROP_OUT_STR");
        {
            stringSanitizer.getSanitizer()["StringSanitizer"] = stringSanitizer.getSanitizer().allocate_element();
            Blaze::GameManager::TemplateAttributeMapping& mapping = *stringSanitizer.getSanitizer()["StringSanitizer"];

            mapping["input"] = mapping.allocate_element();
            mapping["input"]->setAttrName("ATTR_INPUT_STR");
            mapping["input"]->setPropertyName("PROP_INPUT_STR");
            mapping["input"]->getDefault().set("");

            mapping["default"] = mapping.allocate_element();
            mapping["default"]->setAttrName("ATTR_DEFAULT_STR");
            mapping["default"]->setPropertyName("PROP_DEFAULT_STR");
            mapping["default"]->getDefault().set("HELLO WORLD");

            mapping["maxLength"] = mapping.allocate_element();
            mapping["maxLength"]->setAttrName("ATTR_MAX_STR");
            mapping["maxLength"]->setPropertyName("PROP_MAX_STR");
            mapping["maxLength"]->getDefault().set(0);

            Blaze::GameManager::ListString emptyList;
            mapping["allowList"] = mapping.allocate_element();
            mapping["allowList"]->setAttrName("ATTR_ALLOW_STR");
            mapping["allowList"]->setPropertyName("PROP_ALLOW_STR");
            mapping["allowList"]->getDefault().set(emptyList);

            mapping["blockList"] = mapping.allocate_element();
            mapping["blockList"]->setAttrName("ATTR_BLOCK_STR");
            mapping["blockList"]->setPropertyName("PROP_BLOCK_STR");
            mapping["blockList"]->getDefault().set(emptyList);

            mapping["failureOp"] = mapping.allocate_element();
            mapping["failureOp"]->setAttrName("ATTR_FAILURE_OP_STR");
            mapping["failureOp"]->setPropertyName("PROP_FAILURE_OP_STR");
            mapping["failureOp"]->getDefault().set(Blaze::GameManager::FAIL_INPUT);
        }

        Blaze::ConfigureValidationErrors validationErrors;
        EXPECT_TRUE(inputSanitizerConfigValidation(sanitizersConfig, validationErrors));


        Blaze::GameManager::SanitizerNameList sanitizersToApply;
        sanitizersToApply.push_back("StringSanitizer");

        const char8_t* failingAttribute = nullptr;
        Blaze::GameManager::TemplateAttributes attributes;
        Blaze::GameManager::PropertyNameMap properties;

        attributes["ATTR_INPUT_STR"] = attributes.allocate_element();
        attributes["ATTR_INPUT_STR"]->set("BOO");

        properties["PROP_INPUT_STR"] = attributes.allocate_element();
        properties["PROP_INPUT_STR"]->set("OKAY");

        // OKAY input  (no limits)
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_TRUE(attributes["ATTR_OUT_STR"]->getValue().asString() == "OKAY");
        EXPECT_TRUE(properties["PROP_OUT_STR"]->getValue().asString() == "OKAY");


        properties["PROP_MAX_STR"] = properties.allocate_element();
        properties["PROP_MAX_STR"]->set(10);
        properties["PROP_INPUT_STR"]->set("A NEW AGE HAS STARTED");

        // "A NEW AGE HAS STARTED" input [max 10]  FAIL_INPUT:
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::GAMEMANAGER_ERR_INPUT_SANITIZER_FAILURE);


        attributes["ATTR_FAILURE_OP_STR"] = attributes.allocate_element();
        attributes["ATTR_FAILURE_OP_STR"]->set(Blaze::GameManager::CLAMP_VALUE);
        properties["PROP_INPUT_STR"]->set("0123456789101214");

        // "0123456789101214" input [max 10]  CLAMP_VALUE:
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_TRUE(attributes["ATTR_OUT_STR"]->getValue().asString() == "0123456789");
        EXPECT_TRUE(properties["PROP_OUT_STR"]->getValue().asString() == "0123456789");


        properties["PROP_MAX_STR"]->set(0);

        Blaze::GameManager::ListString allowList;
        allowList.push_back("OKAY");
        allowList.push_back("BOO");
        allowList.push_back("HELLO WORLD");

        properties["PROP_ALLOW_STR"] = properties.allocate_element();
        properties["PROP_ALLOW_STR"]->set(allowList);
        properties["PROP_DEFAULT_STR"] = properties.allocate_element();
        properties["PROP_DEFAULT_STR"]->set("HELLO WORLD");
        attributes["ATTR_FAILURE_OP_STR"]->set(Blaze::GameManager::USE_DEFAULT);
        properties["PROP_INPUT_STR"]->set("YUMMY");

        // "YUMMY" input [OKAY, BOO, HELLO WORLD allowed]  USE_DEFAULT:
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_TRUE(attributes["ATTR_OUT_STR"]->getValue().asString() == "HELLO WORLD");
        EXPECT_TRUE(properties["PROP_OUT_STR"]->getValue().asString() == "HELLO WORLD");

        attributes["ATTR_FAILURE_OP_STR"]->set(Blaze::GameManager::CHOOSE_RANDOM);
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_TRUE(attributes["ATTR_OUT_STR"]->getValue().asString() != "YUMMY");
        EXPECT_TRUE(properties["PROP_OUT_STR"]->getValue().asString() != "YUMMY");


        allowList.clear();
        properties["PROP_ALLOW_STR"]->set(allowList);

        Blaze::GameManager::ListString blockList;
        blockList.push_back("OKAY");
        blockList.push_back("BOO");
        blockList.push_back("HELLO GUYS");

        properties["PROP_BLOCK_STR"] = properties.allocate_element();
        properties["PROP_BLOCK_STR"]->set(blockList);
        attributes["ATTR_FAILURE_OP_STR"]->set(Blaze::GameManager::USE_DEFAULT);

        // "YUMMY" input [OKAY, BOO, HELLO WORLD blocked]  USE_DEFAULT:
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_TRUE(attributes["ATTR_OUT_STR"]->getValue().asString() == "YUMMY");
        EXPECT_TRUE(properties["PROP_OUT_STR"]->getValue().asString() == "YUMMY");


        properties["PROP_INPUT_STR"]->set("BOO");

        // "BOO" input [OKAY, BOO, HELLO WORLD blocked]  USE_DEFAULT:
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_TRUE(attributes["ATTR_OUT_STR"]->getValue().asString() == "HELLO WORLD");
        EXPECT_TRUE(properties["PROP_OUT_STR"]->getValue().asString() == "HELLO WORLD");

        attributes["ATTR_FAILURE_OP_STR"]->set(Blaze::GameManager::CHOOSE_RANDOM);
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::GAMEMANAGER_ERR_INPUT_SANITIZER_FAILURE);

    }
    TEST_F(InputSanitizerTest, ListStringSanitizerTests)
    {
        Blaze::GameManager::SanitizerDefinitionMap sanitizersConfig;

        sanitizersConfig["ListStringSanitizer"] = sanitizersConfig.allocate_element();
        Blaze::GameManager::SanitizerDefinition& stringSanitizer = *sanitizersConfig["ListStringSanitizer"];
        stringSanitizer.setOutAttr("ATTR_OUT_STR");
        stringSanitizer.setOutProperty("PROP_OUT_STR");
        {
            stringSanitizer.getSanitizer()["ListStringSanitizer"] = stringSanitizer.getSanitizer().allocate_element();
            Blaze::GameManager::TemplateAttributeMapping& mapping = *stringSanitizer.getSanitizer()["ListStringSanitizer"];

            mapping["emptyFailureOp"] = mapping.allocate_element();
            mapping["emptyFailureOp"]->setAttrName("ATTR_EMPTY_FAILURE_OP_STR");
            mapping["emptyFailureOp"]->setPropertyName("PROP_EMPTY_FAILURE_OP_STR");
            mapping["emptyFailureOp"]->getDefault().set(Blaze::GameManager::FAIL_INPUT);

            mapping["input"] = mapping.allocate_element();
            mapping["input"]->setAttrName("ATTR_INPUT_LIST_STR");
            mapping["input"]->setPropertyName("PROP_INPUT_LIST_STR");
            // mapping["input"]->getDefault().set("");
/*
            mapping["key.input"] = mapping.allocate_element();
            mapping["key.input"]->setAttrName("ATTR_INPUT_KEY_STR");
            mapping["key.input"]->setPropertyName("PROP_INPUT_KEY_STR");
            mapping["key.input"]->getDefault().set("");
                     
            mapping["key.default"] = mapping.allocate_element();
            mapping["key.default"]->setAttrName("ATTR_DEFAULT_KEY_STR");
            mapping["key.default"]->setPropertyName("PROP_DEFAULT_KEY_STR");
            mapping["key.default"]->getDefault().set("HELLO WORLD");
                     
            mapping["key.maxLength"] = mapping.allocate_element();
            mapping["key.maxLength"]->setAttrName("ATTR_MAX_KEY_STR");
            mapping["key.maxLength"]->setPropertyName("PROP_MAX_KEY_STR");
            mapping["key.maxLength"]->getDefault().set(0);

            mapping["key.allowList"] = mapping.allocate_element();
            mapping["key.allowList"]->setAttrName("ATTR_ALLOW_KEY_STR");
            mapping["key.allowList"]->setPropertyName("PROP_ALLOW_KEY_STR");
            mapping["key.allowList"]->getDefault().set(emptyList);
                    
            mapping["key.blockList"] = mapping.allocate_element();
            mapping["key.blockList"]->setAttrName("ATTR_BLOCK_KEY_STR");
            mapping["key.blockList"]->setPropertyName("PROP_BLOCK_KEY_STR");
            mapping["key.blockList"]->getDefault().set(emptyList);
                    
            mapping["key.failureOp"] = mapping.allocate_element();
            mapping["key.failureOp"]->setAttrName("ATTR_FAILURE_OP_KEY_STR");
            mapping["key.failureOp"]->setPropertyName("PROP_FAILURE_OP_KEY_STR");
            mapping["key.failureOp"]->getDefault().set(Blaze::GameManager::FAIL_INPUT);
*/

            mapping["value.input"] = mapping.allocate_element();
            mapping["value.input"]->setAttrName("ATTR_INPUT_STR");
            mapping["value.input"]->setPropertyName("PROP_INPUT_STR");
            mapping["value.input"]->getDefault().set("");

            mapping["value.default"] = mapping.allocate_element();
            mapping["value.default"]->setAttrName("ATTR_DEFAULT_STR");
            mapping["value.default"]->setPropertyName("PROP_DEFAULT_STR");
            mapping["value.default"]->getDefault().set("HELLO WORLD");

            mapping["value.maxLength"] = mapping.allocate_element();
            mapping["value.maxLength"]->setAttrName("ATTR_MAX_STR");
            mapping["value.maxLength"]->setPropertyName("PROP_MAX_STR");
            mapping["value.maxLength"]->getDefault().set(0);

            Blaze::GameManager::ListString emptyList;
            mapping["value.allowList"] = mapping.allocate_element();
            mapping["value.allowList"]->setAttrName("ATTR_ALLOW_STR");
            mapping["value.allowList"]->setPropertyName("PROP_ALLOW_STR");
            mapping["value.allowList"]->getDefault().set(emptyList);

            mapping["value.blockList"] = mapping.allocate_element();
            mapping["value.blockList"]->setAttrName("ATTR_BLOCK_STR");
            mapping["value.blockList"]->setPropertyName("PROP_BLOCK_STR");
            mapping["value.blockList"]->getDefault().set(emptyList);

            mapping["value.failureOp"] = mapping.allocate_element();
            mapping["value.failureOp"]->setAttrName("ATTR_FAILURE_OP_STR");
            mapping["value.failureOp"]->setPropertyName("PROP_FAILURE_OP_STR");
            mapping["value.failureOp"]->getDefault().set(Blaze::GameManager::FAIL_INPUT);
        }

        Blaze::ConfigureValidationErrors validationErrors;
        EXPECT_TRUE(inputSanitizerConfigValidation(sanitizersConfig, validationErrors));


        Blaze::GameManager::SanitizerNameList sanitizersToApply;
        sanitizersToApply.push_back("ListStringSanitizer");

        const char8_t* failingAttribute = nullptr;
        Blaze::GameManager::TemplateAttributes attributes;
        Blaze::GameManager::PropertyNameMap properties;

        attributes["ATTR_INPUT_STR"] = attributes.allocate_element();
        attributes["ATTR_INPUT_STR"]->set("BOO");

        properties["PROP_INPUT_STR"] = attributes.allocate_element();
        properties["PROP_INPUT_STR"]->set("OKAY");

        Blaze::GameManager::ListString inputList;
        inputList.push_back("OKAY");
        inputList.push_back("BOO");
        inputList.push_back("HELLO WORLD");

        attributes["ATTR_INPUT_LIST_STR"] = attributes.allocate_element();
        attributes["ATTR_INPUT_LIST_STR"]->set(inputList);

        // OKAY, BOO, HELLO WORLD input  (no limits)
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_TRUE(attributes["ATTR_OUT_STR"]->getValue().asList().vectorSize() == 3);
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&attributes["ATTR_OUT_STR"]->getValue().asList())[0] == "OKAY");
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&attributes["ATTR_OUT_STR"]->getValue().asList())[1] == "BOO");
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&attributes["ATTR_OUT_STR"]->getValue().asList())[2] == "HELLO WORLD");
        EXPECT_TRUE(properties["PROP_OUT_STR"]->getValue().asList().vectorSize() == 3);
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&properties["PROP_OUT_STR"]->getValue().asList())[0] == "OKAY");
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&properties["PROP_OUT_STR"]->getValue().asList())[1] == "BOO");
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&properties["PROP_OUT_STR"]->getValue().asList())[2] == "HELLO WORLD");


        properties["PROP_MAX_STR"] = properties.allocate_element();
        properties["PROP_MAX_STR"]->set(10);
        inputList.clear();
        attributes["ATTR_INPUT_LIST_STR"]->set(inputList);

        //  no input [max 10]  FAIL_INPUT:
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::GAMEMANAGER_ERR_INPUT_SANITIZER_FAILURE);


        attributes["ATTR_FAILURE_OP_STR"] = attributes.allocate_element();
        attributes["ATTR_FAILURE_OP_STR"]->set(Blaze::GameManager::CLAMP_VALUE);
        inputList.push_back("0123456789101214");
        attributes["ATTR_INPUT_LIST_STR"]->set(inputList);

        // "0123456789101214" input [max 10]  CLAMP_VALUE:
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_TRUE(attributes["ATTR_OUT_STR"]->getValue().asList().vectorSize() == 1);
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&attributes["ATTR_OUT_STR"]->getValue().asList())[0] == "0123456789");
        EXPECT_TRUE(properties["PROP_OUT_STR"]->getValue().asList().vectorSize() == 1);
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&properties["PROP_OUT_STR"]->getValue().asList())[0] == "0123456789");


        properties["PROP_MAX_STR"]->set(0);

        Blaze::GameManager::ListString allowList;
        allowList.push_back("OKAY");
        allowList.push_back("BOO");
        allowList.push_back("HELLO WORLD");

        properties["PROP_ALLOW_STR"] = properties.allocate_element();
        properties["PROP_ALLOW_STR"]->set(allowList);
        properties["PROP_DEFAULT_STR"] = properties.allocate_element();
        properties["PROP_DEFAULT_STR"]->set("HELLO WORLD");
        attributes["ATTR_FAILURE_OP_STR"]->set(Blaze::GameManager::USE_DEFAULT);
        inputList.clear();
        inputList.push_back("YUMMY");
        inputList.push_back("BOO");
        attributes["ATTR_INPUT_LIST_STR"]->set(inputList);

        // "YUMMY" input [OKAY, BOO, HELLO WORLD allowed]  USE_DEFAULT:
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_TRUE(attributes["ATTR_OUT_STR"]->getValue().asList().vectorSize() == 2);
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&attributes["ATTR_OUT_STR"]->getValue().asList())[0] == "HELLO WORLD");
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&attributes["ATTR_OUT_STR"]->getValue().asList())[1] == "BOO");
        EXPECT_TRUE(properties["PROP_OUT_STR"]->getValue().asList().vectorSize() == 2);
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&properties["PROP_OUT_STR"]->getValue().asList())[0] == "HELLO WORLD");
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&properties["PROP_OUT_STR"]->getValue().asList())[1] == "BOO");


        allowList.clear();
        properties["PROP_ALLOW_STR"]->set(allowList);

        Blaze::GameManager::ListString blockList;
        blockList.push_back("OKAY");
        blockList.push_back("BOO");
        blockList.push_back("HELLO GUYS");

        properties["PROP_BLOCK_STR"] = properties.allocate_element();
        properties["PROP_BLOCK_STR"]->set(blockList);

        // "YUMMY" input [OKAY, BOO, HELLO WORLD blocked]  USE_DEFAULT:
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_TRUE(attributes["ATTR_OUT_STR"]->getValue().asList().vectorSize() == 2);
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&attributes["ATTR_OUT_STR"]->getValue().asList())[0] == "YUMMY");
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&attributes["ATTR_OUT_STR"]->getValue().asList())[1] == "HELLO WORLD");
        EXPECT_TRUE(properties["PROP_OUT_STR"]->getValue().asList().vectorSize() == 2);
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&properties["PROP_OUT_STR"]->getValue().asList())[0] == "YUMMY");
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&properties["PROP_OUT_STR"]->getValue().asList())[1] == "HELLO WORLD");


        attributes["ATTR_FAILURE_OP_STR"]->set(Blaze::GameManager::REMOVE_ENTRY);
        

        // "BOO" input [OKAY, BOO, HELLO WORLD blocked]  USE_DEFAULT:
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_TRUE(attributes["ATTR_OUT_STR"]->getValue().asList().vectorSize() == 1);
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&attributes["ATTR_OUT_STR"]->getValue().asList())[0] == "YUMMY");
        EXPECT_TRUE(properties["PROP_OUT_STR"]->getValue().asList().vectorSize() == 1);
        EXPECT_TRUE((*(Blaze::GameManager::ListString*)&properties["PROP_OUT_STR"]->getValue().asList())[0] == "YUMMY");
    }

    TEST_F(InputSanitizerTest, PingsiteSanitizerTests)
    {
        Blaze::GameManager::SanitizerDefinitionMap sanitizersConfig;

        sanitizersConfig["PingSiteSanitizer"] = sanitizersConfig.allocate_element();
        Blaze::GameManager::SanitizerDefinition& psSanitizer = *sanitizersConfig["PingSiteSanitizer"];
        psSanitizer.setOutProperty("PROP_OUT_PINGSITES");
        {
            psSanitizer.getSanitizer()["PingSiteSanitizer"] = psSanitizer.getSanitizer().allocate_element();
            Blaze::GameManager::TemplateAttributeMapping& mapping = *psSanitizer.getSanitizer()["PingSiteSanitizer"];

            mapping["hostedPingSites"] = mapping.allocate_element();
            mapping["hostedPingSites"]->setAttrName("ATTR_HOSTED_PS_INT");

            mapping["requestedPingSites"] = mapping.allocate_element();
            mapping["requestedPingSites"]->setAttrName("ATTR_REQ_PS_INT");

            mapping["maximumLatency"] = mapping.allocate_element();
            mapping["maximumLatency"]->getDefault().set(150);

            mapping["mergedPingsiteLatencyMap"] = mapping.allocate_element();
            mapping["mergedPingsiteLatencyMap"]->setAttrName("ATTR_PS_LAT_MAP_LIST");
            mapping["mergedPingsiteLatencyMap"]->setMergeOp(Blaze::GameManager::MERGE_AVERAGE);
        }

        Blaze::ConfigureValidationErrors validationErrors;
        EXPECT_TRUE(inputSanitizerConfigValidation(sanitizersConfig, validationErrors));


        Blaze::GameManager::SanitizerNameList sanitizersToApply;
        sanitizersToApply.push_back("PingSiteSanitizer");

        const char8_t* failingAttribute = nullptr;
        Blaze::GameManager::TemplateAttributes attributes;
        Blaze::GameManager::PropertyNameMap properties;

        Blaze::GameManager::ListString hostedPingsites;
        hostedPingsites.push_back("A");
        hostedPingsites.push_back("B");
        hostedPingsites.push_back("C");
        hostedPingsites.push_back("D");
        //hostedPingsites.push_back("E");

        attributes["ATTR_HOSTED_PS_INT"] = attributes.allocate_element();
        attributes["ATTR_HOSTED_PS_INT"]->set(hostedPingsites);


        Blaze::GameManager::ListString requestedPingsites;
        requestedPingsites.push_back("A");
        //requestedPingsites.push_back("B");
        requestedPingsites.push_back("C");
        requestedPingsites.push_back("D");
        requestedPingsites.push_back("E");

        attributes["ATTR_REQ_PS_INT"] = attributes.allocate_element();
        attributes["ATTR_REQ_PS_INT"]->set(requestedPingsites);


        Blaze::GameManager::PropertyValueMapList pingsiteLatency;  // Latency for the 'players'
        auto curLatencyMap = pingsiteLatency.allocate_element();
        (*curLatencyMap)["A"] = 50;
        (*curLatencyMap)["B"] = 50;
        (*curLatencyMap)["C"] = 50;
        (*curLatencyMap)["D"] = 250;  // Exceed Max latency
        (*curLatencyMap)["E"] = 50;
        (*curLatencyMap)["F"] = 50;
        pingsiteLatency.push_back(curLatencyMap);

        attributes["ATTR_PS_LAT_MAP_LIST"] = attributes.allocate_element();
        attributes["ATTR_PS_LAT_MAP_LIST"]->set(pingsiteLatency);

        // 100 input [min, max]  (no range limits)
        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_EQ(properties["PROP_OUT_PINGSITES"]->getValue().asList().vectorSize(), 2);
        EXPECT_EQ(properties["PROP_OUT_PINGSITES"]->getValue().asList().getValueType(), EA::TDF::TDF_ACTUAL_TYPE_STRING);

        auto result = (Blaze::GameManager::ListString*) &properties["PROP_OUT_PINGSITES"]->getValue().asList();
        EXPECT_EQ((*result)[0], "A");
        EXPECT_EQ((*result)[1], "C");


        curLatencyMap = pingsiteLatency.allocate_element();
        (*curLatencyMap)["A"] = 500;
        (*curLatencyMap)["B"] = 50;
        (*curLatencyMap)["C"] = 50;
        (*curLatencyMap)["D"] = 25;  // 
        (*curLatencyMap)["E"] = 50;
        (*curLatencyMap)["F"] = 50;
        pingsiteLatency.push_back(curLatencyMap);
        attributes["ATTR_PS_LAT_MAP_LIST"]->set(pingsiteLatency);

        EXPECT_EQ(Blaze::GameManager::applySanitizers(sanitizersConfig, sanitizersToApply, attributes, properties, failingAttribute), Blaze::ERR_OK);
        EXPECT_EQ(properties["PROP_OUT_PINGSITES"]->getValue().asList().vectorSize(), 2);
        EXPECT_EQ(properties["PROP_OUT_PINGSITES"]->getValue().asList().getValueType(), EA::TDF::TDF_ACTUAL_TYPE_STRING);

        result = (Blaze::GameManager::ListString*) &properties["PROP_OUT_PINGSITES"]->getValue().asList();
        EXPECT_EQ((*result)[0], "C");
        EXPECT_EQ((*result)[1], "D");

    }
}
}