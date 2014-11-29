/*
 * utils.h
 *
 *  Created on: 30.11.2014
 *      Author: mueller
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <cstdarg>
using namespace std;

#ifdef __GNUC__
#define PRINTFATTR(i) __attribute__((format(printf, i, i+1)))
#define NORETURNATTR __attribute__ ((__noreturn__))
#else
#define PRINTFATTR(i)
#define NORETURNATTR
#endif

string vstringf(const char* format, va_list va);
string stringf(const char* format, ...) PRINTFATTR(1);

string relpath(const string& context, const string& rel);

#endif // UTILS_H_
