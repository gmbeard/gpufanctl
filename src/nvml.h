/*
 * Copyright 1993-2024 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO USER:
 *
 * This source code is subject to NVIDIA ownership rights under U.S. and
 * international Copyright laws.  Users and possessors of this source code
 * are hereby granted a nonexclusive, royalty-free license to use this code
 * in individual and commercial software.
 *
 * NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE
 * CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR
 * IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOURCE CODE.
 *
 * U.S. Government End Users.   This source code is a "commercial item" as
 * that term is defined at  48 C.F.R. 2.101 (OCT 1995), consisting  of
 * "commercial computer  software"  and "commercial computer software
 * documentation" as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995)
 * and is provided to the U.S. Government only as a commercial end item.
 * Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through
 * 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the
 * source code with only those rights set forth herein.
 *
 * Any use of this source code in individual and commercial software must
 * include, in the user documentation and internal comments to the code,
 * the above Disclaimer and U.S. Government End Users Notice.
 */

/*
NVML API Reference

The NVIDIA Management Library (NVML) is a C-based programmatic interface for
monitoring and managing various states within NVIDIA Tesla &tm; GPUs. It is
intended to be a platform for building 3rd party applications, and is also the
underlying library for the NVIDIA-supported nvidia-smi tool. NVML is thread-safe
so it is safe to make simultaneous NVML calls from multiple threads.

API Documentation

Supported platforms:
- Windows:     Windows Server 2008 R2 64bit, Windows Server 2012 R2 64bit,
Windows 7 64bit, Windows 8 64bit, Windows 10 64bit
- Linux:       32-bit and 64-bit
- Hypervisors: Windows Server 2008R2/2012 Hyper-V 64bit, Citrix XenServer 6.2
SP1+, VMware ESX 5.1/5.5

Supported products:
- Full Support
    - All Tesla products, starting with the Fermi architecture
    - All Quadro products, starting with the Fermi architecture
    - All vGPU Software products, starting with the Kepler architecture
    - Selected GeForce Titan products
- Limited Support
    - All Geforce products, starting with the Fermi architecture

The NVML library can be found at \%ProgramW6432\%\\"NVIDIA
Corporation"\\NVSMI\\ on Windows. It is not be added to the system path by
default. To dynamically link to NVML, add this path to the PATH environmental
variable. To dynamically load NVML, call LoadLibrary with this path.

On Linux the NVML library will be found on the standard library path. For 64 bit
Linux, both the 32 bit and 64 bit NVML libraries will be installed.

Online documentation for this library is available at
http://docs.nvidia.com/deploy/nvml-api/index.html
*/

#ifndef __nvml_nvml_h__
#define __nvml_nvml_h__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return values for NVML API calls.
 */
