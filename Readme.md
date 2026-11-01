python-u-dma-buf-ioctl
==================================================================================

Python interface for u-dma-buf IOCTL operations.

`python-u-dma-buf-ioctl` is a Python C extension module that provides access to the IOCTL interface of the Linux [u-dma-buf](https://github.com/ikwzm/udmabuf) device driver.

It allows Python applications to retrieve device and driver information, manage DMA buffer synchronization, and export DMA buffers as DMA-BUF file descriptors.

The extension uses the Python C API to invoke Linux IOCTL operations directly. It does not require `ctypes`.

Features
----------------------------------------------------------------------------------

- Retrieve u-dma-buf driver information.
- Retrieve DMA buffer size and DMA address.
- Retrieve device configuration, including DMA mask, coherency, and mmap mode.
- Retrieve synchronization settings and ownership information.
- Configure DMA buffer synchronization.
- Synchronize DMA buffers for the CPU or device.
- Export a DMA buffer range as a DMA-BUF file descriptor.

Requirements
----------------------------------------------------------------------------------

- Linux
- Python 3.9 or later
- A C compiler
- Python development headers
- setuptools 74.1 or later
- A compatible u-dma-buf kernel driver with IOCTL support

The available IOCTL operations depend on the version and configuration of the u-dma-buf kernel driver.

Installation
----------------------------------------------------------------------------------

Install the build dependencies on Ubuntu:

```bash
sudo apt update
sudo apt install build-essential python3-dev python3-venv
```

Clone the repository and enter the project directory:

```bash
git clone https://github.com/ikwzm/python-u-dma-buf-ioctl.git
cd python-u-dma-buf-ioctl
```

Create and activate a virtual environment:

```bash
python3 -m venv .venv
source .venv/bin/activate
```

Install the package in editable mode:

```bash
python -m pip install --upgrade pip setuptools
python -m pip install -e .
```

Quick Start
----------------------------------------------------------------------------------

Open a u-dma-buf device file and retrieve its information:

```python
import u_dma_buf_ioctl

with open("/dev/udmabuf0", "r+b", buffering=0) as fp:
    drv_info = u_dma_buf_ioctl.get_drv_info(fp)
    dev_info = u_dma_buf_ioctl.get_dev_info(fp)
    size = u_dma_buf_ioctl.get_size(fp)
    dma_addr = u_dma_buf_ioctl.get_addr(fp)

    print("Driver information:", drv_info)
    print("Device information:", dev_info)
    print(f"Buffer size: {size} bytes")
    print(f"DMA address: 0x{dma_addr:X}")
```

The device file must be accessible to the current user. Depending on the system configuration, this may require appropriate device permissions.

## API Reference

All functions that operate on a device file accept an open Python file object. The extension obtains its file descriptor using `PyObject_AsFileDescriptor()`.

The extension does not close the file descriptor supplied by the caller.

### `get_drv_info(file)`

Retrieve driver information.

```python
info = u_dma_buf_ioctl.get_drv_info(fp)
```

Returns a dictionary containing driver information, including:

| Key                   | Description                                                        |
| --------------------- | ------------------------------------------------------------------ |
| `version`             | Driver version string                                              |
| `ioctl_version`       | IOCTL interface version                                            |
| `in_kernel_functions` | Whether in-kernel functions are available                          |
| `use_of_dma_config`   | Whether the driver uses the DMA configuration from the device tree |
| `use_of_reserved_mem` | Whether the driver uses reserved memory                            |
| `use_quirk_mmap`      | Whether the driver supports the quirk mmap mode                    |
| `use_quirk_mmap_page` | Whether the driver supports the quirk mmap page mode               |
| `flags`               | Raw driver feature flags                                           |

The feature flags describe the capabilities reported by the running kernel driver.

### `get_dev_info(file)`

Retrieve device information.

```python
info = u_dma_buf_ioctl.get_dev_info(fp)
```

Returns a dictionary containing:

| Key            | Description                                  |
| -------------- | -------------------------------------------- |
| `addr`         | DMA address                                  |
| `size`         | DMA buffer size in bytes                     |
| `dma_mask`     | DMA mask width in bits                       |
| `dma_coherent` | Whether DMA coherency is reported as enabled |
| `mmap_mode`    | Configured mmap mode                         |
| `flags`        | Raw device information flags                 |

The `addr` field is a DMA address. It should not automatically be treated as a CPU virtual address or a universally valid physical address.

The `mmap_mode` values are:

| Value | Meaning    |
| ----: | ---------- |
| 0     | Undefined  |
| 1     | Always off |
| 2     | Always on  |
| 3     | Automatic  |
| 4     | Page       |

### `get_size(file)`

Retrieve the DMA buffer size in bytes.

```python
size = u_dma_buf_ioctl.get_size(fp)
```

Returns a Python integer.

### `get_addr(file)`

Retrieve the DMA address of the buffer.

```python
dma_addr = u_dma_buf_ioctl.get_addr(fp)
```

Returns a Python integer representing the unsigned 64-bit DMA address.

### `get_owner(file)`

Retrieve the current synchronization owner.

```python
owner = u_dma_buf_ioctl.get_owner(fp)
```

The return value is:

| Value | Meaning |
| ----: | ------- |
| 0     | CPU     |
| 1     | Device  |

This information describes the synchronization ownership reported by the driver.

### `get_sync(file)`

Retrieve the current synchronization settings.

```python
settings = u_dma_buf_ioctl.get_sync(fp)
```

Returns a dictionary containing:

| Key         | Description                           |
| ----------- | ------------------------------------- |
| `offset`    | Synchronization range offset in bytes |
| `size`      | Synchronization range size in bytes   |
| `direction` | DMA synchronization direction         |
| `mode`      | Synchronization mode                  |
| `owner`     | Synchronization owner                 |
| `flags`     | Raw synchronization flags             |

The direction values are:

| Value | Meaning           |
| ----: | ----------------- |
| 0     | Direction value 0 |
| 1     | DMA to device     |
| 2     | DMA from device   |
| 3     | Bidirectional DMA |

The interpretation of synchronization settings depends on the kernel driver implementation.

### `set_sync(file, settings)`

Configure synchronization settings.

```python
u_dma_buf_ioctl.set_sync(fp, {"size": 4096, "offset": 0, "direction": 1})
```

The `settings` argument is a dictionary. The `size` and `offset` fields specify the synchronization range. The optional `command`, `direction`, `fast`, and `mode` fields configure the corresponding synchronization flags.

The supported fields and their semantics must match the IOCTL interface implemented by the running kernel driver.

### `sync_for_cpu(file, command, offset, size, direction)`

Synchronize a DMA buffer range for CPU access.

```python
u_dma_buf_ioctl.sync_for_cpu(fp, 1, 0, 4096, 2)
```

The `command` argument controls whether cache synchronization is performed:

- `0`: Do nothing; cache synchronization is not performed.
- `1`: Perform cache synchronization.

The `offset`, `size`, and `direction` arguments are optional, but must be specified together.

When these three arguments are omitted, the synchronization parameters previously configured in the driver are used.

The range arguments are encoded into a 64-bit synchronization command:

- `offset` must fit in a 32-bit field.
- `size` must fit in the encoded size field and be aligned to 16 bytes.
- `direction` must be between 0 and 3.

The maximum encoded size is `0xFFFFFFF0` bytes.

### `sync_for_device(file, command, offset, size, direction)`

Synchronize a DMA buffer range for device access.

```python
u_dma_buf_ioctl.sync_for_device(fp, 1, 0, 4096, 1)
```

The argument format and validation rules are the same as those of `sync_for_cpu()`.

Both synchronization functions return `None` on success and raise `OSError` if the IOCTL operation fails.

### `export(file, offset, size, flag=0)`

Export a range of a u-dma-buf buffer as a DMA-BUF file descriptor.

```python
dma_buf_fd, dma_addr = u_dma_buf_ioctl.export(fp, 0, 4096)
```

The `flag` argument is optional. It specifies the file descriptor flags supported by the driver, such as `os.O_CLOEXEC`, `os.O_SYNC`, and access mode flags.

For example:

```python
import os
import u_dma_buf_ioctl

with open("/dev/udmabuf0", "r+b", buffering=0) as fp:
    dma_buf_fd, dma_addr = u_dma_buf_ioctl.export(
        fp,
        0,
        4096,
        os.O_CLOEXEC | os.O_RDWR,
    )

    print(f"DMA-BUF FD: {dma_buf_fd}")
    print(f"DMA address: 0x{dma_addr:X}")
```

The return value is a tuple:

```python
(dma_buf_fd, dma_addr)
```

- `dma_buf_fd` is the newly exported DMA-BUF file descriptor.
- `dma_addr` is the DMA address returned by the driver for the exported buffer range.

**The caller is responsible for closing the returned file descriptor.** It is independent of the file descriptor used to open the original u-dma-buf device.

To convert the returned descriptor into a Python file object, use `os.fdopen()`:

```python
import os

dma_buf_file = os.fdopen(dma_buf_fd, "r+b", buffering=0)

try:
    # Use dma_buf_file where a Python file object is required.
    pass
finally:
    dma_buf_file.close()
```

Closing the resulting file object also closes `dma_buf_fd`. Do not close the same descriptor a second time.

The appropriate file mode depends on how the exported DMA-BUF is intended to be used. Exporting a DMA-BUF does not itself guarantee cache coherency or synchronize access between the CPU and other devices.

Error Handling
----------------------------------------------------------------------------------

IOCTL failures raise `OSError`.

For example:

```python
import u_dma_buf_ioctl

try:
    with open("/dev/udmabuf0", "r+b", buffering=0) as fp:
        size = u_dma_buf_ioctl.get_size(fp)
except OSError as exc:
    print(f"IOCTL failed: {exc}")
```

Invalid argument types or argument counts may raise `TypeError`. Values outside supported ranges may raise `OverflowError`, and invalid parameter values may raise `ValueError`.

DMA Buffer Synchronization
----------------------------------------------------------------------------------

DMA buffer synchronization is important when the CPU and a DMA-capable device access the same memory.

The required synchronization depends on the hardware, the device tree configuration, and the cache behavior configured for u-dma-buf.

For non-coherent DMA buffers, the application may need to synchronize the buffer before the CPU reads data written by a device or before a device reads data written by the CPU.

For example:

```python
# Prepare the buffer for device access.
u_dma_buf_ioctl.sync_for_device(fp, 1, 0, 4096, 1)

# Submit work to the device and wait for completion here.

# Prepare the buffer for CPU access.
u_dma_buf_ioctl.sync_for_cpu(fp, 1, 0, 4096, 2)
```

The example illustrates the intended sequence. The actual synchronization commands and directions must match the driver implementation and the DMA operation being performed.

Cache synchronization does not replace the need to coordinate access between the CPU and the device. The application must ensure that a device has completed its operation before the CPU accesses the affected buffer range.

Development
----------------------------------------------------------------------------------

To install the package in editable mode:

```bash
python -m pip install -e .
```

The C extension is built from:

```text
src/u_dma_buf_ioctl/u-dma-buf-ioctl.c
```

The IOCTL definitions are provided by:

```text
src/u_dma_buf_ioctl/u-dma-buf-ioctl.h
```

License
----------------------------------------------------------------------------------

BSD 2-Clause License.