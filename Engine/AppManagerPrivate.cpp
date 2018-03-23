/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "AppManagerPrivate.h"

#if defined(Q_OS_UNIX)
#include <sys/time.h>     // for getrlimit on linux
#include <sys/resource.h> // for getrlimit
#if defined(__APPLE__)
#include <sys/syslimits.h> // OPEN_MAX
#endif
#endif
#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryFile>
#include <QtCore/QCoreApplication>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

#include "Global/QtCompat.h" // for removeRecursively
#include "Global/GlobalDefines.h"
#include "Global/GLIncludes.h"
#include "Global/ProcInfo.h"
#include "Global/StrUtils.h"
#include "Global/FStreamsSupport.h"

#include "Engine/CLArgs.h"
#include "Engine/Cache.h"
#include "Engine/Format.h"
#include "Engine/MultiThread.h"
#include "Engine/Image.h"
#include "Engine/OfxHost.h"
#include "Engine/OSGLContext.h"
#include "Engine/Settings.h"
#include "Engine/ProcessHandler.h" // ProcessInputChannel
#include "Engine/StandardPaths.h"

#include "Serialization/SerializationIO.h"

// Don't forget to update glad.h and glad.c aswell when updating theses
#define NATRON_OPENGL_VERSION_REQUIRED_MAJOR 2
#define NATRON_OPENGL_VERSION_REQUIRED_MINOR 0


NATRON_NAMESPACE_ENTER


AppManagerPrivate::AppManagerPrivate()
    : globalTLS()
    , _appType(AppManager::eAppTypeBackground)
    , _appInstancesMutex()
    , _appInstances()
    , _availableID(0)
    , _topLevelInstanceID(0)
    , _settings()
    , _formats()
    , _plugins()
    , readerPlugins()
    , writerPlugins()
    , ofxHost( new OfxHost() )
    , pythonTLS()
    , multiThreadSuite(new MultiThread())
    , _knobFactory( new KnobFactory() )
    , generalPurposeCache()
    , tileCache()
    , _backgroundIPC()
    , _loaded(false)
    , binaryPath()
    , errorLogMutex()
    , errorLog()
    , hardwareThreadCount(0)
    , physicalThreadCount(0)
    , commandLineArgsUtf8()
    , nArgs(0)
    , mainModule(0)
    , mainThreadState(0)
#ifdef NATRON_USE_BREAKPAD
    , breakpadProcessExecutableFilePath()
    , breakpadProcessPID(0)
    , breakpadHandler()
    , breakpadAliveThread()
#endif
    , natronPythonGIL(QMutex::Recursive)
    , pythonGILRCount(0)
    , glRequirements()
    , glHasTextureFloat(false)
    , hasInitializedOpenGLFunctions(false)
    , openGLFunctionsMutex()
    , glVersionMajor(0)
    , glVersionMinor(0)
    , renderingContextPool()
    , openGLRenderers()
    , tasksQueueManager()
{
    pythonTLS = boost::make_shared<TLSHolder<AppManager::PythonTLSData> >();
    setMaxCacheFiles();
    tasksQueueManager = boost::make_shared<TreeRenderQueueManager>();
}

AppManagerPrivate::~AppManagerPrivate()
{
    

    for (std::size_t i = 0; i < commandLineArgsUtf8Original.size(); ++i) {
        free(commandLineArgsUtf8Original[i]);
    }
#if PY_MAJOR_VERSION >= 3
    // Python 3
    for (std::size_t i = 0; i < commandLineArgsWideOriginal.size(); ++i) {
        free(commandLineArgsWideOriginal[i]);
    }
#endif
}

#ifdef NATRON_USE_BREAKPAD
void
AppManagerPrivate::initBreakpad(const QString& breakpadPipePath,
                                const QString& breakpadComPipePath,
                                int breakpad_client_fd)
{
    assert(!breakpadHandler);
    createBreakpadHandler(breakpadPipePath, breakpad_client_fd);

    /*
       We check periodically that the crash reporter process is still alive. If the user killed it somehow, then we want
       the Natron process to terminate
     */
    breakpadAliveThread = boost::make_shared<ExistenceCheckerThread>(QString::fromUtf8(NATRON_NATRON_TO_BREAKPAD_EXISTENCE_CHECK),
                                                                     QString::fromUtf8(NATRON_NATRON_TO_BREAKPAD_EXISTENCE_CHECK_ACK),
                                                                     breakpadComPipePath);
    QObject::connect( breakpadAliveThread.get(), SIGNAL(otherProcessUnreachable()), appPTR, SLOT(onCrashReporterNoLongerResponding()) );
    breakpadAliveThread->start();
}

