// GDB Server Utilties: definition
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef EMBDEBUG_UTILS_H
#define EMBDEBUG_UTILS_H

#include <cstdint>
#include <string>
#include <vector>

namespace EmbDebug {

namespace Utils {

//! \brief Is this a valid hex string
//!
//! Checks that the first \p len characters are hex digits
//!
//! \param [in] buf Buffer containing >= \p len characters
//! \param [in] len Number of characters (>0)
//! \return True if the first \p len characters of \p buf are valid
//!         hex digits. False otherwise.
bool isHexStr(const char *buf, const std::size_t len);

//! \brief Determine the integer value of a hex digit
//!
//! \param[in] ch A character representing a hexadecimal digit
//! \return The numeric value of the hex character, or -1 if the character
//!         is invalid.
uint8_t char2Hex(int c);

//! \brief Utility mapping a hexadecimal value to a character value
//!
//! \param[in] d A hexadecimal value (0-15). Any non-hex value results in
//!              '\0' being returned.
//! \return The lowercase character representing a given hexadecimal value,
//!         or '\0' if an invalid value was provided.
char hex2Char(uint8_t d);

//! \brief Convert a register value to a hex digit string
//!
//! The supplied value is converted to a (\p numBytes * 2) digit hex string. The
//! string is null terminated for convenience.
//!
//! Bytes will be packed into the string either in little or big endian
//! order, depending on the value of /p isLittleEndianP .
//!
//! \param[in]  val             The value to convert
//! \param[out] buf             The buffer for the text string. This must
//!                             be large enough for the resultant string plus
//!                             a null terminator.
//! \param[in]  numBytes        The number of significant bytes in val
//! \param[in]  isLittleEndianP True if the bytes should be written out in
//!                             little endian order.
void regVal2Hex(uint64_t val, char *buf, std::size_t numBytes,
                bool isLittleEndianP);

//! \brief Convert a hex digit string to a register value
//!
//! The supplied (\p numBytes * 2) digit hex string
//!
//! Bytes will be extracted from the string either in little or big
//! endian order, depending on the value of /p isLittleEndianP
//!
//! \param[in] buf             The buffer with the hex string. This must
//!                            contain (\p numBytes * 2) valid hexadecimal
//!                            digits.
//! \param[in] numBytes        The number of significant bytes in \p buf
//! \param[in] isLittleEndianP True if this is a little endian architecture.
//! \return The hexadecimal string converted to a register value.
uint64_t hex2RegVal(const char *buf, std::size_t numBytes,
                    bool isLittleEndianP);

//! \brief Convert any non-negative value to a hex digit string
//!
//! The supplied value is converted to a hex string. The string is null
//! terminated for convenience. The endianness is always big-endian (how do
//! you do little-endian with an odd number of digits, since bytes are always
//! big-endian).
//!
//! We null terminate the string.
//!
//! \param[in]  val  the value to convert
//! \param[out] buf  the buffer for the text string (assumed to be large
//!                  enough)
//! \return The length of the hex string
std::size_t val2Hex(uint64_t val, char *buf);

//! \brief Convert a hex digit string to a general non-negative value
//!
//! The supplied hex-string is converted to a value. The string is null
//! terminated for convenience. The endianness is always big-endian (how do
//! you do little-endian with an odd number of digits, since bytes are always
//! big-endian).
//!
//! \param[in] buf  The buffer with the hex string
//! \param[in] len  The number of chars
//! \return  The value converted
uint64_t hex2Val(const char *buf, std::size_t len);

//! \brief Convert an ASCII character string to pairs of hex digits
//!
//! Both source and destination are null terminated.
//!
//! @param[out] dest  Buffer to store the hex digit pairs (null terminated)
//! @param[in]  src   The ASCII string (null terminated)
void ascii2Hex(char *dest, const char *src);

//! \brief Convert pairs of hex digits to an ASCII character string
//!
//! Both source and destination are null terminated.
//!
//! \param[out] dest  The ASCII string (null terminated)
//! \param[in]  src   Buffer holding the hex digit pairs (null terminated)
void hex2Ascii(char *dest, const char *src);

//! \brief "Unescape" RSP binary data
//!
//! '#', '$' and '}' are escaped by preceding them by '}' and oring with 0x20.
//!
//! This function reverses that, modifying the data in place.
//!
//! \param[in] buf  The array of bytes to convert
//! \para[in]  len   The number of bytes to be converted
//! \return  The number of bytes AFTER conversion
std::size_t rspUnescape(char *buf, std::size_t len);

//! \brief Split a string into delimited tokens
//!
//! \param[in]  s      The string of tokes
//! \param[in]  delim  The delimiter characters
//! \param[out] elems  Vector the individual tokens
//! \return  The vector of tokens
std::vector<std::string> &split(const std::string &s, const std::string &delim,
                                std::vector<std::string> &elems);

//! \brief Report a fatal error to stderr and abort
//!
//! \param[in] error A plain-text error message to be reported
void fatalError(std::string);

} // namespace Utils

} // namespace EmbDebug

#endif
