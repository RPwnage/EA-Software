#include "MounterOSX.h"

#include <QDebug>

#include <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>
#include <Foundation/NSAutoreleasePool.h>

#include "services/downloader/MounterQt.h"

namespace
{
    
}

namespace Origin
{
namespace Platform
{

class MounterOSX::NSInternals {
    public:
        NSTask *task;
        NSPipe *out_pipe;
        NSFileHandle *out_file;
};

MounterOSX::MounterOSX()
{
    NS = new MounterOSX::NSInternals;

    NS->task = [[NSTask alloc] init];

    NS->out_pipe = [[NSPipe alloc] init];  
    NS->out_file = [NS->out_pipe fileHandleForReading];
}

void MounterOSX::mount(const QString& path, Origin::Downloader::MounterQt* caller) 
{
 
    // Specifying the correct encoding is massively important for hdiutils to actually load the dmg
    //    
    CFStringRef image_path = CFStringCreateWithCString(NULL, path.toStdString().c_str(), kCFStringEncodingUTF8);

    [NS->task setLaunchPath:@"/usr/bin/hdiutil"];
    [NS->task setArguments:[NSArray arrayWithObjects: @"attach", (NSString *)image_path, @"-noverify", nil]];

    [NS->task setStandardOutput: NS->out_pipe];

    // The notification center isn't delivering NSTaskDidTerminate events, so synchronously wait for the task
    // to finish
    
    [NS->task launch];
    [NS->task waitUntilExit];
    
    CFRelease(image_path);
    
    // Read stdout so caller can parse mount location if they wsih
    NSData *data = [[NS->out_pipe fileHandleForReading] readDataToEndOfFile];
    NSString *str = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    
    const char* bytes = [str UTF8String];
    QString qbytes(bytes);
    
    [str release];
    
    int exit_code = [NS->task terminationStatus];
    
    caller->mount_terminated(exit_code, qbytes);
}
    
void MounterOSX::unmount(const QString& path, Origin::Downloader::MounterQt* caller) 
{
        
    // Specifying the correct encoding is massively important for hdiutils to actually load the dmg
    //
    CFStringRef image_path = CFStringCreateWithCString(NULL, path.toStdString().c_str(), kCFStringEncodingUTF8);
        
    [NS->task setLaunchPath:@"/usr/bin/hdiutil"];
    [NS->task setArguments:[NSArray arrayWithObjects: @"detach", (NSString *)image_path, nil]];
        
    [NS->task setStandardOutput: NS->out_pipe];
        
    // The notification center isn't delivering NSTaskDidTerminate events, so synchronously wait for the task
    // to finish
        
    [NS->task launch];
    [NS->task waitUntilExit];
        
    CFRelease(image_path);
        
    // Read stdout so caller can parse mount location if they wsih
    NSData *data = [[NS->out_pipe fileHandleForReading] readDataToEndOfFile];
    NSString *str = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        
    const char* bytes = [str UTF8String];
    QString qbytes(bytes);
        
    [str release];
    
    int exit_code = [NS->task terminationStatus];
    caller->unmount_terminated(exit_code, qbytes);
}

MounterOSX::~MounterOSX()
{
    delete NS;
}

}
}