void
AppManagerPrivate::createBreakpadHandler(const QString& breakpadPipePath,
                                         int breakpad_client_fd)
{
    QString dumpPath = StandardPaths::writableLocation(StandardPaths::eStandardLocationTemp);

    Q_UNUSED(breakpad_client_fd);
    try {
#if defined(Q_OS_MAC)
        Q_UNUSED(breakpad_client_fd);
        breakpadHandler = boost::make_shared<google_breakpad::ExceptionHandler>( dumpPath.toStdString(),
                                                                                 google_breakpad::ExceptionHandler::FilterCallback(NULL),
                                                                                 google_breakpad::ExceptionHandler::MinidumpCallback(NULL) /*dmpcb*/,
                                                                                 (void*)NULL,
                                                                                 true,
                                                                                 breakpadPipePath.toStdString().c_str() );
#elif defined(Q_OS_LINUX)
        Q_UNUSED(breakpadPipePath);
        breakpadHandler = boost::make_shared<google_breakpad::ExceptionHandler>( google_breakpad::MinidumpDescriptor( dumpPath.toStdString() ),
                                                                                 google_breakpad::ExceptionHandler::FilterCallback(NULL),
                                                                                 google_breakpad::ExceptionHandler::MinidumpCallback(NULL) /*dmpCb*/,
                                                                                 (void*)NULL,
                                                                                 true,
                                                                                 breakpad_client_fd);
#elif defined(Q_OS_WIN32)
        Q_UNUSED(breakpad_client_fd);
        breakpadHandler = boost::make_shared<google_breakpad::ExceptionHandler>( dumpPath.toStdWString(),
                                                                                 google_breakpad::ExceptionHandler::FilterCallback(NULL), //filter callback
                                                                                 google_breakpad::ExceptionHandler::MinidumpCallback(NULL) /*dmpcb*/,
                                                                                 (void*)NULL, //context
                                                                                 google_breakpad::ExceptionHandler::HANDLER_ALL,
                                                                                 MiniDumpNormal,
                                                                                 breakpadPipePath.toStdWString().c_str(),
                                                                                 (google_breakpad::CustomClientInfo*)NULL);
#endif
    } catch (const std::exception& e) {
        qDebug() << e.what();

        return;
    }
}

#endif // NATRON_USE_BREAKPAD


void
AppManagerPrivate::initProcessInputChannel(const QString & mainProcessServerName)
{
    _backgroundIPC.reset( new ProcessInputChannel(mainProcessServerName) );
}

void
AppManagerPrivate::loadBuiltinFormats()
{
    // Initializing list of all Formats available by default in a project, the user may add formats to this list via Python in the init.py script, see
    // example here: http://natron.readthedocs.io/en/workshop/startupscripts.html#init-py
    std::vector<std::string> formatNames;
    struct BuiltinFormat
    {
        const char *name;
        int w;
        int h;
        double par;
    };

    // list of formats: the names must match those in ofxsFormatResolution.h
    BuiltinFormat formats[] = {
        { "PC_Video",              640,  480, 1 },
        { "NTSC",                  720,  486, 0.91 },
        { "PAL",                   720,  576, 1.09 },
        { "NTSC_16:9",             720,  486, 1.21 },
        { "PAL_16:9",              720,  576, 1.46 },
        { "HD_720",               1280,  720, 1 },
        { "HD",                   1920, 1080, 1 },
        { "UHD_4K",               3840, 2160, 1 },
        { "1K_Super_35(full-ap)", 1024,  778, 1 },
        { "1K_Cinemascope",        914,  778, 2 },
        { "2K_Super_35(full-ap)", 2048, 1556, 1 },
        { "2K_Cinemascope",       1828, 1556, 2 },
        { "2K_DCP",               2048, 1080, 1 },
        { "4K_Super_35(full-ap)", 4096, 3112, 1 },
        { "4K_Cinemascope",       3656, 3112, 2 },
        { "4K_DCP",               4096, 2160, 1 },
        { "square_256",            256,  256, 1 },
        { "square_512",            512,  512, 1 },
        { "square_1K",            1024, 1024, 1 },
        { "square_2K",            2048, 2048, 1 },
        { NULL, 0, 0, 0 }
    };

    for (BuiltinFormat* f = &formats[0]; f->name != NULL; ++f) {
        Format _frmt(0, 0, f->w, f->h, f->name, f->par);
        _formats.push_back(_frmt);
    }
} // loadBuiltinFormats

