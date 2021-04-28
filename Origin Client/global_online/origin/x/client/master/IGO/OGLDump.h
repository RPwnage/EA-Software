//
//  OGLDump.h
//  IGO
//
//  Created by Frederic Meraud on 11/1/12.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//

#ifndef OGLDUMP_H
#define OGLDUMP_H

#ifdef ORIGIN_MAC

#import <OpenGL/gl.h>

namespace OriginIGO
{
    class OGLDump
    {
        public:
            OGLDump();
            ~OGLDump();
        
            void StartUpdate();
            void EndUpdate();

        private:
    };
}

#endif // MAC_OSX

#endif
