/*
  HalfDuplexSerial.cpp - Stream Wrapper for Raplh Doncaster's

  Wrapper by J.Sleeman (sparks@gogo.co.nz), Derived from TinySoftwareSerial

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Modified by MCUdude for the MicroCore
  https://github.com/MCUdude/MicroCore
*/

#include "Arduino.h"
#include "HalfDuplexSerial.h"

HalfDuplexSerial Serial;


// Public Methods //////////////////////////////////////////////////////////////

int HalfDuplexSerial::peek(void)
{
  // We have no buffer, no peeking
  return -1;
}

int HalfDuplexSerial::read(void)
{
  #ifdef HALF_DUPLEX_SERIAL_DISABLE_READ
  return -1;
  #else
  return RxByteNBNegOneReturn();
  #endif
}

// This is the same as Serial.read() in that it is a
// non blocking read, returning -1 if no data was read
int HalfDuplexSerial::read_byte(void)
{
  return RxByteNBNegOneReturn();
}

char HalfDuplexSerial::read_char(void)
{
  return RxByteNBZeroReturn();
}

char HalfDuplexSerial::read_char_blocking(void)
{
  cli();
  return RxByte();
}

void HalfDuplexSerial::read_str(char buf[], uint8_t length)
{
  uint16_t t = 0xFFFF; // every time we attempt to read and fail, we decrement,
                       // when we hit zero time is up.  FIXME to use a more
                       // definite timing!
  uint8_t i = 0;
  buf[length - 1] = 0; // enforce null termination

  // Caution! There's no buffering in our serial routine, no interrupt driven
  // collection, so we need to have this loop pretty tight to avoid missed-bits
  // and corrupted bytes.
  //
  // The usefulness of this routine is questionable at best.
  //
  // Interrupts are disabled during the entire read
  uint8_t oldSREG = SREG;
  cli();
  do
  {
    while( (!(buf[i] = RxByteNBZeroReturn())) && --t);
  } while((++i < (length-1)) && t);
  SREG = oldSREG; // Put back interrupts again

  // i ends up as the index of the next character to read
  // this is at most length-1 so we can simply set i to be
  // null and we have our null-terminated string
  buf[i] = 0;
}

size_t HalfDuplexSerial::write(uint8_t ch)
{
  TxByte(ch);
  return 1;
}

size_t HalfDuplexSerial::write(const uint8_t *buffer, size_t size)
{
  for(size_t n = 0; n < size; n++)
    write(buffer[n]);
  return size;
}

size_t HalfDuplexSerial::print(const __FlashStringHelper *ifsh)
{
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  size_t n = 0;

  do
  {
    uint8_t c = pgm_read_byte(p++);
    if (c == 0) break;
    write(c);
  } while(++n);

  return n;
}

size_t HalfDuplexSerial::print(const String &s)
{
  size_t n = 0;
  for (uint16_t i = 0; i < s.length(); i++)
    n += write(s[i]);
  return n;
}

size_t HalfDuplexSerial::print(const char str[])
{
  return write(str);
}

size_t HalfDuplexSerial::print(char c)
{
  return write(c);
}

#if PRINT_MAX_INT_TYPE != PRINT_INT_TYPE_BYTE
size_t HalfDuplexSerial::print(unsigned char b, uint8_t base)
{
  return print((UNSIGNED_PRINT_INT_TYPE) b, base);
}
#endif

#if PRINT_MAX_INT_TYPE != PRINT_INT_TYPE_INT
size_t HalfDuplexSerial::print(int n, uint8_t base)
{
  return print((PRINT_INT_TYPE) n, base);
}

size_t HalfDuplexSerial::print(unsigned int n, uint8_t base)
{
  return print((UNSIGNED_PRINT_INT_TYPE) n, base);
}
#endif

#if PRINT_MAX_INT_TYPE != PRINT_INT_TYPE_LONG
size_t HalfDuplexSerial::print(long n, uint8_t base)
{
  return print((PRINT_INT_TYPE) n, base);
}

size_t HalfDuplexSerial::print(unsigned long n, uint8_t base)
{
  return print((UNSIGNED_PRINT_INT_TYPE) n, base);
}
#endif

size_t HalfDuplexSerial::print(PRINT_INT_TYPE n, uint8_t base)
{
  if (base == 10 && n < 0)
  {
    write('-');
    return printNumber(-n,base)+1;
  }
  else
    return printNumber(n, base);
}

size_t HalfDuplexSerial::print(UNSIGNED_PRINT_INT_TYPE n, uint8_t base)
{
  return printNumber(n,base);
}

size_t HalfDuplexSerial::print(double n, uint8_t digits)
{
  return printFloat(n, digits);
}

size_t HalfDuplexSerial::println(const __FlashStringHelper *ifsh)
{
  size_t n = print(ifsh);
  n += println();
  return n;
}

