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
    static const quint16 VID = 0x04d8;        // Default USB vendor ID
    static const quint16 PID = 0x00dd;        // Default USB product ID
    static const int SUCCESS = 0;             // Returned by open() if successful
    static const int ERROR_INIT = 1;          // Returned by open() in case of a libusb initialization failure
    static const int ERROR_NOT_FOUND = 2;     // Returned by open() if the device was not found
    static const int ERROR_BUSY = 3;          // Returned by open() if the device is already in use
    static const size_t COMMAND_SIZE = 64;    // HID command size
    static const size_t PREAMBLE_SIZE = 2;    // HID command preamble size
    static const size_t PASSWORD_MAXLEN = 8;  // Maximum length for the password

    // Descriptor specific definitions
    static const size_t DESC_MAXLEN = 30;  // Maximum length for any descriptor

    // HID command IDs
    static const quint8 READ_FLASH_DATA = 0xb0;   // Read flash memory data
    static const quint8 WRITE_FLASH_DATA = 0xb1;  // Write flash memory data
    static const quint8 SEND_PASSWORD = 0xb2;     // Send password

    // Flash data sub-command IDs
    static const quint8 CHIP_SETTINGS = 0x00;      // Chip settings
    static const quint8 GP_SETTINGS = 0x01;        // GP pin settings
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

    // The following values are applicable to ChipSettings.ADCParameters/ChipSettings.DACParameters/getChipSettings()/writeChipSettings()
    static const quint8 VRMOFF = 0x00;    // Internal voltage reference (VRM) set to be off
    static const quint8 VRM1V024 = 0x01;  // Value corresponding to a reference voltage (Vrm) of 1.024 V
    static const quint8 VRM2V048 = 0x02;  // Value corresponding to a reference voltage (Vrm) of 2.048 V
    static const quint8 VRM4V096 = 0x03;  // Value corresponding to a reference voltage (Vrm) of 4.096 V
    static const bool REFOPTVDD = false;  // ADC or DAC reference set to Vdd
    static const bool REFOPTVRM = true;   // ADC or DAC reference set to Vrm

    // The following values are applicable to ChipSettings.Clock/getChipSettings()/writeChipSettings()
    static const quint8 CLKDIV24M = 0x01;   // Clock output set to 24 MHz
    static const quint8 CLKDIV12M = 0x02;   // Clock output set to 12 MHz
    static const quint8 CLKDIV6M = 0x03;    // Clock output set to 6 MHz
    static const quint8 CLKDIV3M = 0x04;    // Clock output set to 3 MHz
    static const quint8 CLKDIV1M5 = 0x05;   // Clock output set to 1.5 MHz
    static const quint8 CLKDIV750K = 0x06;  // Clock output set to 750 KHz
    static const quint8 CLKDIV375K = 0x07;  // Clock output set to 375 KHz
    static const quint8 CLKDC0 = 0x00;      // Value corresponding to a duty cycle of 0%
    static const quint8 CLKDC25 = 0x01;     // Value corresponding to a duty cycle of 25%
    static const quint8 CLKDC50 = 0x02;     // Value corresponding to a duty cycle of 50%
    static const quint8 CLKDC75 = 0x03;     // Value corresponding to a duty cycle of 75%

    // The following values are applicable to ChipSettings.USBParameters/getChipSettings()/writeChipSettings()
    static const bool PMBUS = false;  // Value corresponding to USB bus-powered mode
    static const bool PMSELF = true;  // Value corresponding to USB self-powered mode

    // The following values are applicable to GPSettings.GPPinParameters/getGPSettings()/writeGPSettings()
    static const quint8 FNGPIO = 0x00;  // Pin configured as GPIO
    static const quint8 FNDED = 0x01;   // Pin configured as a dedicated function pin
    static const quint8 FNALT0 = 0x02;  // Pin assigned to alternate function 0
    static const quint8 FNALT1 = 0x03;  // Pin assigned to alternate function 1
    static const quint8 FNALT2 = 0x04;  // Pin assigned to alternate function 2
    static const bool DIROUT = false;   // Pin direction set to output
    static const bool DIRIN = true;     // Pin direction set to input

    // Member of ChipSettings
    struct ADCParameters {
        quint8 vrm;   // ADC reference voltage (Vrm)
        bool refopt;  // ADC reference option

        bool operator ==(const ADCParameters &other) const;
        bool operator !=(const ADCParameters &other) const;
    };

    // Member of ChipSettings
    struct ClockParameters{
        quint8 div;  // Clock output divider
        quint8 dc;   // Clock output duty cycle

        bool operator ==(const ClockParameters &other) const;
        bool operator !=(const ClockParameters &other) const;
    };

    // Member of ChipSettings
    struct DACParameters{
        quint8 vrm;     // DAC reference voltage (Vrm)
        bool refopt;    // DAC reference option
        quint8 defval;  // DAC default value on power-up

        bool operator ==(const DACParameters &other) const;
        bool operator !=(const DACParameters &other) const;
    };

    // Member of ChipSettings
    struct InterruptParameters {
        bool detpos;  // Detect positive (rising) edge
        bool detneg;  // Detect negative (falling) edge

        bool operator ==(const InterruptParameters &other) const;
        bool operator !=(const InterruptParameters &other) const;
    };

    // Member of ChipSettings
    struct USBParameters {
        bool serialen;  // Serial number enable
        quint16 vid;    // Vendor ID
        quint16 pid;    // Product ID
        quint8 maxpow;  // Maximum consumption current (raw value in 2 mA units)
        bool powmode;   // Power mode (false for bus-powered, true for self-powered)
        bool rmwakeup;  // Remote wake-up capability

        bool operator ==(const USBParameters &other) const;
        bool operator !=(const USBParameters &other) const;
    };

    struct ChipSettings {
        ClockParameters clock;     // Clock output parameters
        InterruptParameters intr;  // Interrupt parameters
        ADCParameters adc;         // ADC parameters
        DACParameters dac;         // DAC parameters
        USBParameters usb;         // USB parameters

        bool operator ==(const ChipSettings &other) const;
        bool operator !=(const ChipSettings &other) const;
    };

    // Menber of GPSettings
    struct GPPinParameters {
        quint8 func;  // Pin function
        bool dir;     // Pin direction
        bool out;     // Pin default output value

        bool operator ==(const GPPinParameters &other) const;
        bool operator !=(const GPPinParameters &other) const;
    };

    struct GPSettings {
        GPPinParameters gp0;  // GP0 pin parameters
        GPPinParameters gp1;  // GP1 pin parameters
        GPPinParameters gp2;  // GP2 pin parameters
        GPPinParameters gp3;  // GP3 pin parameters

        bool operator ==(const GPSettings &other) const;
        bool operator !=(const GPSettings &other) const;
    };

    struct SecurityOptions {
        bool password{false};  // To prevent inadvertent locking of the device, these variables are initialized to "false"
        bool lock{false};

        bool operator ==(const SecurityOptions &other) const;
        bool operator !=(const SecurityOptions &other) const;
    };

    explicit MCP2221();
    ~MCP2221();

    bool disconnected() const;
    bool isOpen() const;

    void close();
    ChipSettings getChipSettings(int &errcnt, QString &errstr);
    GPSettings getGPSettings(int &errcnt, QString &errstr);
    QString getFactorySerial(int &errcnt, QString &errstr);
    QString getManufacturerDesc(int &errcnt, QString &errstr);
    QString getProductDesc(int &errcnt, QString &errstr);
    SecurityOptions getSecurityOptions(int &errcnt, QString &errstr);
    QString getSerialDesc(int &errcnt, QString &errstr);
    QVector<quint8> hidTransfer(const QVector<quint8> &data, int &errcnt, QString &errstr);
    int open(quint16 vid, quint16 pid, const QString &serial = QString());
    quint8 usePassword(const QString &password, int &errcnt, QString &errstr);
    quint8 writeChipSettings(const ChipSettings &settings, SecurityOptions &options, const QString &password, int &errcnt, QString &errstr);
    quint8 writeChipSettings(const ChipSettings &settings, int &errcnt, QString &errstr);
    quint8 writeGPSettings(const GPSettings &settings, int &errcnt, QString &errstr);
    quint8 writeManufacturerDesc(const QString &manufacturer, int &errcnt, QString &errstr);
    quint8 writeProductDesc(const QString &product, int &errcnt, QString &errstr);
    quint8 writeSerialDesc(const QString &product, int &errcnt, QString &errstr);

    static QStringList listDevices(quint16 vid, quint16 pid, int &errcnt, QString &errstr);
};

#endif  // MCP2221_H
