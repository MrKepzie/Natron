/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef NATRON_GLOBAL_GLOBALDEFINES_H
#define NATRON_GLOBAL_GLOBALDEFINES_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <utility>
#if defined(_WIN32)
#include <string>
#include <windows.h>
#include <fcntl.h>
#include <sys/stat.h>
#else
#include <cstdlib>
#endif
#include <fstream>

#include "Global/Macros.h"

// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
#include <ofxPixels.h>

#undef toupper
#undef tolower
#undef isalpha
#undef isalnum

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#endif
#include <QtCore/QForeachContainer>
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMetaType>
CLANG_DIAG_ON(deprecated)
#include "Global/Enums.h"

#if defined(__NATRON_WIN32__) &&  defined(__GLIBCXX__)
#include <ext/stdio_filebuf.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif

// boost and C++11 also have a foreach. this breaks it. DON'T UNCOMMENT THIS.
//#undef foreach
//#define foreach Q_FOREACH


typedef boost::uint32_t U32;
typedef boost::uint64_t U64;
typedef boost::uint8_t U8;
typedef boost::uint16_t U16;


CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare)
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
#include <ofxPixels.h>

NATRON_NAMESPACE_ENTER;

typedef int SequenceTime;

struct RenderScale : public OfxPointD {
    RenderScale() { x = y = 1.; }
    RenderScale(double scale) { x = y = scale; }
    RenderScale(double scaleX, double scaleY) { x = scaleX; y = scaleY; }
};

typedef OfxPointD Point;

typedef OfxRGBAColourF RGBAColourF;
typedef OfxRangeD RangeD;



///these are used between process to communicate via the pipes
#define kRenderingStartedLong "Rendering started"
#define kRenderingStartedShort "-b"

#define kFrameRenderedStringLong "Frame rendered: "
#define kFrameRenderedStringShort "-r"

#define kProgressChangedStringLong "Progress changed: "
#define kProgressChangedStringShort "-p"

#define kRenderingFinishedStringLong "Rendering finished"
#define kRenderingFinishedStringShort "-e"

#define kAbortRenderingStringLong "Abort rendering"
#define kAbortRenderingStringShort "-a"

#define kBgProcessServerCreatedShort "--bg_server_created"

//Increment this to wipe all disk cache structure and ensure that the user has a clean cache when starting the next version of Natron
#define NATRON_CACHE_VERSION 3
#define kNatronCacheVersionSettingsKey "NatronCacheVersionSettingsKey"


#define kNodeGraphObjectName "nodeGraph"
#define kCurveEditorObjectName "curveEditor"
#define kDopeSheetEditorObjectName "dopeSheetEditor"

#define kCurveEditorMoveMultipleKeysCommandCompressionID 2
#define kKnobUndoChangeCommandCompressionID 3
#define kNodeGraphMoveNodeCommandCompressionID 4
#define kRotoMoveControlPointsCompressionID 5
#define kRotoMoveTangentCompressionID 6
#define kRotoCuspSmoothCompressionID 7
#define kRotoMoveFeatherBarCompressionID 8
#define kRotoMakeBezierCompressionID 9
#define kRotoMakeEllipseCompressionID 10
#define kRotoMakeRectangleCompressionID 11
#define kRotoTransformCompressionID 12
#define kMultipleKnobsUndoChangeCommandCompressionID 13
#define kNodeGraphMoveNodeBackdropCommandCompressionID 14
#define kNodeGraphResizeNodeBackdropCommandCompressionID 15
#define kCurveEditorMoveTangentsCommandCompressionID 16
#define kCurveEditorTransformKeysCommandCompressionID 17
#define kDopeSheetEditorMoveKeysCommandCompressionID 18
#define kDopeSheetEditorLeftTrimCommandCompressionID 19
#define kDopeSheetEditorRightTrimCommandCompressionID 20
#define kDopeSheetEditorTransformKeysCommandCompressionID 21
#define kDopeSheetEditorSlipReaderCommandCompressionID 23

#define PY_VERSION_STRINGIZE_(major,minor) \
# major "." # minor

#define PY_VERSION_STRINGIZE(major,minor) \
PY_VERSION_STRINGIZE_(major,minor)

#if PY_MAJOR_VERSION == 2
#define IS_PYTHON_2
#endif

#define NATRON_PY_VERSION_STRING PY_VERSION_STRINGIZE(PY_MAJOR_VERSION,PY_MINOR_VERSION)