//
// only used for Natron internal plugins (ViewerGroup, Dot, DiskCache, BackDrop, Roto)
// see also AppManager::getPluginBinary(), OFX::Host::PluginCache::getPluginById()
//
PluginPtr
AppManagerPrivate::findPluginById(const std::string& newId,
                                  int major,
                                  int minor) const
{
    for (PluginsMap::const_iterator it = _plugins.begin(); it != _plugins.end(); ++it) {
        for (PluginVersionsOrdered::const_reverse_iterator itver = it->second.rbegin();
             itver != it->second.rend();
             ++itver) {
            if ( ( (*itver)->getPluginID() == newId ) &&
                   (major == -1 ||
                    (*itver)->getPropertyUnsafe<unsigned int>(kNatronPluginPropVersion, 0) == (unsigned int)major) &&
                   (minor == -1 ||
                    (*itver)->getPropertyUnsafe<unsigned int>(kNatronPluginPropVersion, 1) == (unsigned int)minor ) ) {
                return (*itver);
            }
        }
    }

    return PluginPtr();
}

void
AppManagerPrivate::declareSettingsToPython()
{
    std::stringstream ss;

    ss <<  NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.settings = " << NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.getSettings()\n";
    const KnobsVec& knobs = _settings->getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        ss <<  NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.settings." <<
        (*it)->getName() << " = " << NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.settings.getParam('" << (*it)->getName() << "')\n";
    }
}


void
AppManagerPrivate::setMaxCacheFiles()
{
    /*Default to something reasonnable if the code below would happen to not work for some reason*/
    //size_t hardMax = NATRON_MAX_CACHE_FILES_OPENED;

#if defined(Q_OS_UNIX) && defined(RLIMIT_NOFILE)
    /*
       Avoid 'Too many open files' on Unix.

       Increase the number of file descriptors that the process can open to the maximum allowed.
       - By default, Mac OS X only allows 256 file descriptors, which can easily be reached.
       - On Linux, the default limit is usually 1024.

        Note that due to a bug in stdio on OS X, the limit on the number of files opened using fopen()
        cannot be changed after the first call to stdio (e.g. printf() or fopen()).
        Consequently, this has to be the first thing to do in main().
     */
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        if (rl.rlim_max > rl.rlim_cur) {
            rl.rlim_cur = rl.rlim_max;
            if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
#             if defined(__APPLE__) && defined(OPEN_MAX)
                // On Mac OS X, setrlimit(RLIMIT_NOFILE, &rl) fails to set
                // rlim_cur above OPEN_MAX even if rlim_max > OPEN_MAX.
                if (rl.rlim_cur > OPEN_MAX) {
                    rl.rlim_cur = OPEN_MAX;
                    //hardMax = rl.rlim_cur;
                    setrlimit(RLIMIT_NOFILE, &rl);
                }
#             endif
            } else {
                //hardMax = rl.rlim_cur;
            }
        }
    }
    //#elif defined(Q_OS_WIN)
    // The following code sets the limit for stdio-based calls only.
    // Note that low-level calls (CreateFile(), WriteFile(), ReadFile(), CloseHandle()...) are not affected by this limit.
    // References:
    // - http://msdn.microsoft.com/en-us/library/6e3b887c.aspx
    // - https://stackoverflow.com/questions/870173/is-there-a-limit-on-number-of-open-files-in-windows/4276338
    // - http://bugs.mysql.com/bug.php?id=24509
    //_setmaxstdio(2048); // sets the limit for stdio-based calls
    // On Windows there seems to be no limit at all. The following test program can prove it:
    /*
       #include <windows.h>
       int
       main(int argc,
         char *argv[])
       {
        const int maxFiles = 10000;

        std::list<HANDLE> files;

        for (int i = 0; i < maxFiles; ++i) {
            std::stringstream ss;
            ss << "C:/Users/Lex/Documents/GitHub/Natron/App/win32/debug/testMaxFiles/file" << i ;
            std::string filename = ss.str();
            HANDLE file_handle = ::CreateFile(filename.c_str(), GENERIC_READ | GENERIC_WRITE,
                                              0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);


            if (file_handle != INVALID_HANDLE_VALUE) {
                files.push_back(file_handle);
                std::cout << "Good files so far: " << files.size() << std::endl;

            } else {
                char* message ;
                FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,GetLastError(),0,(LPSTR)&message,0,NULL);
                std::cout << "Failed to open " << filename << ": " << message << std::endl;
                LocalFree(message);
            }
        }
        std::cout << "Total opened files: " << files.size() << std::endl;
        for (std::list<HANDLE>::iterator it = files.begin(); it != files.end(); ++it) {
            CloseHandle(*it);
        }
       }
     */
