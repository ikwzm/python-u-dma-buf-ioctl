__version__     = "0.1.0"
__author__      = "Ichiro Kawazome"
__copyright__   = "Copyright (c) 2026 Ichiro Kawazome"
__license__     = "BSD 2-Clause"
__email__       = "ichiro_k@ca2-so-net.ne.jp"
__description__ = "Python interface for u-dma-buf IOCTL operations"

from ._u_dma_buf_ioctl import get_device_info
from ._u_dma_buf_ioctl import get_driver_info
from ._u_dma_buf_ioctl import get_size
from ._u_dma_buf_ioctl import get_dma_address
from ._u_dma_buf_ioctl import get_sync_owner
from ._u_dma_buf_ioctl import get_sync
from ._u_dma_buf_ioctl import set_sync
from ._u_dma_buf_ioctl import sync_for_cpu
from ._u_dma_buf_ioctl import sync_for_device
from ._u_dma_buf_ioctl import export_dma_buf

__all__ = [
    "get_device_info"  ,
    "get_driver_info"  ,
    "get_size"         ,
    "get_dma_address"  ,
    "get_sync_owner"   ,
    "get_sync"         ,
    "set_sync"         ,
    "sync_for_cpu"     ,
    "sync_for_device"  ,
    "export_dma_buf"   ,
]
