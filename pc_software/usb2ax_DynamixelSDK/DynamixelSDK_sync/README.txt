DynamixelSDK_sync (with SYNC_READ and SYNC_WRITE)
=================================================

This patched version of the DynamixelSDK has functions to use the full potential of the USB2AX, with easy access to SYNC_READ and SYNC_WRITE.
This works with all versions of the SDK (but on Linux you need to use the linux_compatibility patch at the same time for the library to work with the USB2AX).

Note: SYNC_WRITE would work with any other hardware interface (like the USB2Dynamixel) as it is part of the original protocol, but SYNC_READ only works with the USB2AX and some other specialized third party controllers.

How to use in your application:
- replace all the files in the DynamixelSDK
- recompile the dynamixel library
- recompile the application
