/*
+------------------------------------------------------------------------------
|  Copyright 2004-2007 Texas Instruments Incorporated. All rights reserved.
|
|  IMPORTANT: Your use of this Software is limited to those specific rights
|  granted under the terms of a software license agreement between the user who
|  downloaded the software, his/her employer (which must be your employer) and
|  Texas Instruments Incorporated (the "License"). You may not use this Software
|  unless you agree to abide by the terms of the License. The License limits
|  your use, and you acknowledge, that the Software may not be modified, copied
|  or distributed unless embedded on a Texas Instruments microcontroller or used
|  solely and exclusively in conjunction with a Texas Instruments radio
|  frequency transceiver, which is integrated into your product. Other than for
|  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
|  works of, modify, distribute, perform, display or sell this Software and/or
|  its documentation for any purpose.
|
|  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
|  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
|  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
|  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
|  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
|  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
|  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES INCLUDING
|  BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
|  CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
|  SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
|  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
|
|  Should you have any questions regarding your right to use this Software,
|  contact Texas Instruments Incorporated at www.TI.com.
|
+------------------------------------------------------------------------------
*/
#ifndef USBDESCRIPTOR_H
#define USBDESCRIPTOR_H
/** \addtogroup module_usb_descriptor  USB Descriptor
 * \brief This module contains contains USB descriptor definitions, and guidelines on how to write 
 * descriptor sets that work with the USB library.
 *
 * Information on the specific USB descriptor types is available in the USB 2.0 specification and
 * in device class documentation. Examples of complete descriptor sets can be found in the Chipcon USB
 * application examples.
 *
 * \section section_usbdsc_standard Standard Descriptors
 * The library requires the USB descriptor set to be organized as follows:
 * - Device descriptor (\ref USB_DEVICE_DESCRIPTOR)
 *     - Configuration descriptor (\ref USB_CONFIGURATION_DESCRIPTOR)
 *         - Interface descriptor (\ref USB_INTERFACE_DESCRIPTOR)
 *             - Endpoint descriptor (\ref USB_ENDPOINT_DESCRIPTOR)
 *             - More endpoint descriptors
 *         - More interface descriptors
 *     - More configuration descriptors
 * - String descriptor (\ref USB_STRING_DESCRIPTOR)
 * - More string descriptors
 *
 * Different USB device classes, such as "HID" and "Audio", may add other standard format descriptor
 * types to the hierarchy, and even extend the existing types. This is also supported by the library.
 *
 * Refer to the \ref module_usb_descriptor_parser module for information on
 * \li Where in memory the descriptor set can be placed
 * \li How to set the required markers (symbols), \ref pUsbDescStart and \ref pUsbDescEnd.
 *
 * \section section_usbdsc_other Other Descriptors
 * Differently formatted descriptors are not supported by the parsing mechanism, and are instead located
 * through a \ref DESC_LUT_INFO look-up table. Each entry in the \ref pUsbDescLut table contains the
 * index and value parameters for relevant \ref GET_DESCRIPTOR requests, and the locations and lengths
 * of the corresponding descriptors:
 * \code
 *                 ; Make the symbols public
 *                 PUBLIC pUsbDescLut;
 *                 PUBLIC pUsbDescLutEnd;
 *
 *                 ...
 *
 * pUsbDescLut:    DB HID_REPORT,  00H                         ; value (MSB, LSB)
 *                 DB 00H,         00H                         ; index (MSB, LSB)
 *                 DW hidReportDesc0Start                      ; pDescStart
 *                 DW hidReportDesc0End - hidReportDesc0Start  ; length
 *
 *                 DB HID_REPORT,  01H                         ; value (MSB, LSB)
 *                 DB 00H,         00H                         ; index (MSB, LSB)
 *                 DW hidReportDesc1Start                      ; pDescStart
 *                 DW hidReportDesc1End - hidReportDesc1Start  ; length
 * pUsbDescLutEnd:
 * \endcode
 *
 * An additional look-up table is needed configure endpoint double-buffering. The table must contain one
 * \ref DBLBUF_LUT_INFO entry for each interface descriptor with non-zero \c bNumEndpoints:
 * \code
 *                 ; Make the symbol public
 *                 PUBLIC pUsbDblbufLut;
 *
 *                 ...
 *
 * pUsbDblbufLut:  DW interface0Desc  ; pInterface
 *                 DB 02H             ; inMask   (example: EP1 IN is double-buffered)
 *                 DB 00H             ; outMask  (example: No double-buffered OUT endpoints)
 *
 *                 DW interface1Desc  ; pInterface
 *                 DB 10H             ; inMask   (example: EP4 IN is double-buffered)
 *                 DB 08H             ; outMask  (example: EP3 OUT is double-buffered)
 * \endcode
 * @{
 */