#endif

    //maxCacheFiles = hardMax * 0.9;
}

#ifdef DEBUG
// logs every gl call to the console
static void
pre_gl_call(const char */*name*/,
            void */*funcptr*/,
            int /*len_args*/,
            ...)
{
#ifdef GL_TRACE_CALLS
    printf("Calling: %s (%d arguments)\n", name, len_args);
#endif
}

// logs every gl call to the console
static void
post_gl_call(const char */*name*/,
             void */*funcptr*/,
             int /*len_args*/,
             ...)
{
#ifdef GL_TRACE_CALLS
    GLenum _glerror_ = glGetError();
    if (_glerror_ != GL_NO_ERROR) {
        std::cout << "GL_ERROR : " << __FILE__ << ":" << __LINE__ << " " << gluErrorString(_glerror_) << std::endl;
        glError();
    }
#endif
}

#endif // debug

void
AppManagerPrivate::initGLAPISpecific()
{
#ifdef Q_OS_WIN32
    wglInfo.reset(new OSGLContext_wgl_data);
    OSGLContext_win::initWGLData( wglInfo.get() );
#elif defined(Q_OS_LINUX)
    glxInfo.reset(new OSGLContext_glx_data);
    OSGLContext_x11::initGLXData( glxInfo.get() );

#endif // Q_OS_WIN32
}

void
AppManagerPrivate::addOpenGLRequirementsString(QString& str, OpenGLRequirementsTypeEnum type, bool displayRenderers)
{
    switch (type) {
        case eOpenGLRequirementsTypeViewer: {
            str += QLatin1Char('\n');
            str += tr("%1 requires at least OpenGL %2.%3 with the following extensions so the viewer works appropriately:").arg(QLatin1String(NATRON_APPLICATION_NAME)).arg(NATRON_OPENGL_VERSION_REQUIRED_MAJOR).arg(NATRON_OPENGL_VERSION_REQUIRED_MINOR);
            str += QLatin1Char('\n');
            str += QString::fromUtf8("GL_ARB_vertex_buffer_object,GL_ARB_pixel_buffer_object");
            str += QLatin1Char('\n');
#ifdef __NATRON_WIN32__
            str += tr("To fix this: Re-start the installer, select \"Package manager\" and then install the \"Software OpenGL\" component.\n");
            str += tr("If you don't have the installer you can manually copy opengl32.dll located in the \"bin\\mesa\" directory of your %1 installation to the \"bin\" directory.").arg(QLatin1String(NATRON_APPLICATION_NAME));
#endif
            break;
        }
        case eOpenGLRequirementsTypeRendering: {
            str += QLatin1Char('\n');
            str += tr("%1 requires at least OpenGL %2.%3 with the following extensions to perform OpenGL rendering:").arg(QLatin1String(NATRON_APPLICATION_NAME)).arg(NATRON_OPENGL_VERSION_REQUIRED_MAJOR).arg(NATRON_OPENGL_VERSION_REQUIRED_MINOR);
            str += QLatin1Char('\n');
            str += QString::fromUtf8("GL_ARB_vertex_buffer_object <br /> GL_ARB_pixel_buffer_object <br /> GL_ARB_vertex_array_object or GL_APPLE_vertex_array_object <br /> GL_ARB_framebuffer_object or GL_EXT_framebuffer_object <br /> GL_ARB_texture_float");
            str += QLatin1Char('\n');
            break;
        }
    }
    if (displayRenderers) {
        std::list<OpenGLRendererInfo> openGLRenderers;
        OSGLContext::getGPUInfos(openGLRenderers);
        if ( !openGLRenderers.empty() ) {
            str += QLatin1Char('\n');
            str += tr("Available OpenGL renderers:");
            str += QLatin1Char('\n');
            for (std::list<OpenGLRendererInfo>::iterator it = openGLRenderers.begin(); it != openGLRenderers.end(); ++it) {
                str += (tr("Vendor:") + QString::fromUtf8(" ") + QString::fromUtf8( it->vendorName.c_str() ) +
                        QLatin1Char('\n') +
                        tr("Renderer:") + QString::fromUtf8(" ") + QString::fromUtf8( it->rendererName.c_str() ) +
                        QLatin1Char('\n') +
                        tr("OpenGL Version:") + QString::fromUtf8(" ") + QString::fromUtf8( it->glVersionString.c_str() ) +
                        QLatin1Char('\n') +
                        tr("GLSL Version:") + QString::fromUtf8(" ") + QString::fromUtf8( it->glslVersionString.c_str() ) );
                str += QLatin1Char('\n');
                str += QLatin1Char('\n');
            }
        }
    }
}

