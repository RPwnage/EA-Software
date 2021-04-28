#include "framework/blaze.h"
#include "framework/config/config_file.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"
#include "framework/test/blazeunittest.h"

using namespace Blaze;

class TestConfig
{
public:
    void testConfigFile()
    {
        Logger::initialize();

        ConfigFile* configFile = ConfigFile::createFromFile("sample.properties");

        if (configFile == nullptr)
        {
            std::cout << "Config file could not be read properly\n";
            return;
        }

        std::cout << "Dumping parsed props file\n";
        while ( configFile->hasNext() )
        {
            const char8_t* key = configFile->nextKey();
            std::cout << "A property is: [" << key << "=" << configFile->getString(key, "MISSING") << "]\n";
        }

        std::cout << "\n" << "Some simple properties...\n";

        std::cout << "The properties file has firstInt: " << configFile->getInt32("firstInt", -1) << "\n";
        std::cout << "The properties file has intAsHex: " << configFile->getInt64("intAsHex", -2) << "\n";
        std::cout << "The properties file has name: " << configFile->getString("name", "MISSING") << "\n";
        std::cout << "The properties file has pi: " << configFile->getDouble("pi", -1.5) << "\n";
        std::cout << "The properties file has tongueTwister: " << configFile->getString("tongueTwister", "DOH") << "\n";

        std::cout << "Dumping the fibonacci sequence...\n";

        const ConfigSequence* configSequence = configFile->getSequence("fibonacci");
        while ( configSequence != nullptr && configSequence->hasNext() )
            std::cout << "Value: " << configSequence->nextInt32(-1) << "\n";

        std::cout << "Dumping the sports sequence...\n";

        configSequence = configFile->getSequence("sports");
        while (configSequence != nullptr && configSequence->hasNext())
            std::cout << "A sport is: " << configSequence->nextString("MISSING") << "\n";

        std::cout << "Dumping the nested map of game params...\n";

        const ConfigMap* configMap = configFile->getMap("game");
        if ( configMap != nullptr )
        {
            const ConfigMap* configMap2 = configMap->getMap("madden");
            if ( configMap2 != nullptr )
            {
                std::cout << "Madden port is " << configMap2->getInt32("port", -1);
                std::cout << " and timeout is " << configMap2->getInt32("timeout", -2) << "\n";
            }
            else
                std::cout << "game.madden properties were not found\n";
        }
        else
            std::cout << "game properties were not found\n";

        //std::cout << "\nPress <enter> to end...\n";

        //char8_t ch = 0;
        //while (!std::cin.get(ch))
        //{
            // Wait for user to hit enter before quitting
        //}
    }
};


// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_CONFIG\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestConfig testConfig;
    testConfig.testConfigFile();
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}

