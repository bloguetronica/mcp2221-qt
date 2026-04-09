/* MCP2221 class for Qt - Version 1.0.0
   Copyright (c) 2026 Samuel Lourenço

   This library is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or (at your
   option) any later version.

   This library is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
   License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this library.  If not, see <https://www.gnu.org/licenses/>.


   Please feel free to contact me via e-mail: samuel.fmlourenco@gmail.com */


// Includes
#include <QObject>
#include "mcp2221.h"
extern "C" {
#include "libusb-extra.h"
}

// Definitions
const quint8 EPIN = 0x83;             // Address of endpoint assuming the IN direction
const quint8 EPOUT = 0x03;            // Address of endpoint assuming the OUT direction
const int IFACE = 2;                  // Interface number
const unsigned int TR_TIMEOUT = 500;  // Transfer timeout in milliseconds

// Private generic function that is used to get any descriptor
QString MCP2221::getDescGeneric(quint8 subcomid, int &errcnt, QString &errstr)
{
    QVector<quint8> command{
        READ_FLASH_DATA, subcomid  // Header
    };
    QVector<quint8> response = hidTransfer(command, errcnt, errstr);
    size_t maxSize = 2 * DESC_MAXLEN;  // Maximum size of the descriptor in bytes (the zero padding at the end takes two more bytes)
    size_t size = response.at(2) - 2;  // Descriptor actual size, excluding the padding
    size = size > maxSize ? maxSize : size;  // This also fixes an erroneous result due to a possible unsigned integer rollover
    QString descriptor;
    for (size_t i = 0; i < size; i += 2) {
        descriptor += QChar(response.at(i + PREAMBLE_SIZE + 3) << 8 | response.at(i + PREAMBLE_SIZE + 2));  // UTF-16LE conversion as per the USB 2.0 specification
    }
    return descriptor;
}

// Private function that is used to perform interrupt transfers
void MCP2221::interruptTransfer(quint8 endpointAddr, unsigned char *data, int length, int *transferred, int &errcnt, QString &errstr)
{
    if (!isOpen()) {
        ++errcnt;
        errstr += QObject::tr("In interruptTransfer(): device is not open.\n");  // Program logic error
    } else {
        int result = libusb_interrupt_transfer(handle_, endpointAddr, data, length, transferred, TR_TIMEOUT);
        if (result != 0 || (transferred != nullptr && *transferred != length)) {  // The number of transferred bytes is also verified, as long as a valid (non-null) pointer is passed via "transferred"
            ++errcnt;
            if (endpointAddr < 0x80) {
                errstr += QObject::tr("Failed interrupt OUT transfer to endpoint %1 (address 0x%2).\n").arg(0x0f & endpointAddr).arg(endpointAddr, 2, 16, QChar('0'));
            } else {
                errstr += QObject::tr("Failed interrupt IN transfer from endpoint %1 (address 0x%2).\n").arg(0x0f & endpointAddr).arg(endpointAddr, 2, 16, QChar('0'));
            }
            if (result == LIBUSB_ERROR_NO_DEVICE || result == LIBUSB_ERROR_IO) {  // Note that libusb_interrupt_transfer() may return "LIBUSB_ERROR_IO" [-1] on device disconnect
                disconnected_ = true;  // This reports that the device has been disconnected
            }
        }
    }
}

// Private generic function that is used to write any descriptor
quint8 MCP2221::writeDescGeneric(const QString &descriptor, quint8 subcomid, int &errcnt, QString &errstr)
{
    int strLength = descriptor.size();  // Descriptor string length
    QVector<quint8> command(2 * strLength + PREAMBLE_SIZE + 2);
    command[0] = WRITE_FLASH_DATA;                        // Header
    command[1] = subcomid;
    command[2] = static_cast<quint8>(2 * strLength + 2);  // Descriptor length in bytes
    command[3] = 0x03;                                    // USB descriptor constant
    for (int i = 0; i < strLength; ++i) {
        command[2 * i + PREAMBLE_SIZE + 2] = static_cast<quint8>(descriptor[i].unicode());
        command[2 * i + PREAMBLE_SIZE + 3] = static_cast<quint8>(descriptor[i].unicode() >> 8);
    }
    QVector<quint8> response = hidTransfer(command, errcnt, errstr);
    return response.at(1);
}

