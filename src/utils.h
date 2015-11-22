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
#include <vector>
#include <string.h>

using namespace std;


#ifdef __GNUC__
#define PRINTFATTR(i) __attribute__((format(printf, i, i+1)))
#define NORETURNATTR __attribute__ ((__noreturn__))
#else
#define PRINTFATTR(i)
#define NORETURNATTR
#endif

/// Like \c vsprintf into std::string.
string vstringf(const char* format, va_list va);
/// Like \c sprintf into std::string.
string stringf(const char* format, ...) PRINTFATTR(1);

/// Convert relative path to absolute one.
/// @param context Use this path as current directory.
/// @param rel Use this path as relative one.
/// @return \c rel if \c rel is an absolute path a concatenation of \c context and \c rel otherwise.
string relpath(const string& context, const string& rel);


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

#endif // UTILS_H_
