// Compatibility head for gdbserver macros
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#ifndef EMBDEBUG_COMPAT_H
#define EMBDEBUG_COMPAT_H

// This header defines useful macros that can be used within the gdbserver for
// using compiler-specific attributes in a portable way. For those compilers
// that do not support a feature, these will resolve to no token.
//
// A list of these macros are defined below:
//
// EMBDEBUG_ATTR_UNUSED - Note that a paramater/variable is unused
// EMBDEBUG_VISIBLE_API - Exports a symbol for shared objects
// EMBDEBUG_PRETTY_FUNCTION - Function name to use for debug messages
// EMBDEBUG_WARN_DEPRECATED - Developer method for warning about use of function

// EMBDEBUG_ATTR_UNUSED
#if defined(__GNUC__)
#define EMBDEBUG_ATTR_UNUSED __attribute__((unused))
#elif defined(__clang__)
#if __has__attribute(unused)
#define EMBDEBUG_ATTR_UNUSED __attribute__((unused))
#endif
#else
#define EMBDEBUG_ATTR_UNUSED
#endif

// EMBDEBUG_VISIBLE_API
#ifdef _MSC_VER
#define EMBDEBUG_VISIBLE_API __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define EMBDEBUG_VISIBLE_API __attribute__((visibility("default")))
#else
#define EMBDEBUG_VISIBLE_API
#endif

// EMBDEBUG_PRETTY_FUNCTION
#ifdef _MSC_VER
#define EMBDEBUG_PRETTY_FUNCTION __FUNCSIG__
#else
#define EMBDEBUG_PRETTY_FUNCTION __PRETTY_FUNCTION__
#endif

// EMBDEBUG_WARN_DEPRECATED
#define EMBDEBUG_WARN_DEPRECATED()                                             \
  std::cerr << "WARNING: Use of " << EMBDEBUG_PRETTY_FUNCTION << std::endl

#endif
