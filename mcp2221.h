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


#ifndef MCP2221_H
#define MCP2221_H 

// Includes
#include <QString>
#include <QStringList>
#include <QVector>
#include <libusb-1.0/libusb.h>

class MCP2221
{
private:
    libusb_context *context_;
    libusb_device_handle *handle_;
    bool disconnected_, kernelWasAttached_;

    QString getDescGeneric(quint8 subcomid, int &errcnt, QString &errstr);
    void interruptTransfer(quint8 endpointAddr, unsigned char *data, int length, int *transferred, int &errcnt, QString &errstr);
    quint8 writeDescGeneric(const QString &descriptor, quint8 subcomid, int &errcnt, QString &errstr);

public:
    // Class definitions
    static const quint16 VID = 0x04d8;      // Default USB vendor ID
    static const quint16 PID = 0x00dd;      // Default USB product ID
    static const int SUCCESS = 0;           // Returned by open() if successful
    static const int ERROR_INIT = 1;        // Returned by open() in case of a libusb initialization failure
    static const int ERROR_NOT_FOUND = 2;   // Returned by open() if the device was not found
    static const int ERROR_BUSY = 3;        // Returned by open() if the device is already in use
    static const size_t COMMAND_SIZE = 64;  // HID command size
    static const size_t PREAMBLE_SIZE = 2;  // HID command preamble size

    // Descriptor specific definitions
    static const size_t DESC_MAXLEN = 30;  // Maximum length for any descriptor

    // HID command IDs
    static const quint8 READ_FLASH_DATA = 0xb0;   // Read flash memory data
    static const quint8 WRITE_FLASH_DATA = 0xb1;  // Write flash memory data

    // Flash data sub-command IDs
    static const quint8 CHIP_SETTINGS = 0x00;      // Chip settings
    static const quint8 MANUFACTURER_DESC = 0x02;  // USB manufacturer descriptor
    static const quint8 PRODUCT_DESC = 0x03;       // USB product descriptor
    static const quint8 SERIAL_DESC = 0x04;        // USB serial descriptor
    static const quint8 FACTORY_SERIAL = 0x05;     // Chip factory serial number

    // HID command responses
    static const quint8 COMPLETED = 0x00;      // Command completed successfully
    static const quint8 BUSY = 0x01;           // I2C engine is busy (command not completed)
    static const quint8 NOT_SUPPORTED = 0x02;  // Command not supported
    static const quint8 NOT_ALLOWED = 0x03;    // Command not allowed
    //TODO
    static const quint8 OTHER_ERROR = 0xff;    // Other error (check errcnt and errstr for details)

    struct ChipSettings {
        // TODO
        quint16 vid;    // Vendor ID
        quint16 pid;    // Product ID
        // TODO

        bool operator ==(const ChipSettings &other) const;
        bool operator !=(const ChipSettings &other) const;
    };

    explicit MCP2221();
    ~MCP2221();

    bool disconnected() const;
    bool isOpen() const;

    void close();
    ChipSettings getChipSettings(int &errcnt, QString &errstr);
    QString getFactorySerial(int &errcnt, QString &errstr);
    QString getManufacturerDesc(int &errcnt, QString &errstr);
    QString getProductDesc(int &errcnt, QString &errstr);
    QString getSerialDesc(int &errcnt, QString &errstr);
    QVector<quint8> hidTransfer(const QVector<quint8> &data, int &errcnt, QString &errstr);
    int open(quint16 vid, quint16 pid, const QString &serial = QString());
    quint8 writeManufacturerDesc(const QString &manufacturer, int &errcnt, QString &errstr);
    quint8 writeProductDesc(const QString &product, int &errcnt, QString &errstr);
    quint8 writeSerialDesc(const QString &product, int &errcnt, QString &errstr);

    static QStringList listDevices(quint16 vid, quint16 pid, int &errcnt, QString &errstr);
};

#endif  // MCP2221_H
