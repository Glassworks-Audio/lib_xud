# This file provides overrides to the Doxyfile configuration

PROJECT_NAME           = lib_xud
PROJECT_BRIEF          = "USB device library"

INPUT                  = ../lib_xud/api ../lib_xud/src/user/control/msos2

PREDEFINED             = MA_C_API= C_API_START= C_API_END= __DOXYGEN__=1 ATTRIB_WEAK=
