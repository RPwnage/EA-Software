//
//  main.m
//  OriginBootstrap
//
//  Created by Kevin Wertman on 8/8/12.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    // EBIBUGS-20922 - Expand the permissions for the app bundle to ensure that all users can update Origin.
    umask( 0 ); // ensure that, by default, file creation calls do not mask out any UNIX permissions.
    
    return NSApplicationMain(argc, (const char **)argv);
}
