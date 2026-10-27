/**
 * @file debug.h
 * This file contains macros for debug statements (i.e. output).
 * Usage: Write "_DEBUG( statement )" instead of "statement" and define DEBUG to switch the statements on.
 * @author Matthias Noack <Ma.Noack@tu-cottbus.de>
 */

#ifndef debug_h
#define debug_h

#ifdef DEBUG
	#define _DEBUG(expr) expr
#else
	#define _DEBUG(expr) ;
#endif

#endif
