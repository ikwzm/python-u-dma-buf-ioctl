#!/usr/bin/env python3
"""Display u-dma-buf driver, device, and synchronization information."""

import argparse
import sys

import u_dma_buf_ioctl


def print_info(title, info):
    """Print a dictionary of information."""
    print(f"\n[{title}]")
    for key, value in info.items():
        if key == "flags":
            print(f"  {key:24s}: 0x{value:016x}")
        elif isinstance(value, int):
            print(f"  {key:24s}: {value} (0x{value:x})")
        else:
            print(f"  {key:24s}: {value}")


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Display driver, device, and synchronization information "
            "for a u-dma-buf device."
        )
    )
    parser.add_argument(
        "device",
        help="Path to the u-dma-buf device file, e.g. /dev/udmabuf0",
    )
    args = parser.parse_args()

    try:
        with open(args.device, "rb", buffering=0) as device:
            print(f"Device file: {args.device}")

            driver_info = u_dma_buf_ioctl.get_driver_info(device)
            print_info("Driver Information", driver_info)

            device_info = u_dma_buf_ioctl.get_device_info(device)
            print_info("Device Information", device_info)

            test_title     = "Synchronization Initial Infomation"
            sync_owner     = u_dma_buf_ioctl.get_sync_owner(device)
            sync_offset    = 0
            sync_size      = device_info["size"]
            sync_direction = 0 # Bidirectional DMA
            sync_command   = 0 # No-operation
            sync_args      = {"command"  : sync_command,
                              "size"     : sync_size,
                              "offset"   : sync_offset,
                              "direction": sync_direction}
            u_dma_buf_ioctl.set_sync(device, sync_args)
            sync_info = u_dma_buf_ioctl.get_sync(device)
            print_info(test_title, sync_info)
            if ((sync_info["offset"]    != sync_offset   ) or
                (sync_info["size"]      != sync_size     ) or
                (sync_info["direction"] != sync_direction) or
                (sync_info["owner"]     != sync_owner    )) :
                print(f"Error: not match sync info")
                return 1

            test_title     = "Synchronization Test.1.1 set_sync()"
            sync_owner     = 1 # DMA device
            sync_offset    = 0
            sync_size      = device_info["size"]//2
            sync_direction = 1 # DMA to device
            sync_command   = 3 # sync_for_device
            sync_args      = {"command"  : sync_command,
                              "size"     : sync_size,
                              "offset"   : sync_offset,
                              "direction": sync_direction}
            u_dma_buf_ioctl.set_sync(device, sync_args)
            sync_info = u_dma_buf_ioctl.get_sync(device)
            print_info(test_title, sync_info)
            if ((sync_info["offset"]    != sync_offset   ) or
                (sync_info["size"]      != sync_size     ) or
                (sync_info["direction"] != sync_direction) or
                (sync_info["owner"]     != sync_owner    )) :
                print(f"Error: not match sync info")
                return 1
            
            test_title     = "Synchronization Test.1.2 set_sync()"
            sync_owner     = 0 # CPU
            sync_offset    = 0
            sync_size      = device_info["size"]//2
            sync_direction = 1 # DMA to device
            sync_command   = 1 # sync_for_cpu
            sync_args      = {"command"  : sync_command,
                              "size"     : sync_size,
                              "offset"   : sync_offset,
                              "direction": sync_direction}
            u_dma_buf_ioctl.set_sync(device, sync_args)
            sync_info = u_dma_buf_ioctl.get_sync(device)
            print_info(test_title, sync_info)
            if ((sync_info["offset"]    != sync_offset   ) or
                (sync_info["size"]      != sync_size     ) or
                (sync_info["direction"] != sync_direction) or
                (sync_info["owner"]     != sync_owner    )) :
                print(f"Error: not match sync info")
                return 1
            
            test_title     = "Synchronization Test.2.1 set_sync()"
            sync_owner     = 1 # DMA device
            sync_offset    = device_info["size"]//2
            sync_size      = device_info["size"]//4
            sync_direction = 2 # DMA from device
            sync_command   = 3 # sync_for_device
            sync_args      = {"command"  : sync_command,
                              "size"     : sync_size,
                              "offset"   : sync_offset,
                              "direction": sync_direction}
            u_dma_buf_ioctl.set_sync(device, sync_args)
            sync_info = u_dma_buf_ioctl.get_sync(device)
            print_info(test_title, sync_info)
            if ((sync_info["offset"]    != sync_offset   ) or
                (sync_info["size"]      != sync_size     ) or
                (sync_info["direction"] != sync_direction) or
                (sync_info["owner"]     != sync_owner    )) :
                print(f"Error: not match sync info")
                return 1
            
            test_title     = "Synchronization Test.2.2 set_sync()"
            sync_owner     = 0 # CPU
            sync_offset    = device_info["size"]//2
            sync_size      = device_info["size"]//4
            sync_direction = 2 # DMA from device
            sync_command   = 1 # sync_for_cpu
            sync_args      = {"command"  : sync_command,
                              "size"     : sync_size,
                              "offset"   : sync_offset,
                              "direction": sync_direction}
            u_dma_buf_ioctl.set_sync(device, sync_args)
            sync_info = u_dma_buf_ioctl.get_sync(device)
            print_info(test_title, sync_info)
            if ((sync_info["offset"]    != sync_offset   ) or
                (sync_info["size"]      != sync_size     ) or
                (sync_info["direction"] != sync_direction) or
                (sync_info["owner"]     != sync_owner    )) :
                print(f"Error: not match sync info")
                return 1

            test_title     = "Synchronization Test.3.1 sync_for_cpu()"
            sync_owner     = 0 # CPU
            sync_offset    = sync_info["offset"]
            sync_size      = sync_info["size"]
            sync_direction = sync_info["direction"]
            u_dma_buf_ioctl.sync_for_cpu(device, 1)
            sync_info = u_dma_buf_ioctl.get_sync(device)
            print_info(test_title, sync_info)
            if ((sync_info["offset"]    != sync_offset   ) or
                (sync_info["size"]      != sync_size     ) or
                (sync_info["direction"] != sync_direction) or
                (sync_info["owner"]     != sync_owner    )) :
                print(f"Error: not match sync info")
                return 1
            
            test_title     = "Synchronization Test.3.2 sync_for_cpu()"
            sync_owner     = 0 # CPU
            sync_offset    = sync_info["offset"]
            sync_size      = sync_info["size"]
            sync_direction = sync_info["direction"]
            u_dma_buf_ioctl.sync_for_cpu(device, 1, 0, 0x1000, 0)
            sync_info = u_dma_buf_ioctl.get_sync(device)
            print_info(test_title, sync_info)
            if ((sync_info["offset"]    != sync_offset   ) or
                (sync_info["size"]      != sync_size     ) or
                (sync_info["direction"] != sync_direction) or
                (sync_info["owner"]     != sync_owner    )) :
                print(f"Error: not match sync info")
                return 1
            
            test_title     = "Synchronization Test.4.1 sync_for_device()"
            sync_owner     = 1 # Device
            sync_offset    = sync_info["offset"]
            sync_size      = sync_info["size"]
            sync_direction = sync_info["direction"]
            u_dma_buf_ioctl.sync_for_device(device, 1)
            sync_info = u_dma_buf_ioctl.get_sync(device)
            print_info(test_title, sync_info)
            if ((sync_info["offset"]    != sync_offset   ) or
                (sync_info["size"]      != sync_size     ) or
                (sync_info["direction"] != sync_direction) or
                (sync_info["owner"]     != sync_owner    )) :
                print(f"Error: not match sync info")
                return 1
            
            test_title     = "Synchronization Test.4.2 sync_for_device()"
            sync_owner     = 1 # Device
            sync_offset    = sync_info["offset"]
            sync_size      = sync_info["size"]
            sync_direction = sync_info["direction"]
            u_dma_buf_ioctl.sync_for_device(device, 1, 0, 0x1000, 0)
            sync_info = u_dma_buf_ioctl.get_sync(device)
            print_info(test_title, sync_info)
            if ((sync_info["offset"]    != sync_offset   ) or
                (sync_info["size"]      != sync_size     ) or
                (sync_info["direction"] != sync_direction) or
                (sync_info["owner"]     != sync_owner    )) :
                print(f"Error: not match sync info")
                return 1
            
            
            
    except OSError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