// "Equal to" operator for ChipSettings
bool MCP2221::ChipSettings::operator ==(const MCP2221::ChipSettings &other) const
{
    return vid == other.vid && pid == other.pid;  // TODO
}

// "Not equal to" operator for ChipSettings
bool MCP2221::ChipSettings::operator !=(const MCP2221::ChipSettings &other) const
{
    return !(operator ==(other));
}

MCP2221::MCP2221() :
    context_(nullptr),
    handle_(nullptr),
    disconnected_(false),
    kernelWasAttached_(false)
{
}

MCP2221::~MCP2221()
{
    close();  // The destructor is used to close the device, and this is essential so the device can be freed when the parent object is destroyed
}

// Diagnostic function used to verify if the device has been disconnected
bool MCP2221::disconnected() const
{
    return disconnected_;  // Returns true if the device has been disconnected, or false otherwise
}

// Checks if the device is open
bool MCP2221::isOpen() const
{
    return handle_ != nullptr;  // Returns true if the device is open, or false otherwise
}

// Closes the device safely, if open
void MCP2221::close()
{
    if (isOpen()) {  // This condition avoids a segmentation fault if the calling algorithm tries, for some reason, to close the same device twice (e.g., if the device is already closed when the destructor is called)
        libusb_release_interface(handle_, IFACE);  // Release the interface
        if (kernelWasAttached_) {  // If a kernel driver was attached to the interface before
            libusb_attach_kernel_driver(handle_, IFACE);  // Reattach the kernel driver
        }
        libusb_close(handle_);  // Close the device
        libusb_exit(context_);  // Deinitialize libusb
        handle_ = nullptr;  // Required to mark the device as closed
    }
}

// Returns chip settings from the MCP2221 flash memory
MCP2221::ChipSettings MCP2221::getChipSettings(int &errcnt, QString &errstr)
{
    QVector<quint8> command{
        READ_FLASH_DATA, CHIP_SETTINGS  // Header
    };
    QVector<quint8> response = hidTransfer(command, errcnt, errstr);
    ChipSettings settings;
    // TODO
    settings.vid = static_cast<quint16>(response.at(9) << 8 | response.at(8));    // Vendor ID corresponds to bytes 8 and 9 (little-endian conversion)
    settings.pid = static_cast<quint32>(response.at(11) << 8 | response.at(10));  // Product ID corresponds to bytes 11 and 10 (little-endian conversion)
    // TODO
    return settings;
}

// Retrieves the factory serial number from the MCP2221 flash memory
QString MCP2221::getFactorySerial(int &errcnt, QString &errstr)
{
    QVector<quint8> command{
        READ_FLASH_DATA, FACTORY_SERIAL  // Header
    };
    QVector<quint8> response = hidTransfer(command, errcnt, errstr);
    size_t maxLength = COMMAND_SIZE - PREAMBLE_SIZE - 2;  // Maximum length for the serial number
    size_t length = response.at(2);  // Serial number actual length
    length = length > maxLength ? maxLength : length;  // This also fixes an erroneous result due to a possible unsigned integer rollover
    QString serial;
    for (size_t i = 0; i < length; ++i) {
        serial += QChar(response.at(i + PREAMBLE_SIZE + 2));
    }
    return serial;
}

// Retrieves the manufacturer descriptor from the MCP2221 flash memory
QString MCP2221::getManufacturerDesc(int &errcnt, QString &errstr)
{
    return getDescGeneric(MANUFACTURER_DESC, errcnt, errstr);
}

// Retrieves the product descriptor from the MCP2221 flash memory
QString MCP2221::getProductDesc(int &errcnt, QString &errstr)
{
    return getDescGeneric(PRODUCT_DESC, errcnt, errstr);
}

