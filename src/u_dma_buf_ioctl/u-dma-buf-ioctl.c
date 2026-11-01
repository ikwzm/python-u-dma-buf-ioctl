/*********************************************************************************
 *
 *       Copyright (C) 2026 Ichiro Kawazome
 *       All rights reserved.
 * 
 *       Redistribution and use in source and binary forms, with or without
 *       modification, are permitted provided that the following conditions
 *       are met:
 * 
 *         1. Redistributions of source code must retain the above copyright
 *            notice, this list of conditions and the following disclaimer.
 * 
 *         2. Redistributions in binary form must reproduce the above copyright
 *            notice, this list of conditions and the following disclaimer in
 *            the documentation and/or other materials provided with the
 *            distribution.
 * 
 *       THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *       "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *       LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *       A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 *       OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *       SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *       LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *       DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *       THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *       (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *       OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 ********************************************************************************/
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdint.h>
#include <sys/ioctl.h>

#include "u-dma-buf-ioctl.h"

#ifndef PACKAGE_NAME
#define PACKAGE_NAME        u_dma_buf_ioctl
#endif

#ifndef MODULE_NAME
#define MODULE_NAME         _u_dma_buf_ioctl
#endif

#define MODULE_VERSION      "0.1.0"
#define MODULE_AUTHOR       "Ichiro Kawazome"
#define MODULE_AUTHOR_EMAIL "ichiro_k@ca2-so-net.ne.jp"
#define MODULE_LICENSE      "BSD 2-Clause"
#define MODULE_DESCRIPTION  "Python interface for u-dma-buf IOCTL operations"

#define TO_STR(x)           #x
#define NAME_TO_STR(x)      TO_STR(x)
#define PACKAGE_NAME_STRING NAME_TO_STR(PACKAGE_NAME)
#define MODULE_NAME_STRING  NAME_TO_STR(MODULE_NAME)

/**
 * dict_get_uint64() - Get a required unsigned 64-bit integer from a Python dictionary.
 * @dict:  Python dictionary.
 * @key:   Dictionary key.
 * @value: Pointer to store the retrieved value.
 *
 * Returns 0 on success, or -1 on error.
 * Raises KeyError if the key is not found.
 */
static int
dict_get_uint64(PyObject* dict, const char* key, uint64_t* value)
{
    PyObject* obj = PyDict_GetItemString(dict, key);

    if (obj == NULL) {
        PyErr_Format(PyExc_KeyError, "Missing key: %s", key);
        return -1;
    }

    unsigned long long v = PyLong_AsUnsignedLongLong(obj);

    if (PyErr_Occurred())
        return -1;

    *value = (uint64_t)v;
    return 0;
}

/**
 * dict_get_uint64_with_default() - Get an unsigned 64-bit integer from a Python dictionary.
 * @dict:          Python dictionary.
 * @key:           Dictionary key.
 * @value:         Pointer to store the retrieved value.
 * @default_value: Default value to use if the key is not found.
 *
 * Returns 0 on success, or -1 on error.
 * Uses @default_value if the key is not found.
 */
static int
dict_get_uint64_with_default(PyObject* dict, const char* key, uint64_t* value, uint64_t default_value)
{
    PyObject* obj = PyDict_GetItemString(dict, key);

    if (obj == NULL) {
        *value = default_value;
        return 0;
    }

    unsigned long long v = PyLong_AsUnsignedLongLong(obj);

    if (PyErr_Occurred())
        return -1;

    *value = (uint64_t)v;
    return 0;
}

/**
 * dict_get_int() - Get a required integer value from a Python dictionary.
 * @dict:  Python dictionary.
 * @key:   Dictionary key.
 * @value: Pointer to store the retrieved value.
 *
 * Returns 0 on success, or -1 on error.
 * Raises KeyError if the key is not found, or OverflowError if the value
 * is outside the range of int.
 */
static int
dict_get_int(PyObject* dict, const char* key, int* value)
{
    PyObject* obj = PyDict_GetItemString(dict, key);

    if (obj == NULL) {
        PyErr_Format(PyExc_KeyError, "Missing key: %s", key);
        return -1;
    }

    long v = PyLong_AsLong(obj);

    if (PyErr_Occurred())
        return -1;

    if (v < INT32_MIN || v > INT32_MAX) {
        PyErr_Format(PyExc_OverflowError, "Value out of range: %s", key);
        return -1;
    }

    *value = (int)v;
    return 0;
}

/**
 * dict_get_int_with_default() - Get an integer value from a Python dictionary.
 * @dict:          Python dictionary.
 * @key:           Dictionary key.
 * @value:         Pointer to store the retrieved value.
 * @default_value: Default value to use if the key is not found.
 *
 * Returns 0 on success, or -1 on error.
 * Uses @default_value if the key is not found.
 */