typedef enum nvmlReturn_enum
{
    // cppcheck-suppress *
    NVML_SUCCESS = 0, //!< The operation was successful
    NVML_ERROR_UNINITIALIZED =
        1, //!< NVML was not first initialized with nvmlInit()
    NVML_ERROR_INVALID_ARGUMENT = 2, //!< A supplied argument is invalid
    NVML_ERROR_NOT_SUPPORTED =
        3, //!< The requested operation is not available on target device
    NVML_ERROR_NO_PERMISSION =
        4, //!< The current user does not have permission for operation
    NVML_ERROR_ALREADY_INITIALIZED =
        5,                    //!< Deprecated: Multiple initializations
                              //!< are now allowed through ref counting
    NVML_ERROR_NOT_FOUND = 6, //!< A query to find an object was unsuccessful
    NVML_ERROR_INSUFFICIENT_SIZE = 7, //!< An input argument is not large enough
    NVML_ERROR_INSUFFICIENT_POWER =
        8, //!< A device's external power cables are not properly attached
    NVML_ERROR_DRIVER_NOT_LOADED = 9, //!< NVIDIA driver is not loaded
    NVML_ERROR_TIMEOUT = 10,          //!< User provided timeout passed
    NVML_ERROR_IRQ_ISSUE =
        11, //!< NVIDIA Kernel detected an interrupt issue with a GPU
    NVML_ERROR_LIBRARY_NOT_FOUND =
        12, //!< NVML Shared Library couldn't be found or loaded
    NVML_ERROR_FUNCTION_NOT_FOUND =
        13, //!< Local version of NVML doesn't implement this function
    NVML_ERROR_CORRUPTED_INFOROM = 14, //!< infoROM is corrupted
    NVML_ERROR_GPU_IS_LOST = 15, //!< The GPU has fallen off the bus or has
                                 //!< otherwise become inaccessible
    NVML_ERROR_RESET_REQUIRED =
        16, //!< The GPU requires a reset before it can be used again
    NVML_ERROR_OPERATING_SYSTEM =
        17, //!< The GPU control device has been blocked
            //!< by the operating system/cgroups
    NVML_ERROR_LIB_RM_VERSION_MISMATCH =
        18,                  //!< RM detects a driver/library version mismatch
    NVML_ERROR_IN_USE = 19,  //!< An operation cannot be performed because the
                             //!< GPU is currently in use
    NVML_ERROR_MEMORY = 20,  //!< Insufficient memory
    NVML_ERROR_NO_DATA = 21, //!< No data
    NVML_ERROR_VGPU_ECC_NOT_SUPPORTED =
        22, //!< The requested vgpu operation is not available on target device,
            //!< becasue ECC is enabled
    NVML_ERROR_INSUFFICIENT_RESOURCES =
        23, //!< Ran out of critical resources, other than memory
    NVML_ERROR_FREQ_NOT_SUPPORTED =
        24, //!< Ran out of critical resources, other than memory
    NVML_ERROR_ARGUMENT_VERSION_MISMATCH =
        25, //!< The provided version is invalid/unsupported
    NVML_ERROR_DEPRECATED =
        26, //!< The requested functionality has been deprecated
    NVML_ERROR_NOT_READY = 27,     //!< The system is not ready for the request
    NVML_ERROR_GPU_NOT_FOUND = 28, //!< No GPUs were found
    NVML_ERROR_INVALID_STATE =
        29, //!< Resource not in correct state to perform requested operation
    NVML_ERROR_UNKNOWN = 999 //!< An internal driver error occurred
} nvmlReturn_t;

/**
 * Generic enable/disable enum.
 */
typedef enum nvmlEnableState_enum
{
    NVML_FEATURE_DISABLED = 0, //!< Feature disabled
    NVML_FEATURE_ENABLED = 1   //!< Feature enabled
} nvmlEnableState_t;

/**
 * Temperature sensors.
 */
typedef enum nvmlTemperatureSensors_enum
{
    NVML_TEMPERATURE_GPU = 0, //!< Temperature sensor for the GPU die

    // Keep this last
    NVML_TEMPERATURE_COUNT
} nvmlTemperatureSensors_t;

typedef struct nvmlUnit_st* nvmlUnit_t;
typedef struct nvmlDevice_st* nvmlDevice_t;

/**
 * Initialize NVML, but don't initialize any GPUs yet.
 *
 * \note nvmlInit_v3 introduces a "flags" argument, that allows passing boolean
 * values modifying the behaviour of nvmlInit(). \note In NVML 5.319 new
 * nvmlInit_v2 has replaced nvmlInit"_v1" (default in NVML 4.304 and older) that
 *       did initialize all GPU devices in the system.
 *
 * This allows NVML to communicate with a GPU
 * when other GPUs in the system are unstable or in a bad state.  When using
 * this API, GPUs are discovered and initialized in nvmlDeviceGetHandleBy*
 * functions instead.
 *
 * \note To contrast nvmlInit_v2 with nvmlInit"_v1", NVML 4.304 nvmlInit"_v1"
 * will fail when any detected GPU is in a bad or unstable state.
 *
 * For all products.
 *
 * This method, should be called once before invoking any other methods in the
 * library. A reference count of the number of initializations is maintained.
 * Shutdown only occurs when the reference count reaches zero.
 *
 * @return
 *         - \ref NVML_SUCCESS                   if NVML has been properly
 * initialized
 *         - \ref NVML_ERROR_DRIVER_NOT_LOADED   if NVIDIA driver is not running
 *         - \ref NVML_ERROR_NO_PERMISSION       if NVML does not have
 * permission to talk to the driver
 *         - \ref NVML_ERROR_UNKNOWN             on any unexpected error
 */
typedef nvmlReturn_t (*PFN_nvmlInit_v2)(void);