// Retrieves the serial descriptor from the MCP2221 flash memory
QString MCP2221::getSerialDesc(int &errcnt, QString &errstr)
{
    return getDescGeneric(SERIAL_DESC, errcnt, errstr);
}

// Sends a HID command based on the given vector, and returns the response
// The command vector can be shorter or longer than 64 bytes, but the resulting command will either be padded with zeros or truncated in order to fit
QVector<quint8> MCP2221::hidTransfer(const QVector<quint8> &data, int &errcnt, QString &errstr)
{
    size_t vecSize = static_cast<size_t>(data.size());
    size_t bytesToFill = vecSize > COMMAND_SIZE ? COMMAND_SIZE : vecSize;
    unsigned char commandBuffer[COMMAND_SIZE] = {0x00};  // It is important to initialize the array in this manner, so that unused indexes are filled with zeros!
    for (size_t i = 0; i < bytesToFill; ++i) {
        commandBuffer[i] = data[i];
    }
    int preverrcnt = errcnt;
#if LIBUSB_API_VERSION >= 0x01000105
    interruptTransfer(EPOUT, commandBuffer, static_cast<int>(COMMAND_SIZE), nullptr, errcnt, errstr);
#else
    int bytesWritten;
    interruptTransfer(EPOUT, commandBuffer, static_cast<int>(COMMAND_SIZE), &bytesWritten, errcnt, errstr);
#endif
    unsigned char responseBuffer[COMMAND_SIZE];
    int bytesRead = 0;  // Important!
    interruptTransfer(EPIN, responseBuffer, static_cast<int>(COMMAND_SIZE), &bytesRead, errcnt, errstr);
    QVector<quint8> retdata(static_cast<int>(COMMAND_SIZE));
    for (int i = 0; i < bytesRead; ++i) {
        retdata[i] = responseBuffer[i];
    }
    if (errcnt == preverrcnt && (bytesRead < static_cast<int>(COMMAND_SIZE) || responseBuffer[0] != commandBuffer[0])) {  // This additional verification only makes sense if the error count does not increase
        ++errcnt;
        errstr += QObject::tr("Received invalid response to HID command.\n");
    }
    return retdata;
}

// Opens the device having the given VID, PID and, optionally, the given serial number, and assigns its handle
int MCP2221::open(quint16 vid, quint16 pid, const QString &serial)
{
    int retval;
    if (isOpen()) {  // Just in case the calling algorithm tries to open a device that was already successfully open, or tries to open different devices concurrently, all while using (or referencing to) the same object
        retval = SUCCESS;
    } else if (libusb_init(&context_) != 0) {  // Initialize libusb. In case of failure
        retval = ERROR_INIT;
    } else {  // If libusb is initialized
        if (serial.isNull()) {  // Note that serial, by omission, is a null QString
            handle_ = libusb_open_device_with_vid_pid(context_, vid, pid);  // If no serial number is specified, this will open the first device found with matching VID and PID
        } else {
            handle_ = libusb_open_device_with_vid_pid_serial(context_, vid, pid, reinterpret_cast<unsigned char *>(serial.toLatin1().data()));
        }
        if (handle_ == nullptr) {  // If the previous operation fails to get a device handle
            libusb_exit(context_);  // Deinitialize libusb
            retval = ERROR_NOT_FOUND;
        } else {  // If the device is successfully opened and a handle obtained
            if (libusb_kernel_driver_active(handle_, IFACE) == 1) {  // If a kernel driver is active on the interface
                libusb_detach_kernel_driver(handle_, IFACE);  // Detach the kernel driver
                kernelWasAttached_ = true;  // Flag that the kernel driver was attached
            } else {
                kernelWasAttached_ = false;  // The kernel driver was not attached
            }
            if (libusb_claim_interface(handle_, IFACE) != 0) {  // Claim the interface. In case of failure
                if (kernelWasAttached_) {  // If a kernel driver was attached to the interface before
                    libusb_attach_kernel_driver(handle_, IFACE);  // Reattach the kernel driver
                }
                libusb_close(handle_);  // Close the device
                libusb_exit(context_);  // Deinitialize libusb
                handle_ = nullptr;  // Required to mark the device as closed
                retval = ERROR_BUSY;
            } else {
                disconnected_ = false;  // Note that this flag is never assumed to be true for a device that was never opened - See constructor for details!
                retval = SUCCESS;
            }
        }
    }
    return retval;
}