size_t HalfDuplexSerial::println(void)
{
  print('\r');
  print('\n');
  return 2;
}

size_t HalfDuplexSerial::println(const String &s)
{
  size_t n = print(s);
  n += println();
  return n;
}

size_t HalfDuplexSerial::println(const char c[])
{
  size_t n = print(c);
  n += println();
  return n;
}

size_t HalfDuplexSerial::println(char c)
{
  size_t n = print(c);
  n += println();
  return n;
}
#if PRINT_MAX_INT_TYPE != PRINT_INT_TYPE_BYTE
size_t HalfDuplexSerial::println(unsigned char b, uint8_t base)
{
  size_t n = print(b, base);
  n += println();
  return n;
}
#endif
#if PRINT_MAX_INT_TYPE != PRINT_INT_TYPE_INT
size_t HalfDuplexSerial::println(int num, uint8_t base)
{
  size_t n = print(num, base);
  n += println();
  return n;
}

size_t HalfDuplexSerial::println(unsigned int num, uint8_t base)
{
  size_t n = print(num, base);
  n += println();
  return n;
}
#endif
#if PRINT_MAX_INT_TYPE != PRINT_INT_TYPE_LONG
size_t HalfDuplexSerial::println(long num, uint8_t base)
{
  size_t n = print(num, base);
  n += println();
  return n;
}

size_t HalfDuplexSerial::println(unsigned long num, uint8_t base)
{
  size_t n = print(num, base);
  n += println();
  return n;
}
#endif
size_t HalfDuplexSerial::println(PRINT_INT_TYPE num, uint8_t base)
{
  size_t n = print(num, base);
  n += println();
  return n;
}

size_t HalfDuplexSerial::println(UNSIGNED_PRINT_INT_TYPE num, uint8_t base)
{
  size_t n = print(num, base);
  n += println();
  return n;
}

size_t HalfDuplexSerial::println(double num, int digits)
{
  size_t n = print(num, digits);
  n += println();
  return n;
}

// Private Methods /////////////////////////////////////////////////////////////


#ifndef PRINT_USE_BASE_ARBITRARY

// This is a more memory (RAM and FLASH) efficient printNumber implementation
// for very low ram machines, eg tiny13, the original was way too big
// for several reasons...
//
// 1. It uses unsigned long
// 2. It uses division (an expensive operation on attiny)
// 3. It uses a character buffer of effectively 33 bytes
//    in order to build the number before writing it out
//
// When you only have 64 bytes of ram in the first place, that doesn't end well.
//
// This produces smaller flash and sram, even though it looks bigger with all these lookup tables
// the reduction in the number of assembly instructions more than makes up for it.
//
// Credit goes to EEVBlog User "Kalvin"
//  http://www.eevblog.com/forum/microcontrollers/memory-efficient-int-to-chars-without-division-(bitshift-ok)-for-binary-bases/msg805172/#msg805172
//
// The only downside is that it can't do arbitrary bases, just 2, 8, 16 and 10
//
// In your pins_arduino.h you will want to set
//
//  #define PRINT_USE_BASE_BIN
//  #define PRINT_USE_BASE_HEX
//  #define PRINT_USE_BASE_OCT
//  #define PRINT_USE_BASE_DEC
//
// if you define PRINT_USE_BASE_ARBITRARY then this function will not be used
// it will suck your ram, but you can use arbitrary bases.


