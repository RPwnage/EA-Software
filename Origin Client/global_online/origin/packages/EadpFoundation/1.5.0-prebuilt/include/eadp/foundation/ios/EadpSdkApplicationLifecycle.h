// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#import <UIKit/UIApplication.h>

@protocol EadpSdkApplicationLifecycleObserver <NSObject>
@optional
- (void)onApplicationLaunchWithOptions:(NSDictionary *)launchOptions;
- (void)onApplicationTerminate;
- (void)onApplicationForeground;
- (void)onApplicationBackground;
- (void)onApplicationActive;
- (void)onApplicationInactive;
- (BOOL)onApplicationOpenedByUrl:(NSURL*)url
               sourceApplication:(NSString*)sourceApplication
                      annotation:(id)annotation;
- (void)onPushNotificationReceivedWithUserInfo:(NSDictionary*)userInfo;

@end

@protocol EadpSdkApplicationLifecycle <NSObject>

/*!
 * Noitify application is opened by url from the AppDelegate method
 *
 * - (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
 *
 * The parameters are pass-throughs from the iOS delegate method.
 * @param application application object
 * @param URL to open
 * @param sourceApplication bundle ID of the application requesting this application to open the URL
 * @param annotation property-list object supplied by source application to communicate information to this application.
 *
 * @return TRUE if the url is handled by a observer, otherwise FALSE
 */
- (BOOL)notifyApplicationOpenedByUrlWithApplication:(UIApplication*)application
                                            openURL:(NSURL*)url
                                  sourceApplication:(NSString*)sourceApplication
                                         annotation:(id)annotation;

/*!
 * Notify PushNotification is received from the AppDelegate method
 *
 * - (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo
 *
 * The parameters are pass-throughs from the iOS delegate method.
 */
- (void)notifyPushNotificationReceivedWithApplication:(UIApplication*)application andUserInfo:(NSDictionary*)userInfo;

/*!
 * Register an application lifecycle observer (internal)
 *
 * @note Internal usage only, should only be used by native code in EADP SDK module to
 * directly hook the necessary lifecycle event trigger, avoiding reroute through c++ code.
 */
- (void)registerApplicationLifecycleObserver:(id<EadpSdkApplicationLifecycleObserver>)observer;

/*!
 * Unregister an application lifecycle observer (internal)
 *
 * @note Internal usage only, should only be used by native code in EADP SDK module to
 * directly unhook the necessary lifecycle event trigger, avoiding reroute through c++ code.
 */
- (void)unregisterApplicationLifecycleObserver:(id<EadpSdkApplicationLifecycleObserver>)observer;

@end

@interface EadpSdkApplicationLifecycle : NSObject

/*!
 * Get EadpSdkApplicationLifecycle instance which implements EadpSdkApplicationLifecycle protocol
 */
+ (id<EadpSdkApplicationLifecycle>)getInstance;

@end