// Writes the manufacturer descriptor to the MCP2221 flash memory
quint8 MCP2221::writeManufacturerDesc(const QString &manufacturer, int &errcnt, QString &errstr)
{
    quint8 retval;
    if (static_cast<size_t>(manufacturer.size()) > DESC_MAXLEN) {
        ++errcnt;
        errstr += QObject::tr("In writeManufacturerDesc(): manufacturer descriptor string cannot be longer than 30 characters.\n");  // Program logic error
        retval = OTHER_ERROR;
    } else {
        retval = writeDescGeneric(manufacturer, MANUFACTURER_DESC, errcnt, errstr);
    }
    return retval;
}

// Writes the product descriptor to the MCP2221 flash memory
quint8 MCP2221::writeProductDesc(const QString &product, int &errcnt, QString &errstr)
{
    quint8 retval;
    if (static_cast<size_t>(product.size()) > DESC_MAXLEN) {
        ++errcnt;
        errstr += QObject::tr("In writeProductDesc(): product descriptor string cannot be longer than 30 characters.\n");  // Program logic error
        retval = OTHER_ERROR;
    } else {
        retval = writeDescGeneric(product, PRODUCT_DESC, errcnt, errstr);
    }
    return retval;
}

// Writes the serial descriptor to the MCP2221 flash memory
quint8 MCP2221::writeSerialDesc(const QString &serial, int &errcnt, QString &errstr)
{
    quint8 retval;
    if (static_cast<size_t>(serial.size()) > DESC_MAXLEN) {
        ++errcnt;
        errstr += QObject::tr("In writeSerialDesc(): serial descriptor string cannot be longer than 30 characters.\n");  // Program logic error
        retval = OTHER_ERROR;
    } else {
        retval = writeDescGeneric(serial, SERIAL_DESC, errcnt, errstr);
    }
    return retval;
}

// Helper function to list devices
QStringList MCP2221::listDevices(quint16 vid, quint16 pid, int &errcnt, QString &errstr)
{
    QStringList devices;
    libusb_context *context;
    if (libusb_init(&context) != 0) {  // Initialize libusb. In case of failure
        ++errcnt;
        errstr += QObject::tr("Could not initialize libusb.\n");
    } else {  // If libusb is initialized
        libusb_device **devs;
        ssize_t devlist = libusb_get_device_list(context, &devs);  // Get a device list
        if (devlist < 0) {  // If the previous operation fails to get a device list
            ++errcnt;
            errstr += QObject::tr("Failed to retrieve a list of devices.\n");
        } else {
            for (ssize_t i = 0; i < devlist; ++i) {  // Run through all listed devices
                libusb_device_descriptor desc;
                if (libusb_get_device_descriptor(devs[i], &desc) == 0 && desc.idVendor == vid && desc.idProduct == pid) {  // If the device descriptor is retrieved, and both VID and PID correspond to the respective given values
                    libusb_device_handle *handle;
                    if (libusb_open(devs[i], &handle) == 0) {  // Open the listed device. If successfull
                        unsigned char str_desc[256];
                        libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, str_desc, static_cast<int>(sizeof(str_desc)));  // Get the serial number string in ASCII format
                        devices += reinterpret_cast<char *>(str_desc);  // Append the serial number string to the list
                        libusb_close(handle);  // Close the device
                    }
                }
            }
            libusb_free_device_list(devs, 1);  // Free device list
        }
        libusb_exit(context);  // Deinitialize libusb
    }
    return devices;
}
