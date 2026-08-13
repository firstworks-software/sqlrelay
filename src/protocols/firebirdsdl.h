// Copyright (c) David Muse
// See the file COPYING for more information

#ifndef FIREBIRDSDL_H
#define FIREBIRDSDL_H

#include <rudiments/private/inttypes.h>

// An interpreter for the SDL (Slice Description Language) buffer that comes
// with op_get_slice (and op_put_slice) in the firebird wire protocol.
//
// It works entirely on the raw bytes the client sent.  It doesn't need
// ibase.h, and doesn't link against libfbclient.
//
// What it has to interpret is what firebird's own client generates.  A client
// that reads an array calls isc_array_lookup_bounds() (or one of its
// relatives) to get an ISC_ARRAY_DESC, then isc_array_get_slice(), which runs
// the descriptor through gen_sdl() and puts the result in the op_get_slice
// request.  So the SDL that actually arrives is what gen_sdl() emits:
//
//	isc_sdl_version1
//	isc_sdl_struct 1 <element blr descriptor>
//	isc_sdl_relation <namelen> <name>
//	isc_sdl_field <namelen> <name>
//	(per dimension, first dimension first)
//		isc_sdl_do1 <var> <upper>		(lower bound is 1)
//	   or	isc_sdl_do2 <var> <lower> <upper>
//	isc_sdl_element 1
//	isc_sdl_scalar 0 <dimensions>
//	(per dimension)
//		isc_sdl_variable <var>
//	isc_sdl_eoc
//
// The bounds in the do1/do2 clauses are the bounds of the slice the client
// asked for.  For a whole-array read they match the column's own bounds, but a
// client can ask for a sub-range, and then they don't, so they're what the
// response has to be built from.
//
// The dead opcodes are rejected rather than interpreted.  isc_sdl_literal is
// commented out in firebird's own consts_pub.h, and so are the comparison,
// logical, loop, and label opcodes.  isc_sdl_begin, isc_sdl_end, and
// isc_sdl_negate survive there, but only for firebird's SDL pretty-printer -
// nothing generates them.  The arithmetic opcodes (add/subtract/multiply/
// divide) and isc_sdl_do3 survive as things firebird's SDL_walk() can run,
// but gen_sdl() never emits them either, so a bound that isn't a plain
// literal is rejected too.
//
// See firebird's src/yvalve/array.cpp (gen_sdl, stuff_literal, stuff_string)
// and src/common/sdl.cpp (SDL_info, SDL_walk, compile, sdl_desc) for what's
// being read here.

// SDL opcodes
// (firebird's isc_sdl_* - see firebird/impl/consts_pub.h.  the gaps are
// opcodes firebird has commented out.)
#define isc_sdl_version1	1
#define isc_sdl_relation	2
#define isc_sdl_rid		3
#define isc_sdl_field		4
#define isc_sdl_fid		5
#define isc_sdl_struct		6
#define isc_sdl_variable	7
#define isc_sdl_scalar		8
#define isc_sdl_tiny_integer	9
#define isc_sdl_short_integer	10
#define isc_sdl_long_integer	11
// 12 isc_sdl_literal - commented out
#define isc_sdl_add		13
#define isc_sdl_subtract	14
#define isc_sdl_multiply	15
#define isc_sdl_divide		16
#define isc_sdl_negate		17
// 18-30 comparison, logical, loop, and label opcodes - commented out
#define isc_sdl_begin		31
#define isc_sdl_end		32
#define isc_sdl_do3		33
#define isc_sdl_do2		34
#define isc_sdl_do1		35
#define isc_sdl_element		36
// isc_sdl_schema arrived in firebird 6.  older clients never send it.
#define isc_sdl_schema		37
#define isc_sdl_eoc		255

// blr type codes an element descriptor can use
// (firebird's blr_* - see firebird/impl/blr.h.  only the ones sdl_desc()
// accepts are here.)
#define blr_short		7
#define blr_long		8
#define blr_quad		9
#define blr_float		10
#define blr_d_float		11
#define blr_sql_date		12
#define blr_sql_time		13
#define blr_text		14
#define blr_text2		15
#define blr_int64		16
#define blr_bool		23
#define blr_dec64		24
#define blr_dec128		25
#define blr_int128		26
#define blr_double		27
#define blr_sql_time_tz		28
#define blr_timestamp_tz	29
#define blr_ex_time_tz		30
#define blr_ex_timestamp_tz	31
#define blr_timestamp		35
#define blr_varying		37
#define blr_varying2		38
#define blr_cstring		40
#define blr_cstring2		41