static int
dict_get_int_with_default(PyObject* dict, const char* key, int* value, int default_value)
{
    PyObject* obj = PyDict_GetItemString(dict, key);

    if (obj == NULL) {
        *value = default_value;
        return 0;
    }

    long v = PyLong_AsLong(obj);

    if (PyErr_Occurred())
        return -1;

    if (v < INT32_MIN || v > INT32_MAX) {
        PyErr_Format(PyExc_OverflowError, "Value out of range: %s", key);
        return -1;
    }

    *value = (int)v;
    return 0;
}


/**
 * u_dma_buf_ioctl_get_dev_info() - Get u-dma-buf device information.
 * @self: Python module object.
 * @args: Tuple containing a file object.
 *
 * Retrieves device information using U_DMA_BUF_IOCTL_GET_DEV_INFO.
 *
 * Returns a Python dictionary containing the following entries:
 * - "size":         DMA buffer size.
 * - "dma_address":  DMA buffer address.
 * - "flags":        Raw device flags.
 * - "dma_mask":     DMA mask width in bits.
 * - "dma_coherent": DMA coherency flag.
 * - "mmap_mode":    Memory mapping mode.
 *
 * Returns NULL with a Python exception on error.
 */
static PyObject* 
u_dma_buf_ioctl_get_dev_info(PyObject* self, PyObject* args)
{
    PyObject* file_obj;
    int       fd;
    u_dma_buf_ioctl_dev_info dev_info = {0};

    if (!PyArg_ParseTuple(args, "O", &file_obj))
        return NULL;

    fd = PyObject_AsFileDescriptor(file_obj);
    if (fd < 0)
        return NULL;

    if (ioctl(fd, U_DMA_BUF_IOCTL_GET_DEV_INFO, &dev_info) < 0)
        return PyErr_SetFromErrno(PyExc_OSError);

    return Py_BuildValue(
        "{s:K,s:K,s:K,s:i,s:i,s:i}",
        "size",         (unsigned long long)dev_info.size,
        "dma_address",  (unsigned long long)dev_info.addr,
        "flags",        (unsigned long long)dev_info.flags,
        "dma_mask",     GET_U_DMA_BUF_IOCTL_FLAGS_DMA_MASK(&dev_info),
        "dma_coherent", GET_U_DMA_BUF_IOCTL_FLAGS_DMA_COHERENT(&dev_info),
        "mmap_mode",    GET_U_DMA_BUF_IOCTL_FLAGS_MMAP_MODE(&dev_info)
    );
}

/**
 * u_dma_buf_ioctl_get_drv_info() - Get u-dma-buf driver information.
 * @self: Python module object.
 * @args: Tuple containing a file object.
 *
 * Retrieves driver information using U_DMA_BUF_IOCTL_GET_DRV_INFO.
 *
 * Returns a Python dictionary containing the following entries:
 * - "flags":                Raw driver flags.
 * - "ioctl_version":        IOCTL interface version.
 * - "in_kernel_functions":  In-kernel functions availability.
 * - "use_of_dma_config":    Device Tree DMA configuration usage.
 * - "use_of_reserved_mem":  Reserved memory usage.
 * - "use_quirk_mmap":       quirk-mmap support.
 * - "use_quirk_mmap_page":  quirk-mmap-page support.
 * - "version":              u-dma-buf driver version string.
 *
 * Returns NULL with a Python exception on error.
 */
static PyObject* 
u_dma_buf_ioctl_get_drv_info(PyObject* self, PyObject* args)
{
    PyObject* file_obj;
    int       fd;
    u_dma_buf_ioctl_drv_info drv_info = {0};

    if (!PyArg_ParseTuple(args, "O", &file_obj))
        return NULL;

    fd = PyObject_AsFileDescriptor(file_obj);
    if (fd < 0)
        return NULL;

    if (ioctl(fd, U_DMA_BUF_IOCTL_GET_DRV_INFO, &drv_info) < 0)
        return PyErr_SetFromErrno(PyExc_OSError);

    return Py_BuildValue(
        "{s:K,s:i,s:i,s:i,s:i,s:i,s:i,s:s}",
        "flags"              , (unsigned long long)drv_info.flags,
        "ioctl_version"      , GET_U_DMA_BUF_IOCTL_FLAGS_IOCTL_VERSION(&drv_info),
        "in_kernel_functions", GET_U_DMA_BUF_IOCTL_FLAGS_IN_KERNEL_FUNCTIONS(&drv_info),
        "use_of_dma_config"  , GET_U_DMA_BUF_IOCTL_FLAGS_USE_OF_DMA_CONFIG(&drv_info),
        "use_of_reserved_mem", GET_U_DMA_BUF_IOCTL_FLAGS_USE_OF_RESERVED_MEM(&drv_info),
        "use_quirk_mmap"     , GET_U_DMA_BUF_IOCTL_FLAGS_USE_QUIRK_MMAP(&drv_info),
        "use_quirk_mmap_page", GET_U_DMA_BUF_IOCTL_FLAGS_USE_QUIRK_MMAP_PAGE(&drv_info),
        "version"            , drv_info.version
    );
}