void
AppManagerPrivate::initGl(bool checkRenderingReq)
{
    // Private should not lock
    assert( !openGLFunctionsMutex.tryLock() );

    hasInitializedOpenGLFunctions = true;

#ifdef DEBUG
    glad_set_pre_callback(pre_gl_call);
    glad_set_post_callback(post_gl_call);
#endif

    bool glLoaded = gladLoadGL();

    // If only EXT_framebuffer is present and not ARB link functions
    if (glLoaded && GLAD_GL_EXT_framebuffer_object && !GLAD_GL_ARB_framebuffer_object) {
        glad_glIsRenderbuffer = glad_glIsRenderbufferEXT;
        glad_glBindRenderbuffer = glad_glBindRenderbufferEXT;
        glad_glDeleteRenderbuffers = glad_glDeleteRenderbuffersEXT;
        glad_glGenRenderbuffers = glad_glGenRenderbuffersEXT;
        glad_glRenderbufferStorage = glad_glRenderbufferStorageEXT;
        glad_glGetRenderbufferParameteriv = glad_glGetRenderbufferParameterivEXT;
        glad_glBindFramebuffer = glad_glBindFramebufferEXT;
        glad_glIsFramebuffer = glad_glIsFramebufferEXT;
        glad_glDeleteFramebuffers = glad_glDeleteFramebuffersEXT;
        glad_glGenFramebuffers = glad_glGenFramebuffersEXT;
        glad_glCheckFramebufferStatus = glad_glCheckFramebufferStatusEXT;
        glad_glFramebufferTexture1D = glad_glFramebufferTexture1DEXT;
        glad_glFramebufferTexture2D = glad_glFramebufferTexture2DEXT;
        glad_glFramebufferTexture3D = glad_glFramebufferTexture3DEXT;
        glad_glFramebufferRenderbuffer = glad_glFramebufferRenderbufferEXT;
        glad_glGetFramebufferAttachmentParameteriv = glad_glGetFramebufferAttachmentParameterivEXT;
        glad_glGenerateMipmap = glad_glGenerateMipmapEXT;
    }

    if (glLoaded && GLAD_GL_APPLE_vertex_array_object && !GLAD_GL_ARB_vertex_buffer_object) {
        glad_glBindVertexArray = glad_glBindVertexArrayAPPLE;
        glad_glDeleteVertexArrays = glad_glDeleteVertexArraysAPPLE;
        glad_glGenVertexArrays = glad_glGenVertexArraysAPPLE;
        glad_glIsVertexArray = glad_glIsVertexArrayAPPLE;
    }

    OpenGLRequirementsData& viewerReq = glRequirements[eOpenGLRequirementsTypeViewer];
    viewerReq.hasRequirements = true;

    OpenGLRequirementsData& renderingReq = glRequirements[eOpenGLRequirementsTypeRendering];
    if (checkRenderingReq) {
        renderingReq.hasRequirements = true;
    }


    if (!GLAD_GL_ARB_texture_float) {
        glHasTextureFloat = false;
    } else {
        glHasTextureFloat = true;
    }

    glVersionMajor = GLVersion.major;
    glVersionMinor = GLVersion.minor;

    if ( !glLoaded ||
        GLVersion.major < NATRON_OPENGL_VERSION_REQUIRED_MAJOR ||
        (GLVersion.major == NATRON_OPENGL_VERSION_REQUIRED_MAJOR && GLVersion.minor < NATRON_OPENGL_VERSION_REQUIRED_MINOR) ||
        !GLAD_GL_ARB_pixel_buffer_object ||
        !GLAD_GL_ARB_vertex_buffer_object) {

        viewerReq.error = tr("Failed to load OpenGL.");
        addOpenGLRequirementsString(viewerReq.error, eOpenGLRequirementsTypeViewer, true);
        viewerReq.hasRequirements = false;
        renderingReq.hasRequirements = false;
#ifdef DEBUG
        std::cerr << viewerReq.error.toStdString() << std::endl;
#endif

        return;
    }

    if (checkRenderingReq) {
        if (!GLAD_GL_ARB_texture_float ||
            (!GLAD_GL_ARB_framebuffer_object && !GLAD_GL_EXT_framebuffer_object) ||
            (!GLAD_GL_ARB_vertex_array_object && !GLAD_GL_APPLE_vertex_array_object))
        {
            renderingReq.error += QLatin1String("<p>");
            renderingReq.error += tr("Failed to load OpenGL.");
            addOpenGLRequirementsString(renderingReq.error, eOpenGLRequirementsTypeRendering, true);


            renderingReq.hasRequirements = false;
        } else {
            renderingReq.hasRequirements = true;
        }
    }
    // OpenGL is now read to be used! just include "Global/GLIncludes.h"
}

