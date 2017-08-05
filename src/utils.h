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
#include <cstdio>
#include <vector>
#include <cstring>

using namespace std;


#ifdef __GNUC__
#define PRINTFATTR(i) __attribute__((format(printf, i, i+1)))
#else
#define PRINTFATTR(i)
#endif

struct unspecified_type;
typedef unspecified_type* bool_only;

/// Declare an enumeration type as flags, i.e. allow the bitwise operators
/// on this type.
#define CLASSFLAGSENUM(T,t) \
inline constexpr friend T operator|(T l, T r) \
{ return (T)((t)l|r); } \
inline constexpr friend T operator&(T l, T r) \
{ return (T)((t)l&r); } \
inline constexpr friend T operator^(T l, T r) \
{ return (T)((t)l^r); } \
inline friend T& operator|=(T& l, T r) \
{ return l = (T)((t)l|r); } \
inline friend T& operator&=(T& l, T r) \
{ return l = (T)((t)l&r); } \
inline friend T& operator^=(T& l, T r) \
{ return l = (T)((t)l^r); } \
inline constexpr friend T operator~(T a) \
{ return (T)~(t)a; }


/// Like \c vsprintf into std::string.
string vstringf(const char* format, va_list va);
/// Like \c sprintf into std::string.
string stringf(const char* format, ...) PRINTFATTR(1);

/// Convert relative path to absolute one.
/// @param context Use this path as current directory.
/// @param rel Use this path as relative one.
/// @return \c rel if \c rel is an absolute path a concatenation of \c context and \c rel otherwise.
string relpath(const string& context, const string& rel);

/// Strips any path component.
/// @param filepath File name with or without path.
/// @return Pointer into \a filepath behind any path component.
const char* strippath(const char* filepath);

/// Remove the file extension from file name.
/// @param filename File name.
/// @return File name without extension.
string stripextension(const char* filename);

/// Path to current executable, including trailing slash.
extern string exepath;

/// Examine path of the current executable.
/// @param argv0 argv[0] from main.
void setexepath(const char* argv0);

/// RAII version of FILE*
class FILEguard
{	FILE* File;
 public:
	FILEguard(FILE* file) : File(file) {}
	FILEguard(const FILEguard&) = delete;
	FILEguard(FILEguard&& r) : File(r.File) { r.File = NULL; }
	~FILEguard() { if (File) fclose(File); }
	FILEguard& operator=(const FILEguard&) = delete;
	operator FILE*() { return File; }
};

/// Checked file open. Like fopen but throws on error.
/// @param file Name of the file
/// @param mode Open mode
/// @return File pointer, never NULL.
/// @exception Message Failed to open: error message
FILE* checkedopen(const char* file, const char* mode);

/// Write to file and throw an exception on error.
/// @param fh Target file handle.
/// @param data Start of then buffer to write.
/// @param size Number of bytes to write.
/// @exception Message Failed to write: error message
void checkedwrite(FILE* fh, const void* data, size_t size);

/// Write to file and throw an exception on error.
/// @param fh Target file handle.
/// @param fmt Format string.
/// @param ... Arguments.
/// @exception Message Failed to write: error message
void checkedfprintf(FILE* fh, const char* fmt, ...) PRINTFATTR(2);

/// Read entire file content into memory.
/// @param file Name of the file.
/// @param maxsize Maximum size of the file. If this size is exceeded a \c Message exception is thrown.
/// @return Content of the file.
/// @exception Message Failed to read: error message
string readcomplete(const char* file, size_t maxsize = 1 << 16);

/// Read a line of an input stream into a string.
/// @param fh Source file handle.
/// @param maxlen Maximum line length. Any content beyond this limit is discarded.
/// @return Line from the file including the terminating new line character
/// if the line length did not exceed \a maxlen.
/// In case of EOF an empty string is returned.
/// @exception Message Failed to read: error message
string fgetstring(FILE* fh, size_t maxlen);


/// Replace any non alphanumeric characters.
/// @param value String to convert. The operation is done in place.
/// @param repl Replacement character, '_' by default.
void replacenonalphanum(string& value, char repl = '_');


/// Find the first occurrence of key in an ordered, constant array of C strings.
/// @tparam T Element type, must be convertible to const char*.
/// Since the string is \0 terminated, the Type T may consist of more than the string.
/// @tparam N Array size
/// @param arr Array. arr must in fact point to the first string.
/// @param key String to search.
/// @return Pointer to the first array element that matches key or NULL if none.
template <typename T, size_t N>
inline T* binary_search(T (&arr)[N], const char* key)
{	size_t l = 0;
	size_t r = N;
	int nohit = -1;
	while (l < r)
	{	size_t m = (l+r) >> 1;
		int cmp = strcmp(key, (const char*)(arr + m));
		if (cmp > 0)
			l = m + 1;
		else
		{	r = m;
			nohit &= cmp;
		}
	}
	return nohit ? NULL : arr + r;
}


/// Refinement of vector<T> that resizes the vector instead of undefined behavior when operator[] is used with an out of bounds index.
/// @tparam T vector's element type
/// @tparam def default value used to enlarge the vector.
template <typename T, T def>
class vector_safe : public vector<T>
{public:
	/// Mutable operator[] that implicitly resizes the vector to contain at least the element number n.
	/// @param n Element index.
	/// @return reference the the existing element #n or to a newly element number #n. The new elements are initialized from \ref def.
	typename vector<T>::reference operator[](typename vector<T>::size_type n)
	{	if (n >= vector<T>::size())
			vector<T>::resize(n+1, def);
		return vector<T>::operator[](n);
	}
};


#include "Message.h"
#include "utils.MSG.h"

#endif // UTILS_H_
