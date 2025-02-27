/*
             LUFA Library
     Copyright (C) Dean Camera, 2013.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2013  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  USB Device Descriptors, for library use when in USB device mode. Descriptors are special
 *  computer-readable structures which the host requests upon device enumeration, to determine
 *  the device's capabilities and functions.
 */

#include "descriptors_detect.h"
#include "progmem.h"

uint8_t realFreq = 16;

const USB_Descriptor_Device_t DeviceDescriptor =
    {
        .Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

        .USBSpecification = VERSION_BCD(1, 1, 0),
        .Class = 0x02,
        .SubClass = 0x00,
        .Protocol = 0x00,

        .Endpoint0Size = FIXED_CONTROL_ENDPOINT_SIZE,

        .VendorID = 0x1209,

        .ProductID = 0x2883,
        .ReleaseNumber = 0x0001,

        .ManufacturerStrIndex = STRING_ID_Manufacturer,
        .ProductStrIndex = STRING_ID_Product,
        .SerialNumStrIndex = USE_INTERNAL_SERIAL,

        .NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS};

const USB_Descriptor_Configuration_t ConfigurationDescriptor =
    {
        .Config =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},

                .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
                .TotalInterfaces = 2,

                .ConfigurationNumber = 1,
                .ConfigurationStrIndex = NO_DESCRIPTOR,

                .ConfigAttributes = 0,

                .MaxPowerConsumption = USB_CONFIG_POWER_MA(100)},

        .CDC_CCI_Interface =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

                .InterfaceNumber = 0,
                .AlternateSetting = 0,

                .TotalEndpoints = 1,

                .Class = 0x02,
                .SubClass = 0x02,
                .Protocol = 0x01,

                .InterfaceStrIndex = NO_DESCRIPTOR},

        .CDC_Functional_Header =
            {
                .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalHeader_t), .Type = CDC_DTYPE_CSInterface},
                .Subtype = CDC_DSUBTYPE_CSInterface_Header,

                .CDCSpecification = VERSION_BCD(1, 1, 0),
            },

        .CDC_Functional_ACM =
            {
                .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalACM_t), .Type = CDC_DTYPE_CSInterface},
                .Subtype = CDC_DSUBTYPE_CSInterface_ACM,

                .Capabilities = 0x06,
            },

        .CDC_Functional_Union =
            {
                .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalUnion_t), .Type = CDC_DTYPE_CSInterface},
                .Subtype = CDC_DSUBTYPE_CSInterface_Union,

                .MasterInterfaceNumber = 0,
                .SlaveInterfaceNumber = 1,
            },

        .CDC_NotificationEndpoint =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

                .EndpointAddress = CDC_NOTIFICATION,
                .Attributes = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                .EndpointSize = ENDPOINT_SIZE,
                .PollingIntervalMS = 0xFF},

        .CDC_DCI_Interface =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

                .InterfaceNumber = 1,
                .AlternateSetting = 0,

                .TotalEndpoints = 2,

                .Class = 0x0A,
                .SubClass = 0x00,
                .Protocol = 0x00,

                .InterfaceStrIndex = NO_DESCRIPTOR},

        .CDC_DataOutEndpoint =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

                .EndpointAddress = DEVICE_EPADDR_OUT,
                .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                .EndpointSize = ENDPOINT_SIZE,
                .PollingIntervalMS = 0x01},

        .CDC_DataInEndpoint =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

                .EndpointAddress = DEVICE_EPADDR_IN,
                .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                .EndpointSize = ENDPOINT_SIZE,
                .PollingIntervalMS = 0x01}};

/** Language descriptor structure. This descriptor, located in FLASH memory, is returned when the host requests
 *  the string descriptor with index 0 (the first index). It is actually an array of 16-bit integers, which indicate
 *  via the language ID table available at USB.org what languages the device supports for its string descriptors.
 */
const USB_Descriptor_String_t LanguageString =
    {
        .Header = {.Size = USB_STRING_LEN(1), .Type = DTYPE_String},

        .UnicodeString = {LANGUAGE_ID_ENG}};

/** Manufacturer descriptor string. This is a Unicode string containing the manufacturer's details in human readable
 *  form, and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t ManufacturerString =
    {
        .Header = {.Size = USB_STRING_LEN(11), .Type = DTYPE_String},

        .UnicodeString = L"sanjay900"};

/** Product descriptor string. This is a Unicode string containing the product's details in human readable form,
 *  and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t ProductString16 =
    {
        .Header = {.Size = USB_STRING_LEN(56), .Type = DTYPE_String},

        .UnicodeString = L"Ardwiino - Detect - 16000000"};

const USB_Descriptor_String_t ProductString8 =
    {
        .Header = {.Size = USB_STRING_LEN(54), .Type = DTYPE_String},

        .UnicodeString = L"Ardwiino - Detect - 8000000"};

/** This function is called by the library when in device mode, and must be overridden (see library "USB Descriptors"
 *  documentation) by the application code so that the address and size of a requested descriptor can be given
 *  to the USB library. When the device receives a Get Descriptor request on the control endpoint, this function
 *  is called so that the descriptor details can be passed back and the appropriate descriptor sent back to the
 *  USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const DescriptorAddress) {
    const uint8_t DescriptorType = (wValue >> 8);
    const uint8_t DescriptorNumber = (wValue & 0xFF);

    const void* Address = NULL;
    uint16_t Size = NO_DESCRIPTOR;

    switch (DescriptorType) {
        case DTYPE_Device:
            Address = &DeviceDescriptor;
            Size = sizeof(USB_Descriptor_Device_t);
            break;
        case DTYPE_Configuration:
            Address = &ConfigurationDescriptor;
            Size = sizeof(USB_Descriptor_Configuration_t);
            break;
        case DTYPE_String:
            switch (DescriptorNumber) {
                case STRING_ID_Language:
                    Address = &LanguageString;
                    Size = LanguageString.Header.Size;
                    break;
                case STRING_ID_Manufacturer:
                    Address = &ManufacturerString;
                    Size = ManufacturerString.Header.Size;
                    break;
                case STRING_ID_Product:
                    if (realFreq == 16) {
                        Address = &ProductString16;
                        Size = ProductString16.Header.Size;
                    } else {
                        Address = &ProductString8;
                        Size = ProductString8.Header.Size;
                    }
                    break;
            }

            break;
    }

    *DescriptorAddress = Address;
    return Size;
}
