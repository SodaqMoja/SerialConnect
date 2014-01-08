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

#ifndef SERIALCONNECT_H_
#define SERIALCONNECT_H_

#include <stdint.h>
#include <Stream.h>

class SerialConnect
{
public:
  SerialConnect();
  void init(Stream & stream, char eol='\n') { _myStream = &stream; _eol = eol; }

  void sendData(const char *line);
  bool receiveData(char *buffer, size_t size, uint16_t timeout=1000);
  bool receiveData(const char *prefix, char *buffer, size_t size, uint16_t timeout=1000);
  bool waitUntilAvailable(uint16_t timeout=1000);

  void flushInput();

  void setDiag(Stream &stream) { _diagStream = &stream; }

private:
  bool readLine(char *buffer, size_t size, uint16_t timeout=1000);

  // Small utility to see if we timed out
  static bool isTimedOut(uint32_t ts) { return (long)(millis() - ts) >= 0; }

  bool findCrc(char *txt, uint16_t &crc, char **cptr);
  uint16_t crc16_ccitt(uint8_t * buf, size_t len);
  uint16_t crc16_xmodem(uint8_t * buf, size_t len);

  // TODO There must be an easier way to do this.
  void diagPrint(const char *str) { if (_diagStream) _diagStream->print(str); }
  void diagPrintLn(const char *str) { if (_diagStream) _diagStream->println(str); }
  void diagPrint(const __FlashStringHelper *str) { if (_diagStream) _diagStream->print(str); }
  void diagPrintLn(const __FlashStringHelper *str) { if (_diagStream) _diagStream->println(str); }
  void diagPrint(int i, int base=DEC) { if (_diagStream) _diagStream->print(i, base); }
  void diagPrintLn(int i, int base=DEC) { if (_diagStream) _diagStream->println(i, base); }
  void diagPrint(unsigned int u, int base=DEC) { if (_diagStream) _diagStream->print(u, base); }
  void diagPrintLn(unsigned int u, int base=DEC) { if (_diagStream) _diagStream->println(u, base); }
  void diagPrint(char c) { if (_diagStream) _diagStream->print(c); }
  void diagPrintLn(char c) { if (_diagStream) _diagStream->println(c); }

private:
  Stream * _myStream;
  Stream * _diagStream;
  char _eol;
};


#endif /* SERIALCONNECT_H_ */
