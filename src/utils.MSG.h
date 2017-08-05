/*
 * utils.MSG.h
 *
 *  Created on: 04.08.2017
 *      Author: mueller
 */

static const constexpr struct
{
	#define F(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_utils, FATAL, major, minor }, text }
	#define E(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_utils, ERROR, major, minor }, text }
	#define W(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_utils, WARN, major, minor }, text }
	#define I(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_utils, INFO, major, minor }, text }
	// Entries must be ordered by major, minor.

	E(FILE_OPEN_FAILED                  , 10, 0, "Failed to open '%s' for %s: %s", const char*, const char*, const char*);
	E(FILE_WRITE_FAILED                 , 11, 1, "Failed to write to file: %s", const char*);
	E(FILE_WRITE_DISK_FULL              , 11,10, "Failed to write to file: Disk full");
	//E(FILE_READ_FAILED                  , 12, 1, "Failed to read from file: %s", const char*);
	E(FILE_READ_FAILED2                 , 12, 2, "Failed to read file content of '%s': %s", const char*, const char*);
	E(FILE_READ_LINE_FAILED             , 12,20, "Failed to read line from file: %s", const char*);
	//E(FILE_SEEK_SET_FAILED              , 13, 1, "Failed to seek to position %i of file: %s", int, const char*);
	E(FILE_SEEK_SET_FAILED2             , 13, 2, "Failed to seek to position %i of file '%s': %s", int, const char*, const char*);
	//E(FILE_SEEK_END_FAILED              , 13,11, "Failed to seek to position -%i of file: %s", int, const char*);
	E(FILE_SEEK_END_FAILED2             , 13,12, "Failed to seek to position -%i of file '%s': %s", int, const char*, const char*);
	//E(FILE_TELL_FAILED                  , 14, 1, "Failed to get file pointer: %s", const char*);
	E(FILE_TELL_END_FAILED2             , 14, 6, "Failed to get size of file '%s': %s", const char*, const char*);
	E(FILE_TOO_LARGE                    , 50, 0, "File '%s' with size %lu is too large (%zu bytes max).", const char*, long, size_t);

	#undef F
	#undef E
	#undef W
	#undef I

	MAKE_MSGTEMPLATE_CONTAINER
} utilsMSG;
