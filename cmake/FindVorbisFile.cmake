# Locate VorbisFile
# This module defines
# VorbisFile_LIBRARIES
# VorbisFile_FOUND, if false, do not try to link to VorbisFile
# VorbisFile_INCLUDE_DIRS, where to find the headers
#
# $VORBISDIR is an environment variable that would
# correspond to the ./configure --prefix=$VORBISDIR
# used in building Vorbis.
#
# Created by Sukender (Benoit Neil). Based on FindOpenAL.cmake module.
# TODO Add hints for linux and Mac

FIND_PATH(VorbisFile_INCLUDE_DIRS
	vorbis/vorbisfile.h
	HINTS
	$ENV{VORBISDIR}
    $ENV{CSP_DEVPACK}
	$ENV{VORBIS_PATH}
	PATH_SUFFIXES include
	PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

FIND_LIBRARY(VorbisFile_LIBRARIES 
	NAMES vorbisfile
	HINTS
	$ENV{VORBISDIR}
	$ENV{VORBIS_PATH}
    $ENV{CSP_DEVPACK}
	PATH_SUFFIXES win32/VorbisFile_Dynamic_Release lib
	PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw
	/opt/local
	/opt/csw
	/opt
)

FIND_LIBRARY(VorbisFile_LIBRARIES_DEBUG 
	NAMES VorbisFile_d
	HINTS
	$ENV{VORBISDIR}
    $ENV{CSP_DEVPACK}
	$ENV{VORBIS_PATH}
	PATH_SUFFIXES win32/VorbisFile_Dynamic_Debug lib
	PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw
	/opt/local
	/opt/csw
	/opt
)

SET(VorbisFile_FOUND "NO")
IF(VorbisFile_LIBRARIES AND VorbisFile_INCLUDE_DIRS)
	SET(VorbisFile_FOUND "YES")
	MARK_AS_ADVANCED(VorbisFile_LIBRARIES VorbisFile_LIBRARIES_DEBUG VorbisFile_INCLUDE_DIRS)
ELSEIF(VorbisFile_FIND_REQUIRED)
	MESSAGE(FATAL_ERROR "Required library VorbisFile not found! Install the library (including dev packages) and try again. If the library is already installed, set the missing variables manually in cmake.")
ENDIF()

