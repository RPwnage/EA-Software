#pragma once

namespace fb
{
    enum MediaHint
    {
        None = 0
    };

    class StreamInstall
    {
    public:
        static void init();
	
	
	    static void deinit();
	

	
	    static bool superbundleExists(const char* superbundle, MediaHint hint);
	
	    static void mountSuperbundle(const char* superbundle, MediaHint hint, bool optional);
	

	    static void unmountSuperbundle(const char* superbundle);

	    static bool isInstalling();
	
	    static bool isSuperbundleInstalled(const char* fileName);

	    static bool isGroupInstalled(const char* groupName);
    };
}//fb