size_t HalfDuplexSerial::printNumber(UNSIGNED_PRINT_INT_TYPE n, uint8_t base)
{
  static const char digits[] PROGMEM = "0123456789ABCDEF";

  #if defined(PRINT_USE_BASE_BIN)
  static const UNSIGNED_PRINT_INT_TYPE base2[]  PROGMEM =
  {
    #if PRINT_MAX_INT_TYPE == PRINT_INT_TYPE_LONG
        0x80000000, 0x40000000, 0x20000000, 0x10000000,  0x800000, 0x400000, 0x200000, 0x100000, 0x80000, 0x40000, 0x20000, 0x10000,
    #endif
    #if (PRINT_MAX_INT_TYPE == PRINT_INT_TYPE_LONG) || (PRINT_MAX_INT_TYPE == PRINT_INT_TYPE_INT)
      0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100,
    #endif
      0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1, 0
  };
  #endif
  #if defined(PRINT_USE_BASE_OCT)
  static const UNSIGNED_PRINT_INT_TYPE base8[]  PROGMEM =
  {
    #if PRINT_MAX_INT_TYPE == PRINT_INT_TYPE_LONG
      010000000000, 01000000000, 0100000000, 010000000, 01000000,
    #endif
    #if (PRINT_MAX_INT_TYPE == PRINT_INT_TYPE_LONG) || (PRINT_MAX_INT_TYPE == PRINT_INT_TYPE_INT)
      0100000, 010000,  01000,
    #endif
      0100, 010, 01,
      0
  };
  #endif
  #if defined(PRINT_USE_BASE_DEC)
  static const UNSIGNED_PRINT_INT_TYPE base10[] PROGMEM =
  {
    #if PRINT_MAX_INT_TYPE == PRINT_INT_TYPE_LONG
      1000000000,100000000,10000000,1000000,100000,
    #endif
    #if (PRINT_MAX_INT_TYPE == PRINT_INT_TYPE_LONG) || (PRINT_MAX_INT_TYPE == PRINT_INT_TYPE_INT)
      10000,  1000,
    #endif
      100,    10,  1,
      0
  };
  #endif
  #if defined(PRINT_USE_BASE_HEX)
  static const UNSIGNED_PRINT_INT_TYPE base16[] PROGMEM =
  {
    #if PRINT_MAX_INT_TYPE == PRINT_INT_TYPE_LONG
      0x10000000, 0x1000000,
        0x100000,  0x010000,
    #endif
    #if (PRINT_MAX_INT_TYPE == PRINT_INT_TYPE_LONG) || (PRINT_MAX_INT_TYPE == PRINT_INT_TYPE_INT)
          0x1000,    0x0100,
    #endif
            0x10,      0x01,
                          0
  };
  #endif

  UNSIGNED_PRINT_INT_TYPE const * bt;
  uint8_t leadingzero = 0;
  switch(base)
  {
    default:
      #if defined(PRINT_USE_BASE_HEX)
        write('x');
        base = 16;
      #elif defined(PRINT_USE_BASE_DEC)
        write('d');
        base = 10;
      #elif defined(PRINT_USE_BASE_OCT)
        write('o');
        base = 8;
      #elif defined(PRINT_USE_BASE_BIN)
        write('b');
        base = 2;
      #endif

    #ifdef PRINT_USE_BASE_HEX
      // Fall through
      case 16: bt = base16; break;
    #endif
    #ifdef PRINT_USE_BASE_DEC
      // Fall through
      case 10: bt = base10; break;
    #endif
    #ifdef PRINT_USE_BASE_OCT
      // Fall through
      case 8:  bt = base8;  break;
    #endif
    #ifdef PRINT_USE_BASE_BIN
      // Fall through
      case 2:  bt = base2;  break;
    #endif
  }

  // Reuse base for counting the digits since we don't need it any more.
  base = 0;
  do
  {
    UNSIGNED_PRINT_INT_TYPE b = PGM_READ_MAX_INT_TYPE(&*bt++);
    uint8_t digit = 0;
    while (n >= b)
    {
      digit++;
      n = n - b;
    }
    leadingzero = leadingzero ? leadingzero : digit;
    if (b == 1 || leadingzero)
    {
      ++base;
      write(pgm_read_byte(&digits[digit]));
    }
  }
  while (PGM_READ_MAX_INT_TYPE(&*bt));
  return base;
}

#else

// This is the original printNumber from Arduino with just the small change to
// allow you to set the integer type in use (see Print.h for how to set
// PRINT_MAX_INT_TYPE for your requirements in pins_arduino.h).
//
// This function allows arbitrary bases, but is very VERY heavy especially
// if you are not using division anywhere else in your program.
//
// VERY, VERY HEAVY

size_t HalfDuplexSerial::printNumber(UNSIGNED_PRINT_INT_TYPE n, uint8_t base)
{

  // This is super wasteful because it assumes you are using binary in the
  // worst case and makes a buffer big enough to show it in binary
  // which is 1 byte per bit
  char buf[8 * sizeof(n) + 1]; // Assumes 8-bit chars plus zero byte.

  char *str = &buf[sizeof(buf) - 1];

  *str = '\0';

  // prevent crash if called with base == 1
  if (base < 2) base = 10;

  do
  {
    UNSIGNED_PRINT_INT_TYPE m = n;
    n /= base;
    char c = m - base * n;
    *--str = c < 10 ? c + '0' : c + 'A' - 10;
  } while(n);

  return write(str);
}
#endif

size_t HalfDuplexSerial::printFloat(double number, uint8_t digits)
{
  size_t n = 0;

  // Handle negative numbers
  if (number < 0.0)
  {
     n += print('-');
     number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for (uint8_t i=0; i<digits; ++i)
  {
    rounding /= 10.0;
  }

  number += rounding;

  // Extract the integer part of the number and print it
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  n += print(int_part);

  // Print the decimal point, but only if there are digits beyond
  if (digits > 0)
  {
    n += print('.');
  }

  // Extract digits from the remainder one at a time
  while (digits-- > 0)
  {
    remainder *= 10.0;
    int toPrint = int(remainder);
    n += print(toPrint);
    remainder -= toPrint;
  }

  return n;
}