/**
 * u_dma_buf_ioctl_get_size() - Get the DMA buffer size.
 * @self: Python module object.
 * @args: Tuple containing a file object.
 *
 * Retrieves the DMA buffer size using U_DMA_BUF_IOCTL_GET_SIZE.
 *
 * Returns a Python integer representing the DMA buffer size in bytes.
 * Returns NULL with a Python exception on error.
 */
static PyObject*
u_dma_buf_ioctl_get_size(PyObject* self, PyObject* args)
{
    PyObject* file_obj;
    int       fd;
    uint64_t  size = 0;

    if (!PyArg_ParseTuple(args, "O", &file_obj))
        return NULL;

    fd = PyObject_AsFileDescriptor(file_obj);
    if (fd < 0)
        return NULL;

    if (ioctl(fd, U_DMA_BUF_IOCTL_GET_SIZE, &size) < 0)
        return PyErr_SetFromErrno(PyExc_OSError);

    return PyLong_FromUnsignedLongLong((unsigned long long)size);
}

/**
 * u_dma_buf_ioctl_get_dma_addr() - Get the DMA buffer address.
 * @self: Python module object.
 * @args: Tuple containing a file object.
 *
 * Retrieves the DMA buffer address using U_DMA_BUF_IOCTL_GET_DMA_ADDR.
 *
 * Returns a Python integer representing the DMA buffer DMA address.
 * Returns NULL with a Python exception on error.
 */
static PyObject*
u_dma_buf_ioctl_get_dma_addr(PyObject* self, PyObject* args)
{
    PyObject* file_obj;
    int       fd;
    uint64_t  dma_addr = 0;

    if (!PyArg_ParseTuple(args, "O", &file_obj))
        return NULL;

    fd = PyObject_AsFileDescriptor(file_obj);
    if (fd < 0)
        return NULL;

    if (ioctl(fd, U_DMA_BUF_IOCTL_GET_DMA_ADDR, &dma_addr) < 0)
        return PyErr_SetFromErrno(PyExc_OSError);

    return PyLong_FromUnsignedLongLong((unsigned long long)dma_addr);
}

/**
 * u_dma_buf_ioctl_get_sync_owner() - Get the DMA buffer synchronization owner.
 * @self: Python module object.
 * @args: Tuple containing a file object.
 *
 * Retrieves the synchronization owner using
 * U_DMA_BUF_IOCTL_GET_SYNC_OWNER.
 *
 * Returns a Python integer representing the synchronization owner.
 * Returns NULL with a Python exception on error.
 */
static PyObject*
u_dma_buf_ioctl_get_sync_owner(PyObject* self, PyObject* args)
{
    PyObject* file_obj;
    int       fd;
    uint32_t  sync_owner = 0;

    if (!PyArg_ParseTuple(args, "O", &file_obj))
        return NULL;

    fd = PyObject_AsFileDescriptor(file_obj);
    if (fd < 0)
        return NULL;

    if (ioctl(fd, U_DMA_BUF_IOCTL_GET_SYNC_OWNER, &sync_owner) < 0)
        return PyErr_SetFromErrno(PyExc_OSError);

    return PyLong_FromUnsignedLong((unsigned long)sync_owner);
}

/**
 * u_dma_buf_ioctl_get_sync() - Get DMA buffer synchronization parameters.
 * @self: Python module object.
 * @args: Tuple containing a file object.
 *
 * Retrieves the synchronization parameters using U_DMA_BUF_IOCTL_GET_SYNC.
 *
 * Returns a Python dictionary containing the following entries:
 * - "offset":    Synchronization range offset in bytes.
 * - "size":      Synchronization range size in bytes.
 * - "direction": Synchronization direction.
 *       1: DMA to device.
 *       2: DMA from device.
 *       3: DMA bidirectional.
 * - "owner":     Synchronization owner.
 *       0: CPU.
 *       1: Device.
 * - "mode":      Synchronization mode.
 * - "flags":     Raw synchronization flags.
 *
 * Returns NULL with a Python exception on error.
 */
