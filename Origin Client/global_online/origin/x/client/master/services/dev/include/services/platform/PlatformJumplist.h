//  PlatformJumplist.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef PLATFORMJUMPLIST_H
#define PLATFORMJUMPLIST_H

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API PlatformJumplist
        {
        public:
            static void init();
            static void release();
            static void create_jumplist(bool isUnderAgeUser);
            static void clear_jumplist();
			static void add_launched_game(QString title, QString product_id, QString icon_path, int itemSubType);
			static void clear_recently_played();
			static void update_progress(double progress, bool paused);
			static unsigned int get_recent_list_size();

        };
    }
}

#endif //PLATFORM_H
