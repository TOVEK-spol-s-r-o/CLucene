#CMake 2.6+ is recommended to an improved Boost module
CMAKE_MINIMUM_REQUIRED(VERSION 3.25 FATAL_ERROR)

PROJECT (clucene)

#Rules for version:
#MAJOR and MINOR versions are purely political (tracks JLucene compatibility)
#REVISION version MUST be revised if the headers or compatibility change
#PATCH should be 0 unless a patch is made that doesn't affect the public signature (i.e. clients don't need to re-compile).
SET(CLUCENE_VERSION_MAJOR "2")
SET(CLUCENE_VERSION_MINOR "3")
SET(CLUCENE_VERSION_REVISION "3")
SET(CLUCENE_VERSION_PATCH "4")

# SOVERSION information
#Must be incremented for releases if the api is not backwards compatible
SET(CLUCENE_SOVERSION "2.3.3.4")

MATH(EXPR CLUCENE_INT_VERSION "(${CLUCENE_VERSION_MAJOR} * 1000000) + (${CLUCENE_VERSION_MINOR} * 10000) + (${CLUCENE_VERSION_REVISION} * 100) + (${CLUCENE_VERSION_PATCH} * 1)" )
SET(CLUCENE_VERSION "${CLUCENE_VERSION_MAJOR}.${CLUCENE_VERSION_MINOR}.${CLUCENE_VERSION_REVISION}.${CLUCENE_VERSION_PATCH}")

if(WIN32)
#    SET(CMAKE_REQUIRED_LIBRARIES legacy_stdio_definitions.lib)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(WIN32)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
  cmake_policy(SET CMP0043 NEW)  
  cmake_policy(SET CMP0054 NEW)
  cmake_policy(SET CMP0074 NEW) 
  cmake_policy(SET CMP0075 NEW) 
  cmake_policy(SET CMP0077 NEW)
  cmake_policy(SET CMP0144 NEW)
  cmake_policy(SET CMP0167 NEW)
endif(COMMAND cmake_policy)


#set various platform specific global options
if(WIN32)
 set(CMAKE_DEBUG_POSTFIX "d")
endif(WIN32)

# include specific modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

#define options...

Include (CLuceneDocs)
Include (FindThreads)
Include (InstallPDB)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ELSE(NOT CMAKE_BUILD_TYPE)
    MESSAGE( "Compiling as ${CMAKE_BUILD_TYPE}" )
ENDIF(NOT CMAKE_BUILD_TYPE)

OPTION(ENABLE_DEBUG
  "enable debug support"
  OFF)
OPTION(ENABLE_MMAP
  "enable mmap support (experimental)"
  OFF)
OPTION(DISABLE_MULTITHREADING
  "disable multithreading - remove all locking code"
  OFF)
OPTION(ENABLE_DMALLOC
  "enable dmalloc memory leak checker"
  OFF)
OPTION(ENABLE_ASCII_MODE
  "enable ascii support"
  OFF)
  
SET(ENABLE_ANSI_MODE OFF)
IF(CMAKE_COMPILER_IS_GNUCXX)
  SET(ENABLE_ANSI_MODE ON)
  
  #exceptions:
  IF(MINGW OR CYGWIN)
    SET(ENABLE_ANSI_MODE OFF)
  ENDIF(MINGW OR CYGWIN)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

OPTION(ENABLE_ANSI_MODE
  "compile with -ansi flag"
  ${ENABLE_ANSI_MODE})
OPTION(LUCENE_USE_INTERNAL_CHAR_FUNCTIONS
  "use internal character functions. required to run tests correctly"
  ON)
OPTION(ENABLE_PACKAGING
  "create build scripts for creating clucene packages"
  OFF)
OPTION(BUILD_STATIC_LIBRARIES
  "create targets for building static libraries"
  OFF)
OPTION(BUILD_CONTRIBS
  "create targets for building the clucene-contribs"
  OFF)
OPTION(BUILD_CONTRIBS_LIB
  "create targets for building the clucene-contribs-lib"
  OFF)
