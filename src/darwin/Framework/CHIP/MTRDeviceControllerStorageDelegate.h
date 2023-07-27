/**
 *    Copyright (c) 2022-2023 Project CHIP Authors
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

#import <Foundation/Foundation.h>
#import <Matter/MTRDeviceController.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, MTRStorageSecurityLevel) {
    // Data must be stored in secure (encrypted) storage.
    MTRStorageSecurityLevelSecure,
    // Data may be stored in the clear.
    MTRStorageSecurityLevelNotSecure,
} MTR_NEWLY_AVAILABLE;

/**
 * The type of data item being stored.
 *
 * TODO: Maybe this should be more like:
 *
 * typedef NS_ENUM(NSUInteger, MTRStorageSharingType) {
 *      MTRStorageSharingTypeNotShared = 0,    (Persist Locally)
 *      MTRStorageSharingTypeAllControllers,   (Share among all controllers with different node IDs)
 *      MTRStorageSharingTypeSharedControllerNodeID, (Share among all controllers with SAME node IDs)
 * } MTR_NEWLY_AVAILABLE;
 */
typedef NS_ENUM(NSUInteger, MTRStorageDataType) {
    /**
     * Data item is related to the controller itself.
     */
    MTRStorageDataTypeController,
    /**
     * Data item is related to a device the controller is interacting with.
     * This is mutually exclusive with ControllerData.  Data items with this
     * flag may be synchronized between different controllers that are on the
     * same fabric.
     */
    MTRStorageDataTypeDevice,
} MTR_NEWLY_AVAILABLE;

/**
 * Protocol for storing and retrieving controller-specific data.
 *
 * Implementations of this protocol MUST keep two things in mind:
 *
 * 1) The controller provided to the delegate methods may not be fully
 *    initialized when the callbacks are called.  The only safe thing to do with
 *    it is to get its controllerID.
 *
 * 2) The delegate method calls will happen serially, not concurrently, on some arbitrary
 *    thread.  During execution of the delegate methods, all Matter work will be
 *    blocked until the method completes.  Attempting to call any Matter API
 *    from inside the delegate methods, apart from de-serializing and
 *    serializing the items being stored, is likely to lead to deadlocks.
 */
MTR_NEWLY_AVAILABLE
@protocol MTRDeviceControllerStorageDelegate <NSObject>
@required
/**
 * Return the stored value for the given key, if any, for the provided
 * controller.  Returns nil if there is no stored value.
 *
 * securityLevel and dataType will always be the same for any given key value
 * and are just present here to help locate the data if storage location is
 * separated out by security level and data type.
 *
 * The set of classes that might be decoded by this function is available by
 * calling MTRDeviceControllerStorageClasses().
 */
- (nullable id<NSSecureCoding>)controller:(MTRDeviceController *)controller
                              valueForKey:(NSString *)key
                            securityLevel:(MTRStorageSecurityLevel)securityLevel
                                 dataType:(MTRStorageDataType)dataType;

/**
 * Store a value for the given key.  Returns whether the store succeeded.
 *
 * securityLevel and dataType will always be the same for any given key value
 * and are present here as a hint to how the value should be stored.
 */
- (BOOL)controller:(MTRDeviceController *)controller
        storeValue:(id<NSSecureCoding>)value
            forKey:(NSString *)key
     securityLevel:(MTRStorageSecurityLevel)securityLevel
          dataType:(MTRStorageDataType)dataType;

/**
 * Remove the stored value for the given key.  Returns whether the remove succeeded.
 *
 * securityLevel and dataType will always be the same for any given key value
 * and are just present here to help locate the data if storage location is
 * separated out by security level and data type.
 */
- (BOOL)controller:(MTRDeviceController *)controller
    removeValueForKey:(NSString *)key
        securityLevel:(MTRStorageSecurityLevel)securityLevel
             dataType:(MTRStorageDataType)dataType;
@end

// TODO: FIXME: Is this a sane place to put this API?
MTR_EXTERN MTR_NEWLY_AVAILABLE NSSet<Class> * MTRDeviceControllerStorageClasses(void);

NS_ASSUME_NONNULL_END