void
AppManagerPrivate::tearDownGL()
{
    // Kill all rendering context
    renderingContextPool.reset();

#ifdef Q_OS_WIN32
    if (wglInfo) {
        OSGLContext_win::destroyWGLData( wglInfo.get() );
    }
#elif defined(Q_OS_LINUX)
    if (glxInfo) {
        OSGLContext_x11::destroyGLXData( glxInfo.get() );
    }
#endif
}

void
AppManagerPrivate::copyUtf8ArgsToMembers(const std::vector<std::string>& utf8Args)
{
    // Copy command line args to local members that live throughout the lifetime of AppManager
#if PY_MAJOR_VERSION >= 3
    // Python 3
    commandLineArgsWideOriginal.resize(utf8Args.size());
#endif
    commandLineArgsUtf8Original.resize(utf8Args.size());
    nArgs = (int)utf8Args.size();
    for (std::size_t i = 0; i < utf8Args.size(); ++i) {
        commandLineArgsUtf8Original[i] = strdup(utf8Args[i].c_str());

        // Python 3 needs wchar_t arguments
#if PY_MAJOR_VERSION >= 3
        // Python 3
        commandLineArgsWideOriginal[i] = char2wchar(utf8Args[i].c_str());
#endif
    }
    commandLineArgsUtf8 = commandLineArgsUtf8Original;
#if PY_MAJOR_VERSION >= 3
    // Python 3
    commandLineArgsWide = commandLineArgsWideOriginal;
#endif
}

void
AppManagerPrivate::handleCommandLineArgs(int argc, char** argv)
{
    // Ensure the arguments are Utf-8 encoded
    std::vector<std::string> utf8Args;
    if (argv) {
        ProcInfo::ensureCommandLineArgsUtf8(argc, argv, &utf8Args);
    } else {
        // If the user didn't specify launch arguments (e.g unit testing),
        // At least append the binary path
        std::string path = ProcInfo::applicationDirPath(0);
        utf8Args.push_back(path);
    }

    copyUtf8ArgsToMembers(utf8Args);
}

void
AppManagerPrivate::handleCommandLineArgsW(int argc, wchar_t** argv)
{
    std::vector<std::string> utf8Args;
    if (argv) {
        for (int i = 0; i < argc; ++i) {
            std::wstring wide(argv[i]);
            std::string str = StrUtils::utf16_to_utf8(wide);
            assert(StrUtils::is_utf8(str.c_str()));
            utf8Args.push_back(str);
        }
    } else {
        // If the user didn't specify launch arguments (e.g unit testing),
        // At least append the binary path
        std::string path = ProcInfo::applicationDirPath(0);
        utf8Args.push_back(path);
    }

    copyUtf8ArgsToMembers(utf8Args);
}

NATRON_NAMESPACE_EXIT
