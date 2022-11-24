/**
 * RS485 Modbus
 * 
 * Demonstrates usage of https://www.banggood.com/DC-12V-24V-8-Isolated-IO-DIN35-C45-Rail-Box-UART-RS485-MOSFET-Module-Modbus-RTU-Control-Switch-Board-for-Relay-PLC-LED-PTZ-p-1800236.html?rmmds=myorder&cur_warehouse=CN
 * MODBUS has some standard commands:
 * - 0x03 Read holding registers
 * - 0x06 Write (single) holding register
 * - 0x10 Write (multiple) holding registers
 * 
 * To find out what values are stored in each register of a particular device, we need to refer to the datasheet
 * This which gives us the following:
 * - 0x0001 - 0x0008 Output port status (0x0000 = Closed, 0x0001 = Open)
 * - 0x0081 - 0x0088 Input port status (0x0000 = Off, 0x0001 = On)
 * - 0x00FD I/O mode (0x0000 unrelated, 0x0001 self-locking, 0x0002 interlocking, 0x0003 momentary)
 * - 0x00FE Baud rate (0x0000 1200, 0x0001 2400, 0x0002 4800, 0x0003 9600, 0x0004 19200) 
 * - 0x00FF Device address (0x0000 - 0x00FE, default 0x0001)
 * Note that all registers contain a 2byte value (i.e. a "word", or uint16)
 * 
 * Commands are sent as an 8byte value, with the format:
 * Target Device ID | Command | Register Address | Num bytes to read | CRC16
 *      (1)              (1)            (2)           (2)               (2)
 *      
 * Examples:
 * To read values
 * 01 03 00 02 00 01 D5 CA 
 * = Read 1 value from register 0002 of device 01 (Output Channel 2 state)
 * 
 * 01 03 00 81 00 08 14 24
 * = Read 8 values starting from register 0081 of device 01 (Input Channels 1-8 states)
 * 
 * To write values
 * 01 06 00 01 00 01 (CRC) Open Output Channel 1
 * 01 06 00 01 00 02 (CRC) Close Output Channel 1
 * 01 06 00 00 00 07 (CRC) Open All 
 * 01 06 00 00 00 08 (CRC) Close All
 */

// INCLUDES
#include <SoftwareSerial.h>
// https://github.com/4-20ma/ModbusMaster/
#include "src/ModbusMaster.h"

// GLOBALS
// Tx/Rx
SoftwareSerial RS485_serial (A1, A0);
// Instantiate ModbusMaster object
ModbusMaster node;
// Return value
uint8_t result;
// The value to send
uint32_t value = 1;

void setup(){
  // Start serial connection (debug only)
  Serial.begin(115200);
  Serial.println(__FILE__ __DATE__);
  // Initialise RS485 interface at 9600 baud rate
  RS485_serial.begin(9600);
  // Connect to Device ID 1
  node.begin(1, RS485_serial);

  // Read 1x 16-bit register from address 0xFE to RX buffer
  result = node.readHoldingRegisters(0xFE, 1);
  // Do something with data if read is successful
  if (result == node.ku8MBSuccess) {
    Serial.print(F("Baud Rate (REG 0xFE):"));
    Serial.println(node.getResponseBuffer(0));
  }
  // Important - need a delay before issuing next command
  delay(500);
  
  // Read 1x 16-bit register from address 0xFF to RX buffer
  result = node.readHoldingRegisters(0xFF, 1);
  // Do something with data if read is successful
  if (result == node.ku8MBSuccess) {
    Serial.print(F("Address (REG 0xFF):"));
    Serial.println(node.getResponseBuffer(0));
    Serial.println("");
  }
  // Delay before starting main program loop
  delay(1000);
}

void loop() {

  Serial.println("Opening channels individually");
  for(uint16_t i=1; i<=8; i++) {
    result = node.writeSingleRegister(i, 0x0100);
    delay(500);
  }  

  Serial.println("Closing channels individually");
  for(int i=1; i<=8; i++) {
    result = node.writeSingleRegister(i, 0x0200);
    delay(500);
  }

  Serial.println("Setting multiple channels");
  node.setTransmitBuffer(0, 0x0200);
  node.setTransmitBuffer(1, 0x0100);
  node.setTransmitBuffer(2, 0x0200);
  node.setTransmitBuffer(3, 0x0100);
  node.setTransmitBuffer(4, 0x0200);
  node.setTransmitBuffer(5, 0x0100);
  node.setTransmitBuffer(6, 0x0200);
  node.setTransmitBuffer(7, 0x0100);
  // Write TX buffer to (8) 16-bit registers starting at register 1
  result = node.writeMultipleRegisters(0x0001, 8);
  if (result == node.ku8MBSuccess) {
    Serial.println(F("Toggled all output states"));
  }
  delay(500);

  Serial.println("Toggling multiple channels");
  // Fill the transmit buffer
  for(int i=0;i<8;i++){
    // Set word 0 of TX buffer to the toggle command, 0x0300
    node.setTransmitBuffer(i, 0x0300);
  }
  for(int repeats = 0; repeats<3; repeats++){
    // Write TX buffer to (8) 16-bit registers starting at register 1
    result = node.writeMultipleRegisters(0x0001, 8);
    if (result == node.ku8MBSuccess) {
      Serial.println(F("Toggled all output states"));
    }
    delay(500);
  }

  Serial.println("Opening all");
  result = node.writeSingleRegister(0, 0x0700);
  delay(500);

  Serial.println("Closing all");
  result = node.writeSingleRegister(0, 0x0800);
  delay(500);


  Serial.println("Reading Inputs");
  // Read 8x 16-bit registers from address 0x81 to RX buffer
  result = node.readHoldingRegisters(0x81, 8);
  // Do something with data if read is successful
  if (result == node.ku8MBSuccess) {
    Serial.print(F("Input Channel States: "));
    for(uint8_t j=0;j<8;j++){
      Serial.print(node.getResponseBuffer(j));
    }
    Serial.println("");
  }
  delay(500);

  // Read 8x 16-bit registers from address 0x01 to RX buffer
  result = node.readHoldingRegisters(0x01, 8);
  // Do something with data if read is successful
  if (result == node.ku8MBSuccess) {
    Serial.print(F("Output Channel States: "));
    for(uint8_t j=0;j<8;j++){
      Serial.print(node.getResponseBuffer(j));
    }
    Serial.println("");
  }
  delay(500);
}