static PyObject*
u_dma_buf_ioctl_get_sync(PyObject* self, PyObject* args)
{
    PyObject* file_obj;
    int       fd;
    u_dma_buf_ioctl_sync_args sync_args = {0};

    if (!PyArg_ParseTuple(args, "O", &file_obj))
        return NULL;

    fd = PyObject_AsFileDescriptor(file_obj);
    if (fd < 0)
        return NULL;

    if (ioctl(fd, U_DMA_BUF_IOCTL_GET_SYNC, &sync_args) < 0)
        return PyErr_SetFromErrno(PyExc_OSError);

    return Py_BuildValue(
        "{s:K,s:K,s:i,s:i,s:i,s:K}",
        "offset",    (unsigned long long)sync_args.offset,
        "size",      (unsigned long long)sync_args.size,
        "direction", GET_U_DMA_BUF_IOCTL_FLAGS_SYNC_DIR(&sync_args),
        "owner",     GET_U_DMA_BUF_IOCTL_FLAGS_SYNC_OWNER(&sync_args),
        "mode",      GET_U_DMA_BUF_IOCTL_FLAGS_SYNC_MODE(&sync_args),
        "flags",     (unsigned long long)sync_args.flags
    );
}

/**
 * u_dma_buf_ioctl_set_sync() - Set DMA buffer synchronization parameters.
 * @self:     Python module object.
 * @args:     Tuple containing a file object and a settings dictionary.
 *
 * Sets the synchronization parameters of a u-dma-buf device using
 * U_DMA_BUF_IOCTL_SET_SYNC.
 *
 * The settings dictionary must contain the following keys:
 * - "size":     Synchronization range size.
 * - "offset":   Synchronization range offset.
 *
 * The following keys are optional and default to 0 if omitted:
 * - "command":   Synchronization command.
 *     0: No operation.
 *     1: Sync for CPU.
 *     2: No operation.
 *     3: Sync for device.
 * - "direction": Synchronization direction.
 *     0: Unchanged.
 *     1: DMA to device.
 *     2: DMA from device.
 *     3: DMA bidirectional.
 * - "fast":      Fast synchronization flag.
 * - "mode":      Synchronization mode.
 *
 * Returns None on success, or NULL with a Python exception on error.
 */
static PyObject*
u_dma_buf_ioctl_set_sync(PyObject* self, PyObject* args)
{
    PyObject* file_obj;
    PyObject* settings;
    int       fd;
    int       sync_cmd, sync_direction, sync_fast, sync_mode;
    u_dma_buf_ioctl_sync_args sync_args = {0};

    if (!PyArg_ParseTuple(args, "OO!", &file_obj, &PyDict_Type, &settings))
        return NULL;

    fd = PyObject_AsFileDescriptor(file_obj);
    if (fd < 0)
        return NULL;

    if (dict_get_uint64(settings, "size", &sync_args.size) < 0)
        return NULL;

    if (dict_get_uint64(settings, "offset", &sync_args.offset) < 0)
        return NULL;

    if (dict_get_int_with_default(settings, "command", &sync_cmd, 0) < 0)
        return NULL;

    if (dict_get_int_with_default(settings, "direction", &sync_direction, 0) < 0)
        return NULL;

    if (dict_get_int_with_default(settings, "fast", &sync_fast, 0) < 0)
        return NULL;

    if (dict_get_int_with_default(settings, "mode", &sync_mode, 0) < 0)
        return NULL;

    SET_U_DMA_BUF_IOCTL_FLAGS_SYNC_CMD( &sync_args, sync_cmd);
    SET_U_DMA_BUF_IOCTL_FLAGS_SYNC_DIR( &sync_args, sync_direction);
    SET_U_DMA_BUF_IOCTL_FLAGS_SYNC_FAST(&sync_args, sync_fast);
    SET_U_DMA_BUF_IOCTL_FLAGS_SYNC_MODE(&sync_args, sync_mode);

    if (ioctl(fd, U_DMA_BUF_IOCTL_SET_SYNC, &sync_args) < 0)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_RETURN_NONE;
}

#define  SYNC_COMMAND_DIR_MASK        (0x000000000000000C)
#define  SYNC_COMMAND_DIR_SHIFT       (2)
#define  SYNC_COMMAND_SIZE_MASK       (0x00000000FFFFFFF0)
#define  SYNC_COMMAND_SIZE_SHIFT      (0)
#define  SYNC_COMMAND_OFFSET_MASK     (0xFFFFFFFF00000000)
#define  SYNC_COMMAND_OFFSET_SHIFT    (32)
#define  SYNC_COMMAND_ARGUMENT_MASK   (0xFFFFFFFFFFFFFFFE)
#define  ENCODE_SYNC_COMMAND_ARGUMENT(command, offset, size, direction) ( \
    (((uint64_t)(offset)    << SYNC_COMMAND_OFFSET_SHIFT) & SYNC_COMMAND_OFFSET_MASK) | \
    (((uint64_t)(size)      << SYNC_COMMAND_SIZE_SHIFT  ) & SYNC_COMMAND_SIZE_MASK  ) | \
    (((uint64_t)(direction) << SYNC_COMMAND_DIR_SHIFT   ) & SYNC_COMMAND_DIR_MASK   ) | \
    (((uint64_t)(command)                               ) & 0x3))