/**
 * Shut down NVML by releasing all GPU resources previously allocated with \ref
 * nvmlInit_v2().
 *
 * For all products.
 *
 * This method should be called after NVML work is done, once for each call to
 * \ref nvmlInit_v2() A reference count of the number of initializations is
 * maintained.  Shutdown only occurs when the reference count reaches zero.  For
 * backwards compatibility, no error is reported if nvmlShutdown() is called
 * more times than nvmlInit().
 *
 * @return
 *         - \ref NVML_SUCCESS                 if NVML has been properly shut
 * down
 *         - \ref NVML_ERROR_UNINITIALIZED     if the library has not been
 * successfully initialized
 *         - \ref NVML_ERROR_UNKNOWN           on any unexpected error
 */
typedef nvmlReturn_t (*PFN_nvmlShutdown)(void);

/**
 * Helper method for converting NVML error codes into readable strings.
 *
 * For all products.
 *
 * @param result                               NVML error code to convert
 *
 * @return String representation of the error.
 *
 */
typedef const char* (*PFN_nvmlErrorString)(nvmlReturn_t result);

/**
 * Retrieves the number of units in the system.
 *
 * For S-class products.
 *
 * @param unitCount                            Reference in which to return the
 * number of units
 *
 * @return
 *         - \ref NVML_SUCCESS                 if \a unitCount has been set
 *         - \ref NVML_ERROR_UNINITIALIZED     if the library has not been
 * successfully initialized
 *         - \ref NVML_ERROR_INVALID_ARGUMENT  if \a unitCount is NULL
 *         - \ref NVML_ERROR_UNKNOWN           on any unexpected error
 */
typedef nvmlReturn_t (*PFN_nvmlUnitGetCount)(unsigned int* unitCount);

/**
 * Retrieves the set of GPU devices that are attached to the specified unit.
 *
 * For S-class products.
 *
 * The \a deviceCount argument is expected to be set to the size of the input \a
 * devices array.
 *
 * @param unit                                 The identifier of the target unit
 * @param deviceCount                          Reference in which to provide the
 * \a devices array size, and to return the number of attached GPU devices
 * @param devices                              Reference in which to return the
 * references to the attached GPU devices
 *
 * @return
 *         - \ref NVML_SUCCESS                 if \a deviceCount and \a devices
 * have been populated
 *         - \ref NVML_ERROR_UNINITIALIZED     if the library has not been
 * successfully initialized
 *         - \ref NVML_ERROR_INSUFFICIENT_SIZE if \a deviceCount indicates that
 * the \a devices array is too small
 *         - \ref NVML_ERROR_INVALID_ARGUMENT  if \a unit is invalid, either of
 * \a deviceCount or \a devices is NULL
 *         - \ref NVML_ERROR_UNKNOWN           on any unexpected error
 */
typedef nvmlReturn_t (*PFN_nvmlUnitGetDevices)(nvmlUnit_t unit,
                                               unsigned int* deviceCount,
                                               nvmlDevice_t* devices);

/**
 * Retrieves the number of compute devices in the system. A compute device is a
 * single GPU.
 *
 * For all products.
 *
 * Note: New nvmlDeviceGetCount_v2 (default in NVML 5.319) returns count of all
 * devices in the system even if nvmlDeviceGetHandleByIndex_v2 returns
 * NVML_ERROR_NO_PERMISSION for such device. Update your code to handle this
 * error, or use NVML 4.304 or older nvml header file. For backward binary
 * compatibility reasons _v1 version of the API is still present in the shared
 *       library.
 *       Old _v1 version of nvmlDeviceGetCount doesn't count devices that NVML
 * has no permission to talk to.
 *
 * @param deviceCount                          Reference in which to return the
 * number of accessible devices
 *
 * @return
 *         - \ref NVML_SUCCESS                 if \a deviceCount has been set
 *         - \ref NVML_ERROR_UNINITIALIZED     if the library has not been
 * successfully initialized
 *         - \ref NVML_ERROR_INVALID_ARGUMENT  if \a deviceCount is NULL
 *         - \ref NVML_ERROR_UNKNOWN           on any unexpected error
 */
typedef nvmlReturn_t (*PFN_nvmlDeviceGetCount_v2)(unsigned int* deviceCount);