//-------------------------------------------------------------------------------------------------------
/// \name Sizes
//@{
#define EP0_PACKET_SIZE  32    ///< The maximum data packet size for endpoint 0
//@}

/// \name Standard Descriptor Types
//@{
#define DESC_TYPE_DEVICE       0x01  ///< Device
#define DESC_TYPE_CONFIG       0x02  ///< Configuration
#define DESC_TYPE_STRING       0x03  ///< String
#define DESC_TYPE_INTERFACE    0x04  ///< Interface
#define DESC_TYPE_ENDPOINT     0x05  ///< Endpoint
//@}

/// \name HID Class Descriptor Types
//@{
#define DESC_TYPE_HID          0x21  ///< HID descriptor (included in the interface descriptor)
#define DESC_TYPE_HIDREPORT    0x22  ///< Report descriptor (referenced in \ref pUsbDescLut)
//@}

/// \name Endpoint Types
//@{
#define EP_ATTR_CTRL     0x00  ///< Control (endpoint 0 only)
#define EP_ATTR_ISO      0x01  ///< Isochronous (not acknowledged)
#define EP_ATTR_BULK     0x02  ///< Bulk
#define EP_ATTR_INT      0x03  ///< Interrupt (guaranteed polling interval)
#define EP_ATTR_TYPE_BM  0x03  ///< Endpoint type bitmask
//@}
//-------------------------------------------------------------------------------------------------------


#ifndef ASM_FILE


//-------------------------------------------------------------------------------------------------------
/// \name USB Descriptor Markers
//@{
extern const BYTE __code pUsbDescStart[];  ///< USB descriptor start
extern const BYTE __code pUsbDescEnd[];    ///< USB descriptor end
//@}
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
/// USB device descriptor
typedef struct {
    BYTE bLength;             ///< Size of this descriptor (in bytes)
    BYTE bDescriptorType;     ///< Descriptor type = \ref DESC_TYPE_DEVICE
    WORD bcdUSB;              ///< USB specification release number (in BCD, e.g. 0110 for USB 1.1)
    BYTE bDeviceClass;        ///< Device class code
    BYTE bDeviceSubClass;     ///< Device subclass code	
    BYTE bDeviceProtocol;     ///< Device protocol code
    BYTE bMaxPacketSize0;     ///< Maximum packet size for EP0
    WORD idVendor;            ///< Vendor ID
    WORD idProduct;           ///< Product ID
    WORD bcdDevice;           ///< Device release number (in BCD)
    BYTE iManufacturer;       ///< Index of the string descriptor for manufacturer
    BYTE iProduct;            ///< Index of the string descriptor for product
    BYTE iSerialNumber;       ///< Index of the string descriptor for serial number
    BYTE bNumConfigurations;  ///< Number of possible configurations
} USB_DEVICE_DESCRIPTOR;

/// USB configuration descriptor
typedef struct {
    BYTE bLength;             ///< Size of this descriptor (in bytes)
    BYTE bDescriptorType;     ///< Descriptor type = \ref DESC_TYPE_CONFIG
    WORD wTotalLength;        ///< Total length of data for this configuration
    BYTE bNumInterfaces;      ///< Number of interfaces supported by this configuration (one-based index)
    BYTE bConfigurationValue; ///< Designator value for this configuration
    BYTE iConfiguration;      ///< Index of the string descriptor for this configuration
    BYTE bmAttributes;        ///< Configuration characteristics
    BYTE bMaxPower;           ///< Maximum power consumption in this configuration (bMaxPower * 2 mA)
} USB_CONFIGURATION_DESCRIPTOR;