/**
 * u_dma_buf_ioctl_sync_for_cpu() - Synchronize DMA buffer for CPU.
 * @self: Python module object.
 * @args: Tuple containing a file object, a command, and optional
 *        synchronization parameters.
 *
 * Synchronizes the DMA buffer for CPU using
 * U_DMA_BUF_IOCTL_SET_SYNC_FOR_CPU.
 *
 * The command is required:
 * - 0: Synchronize using the specified or previously configured
 *      synchronization parameters.
 * - 1: Synchronize using the specified or previously configured
 *      synchronization parameters.
 *
 * The following synchronization parameters are optional, but must be
 * specified together:
 * - @offset: Synchronization range offset in bytes (uint32_t).
 * - @size: Synchronization range size in bytes.
 *     Must be aligned to 16 bytes and fit in the 32-bit size field.
 * - @direction: DMA data direction.
 *     1: DMA to device.
 *     2: DMA from device.
 *     3: DMA bidirectional.
 *
 * If all synchronization parameters are omitted, the previously configured
 * synchronization parameters are used.
 *
 * Returns None on success, or NULL with a Python exception on error.
 */
static PyObject*
u_dma_buf_ioctl_sync_for_cpu(PyObject* self, PyObject* args)
{
    PyObject*          file_obj;
    int                fd;
    int                command;
    unsigned long long offset    = 0;
    unsigned long long size      = 0;
    int                direction = 0;
    uint64_t           sync_args = 0;
    Py_ssize_t         nargs     = PyTuple_GET_SIZE(args);

    if (nargs != 2 && nargs != 5) {
        PyErr_SetString(PyExc_TypeError,
                        "Expected either file, command or "
                        "file, command, offset, size, direction");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "Oi|KKi",
                          &file_obj, &command,
                          &offset, &size, &direction))
        return NULL;

    if (command < 0 || command > 1) {
        PyErr_SetString(PyExc_ValueError, "command must be 0 or 1");
        return NULL;
    }

    if (nargs == 5) {
        if (offset > UINT32_MAX) {
            PyErr_SetString(PyExc_OverflowError, "offset must fit in uint32_t");
            return NULL;
        }

        if (size > 0xFFFFFFF0ULL) {
            PyErr_SetString(PyExc_OverflowError, "size must fit in the synchronization command field");
            return NULL;
        }

        if ((size & 0x0F) != 0) {
            PyErr_SetString(PyExc_ValueError, "size must be aligned to 16 bytes");
            return NULL;
        }

        if (direction < 0 || direction > 3) {
            PyErr_SetString(PyExc_ValueError, "direction must be between 0 and 3");
            return NULL;
        }
    }

    sync_args = ENCODE_SYNC_COMMAND_ARGUMENT(command, offset, size, direction);

    fd = PyObject_AsFileDescriptor(file_obj);
    if (fd < 0)
        return NULL;

    if (ioctl(fd, U_DMA_BUF_IOCTL_SET_SYNC_FOR_CPU, &sync_args) < 0)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_RETURN_NONE;
}

/**
 * u_dma_buf_ioctl_sync_for_device() - Synchronize DMA buffer for deivce.
 * @self: Python module object.
 * @args: Tuple containing a file object, a command, and optional
 *        synchronization parameters.
 *
 * Synchronizes the DMA buffer for device using
 * U_DMA_BUF_IOCTL_SET_SYNC_FOR_DEVICE.
 *
 * The command is required:
 * - 0: Synchronize using the specified or previously configured
 *      synchronization parameters.
 * - 1: Synchronize using the specified or previously configured
 *      synchronization parameters.
 *
 * The following synchronization parameters are optional, but must be
 * specified together:
 * - @offset: Synchronization range offset in bytes (uint32_t).
 * - @size: Synchronization range size in bytes.
 *     Must be aligned to 16 bytes and fit in the 32-bit size field.
 * - @direction: DMA data direction.
 *     1: DMA to device.
 *     2: DMA from device.
 *     3: DMA bidirectional.
 *
 * If all synchronization parameters are omitted, the previously configured
 * synchronization parameters are used.
 *
 * Returns None on success, or NULL with a Python exception on error.
 */