/**
 * Acquire the handle for a particular device, based on its index.
 *
 * For all products.
 *
 * Valid indices are derived from the \a accessibleDevices count returned by
 *   \ref nvmlDeviceGetCount_v2(). For example, if \a accessibleDevices is 2 the
 * valid indices are 0 and 1, corresponding to GPU 0 and GPU 1.
 *
 * The order in which NVML enumerates devices has no guarantees of consistency
 * between reboots. For that reason it is recommended that devices be looked up
 * by their PCI ids or UUID. See \ref nvmlDeviceGetHandleByUUID() and \ref
 * nvmlDeviceGetHandleByPciBusId_v2().
 *
 * Note: The NVML index may not correlate with other APIs, such as the CUDA
 * device index.
 *
 * Starting from NVML 5, this API causes NVML to initialize the target GPU
 * NVML may initialize additional GPUs if:
 *  - The target GPU is an SLI slave
 *
 * Note: New nvmlDeviceGetCount_v2 (default in NVML 5.319) returns count of all
 * devices in the system even if nvmlDeviceGetHandleByIndex_v2 returns
 * NVML_ERROR_NO_PERMISSION for such device. Update your code to handle this
 * error, or use NVML 4.304 or older nvml header file. For backward binary
 * compatibility reasons _v1 version of the API is still present in the shared
 *       library.
 *       Old _v1 version of nvmlDeviceGetCount doesn't count devices that NVML
 * has no permission to talk to.
 *
 *       This means that nvmlDeviceGetHandleByIndex_v2 and _v1 can return
 * different devices for the same index. If you don't touch macros that map old
 * (_v1) versions to _v2 versions at the top of the file you don't need to worry
 * about that.
 *
 * @param index                                The index of the target GPU, >= 0
 * and < \a accessibleDevices
 * @param device                               Reference in which to return the
 * device handle
 *
 * @return
 *         - \ref NVML_SUCCESS                  if \a device has been set
 *         - \ref NVML_ERROR_UNINITIALIZED      if the library has not been
 * successfully initialized
 *         - \ref NVML_ERROR_INVALID_ARGUMENT   if \a index is invalid or \a
 * device is NULL
 *         - \ref NVML_ERROR_INSUFFICIENT_POWER if any attached devices have
 * improperly attached external power cables
 *         - \ref NVML_ERROR_NO_PERMISSION      if the user doesn't have
 * permission to talk to this device
 *         - \ref NVML_ERROR_IRQ_ISSUE          if NVIDIA kernel detected an
 * interrupt issue with the attached GPUs
 *         - \ref NVML_ERROR_GPU_IS_LOST        if the target GPU has fallen off
 * the bus or is otherwise inaccessible
 *         - \ref NVML_ERROR_UNKNOWN            on any unexpected error
 *
 * @see nvmlDeviceGetIndex
 * @see nvmlDeviceGetCount
 */
typedef nvmlReturn_t (*PFN_nvmlDeviceGetHandleByIndex_v2)(unsigned int index,
                                                          nvmlDevice_t* device);

/**
 * Retrieves the current temperature readings for the device, in degrees C.
 *
 * For all products.
 *
 * See \ref nvmlTemperatureSensors_t for details on available temperature
 * sensors.
 *
 * @param device                               The identifier of the target
 * device
 * @param sensorType                           Flag that indicates which sensor
 * reading to retrieve
 * @param temp                                 Reference in which to return the
 * temperature reading
 *
 * @return
 *         - \ref NVML_SUCCESS                 if \a temp has been set
 *         - \ref NVML_ERROR_UNINITIALIZED     if the library has not been
 * successfully initialized
 *         - \ref NVML_ERROR_INVALID_ARGUMENT  if \a device is invalid, \a
 * sensorType is invalid or \a temp is NULL
 *         - \ref NVML_ERROR_NOT_SUPPORTED     if the device does not have the
 * specified sensor
 *         - \ref NVML_ERROR_GPU_IS_LOST       if the target GPU has fallen off
 * the bus or is otherwise inaccessible
 *         - \ref NVML_ERROR_UNKNOWN           on any unexpected error
 */
typedef nvmlReturn_t (*PFN_nvmlDeviceGetTemperature)(
    nvmlDevice_t device,
    nvmlTemperatureSensors_t sensorType,
    unsigned int* temp);

/**
 * Sets the speed of a specified fan.
 *
 * WARNING: This function changes the fan control policy to manual. It means
 * that YOU have to monitor the temperature and adjust the fan speed
 * accordingly. If you set the fan speed too low you can burn your GPU! Use
 * nvmlDeviceSetDefaultFanSpeed_v2 to restore default control policy.
 *
 * For all cuda-capable discrete products with fans that are Maxwell or Newer.
 *
 * device                                The identifier of the target device
 * fan                                   The index of the fan, starting at zero
 * speed                                 The target speed of the fan [0-100] in
 * % of max speed
 *
 * return
 *        NVML_SUCCESS                   if the fan speed has been set
 *        NVML_ERROR_UNINITIALIZED       if the library has not been
 * successfully initialized NVML_ERROR_INVALID_ARGUMENT    if the device is not
 * valid, or the speed is outside acceptable ranges, or if the fan index doesn't
 * reference an actual fan. NVML_ERROR_NOT_SUPPORTED       if the device is
 * older than Maxwell. NVML_ERROR_UNKNOWN             if there was an unexpected
 * error.
 */