/// USB interface descriptor
typedef struct {
    BYTE bLength;             ///< Size of this descriptor (in bytes)
    BYTE bDescriptorType;     ///< Descriptor type = \ref DESC_TYPE_INTERFACE
    BYTE bInterfaceNumber;    ///< Number of *this* interface (zero-based index)
    BYTE bAlternateSetting;   ///< Alternate setting index for this descriptor (zero-based index)
    BYTE bNumEndpoints;       ///< Number of endpoints for this interface (excl. EP0)
    BYTE bInterfaceClass;     ///< Interface class code
    BYTE bInterfaceSubClass;  ///< Interface subclass code
    BYTE bInterfaceProtocol;  ///< Interface protocol code
    BYTE iInterface;          ///< Index of the string descriptor for this interface
} USB_INTERFACE_DESCRIPTOR;

/// USB endpoint descriptor
typedef struct {
    BYTE bLength;             ///< Size of this descriptor (in bytes)
    BYTE bDescriptorType;     ///< Descriptor type = \ref DESC_TYPE_ENDPOINT
    BYTE bEndpointAddress;    ///< Endpoint address (direction[7] + number[3:0])
    BYTE bmAttributes;        ///< Endpoint attributes (ISO / BULK / INT)
    WORD wMaxPacketSize;	  ///< Maximum endpoint packet size
    BYTE bInterval;           ///< \ref EP_ATTR_INT : Polling interval (in ms)
} USB_ENDPOINT_DESCRIPTOR;

/// USB string descriptor
typedef struct {
    BYTE bLength;             ///< Size of this descriptor (in bytes)
    BYTE bDescriptorType;     ///< Descriptor type = \ref DESC_TYPE_STRING
    WORD pString[1];          ///< Unicode string
} USB_STRING_DESCRIPTOR;

/// USB HID descriptor
typedef struct {
    BYTE bLength;               ///< Size of this descriptor (in bytes)
    BYTE bDescriptorType;       ///< HID descriptor type
    WORD bscHID;                ///< HID specification release number (in BCD)
    BYTE bCountryCode;          ///< Hardware target country
    BYTE bNumDescriptors;       ///< Number of HID class descriptors to follow
    BYTE bRDescriptorType;      ///< Report descriptor type
    WORD wDescriptorLength;     ///< Total length of the associated report descriptor
} USB_HID_DESCRIPTOR;
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
/// Look-up table entry for non-standard descriptor types (used with \ref usbsrGetDescriptor)
typedef struct {
    BYTE valueMsb;            ///< LSB of the \ref USB_SETUP_HEADER.value request parameter
    BYTE valueLsb;            ///< MSB of the \ref USB_SETUP_HEADER.value request parameter
    BYTE indexMsb;            ///< LSB of the \ref USB_SETUP_HEADER.index request parameter
    BYTE indexLsb;            ///< MSB of the \ref USB_SETUP_HEADER.index request parameter
    BYTE __xdata *pDescStart;  ///< A pointer to the descriptor to be returned for the given index/value
    UINT16 length;            ///< The length of the returned descriptor
} DESC_LUT_INFO;

/// Look-up table for double-buffer settings
typedef struct {
    USB_INTERFACE_DESCRIPTOR __xdata *pInterface; ///< Pointer to an interface descriptor
    BYTE inMask;                                 ///< Bitmask for IN endpoints (bit x maps to EPx IN)
    BYTE outMask;                                ///< Bitmask for OUT endpoints (bit x maps to EPx OUT)
} DBLBUF_LUT_INFO;

/// \name Look-up Table Markers
//@{
extern const DESC_LUT_INFO __xdata pUsbDescLut[]; ///< Start of USB descriptor look-up table
extern const DESC_LUT_INFO __xdata pUsbDescLutEnd[]; ///< End of USB descriptor look-up table
extern const DBLBUF_LUT_INFO __xdata pUsbDblbufLut[]; ///< Start of double-buffering look-up table
//@}
//-------------------------------------------------------------------------------------------------------


#endif // ASM_FILE
//@}


#endif