static PyObject *
u_dma_buf_ioctl_sync_for_device(PyObject* self, PyObject* args)
{
    PyObject*          file_obj;
    int                fd;
    int                command;
    int                direction = 0;
    unsigned long long offset    = 0;
    unsigned long long size      = 0;
    uint64_t           sync_args = 0;
    Py_ssize_t         nargs     = PyTuple_GET_SIZE(args);

    if (nargs != 2 && nargs != 5) {
        PyErr_SetString(PyExc_TypeError, "expected 2 or 5 arguments");
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "Oi|KKi",
                          &file_obj,
                          &command,
                          &offset,
                          &size,
                          &direction)) {
        return NULL;
    }

    if (command < 0 || command > 1) {
        PyErr_SetString(PyExc_ValueError, "command must be 0 or 1");
        return NULL;
    }

    if (nargs == 5) {
        if (offset > UINT32_MAX) {
            PyErr_SetString(PyExc_OverflowError, "offset must fit in a 32-bit field");
            return NULL;
        }

        if (size > 0xFFFFFFF0ULL) {
            PyErr_SetString(PyExc_OverflowError, "size must fit in the synchronization command field");
            return NULL;
        }

        if ((size & 0x0F) != 0) {
            PyErr_SetString(PyExc_ValueError, "size must be aligned to 16 bytes");
            return NULL;
        }

        if (direction < 0 || direction > 3) {
            PyErr_SetString(PyExc_ValueError, "direction must be between 0 and 3");
            return NULL;
        }
    }

    sync_args = ENCODE_SYNC_COMMAND_ARGUMENT(command, offset, size, direction);

    fd = PyObject_AsFileDescriptor(file_obj);
    if (fd < 0)
        return NULL;

    if (ioctl(fd, U_DMA_BUF_IOCTL_SET_SYNC_FOR_DEVICE,
              &sync_args) < 0) {
        return PyErr_SetFromErrno(PyExc_OSError);
    }

    Py_RETURN_NONE;
}

/**
 * u_dma_buf_ioctl_export() - Export a DMA buffer as a DMA-BUF file descriptor.
 * @self: Python object associated with this function.
 * @args: Tuple containing the arguments:
 *     - file: An open u-dma-buf device file object.
 *     - offset: Offset of the DMA buffer range to export, in bytes.
 *     - size: Size of the DMA buffer range to export, in bytes.
 *     - flag: Optional file descriptor flags (default: 0).
 *
 * Issues U_DMA_BUF_IOCTL_EXPORT to export the specified DMA buffer range.
 *
 * Return: A tuple containing the exported file descriptor and DMA address
 * on success, or NULL with a Python exception set on failure.
 *
 * The returned file descriptor is owned by the caller and must be closed
 * when it is no longer needed. It can be converted to a Python file object
 * using os.fdopen() or io.FileIO(). Closing that file object also closes
 * the file descriptor.
 *
 * Possible exceptions include TypeError for invalid arguments,
 * OverflowError for values outside the supported range, and OSError
 * if the IOCTL operation fails.
 */
static PyObject*
u_dma_buf_ioctl_export(PyObject* self, PyObject* args)
{
    PyObject*          file_obj;
    PyObject*          offset_obj;
    PyObject*          size_obj;
    PyObject*          flag_obj = NULL;
    int                fd;
    unsigned long long offset;
    unsigned long long size;
    unsigned long long flag = 0;
    u_dma_buf_ioctl_export_args export_args = {0};

    if (!PyArg_ParseTuple(args, "OOO|O",
                          &file_obj,
                          &offset_obj,
                          &size_obj,
                          &flag_obj)) {
        return NULL;
    }

    offset = PyLong_AsUnsignedLongLong(offset_obj);
    if (PyErr_Occurred())
        return NULL;

    size = PyLong_AsUnsignedLongLong(size_obj);
    if (PyErr_Occurred())
        return NULL;

    if (flag_obj != NULL) {
        flag = PyLong_AsUnsignedLongLong(flag_obj);
        if (PyErr_Occurred())
            return NULL;
    }

    if (flag > UINT32_MAX) {
        PyErr_SetString(PyExc_OverflowError, "flag must fit in a 32-bit field");
        return NULL;
    }

    fd = PyObject_AsFileDescriptor(file_obj);
    if (fd < 0)
        return NULL;

    export_args.offset = (uint64_t)offset;
    export_args.size   = (uint64_t)size;

    SET_U_DMA_BUF_IOCTL_FLAGS_EXPORT_FD_FLAGS(&export_args, (int)flag);

    if (ioctl(fd, U_DMA_BUF_IOCTL_EXPORT, &export_args) < 0)
        return PyErr_SetFromErrno(PyExc_OSError);

    return Py_BuildValue(
        "(iK)",
        export_args.fd,
        (unsigned long long)export_args.addr
    );
}

/*
 * Module definition
 */