// the most dimensions firebird allows
// (firebird's MAX_ARRAY_DIMENSIONS)
#define SQLRFIREBIRDSDL_MAX_DIMENSIONS	16

// the bounds of one dimension of the requested slice
struct sqlrfirebirdsdlbound {
	// which loop variable the do1/do2 clause assigned to this dimension
	uint8_t	variable;
	int32_t	lower;
	int32_t	upper;
};

class sqlrfirebirdsdl {
	public:
			sqlrfirebirdsdl();
			~sqlrfirebirdsdl();

		// Interprets sdllen bytes of sdl.  Returns false, and leaves a
		// reason in getError(), if the buffer is truncated, the
		// version isn't isc_sdl_version1, or an opcode outside of what
		// gen_sdl() emits turns up.  Never throws.
		bool	parse(const byte_t *sdl, uint32_t sdllen);

		// what went wrong with the last parse(), or NULL if it worked
		const char	*getError() const;

		// which column the slice came out of.  a generated SDL names
		// the relation and field, but the id forms are read too, and
		// then the names are NULL and the ids are set.
		const char	*getSchemaName() const;
		const char	*getRelationName() const;
		const char	*getFieldName() const;
		bool		getRelationIdSet() const;
		uint16_t	getRelationId() const;
		bool		getFieldIdSet() const;
		uint16_t	getFieldId() const;

		// what one element of the array looks like.  the scale is
		// only carried for the exact-numeric types and the length only
		// for the character types - both are 0 otherwise, and the blr
		// type says how wide the element really is.
		byte_t		getElementType() const;
		int8_t		getElementScale() const;
		uint16_t	getElementLength() const;

		// the slice the client asked for
		uint16_t	getDimensionCount() const;
		const sqlrfirebirdsdlbound	*getBound(uint16_t dim) const;
		int32_t		getLowerBound(uint16_t dim) const;
		int32_t		getUpperBound(uint16_t dim) const;

		// How many elements are in the slice - the product of
		// (upper-lower+1) over every dimension.  0 if any dimension is
		// empty, or if nothing has been parsed.
		uint64_t	getElementCount() const;

		// Fills subscripts (getDimensionCount() of them) with the
		// subscripts of element index of the slice, counting the way
		// firebird lays an array out: row-major, so the last dimension
		// varies fastest.  (ISC_ARRAY_DESC's array_desc_flags is 0 for
		// row-major.)  Returns false if index is past the end.
		bool	getSubscripts(uint64_t index, int32_t *subscripts) const;

		// Works out where the element with those subscripts is in an
		// array whose own bounds are arraylower/arrayupper (which is
		// what isc_array_lookup_bounds() would have handed the client,
		// and needn't match the slice's bounds).  The result is an
		// element index, not a byte offset - multiply by the element
		// width to get one of those.  Returns false if a subscript is
		// outside of the array's bounds.
		bool	getArrayIndex(const int32_t *subscripts,
					const int32_t *arraylower,
					const int32_t *arrayupper,
					uint64_t *index) const;

	private:
		void	init();
		void	clear();

		bool	parseElementDescriptor();
		bool	parseName(char **name);
		bool	parseId(uint16_t *id, bool *idset);
		bool	parseLoop(byte_t op);
		bool	parseElement();
		bool	parseScalar();
		bool	parseLiteral(int32_t *value);

		bool	getByte(byte_t *value);
		bool	getWord(uint16_t *value);
		bool	getLong(uint32_t *value);
		bool	peekByte(byte_t *value);

		bool	setError(const char *e);

		const byte_t	*ptr;
		const byte_t	*end;

		const char	*err;

		char		*schemaname;
		char		*relationname;
		char		*fieldname;
		bool		relationidset;
		uint16_t	relationid;
		bool		fieldidset;
		uint16_t	fieldid;

		byte_t		elementtype;
		int8_t		elementscale;
		uint16_t	elementlength;

		uint16_t	dimensioncount;
		sqlrfirebirdsdlbound	bounds[SQLRFIREBIRDSDL_MAX_DIMENSIONS];
};

#endif
