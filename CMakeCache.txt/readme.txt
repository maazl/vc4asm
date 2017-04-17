Just another work around for hard coded paths in cmake:
Prevent creation of CMakeCache.txt to allow out of source build for different platforms.
The name clash of this directory prevents the creation of the cache file and enables cross platform builds.
And this file forces git to create the directory.