static PyMethodDef u_dma_buf_ioctl_methods[] = {
    {
        "get_device_info",
        u_dma_buf_ioctl_get_dev_info,
        METH_VARARGS,
        "Get u-dma-buf device information.\n"
        "\n"
        "Args:\n"
        "    file: An open u-dma-buf device file object.\n"
        "\n"
        "Returns:\n"
        "    dict: A dictionary containing the following entries:\n"
        "        size:         DMA buffer size in bytes.\n"
        "        dma_address:  DMA buffer physical address.\n"
        "        flags:        Raw device flags.\n"
        "        dma_mask:     DMA mask width in bits.\n"
        "        dma_coherent: DMA coherency flag (0 or 1).\n"
        "        mmap_mode:    Memory mapping mode.\n"
        "            0: undefined\n"
        "            1: always off\n"
        "            2: always on\n"
        "            3: auto\n"
        "            4: page\n"
        "\n"
        "Raises:\n"
        "    OSError: If the IOCTL operation fails.\n"
        "    TypeError: If the argument does not provide a valid file descriptor.\n"
    },
    {
        "get_driver_info",
        u_dma_buf_ioctl_get_drv_info,
        METH_VARARGS,
        "Get u-dma-buf driver information.\n"
        "\n"
        "Args:\n"
        "    file: An open u-dma-buf device file object.\n"
        "\n"
        "Returns:\n"
        "    dict: A dictionary containing the following entries:\n"
        "        flags:                Raw driver flags.\n"
        "        ioctl_version:        IOCTL interface version.\n"
        "        in_kernel_functions:  In-kernel functions availability.\n"
        "        use_of_dma_config:    Device Tree DMA configuration usage.\n"
        "        use_of_reserved_mem:  Reserved memory usage.\n"
        "        use_quirk_mmap:       quirk-mmap support.\n"
        "        use_quirk_mmap_page:  quirk-mmap-page support.\n"
        "        version:              u-dma-buf driver version string.\n"
        "\n"
        "Raises:\n"
        "    OSError: If the IOCTL operation fails.\n"
        "    TypeError: If the argument is not a valid file object.\n"
    },
    {
        "get_size",
        u_dma_buf_ioctl_get_size,
        METH_VARARGS,
        "Get the DMA buffer size.\n"
        "\n"
        "Args:\n"
        "    file: An open u-dma-buf device file object.\n"
        "\n"
        "Returns:\n"
        "    int: DMA buffer size in bytes.\n"
        "\n"
        "Raises:\n"
        "    OSError: If the IOCTL operation fails.\n"
        "    TypeError: If the argument does not provide a valid file descriptor.\n"
    },
    {
        "get_dma_address",
        u_dma_buf_ioctl_get_dma_addr,
        METH_VARARGS,
        "Get the DMA buffer address.\n"
        "\n"
        "Args:\n"
        "    file: An open u-dma-buf device file object.\n"
        "\n"
        "Returns:\n"
        "    int: DMA buffer DMA address.\n"
        "\n"
        "Raises:\n"
        "    OSError: If the IOCTL operation fails.\n"
        "    TypeError: If the argument does not provide a valid file descriptor.\n"
    },
    {
        "get_sync_owner",
        u_dma_buf_ioctl_get_sync_owner,
        METH_VARARGS,
        "Get the DMA buffer synchronization owner.\n"
        "\n"
        "Args:\n"
        "    file: An open u-dma-buf device file object.\n"
        "\n"
        "Returns:\n"
        "    int: DMA buffer synchronization owner.\n"
        "       0: CPU.\n"
        "       1: Device.\n"
        "\n"
        "Raises:\n"
        "    OSError: If the IOCTL operation fails.\n"
        "    TypeError: If the argument does not provide a valid file descriptor.\n"
    },
    {
        "get_sync",
        u_dma_buf_ioctl_get_sync,
        METH_VARARGS,
        "Get DMA buffer synchronization parameters.\n"
        "\n"
        "Args:\n"
        "    file: An open u-dma-buf device file object.\n"
        "\n"
        "Returns:\n"
        "    dict: A dictionary containing the following entries:\n"
        "        offset:    Synchronization range offset in bytes.\n"
        "        size:      Synchronization range size in bytes.\n"
        "        direction: Synchronization direction.\n"
        "            1: DMA to device.\n"
        "            2: DMA from device.\n"
        "            3: DMA bidirectional.\n"
        "        owner:     Synchronization owner.\n"
        "            0: CPU.\n"
        "            1: Device.\n"
        "        mode:      Synchronization mode.\n"
        "        flags:     Raw synchronization flags.\n"
        "\n"
        "Raises:\n"
        "    OSError: If the IOCTL operation fails.\n"
        "    TypeError: If the argument does not provide a valid file descriptor.\n"
    },
    {
        "set_sync",
        u_dma_buf_ioctl_set_sync,
        METH_VARARGS,
        "Set DMA buffer synchronization parameters.\n"
        "\n"
        "Args:\n"
        "    file: An open u-dma-buf device file object.\n"
        "    settings: A dictionary containing synchronization parameters.\n"
        "        Required entries:\n"
        "            size:   Synchronization range size in bytes.\n"
        "            offset: Synchronization range offset in bytes.\n"
        "        Optional entries (default to 0 if omitted):\n"
        "            command:   Synchronization command.\n"
        "                0: No operation.\n"
        "                1: Sync for CPU.\n"
        "                2: No operation.\n"
        "                3: Sync for device.\n"
        "            direction: Synchronization direction.\n"
        "                0: Unchanged.\n"
        "                1: DMA to device.\n"
        "                2: DMA from device.\n"
        "                3: DMA bidirectional.\n"
        "            fast:      Fast synchronization flag.\n"
        "            mode:      Synchronization mode.\n"
        "\n"
        "Returns:\n"
        "    None on success.\n"
        "\n"
        "Raises:\n"
        "    KeyError: If a required settings entry is missing.\n"
        "    TypeError: If the arguments have invalid types or the file\n"
        "        object does not provide a valid file descriptor.\n"
        "    OverflowError: If a numeric value is out of range.\n"
        "    OSError: If the IOCTL operation fails.\n"
    },
    {
        "sync_for_cpu",
        u_dma_buf_ioctl_sync_for_cpu,
        METH_VARARGS,
        "Synchronize DMA buffer for CPU.\n"
        "\n"
        "Args:\n"
        "    file: An open u-dma-buf device file object.\n"
        "    command: Synchronization command (0 or 1).\n"
        "    offset: Synchronization range offset in bytes (uint32_t).\n"
        "    size: Synchronization range size in bytes (32-bit field, 16-byte aligned).\n"
        "    direction: DMA data direction.\n"
        "        1: DMA to device.\n"
        "        2: DMA from device.\n"
        "        3: DMA bidirectional.\n"
        "    offset, size, and direction are optional, but must be specified\n"
        "    together. If omitted, the previously configured synchronization\n"
        "    parameters are used.\n"
        "\n"
        "Returns:\n"
        "    None on success.\n"
        "\n"
        "Raises:\n"
        "    TypeError: If the arguments have invalid types or an invalid number\n"
        "        of arguments is provided.\n"
        "    ValueError: If command or direction is outside its supported range.\n"
        "    OverflowError: If offset or size exceeds its supported range.\n"
        "    OSError: If the IOCTL operation fails.\n"
    },
    {
        "sync_for_device",
        u_dma_buf_ioctl_sync_for_device,
        METH_VARARGS,
        "Synchronize DMA buffer for device.\n"
        "\n"
        "Args:\n"
        "    file: An open u-dma-buf device file object.\n"
        "    command: Synchronization command (0 or 1).\n"
        "    offset: Synchronization range offset in bytes (uint32_t).\n"
        "    size: Synchronization range size in bytes (32-bit field, 16-byte aligned).\n"
        "    direction: DMA data direction.\n"
        "        1: DMA to device.\n"
        "        2: DMA from device.\n"
        "        3: DMA bidirectional.\n"
        "    offset, size, and direction are optional, but must be specified\n"
        "    together. If omitted, the previously configured synchronization\n"
        "    parameters are used.\n"
        "\n"
        "Returns:\n"
        "    None on success.\n"
        "\n"
        "Raises:\n"
        "    TypeError: If the arguments have invalid types or an invalid number\n"
        "        of arguments is provided.\n"
        "    ValueError: If command or direction is outside its supported range.\n"
        "    OverflowError: If offset or size exceeds its supported range.\n"
        "    OSError: If the IOCTL operation fails.\n"
    },
    {
        "export_dma_buf",
        u_dma_buf_ioctl_export,
        METH_VARARGS,
        "Export a DMA buffer as a DMA-BUF file descriptor.\n"
        "\n"
        "Args:\n"
        "    file: An open u-dma-buf device file object.\n"
        "    offset: Offset of the DMA buffer range to export, in bytes.\n"
        "    size: Size of the DMA buffer range to export, in bytes.\n"
        "    flag: Optional file descriptor flags (default: 0).\n"
        "\n"
        "Returns:\n"
        "    A tuple containing the exported file descriptor (fd) and DMA address.\n"
        "\n"
        "    The returned fd is a new file descriptor owned by the caller.\n"
        "    The caller is responsible for closing it when it is no longer needed.\n"
        "    To create a Python file object from fd, use os.fdopen() or io.FileIO().\n"
        "    Closing the resulting file object also closes fd.\n"
        "\n"
        "Raises:\n"
        "    TypeError: If the arguments have invalid types or an invalid number\n"
        "        of arguments is provided.\n"
        "    OverflowError: If offset, size, or flag is outside its supported range.\n"
        "    OSError: If the IOCTL operation fails.\n"
    },
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef u_dma_buf_ioctl_module = {
    PyModuleDef_HEAD_INIT,
    "_u_dma_buf_ioctl",
    "u-dma-buf IOCTL interface.",
    -1,
    u_dma_buf_ioctl_methods
};

#define PYINIT_FUNC_NAME(x) PyInit_ ## x
#define PYINIT_FUNC(x) PyMODINIT_FUNC PYINIT_FUNC_NAME(x)(void) 
PYINIT_FUNC(MODULE_NAME)
{
    return PyModule_Create(&u_dma_buf_ioctl_module);
}