SET(LUCENE_SYS_INCLUDES "" CACHE PATH
      "location for non-system independent files. defaults to CMAKE_INSTALL_PREFIX. see INSTALL documentation for further information."
      )
#install path options
SET(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
SET(LIB_DESTINATION "lib${LIB_SUFFIX}")


SET ( ENABLE_COMPILE_TESTS_VALUE ON )
IF ( MSVC_IDE )
	#this is annoying...
	SET ( ENABLE_COMPILE_TESTS_VALUE OFF )
ENDIF( MSVC_IDE )

OPTION(ENABLE_COMPILE_TESTS
  "enable various projects that test alternative build switches"
  ${ENABLE_COMPILE_TESTS_VALUE})

#check flags...
INCLUDE (TestCXXAcceptsFlag)
IF ( CMAKE_COMPILER_IS_GNUCC OR (${CMAKE_C_COMPILER_ID} STREQUAL "Clang") )
	#required standard
	string(CONCAT CMAKE_C_FLAGS ${CMAKE_C_FLAGS} " -std=c11")
	string(CONCAT CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} " -std=c++11")
	string(CONCAT CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} " -D_GLIBCXX_USE_C99")
    
	CHECK_CXX_ACCEPTS_FLAG(-pg GccFlagPg)
    IF ( GccFlagPg )
        OPTION(ENABLE_GPROF
          "turn on gprof profiling support"
          OFF)

        IF ( ENABLE_GPROF )
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
            SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
            SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
            SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -pg")
        ENDIF ( ENABLE_GPROF )
    ENDIF ( GccFlagPg )
    
    IF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC"  )
    ENDIF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")

   IF( ENABLE_ANSI_MODE )
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ansi")
   ENDIF ( ENABLE_ANSI_MODE )

   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

   set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS $<$<CONFIG:Release>:-ggdb>)

ELSE ( CMAKE_COMPILER_IS_GNUCC OR (${CMAKE_C_COMPILER_ID} STREQUAL "Clang") )

    string(REPLACE "/Ob1" "/Ob2" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "-Ob1" "/Ob2" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2 /Oi /Ot /GL")
	SET(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
	SET(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
	SET(CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")

ENDIF(CMAKE_COMPILER_IS_GNUCC OR (${CMAKE_C_COMPILER_ID} STREQUAL "Clang") ) 


#Single output directory for building all executables and libraries.
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH "Executable Output Directory" FORCE)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH "Library Output Directory" FORCE)

#add tests
ENABLE_TESTING()
ADD_TEST(SimpleTest ${EXECUTABLE_OUTPUT_PATH}/cl_test )

#use single output directory
INCLUDE_DIRECTORIES( ${clucene_SOURCE_DIR}/src/shared )
INCLUDE_DIRECTORIES( ${clucene_BINARY_DIR}/src/shared )
INCLUDE_DIRECTORIES( ${clucene_SOURCE_DIR}/src/core )

#include the projects
ADD_SUBDIRECTORY (src/shared)
ADD_SUBDIRECTORY (src/core)
ADD_SUBDIRECTORY (src/test)
ADD_SUBDIRECTORY (src/demo EXCLUDE_FROM_ALL)
IF ( BUILD_CONTRIBS )
  ADD_SUBDIRECTORY (src/contribs EXCLUDE_FROM_ALL)
  SET(BUILD_CONTRIBS_LIB 1)
ENDIF ( BUILD_CONTRIBS )
IF ( BUILD_CONTRIBS_LIB )
  ADD_SUBDIRECTORY (src/contribs-lib)
ENDIF ( BUILD_CONTRIBS_LIB )

# export targets
install(EXPORT CLuceneTargets
        FILE CLuceneConfig.cmake
        DESTINATION share/cmake/CLucene
        NAMESPACE CLucene::
)

#add uninstall command
CONFIGURE_FILE(
  "${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
  
ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

#this must go last...
IF (ENABLE_PACKAGING)
  INCLUDE(CreateClucenePackages)
ENDIF ( ENABLE_PACKAGING)