typedef nvmlReturn_t (*PFN_nvmlDeviceSetFanSpeed_v2)(nvmlDevice_t device,
                                                     unsigned int fan,
                                                     unsigned int speed);

/**
 * Sets the speed of the fan control policy to default.
 *
 * For all cuda-capable discrete products with fans
 *
 * @param device                        The identifier of the target device
 * @param fan                           The index of the fan, starting at zero
 *
 * return
 *         NVML_SUCCESS                 if speed has been adjusted
 *         NVML_ERROR_UNINITIALIZED     if the library has not been successfully
 * initialized NVML_ERROR_INVALID_ARGUMENT  if device is invalid
 *         NVML_ERROR_NOT_SUPPORTED     if the device does not support this
 *                                      (doesn't have fans)
 *         NVML_ERROR_UNKNOWN           on any unexpected error
 */
typedef nvmlReturn_t (*PFN_nvmlDeviceSetDefaultFanSpeed_v2)(nvmlDevice_t device,
                                                            unsigned int fan);

/**
 * Retrieves the number of fans on the device.
 *
 * For all discrete products with dedicated fans.
 *
 * @param device                               The identifier of the target
 * device
 * @param numFans                              The number of fans
 *
 * @return
 *         - \ref NVML_SUCCESS                 if \a fan number query was
 * successful
 *         - \ref NVML_ERROR_UNINITIALIZED     if the library has not been
 * successfully initialized
 *         - \ref NVML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a
 * numFans is NULL
 *         - \ref NVML_ERROR_NOT_SUPPORTED     if the device does not have a fan
 *         - \ref NVML_ERROR_GPU_IS_LOST       if the target GPU has fallen off
 * the bus or is otherwise inaccessible
 *         - \ref NVML_ERROR_UNKNOWN           on any unexpected error
 */
typedef nvmlReturn_t (*PFN_nvmlDeviceGetNumFans)(nvmlDevice_t device,
                                                 unsigned int* numFans);

/**
 * Set the persistence mode for the device.
 *
 * For all products.
 * For Linux only.
 * Requires root/admin permissions.
 *
 * The persistence mode determines whether the GPU driver software is torn down
 * after the last client exits.
 *
 * This operation takes effect immediately. It is not persistent across reboots.
 * After each reboot the persistence mode is reset to "Disabled".
 *
 * See \ref nvmlEnableState_t for available modes.
 *
 * After calling this API with mode set to NVML_FEATURE_DISABLED on a device
 * that has its own NUMA memory, the given device handle will no longer be
 * valid, and to continue to interact with this device, a new handle should be
 * obtained from one of the nvmlDeviceGetHandleBy*() APIs. This limitation is
 * currently only applicable to devices that have a coherent NVLink connection
 * to system memory.
 *
 * @param device                               The identifier of the target
 * device
 * @param mode                                 The target persistence mode
 *
 * @return
 *         - \ref NVML_SUCCESS                 if the persistence mode was set
 *         - \ref NVML_ERROR_UNINITIALIZED     if the library has not been
 * successfully initialized
 *         - \ref NVML_ERROR_INVALID_ARGUMENT  if \a device is invalid or \a
 * mode is invalid
 *         - \ref NVML_ERROR_NOT_SUPPORTED     if the device does not support
 * this feature
 *         - \ref NVML_ERROR_NO_PERMISSION     if the user doesn't have
 * permission to perform this operation
 *         - \ref NVML_ERROR_GPU_IS_LOST       if the target GPU has fallen off
 * the bus or is otherwise inaccessible
 *         - \ref NVML_ERROR_UNKNOWN           on any unexpected error
 *
 * @see nvmlDeviceGetPersistenceMode()
 */
typedef nvmlReturn_t (*PFN_nvmlDeviceSetPersistenceMode)(
    nvmlDevice_t device, nvmlEnableState_t mode);

#ifdef __cplusplus
}
#endif

#endif // __nvml_nvml_h__