namespace Global {

/*Converts a std::string to wide string*/
inline std::wstring
s2ws(const std::string & s)
{


#ifdef __NATRON_WIN32__
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
#else
    std::wstring dest;

    size_t max = s.size() * 4;
    mbtowc (NULL, NULL, max);  /* reset mbtowc */

    const char* cstr = s.c_str();

    while (max > 0) {
        wchar_t w;
        size_t length = mbtowc(&w,cstr,max);
        if (length < 1) {
            break;
        }
        dest.push_back(w);
        cstr += length;
        max -= length;
    }
    return dest;
#endif
    
} // s2ws

#ifdef __NATRON_WIN32__


//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
inline 
std::string GetLastErrorAsString()
{
    //Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if(errorMessageID == 0)
        return std::string(); //No error message has been recorded

    LPSTR messageBuffer = 0;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
} // GetLastErrorAsString

#endif // __NATRON_WIN32__

#if defined(__NATRON_WIN32__) &&  defined(__GLIBCXX__)
    
    
// MingW
inline
std::istream* open_ifstream_impl(const std::string &filename)
{
    std::wstring wfilename = s2ws(filename);
    int fd;
    errno_t errcode = _wsopen_s(&fd, wfilename.c_str(), _O_RDONLY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (errcode != 0) {
        return 0;
    }
    __gnu_cxx::stdio_filebuf<char>* buffer = new __gnu_cxx::stdio_filebuf<char>(fd, std::ios_base::in, 1);

    
    if (!buffer) {
        return 0;
    }
    return new std::istream(buffer);
}

inline
std::ostream* open_ofstream_impl(const std::string &filename)
{
    std::wstring wfilename = s2ws(filename);
    int fd;
    errno_t errcode = _wsopen_s(&fd, wfilename.c_str(), _O_WRONLY | _O_TRUNC, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (errcode != 0) {
        return 0;
    }
    __gnu_cxx::stdio_filebuf<char>* buffer = new __gnu_cxx::stdio_filebuf<char>(c_file, std::ios_base::out, 1);
    if (!buffer) {
        return 0;
    }
    return new std::ostream(buffer);
}
#else
// Msvc or unix
inline
std::ifstream* open_ifstream_impl(const std::string &filename)
{
#if defined(__NATRON_WIN32__)
#ifndef _MSC_VER_
#error "Windows builds only support GCC or MSVC"
#endif
    std::wstring wfilename = s2ws(filename);
#endif
    std::ifstream *ret = new std::ifstream();
    if (!ret) {
        return 0;
    }
    try {
        ret->open(
#ifdef __NATRON_WIN32__
                  wfilename.c_str(),
#else
                  filename.c_str(),
#endif
                  std::ifstream::in);
    } catch (const std::exception & e) {
        delete ret;
        return 0;
    }
    
    if (!*ret) {
        delete ret;
        return 0;
    }

    return ret;
} // open_ifstream_impl

inline
std::ofstream* open_ofstream_impl(const std::string &filename)
{
#if defined(__NATRON_WIN32__)
    std::wstring wfilename = s2ws(filename);
#endif
    std::ofstream *ret = new std::ofstream();
    if (!ret) {
        return 0;
    }
    try {
        ret->open(
#ifdef __NATRON_WIN32__
                  wfilename.c_str(),
#else
                  filename.c_str(),
#endif
                  std::ofstream::out);
    } catch (const std::exception & e) {
        delete ret;
        return 0;
    }
    
    if (!*ret) {
        delete ret;
        return 0;
    }
    
    return ret;
}
#endif //  defined(__NATRON_WIN32__) &&  defined(__GLIBCXX__)

inline
boost::shared_ptr<std::istream> open_ifstream(const std::string& filename)
{
    std::istream* ret = open_ifstream_impl(filename);
    if (ret) {
        return boost::shared_ptr<std::istream>(ret);
    } else {
        return boost::shared_ptr<std::istream>();
    }
}
 
inline
boost::shared_ptr<std::ostream> open_ofstream(const std::string& filename)
{
    std::ostream* ret = open_ofstream_impl(filename);
    if (ret) {
        return boost::shared_ptr<std::ostream>(ret);
    } else {
        return boost::shared_ptr<std::ostream>();
    }
}
    
} // namespace Global

NATRON_NAMESPACE_EXIT;

Q_DECLARE_METATYPE(NATRON_NAMESPACE::SequenceTime)

#endif // ifndef NATRON_GLOBAL_GLOBALDEFINES_H
