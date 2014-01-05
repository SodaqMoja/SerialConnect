/*
 * Copyright (c) 2013 Kees Bakker.  All rights reserved.
 *
 * This file is part of SerialConnect.
 *
 * SerialConnect is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published bythe Free Software Foundation, either version 3 of
 * the License, or(at your option) any later version.
 *
 * SerialConnect is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SerialConnect.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <util/crc16.h>
#include <Arduino.h>
#include "SerialConnect.h"


SerialConnect::SerialConnect()
{
  _myStream = 0;
  _diagStream = 0;
  _eol = '\n';
}

/*
 * Send data via the serial connection
 *
 * This is assuming that the line only contains ASCII characters
 * and no end of line.
 * A comma and a CRC16 is appended and an end of line character.
 */
void SerialConnect::sendData(const char *line)
{
  diagPrint(F("sendLine: '")); diagPrint(line); diagPrintLn('\'');
  uint16_t crc = crc16_xmodem((uint8_t *)line, strlen(line));
  _myStream->print(line);
  _myStream->print(',');
  _myStream->print(crc);
  _myStream->print(_eol);
}

/*
 * Read a non-empty line from the input
 *
 * \param buffer pointer to store the result
 * \param size how many bytes can be stored in the result (including \0)
 * \param timeout
 */
bool SerialConnect::receiveData(char *buffer, size_t size, uint16_t timeout)
{
  bool retval = false;
  char *ptr;
  uint32_t ts_max = millis() + timeout;
  while (!isTimedOut(ts_max)) {
    if (readLine(buffer, size)) {
      uint16_t crc;
      if (findCrc(buffer, crc, &ptr)) {
        *ptr = '\0';      // Strip the CRC
        uint16_t crc1 = crc16_xmodem((uint8_t *)buffer, strlen(buffer));
        if (crc1 == crc) {
          retval = true;
          break;
        }
      }
    }
  }
  return retval;
}

/*
 * Read a non-empty line from the input
 *
 * \param prefix only accept a line that starts with this
 * \param buffer pointer to store the result
 * \param size how many bytes can be stored in the result (including \0)
 * \param timeout
 */
bool SerialConnect::receiveData(const char *prefix, char *buffer, size_t size, uint16_t timeout)
{
  bool retval = false;
  char *ptr;
  uint32_t ts_max = millis() + timeout;
  while (!isTimedOut(ts_max)) {
    if (readLine(buffer, size)) {
      uint16_t crc;
      if (findCrc(buffer, crc, &ptr)) {
        *ptr = '\0';      // Strip the CRC
        uint16_t crc1 = crc16_xmodem((uint8_t *)buffer, strlen(buffer));
        if (crc1 == crc) {
          // Checksum is OK
          // Does it have the expected prefix?
          size_t len = strlen(prefix);
          if (strncmp(buffer, prefix, len) == 0) {
            // Remove the prefix from the result
            strcpy(buffer, buffer + len);
            retval = true;
            break;
          }
        }
      }
    }
  }
  return retval;
}

/*
 * Wait until a character is available
 *
 * The character is not touched. It remains in the input
 * buffer for the next read().
 * This function can be used to quickly find out if input
 * is available.
 * Notice that while the system (the MCU) is in sleep mode that
 * it may not receive any character.
 */
bool SerialConnect::waitUntilAvailable(uint16_t timeout)
{
  uint32_t ts_max = millis() + timeout;
  while (!isTimedOut(ts_max)) {
    if (_myStream->available() > 0) {
      return true;
    }
  }
  return false;
}

/*
 * Read a non-empty line from the input
 *
 * \param buffer pointer to store the result
 * \param size how many bytes can be stored in the result (including \0)
 * \param timeout
 */
bool SerialConnect::readLine(char *buffer, size_t size, uint16_t timeout)
{
  size_t len;
  bool seenCR = false;
  uint32_t ts_waitLF = 0;
  int c;

  len = 0;
  uint32_t ts_max = millis() + timeout;
  while (!isTimedOut(ts_max)) {
    if (seenCR) {
      c = _myStream->peek();
      if (c != '\n' || isTimedOut(ts_waitLF)) {
        // Line ended with just <CR>. That's OK too.
        goto ok;
      }
    }
    c = _myStream->read();
    if (c < 0) {
      continue;
    }
    seenCR = c == '\r';
    if (c == '\r') {
      ts_waitLF = millis() + 50;       // Wait a short while for an optional LF
    } else if (c == '\n') {
      if (len > 0) {
        goto ok;
      }
      // An empty line. Continue to wait.
    } else {
      if (len < size - 1) {
        buffer[len++] = c;
      }
    }
  }
  diagPrintLn(F("readLine timed out"));
  return false;         // This indicates: timed out

ok:
  buffer[len++] = '\0';
  //diagPrint(F("readLine: '")); diagPrint(buffer); diagPrintLn('\'');
  return true;
}

/*
 * Find the CRC in the line of text
 *
 * First step is to find the last comma. And if it is
 * found return the character after it.
 * The help stripping the CRC later we also return the pointer
 * to the comma before the checksum.
 *
 * \param txt a text string
 * \param crc pointer for the result
 * \param cptr pointer to where CRC starts (the comma before the checksum)
 * \return true if a CRC was found
 */
bool SerialConnect::findCrc(char *txt, uint16_t &crc, char **cptr)
{
  char *ptr;
  ptr = txt + strlen(txt);
  while (ptr > txt) {
    *cptr = ptr - 1;
    if (**cptr == ',') {
      char *eptr;
      crc = strtoul(ptr, &eptr, 0);
      if (eptr != ptr) {
        return true;
      }
      break;
    }
    --ptr;
  }

  return false;
}

/*
 * \brief Compute CRC16 of a byte buffer (CCITT)
 */
uint16_t SerialConnect::crc16_ccitt(uint8_t * buf, size_t len)
{
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc = _crc_ccitt_update(crc, *buf++);
    }
    return crc;
}

/*
 * \brief Compute CRC16 of a byte buffer (XMODEM)
 */
uint16_t SerialConnect::crc16_xmodem(uint8_t * buf, size_t len)
{
    uint16_t crc = 0;
    while (len--) {
        crc = _crc_xmodem_update(crc, *buf++);
    }
    return crc;
}
