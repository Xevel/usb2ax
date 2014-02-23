Linux compatibility for DynamixelSDK
====================================

The DynamixelSDK is not compatible with the USB2AX out of the box due to differences in the drivers betwee the USB2AX and ROBOTIS' USB2Dynamixel.
This file makes it possible to use the DynamixelSDK on Linux.

How to use:
- replace the file in the DynamixelSDK
- recompile the Dynamixel SDK library
- recompile the application