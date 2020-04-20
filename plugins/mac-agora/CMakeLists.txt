project(mac-agora)

set(CMAKE_CXX_FLAGS "-stdlib=libc++")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".tbd")

include (CheckFunctionExists)

set(CMAKE_REQUIRED_LIBRARIES resolv)
check_function_exists(__b64_ntop HAVE_B64_IN_LIBRESOLV)
set(CMAKE_REQUIRED_LIBRARIES)
set(LIBRESOLV "resolv")

find_library(AGORA
    NAMES AgoraRtcKit
    PATHS .
    PATH_SUFFIXES Frameworks
    NO_DEFAULT_PATH)

find_library(COCOA Cocoa)
find_library(IOKIT IOKit)
find_library(COREWLAN CoreWLAN)
find_library(ACCELERATE Accelerate)
find_library(SYSTEMCONFIGURATION SystemConfiguration)

find_library(AVFOUNDATION AVFoundation)
find_library(COREMEDIA CoreMedia)
find_library(COREVIDEO CoreVideo)
find_library(COREAUDIO CoreAudio)
find_library(VIDEOTOOLBOX VideoToolBox)
find_library(AUDIOTOOLBOX AudioToolBox)
find_library(OPENGL OpenGL)


include_directories(${COCOA}
		    ${IOKIT}
		    ${COREWLAN}
		    ${ACCELERATE}
		    ${SYSTEMCONFIGURATION}
		    ${LIBRESOLV}
		    ${AVFOUNDATION}
		    ${COREMEDIA}
		    ${COREVIDEO}
		    ${COREAUDIO}
		    ${AUDIOTOOLBOX}
		    ${VIDEOTOOLBOX}
		    ${OPENGL}
		    ${AGORA}
)

set(mac-agora_HEADERS
	mac-streaming-agora.hpp
	mac-agora-rtc-engine.hpp
	mac-agora-raw-data.hpp
        mac-agora-encoded-data.hpp)

set(mac-agora_SOURCES
        mac-streaming-agora.cpp
        mac-agora-rtc-engine.cpp)

set(mac-agora_DEPS
    agora_rtc_sdk)

add_library(mac-agora MODULE
    ${mac-agora_HEADERS}
    ${mac-agora_SOURCES})

target_link_libraries(mac-agora
    	libobs
	${COCOA}
	${IOKIT}
	${COREWLAN}
	${ACCELERATE}
	${SYSTEMCONFIGURATION}
	${LIBRESOLV}
	${AVFOUNDATION}
	${COREMEDIA}
	${COREVIDEO}
	${COREAUDIO}
	${AUDIOTOOLBOX}
	${VIDEOTOOLBOX}
	${OPENGL}
	${AGORA}
)

install_obs_plugin_with_data(mac-agora data)
