#this macro throws an error if any of the vars are not defined

MACRO(MUSTDEFINE_VAR VARS)
    foreach(ARG ${VARS})
    	IF ( NOT ${ARG} )
    		MESSAGE( FATAL_ERROR "The symbol '${ARG}' was not defined." )
    	ENDIF ( NOT ${ARG} )
    endforeach(ARG)
ENDMACRO(MUSTDEFINE_VAR)

#required headers
MACRO(CHECK_REQUIRED_HEADERS COMPULSARY_HEADERS)
    FOREACH(func ${ARGV})
    	STRING(TOUPPER ${func} FUNC)
    	STRING(REPLACE . _ FUNC ${FUNC})
    	CHECK_INCLUDE_FILE_CXX (${func} HAVE_${FUNC})
    	IF ( HAVE_${FUNC} )
    	    SET(_CL_HAVE_${FUNC} ${FUNC})
			list(APPEND _CL_HAVE_HEADERS "${func}")
    	ENDIF ( HAVE_${FUNC} )
    	IF ( NOT HAVE_${FUNC} )
    		MESSAGE ( FATAL_ERROR "${func} could not be found" )
    	ENDIF ( NOT HAVE_${FUNC} )
    ENDFOREACH(func ${COMPULSARY_HEADERS})
ENDMACRO(CHECK_REQUIRED_HEADERS)

#optional headers
MACRO(CHECK_OPTIONAL_HEADERS OPTIONAL_HEADERS)
    FOREACH(func ${ARGV})
    	STRING(TOUPPER ${func} FUNC)
    	STRING(REPLACE . _ FUNC ${FUNC})
    	STRING(REPLACE / _ FUNC ${FUNC})
    	CHECK_INCLUDE_FILE_CXX (${func} HAVE_${FUNC})
    	IF ( HAVE_${FUNC} )
    	    SET(_CL_HAVE_${FUNC} ${FUNC})
			list(APPEND _CL_HAVE_HEADERS "${func}")
    	ENDIF ( HAVE_${FUNC} )
    ENDFOREACH(func ${OPTIONAL_HEADERS})
ENDMACRO(CHECK_OPTIONAL_HEADERS)

#check for compulsary functions
MACRO(CHECK_REQUIRED_FUNCTIONS COMPULSARY_FUNCTIONS)
    FOREACH(func ${ARGV})
    	CHECK_OPTIONAL_FUNCTIONS ( ${func} )
    	IF ( NOT _CL_HAVE_FUNCTION_${FUNC} )
    		MESSAGE ( FATAL_ERROR "${func} could not be found" )
    	ENDIF ( NOT _CL_HAVE_FUNCTION_${FUNC} )
    ENDFOREACH(func ${COMPULSARY_FUNCTIONS})
    
    SET( CMAKE_EXTRA_INCLUDE_FILES )
ENDMACRO(CHECK_REQUIRED_FUNCTIONS)

#check for optional functions
MACRO(CHECK_OPTIONAL_FUNCTIONS OPTIONAL_FUNCTIONS)
    FOREACH(func ${ARGV})
    	STRING(TOUPPER ${func} FUNC)
        
        STRING(REGEX MATCH "[(|)]+" CHECK_OPTIONAL_FUNCTIONS_MATCH ${func} )
        IF ( CHECK_OPTIONAL_FUNCTIONS_MATCH STREQUAL "" )
            CHECK_CXX_SYMBOL_EXISTS (${func} "${_CL_HAVE_HEADERS}" _CL_HAVE_FUNCTION_${FUNC})
			if ( NOT _CL_HAVE_FUNCTION_${FUNC} )
				CHECK_FUNCTION_EXISTS(${func} _CL_HAVE_FUNCTION_${FUNC}_TMP)
				if ( _CL_HAVE_FUNCTION_${FUNC}_TMP )
					set( _CL_HAVE_FUNCTION_${FUNC} 1)
				endif( _CL_HAVE_FUNCTION_${FUNC}_TMP )
			endif( NOT _CL_HAVE_FUNCTION_${FUNC} )
        ELSE ( CHECK_OPTIONAL_FUNCTIONS_MATCH STREQUAL "" )
            STRING(REGEX REPLACE "(\\(.*\\))" "" CHECK_OPTIONAL_FUNCTIONS_MATCH ${func} )
            STRING( TOUPPER ${CHECK_OPTIONAL_FUNCTIONS_MATCH} FUNC )
            
            CHECK_STDCALL_FUNCTION_EXISTS (${func} _CL_HAVE_FUNCTION_${FUNC})
        ENDIF ( CHECK_OPTIONAL_FUNCTIONS_MATCH STREQUAL "" )
        
    ENDFOREACH(func ${OPTIONAL_FUNCTIONS})

ENDMACRO(CHECK_OPTIONAL_FUNCTIONS)
