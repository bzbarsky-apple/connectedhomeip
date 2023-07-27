/**
 *    Copyright (c) 2023 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "MTRDeviceControllerDataStore.h"

static NSString * ResumptionByNodeIDKey(NSNumber * nodeID)
{
    // TODO: FIXME: Figure out whether this is a good key string.
    return [NSString stringWithFormat:@"caseResumptionByNodeID/%llx", nodeID.unsignedLongLongValue];
}

static NSString * ResumptionByResumptionIDKey(NSData * resumptionID)
{
    // TODO: FIXME: Figure out whether this is a good key string.
    return
        [NSString stringWithFormat:@"caseResumptionByResumptionID/%s", [resumptionID base64EncodedStringWithOptions:0].UTF8String];
}

static dispatch_queue_t sWorkQueue;

@implementation MTRDeviceControllerDataStore {
    id<MTRDeviceControllerStorageDelegate> _delegate;
    MTRDeviceController * _controller;
}

+ (void)initialize
{
    sWorkQueue = dispatch_queue_create(
        "org.csa-iot.matter.framework.controller-storage.workqueue", DISPATCH_QUEUE_SERIAL_WITH_AUTORELEASE_POOL);
}

- (instancetype)initWithController:(MTRDeviceController *)controller
                   storageDelegate:(id<MTRDeviceControllerStorageDelegate>)storageDelegate
{
    if (!(self = [super init])) {
        return nil;
    }

    _controller = controller;
    _delegate = storageDelegate;
    return self;
}

- (nullable MTRCASESessionResumptionInfo *)findResumptionInfoByNodeID:(NSNumber *)nodeID
{
    return [self _findResumptionInfoWithKey:ResumptionByNodeIDKey(nodeID)];
}

- (nullable MTRCASESessionResumptionInfo *)findResumptionInfoByResumptionID:(NSData *)resumptionID
{
    return [self _findResumptionInfoWithKey:ResumptionByResumptionIDKey(resumptionID)];
}

- (void)storeResumptionInfo:(MTRCASESessionResumptionInfo *)resumptionInfo
{
    auto * oldInfo = [self findResumptionInfoByNodeID:resumptionInfo.nodeID];
    dispatch_sync(sWorkQueue, ^{
        if (oldInfo != nil) {
            // Remove old resumption id key.  No need to do that for the
            // node id, because we are about to overwrite it.
            [_delegate controller:_controller
                removeValueForKey:ResumptionByResumptionIDKey(oldInfo.resumptionID)
                    securityLevel:MTRStorageSecurityLevelSecure
                         dataType:MTRStorageDataTypeController];
        }

        [_delegate controller:_controller
                   storeValue:resumptionInfo
                       forKey:ResumptionByNodeIDKey(resumptionInfo.nodeID)
                securityLevel:MTRStorageSecurityLevelSecure
                     dataType:MTRStorageDataTypeController];
        [_delegate controller:_controller
                   storeValue:resumptionInfo
                       forKey:ResumptionByResumptionIDKey(resumptionInfo.resumptionID)
                securityLevel:MTRStorageSecurityLevelSecure
                     dataType:MTRStorageDataTypeController];
    });
}

- (nullable MTRCASESessionResumptionInfo *)_findResumptionInfoWithKey:(NSString *)key
{
    __block id resumptionInfo;
    dispatch_sync(sWorkQueue, ^{
        resumptionInfo = [_delegate controller:_controller
                                   valueForKey:key
                                 securityLevel:MTRStorageSecurityLevelSecure
                                      dataType:MTRStorageDataTypeController];
    });

    if (resumptionInfo == nil) {
        return nil;
    }

    if (![resumptionInfo isKindOfClass:[MTRCASESessionResumptionInfo class]]) {
        return nil;
    }

    return resumptionInfo;
}

@end

@implementation MTRCASESessionResumptionInfo

#pragma mark - NSSecureCoding

static NSString * const sNodeIDKey = @"nodeID";
static NSString * const sResumptionIDKey = @"resumptionID";
static NSString * const sSharedSecretKey = @"sharedSecret";
static NSString * const sCATsKey = @"CATs";

+ (BOOL)supportsSecureCoding
{
    return YES;
}

- (instancetype _Nullable)initWithCoder:(NSCoder *)decoder
{
    _nodeID = [decoder decodeObjectOfClass:[NSNumber class] forKey:sNodeIDKey];
    _resumptionID = [decoder decodeObjectOfClass:[NSData class] forKey:sResumptionIDKey];
    _sharedSecret = [decoder decodeObjectOfClass:[NSData class] forKey:sSharedSecretKey];
    _caseAuthenticatedTags = [decoder decodeObjectOfClass:[NSSet class] forKey:sCATsKey];
    return self;
}

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeObject:self.nodeID forKey:sNodeIDKey];
    [coder encodeObject:self.resumptionID forKey:sResumptionIDKey];
    [coder encodeObject:self.sharedSecret forKey:sSharedSecretKey];
    [coder encodeObject:self.caseAuthenticatedTags forKey:sCATsKey];
}

@end

NSSet<Class> * MTRDeviceControllerStorageClasses()
{
    static NSSet * const sStorageClasses =
        [NSSet setWithArray:@[ [NSNumber class], [NSData class], [NSSet class], [MTRCASESessionResumptionInfo class] ]];
    return sStorageClasses;
}
