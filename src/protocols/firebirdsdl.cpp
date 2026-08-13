// Copyright (c) David Muse
// See the file COPYING for more information

#include "firebirdsdl.h"

#include <rudiments/charstring.h>

sqlrfirebirdsdl::sqlrfirebirdsdl() {
	schemaname=NULL;
	relationname=NULL;
	fieldname=NULL;
	init();
}

sqlrfirebirdsdl::~sqlrfirebirdsdl() {
	clear();
}

void sqlrfirebirdsdl::init() {
	ptr=NULL;
	end=NULL;
	err=NULL;
	relationidset=false;
	relationid=0;
	fieldidset=false;
	fieldid=0;
	elementtype=0;
	elementscale=0;
	elementlength=0;
	dimensioncount=0;
	for (uint16_t i=0; i<SQLRFIREBIRDSDL_MAX_DIMENSIONS; i++) {
		bounds[i].variable=0;
		bounds[i].lower=0;
		bounds[i].upper=0;
	}
}

void sqlrfirebirdsdl::clear() {
	delete[] schemaname;
	schemaname=NULL;
	delete[] relationname;
	relationname=NULL;
	delete[] fieldname;
	fieldname=NULL;
	init();
}

bool sqlrfirebirdsdl::setError(const char *e) {
	err=e;
	return false;
}

const char *sqlrfirebirdsdl::getError() const {
	return err;
}

bool sqlrfirebirdsdl::getByte(byte_t *value) {
	if (ptr>=end) {
		return false;
	}
	*value=*ptr;
	ptr++;
	return true;
}

bool sqlrfirebirdsdl::peekByte(byte_t *value) {
	if (ptr>=end) {
		return false;
	}
	*value=*ptr;
	return true;
}

bool sqlrfirebirdsdl::getWord(uint16_t *value) {
	if (ptr+2>end) {
		return false;
	}
	// little-endian, like firebird's get_word()
	*value=(uint16_t)ptr[0]|((uint16_t)ptr[1]<<8);
	ptr+=2;
	return true;
}

bool sqlrfirebirdsdl::getLong(uint32_t *value) {
	if (ptr+4>end) {
		return false;
	}
	// little-endian, like firebird's stuff_sdl_long()
	*value=(uint32_t)ptr[0]|
		((uint32_t)ptr[1]<<8)|
		((uint32_t)ptr[2]<<16)|
		((uint32_t)ptr[3]<<24);
	ptr+=4;
	return true;
}

bool sqlrfirebirdsdl::parse(const byte_t *sdl, uint32_t sdllen) {

	clear();

	if (!sdl || !sdllen) {
		return setError("empty SDL");
	}

	ptr=sdl;
	end=sdl+sdllen;

	// version
	byte_t	version;
	if (!getByte(&version)) {
		return setError("truncated SDL");
	}
	if (version!=isc_sdl_version1) {
		return setError("unsupported SDL version");
	}

	// the rest is a series of clauses, ending with an end-of-clauses byte
	bool	gotstruct=false;
	bool	gotelement=false;
	bool	goteoc=false;
	while (!goteoc) {

		byte_t	op;
		if (!getByte(&op)) {
			return setError("truncated SDL");
		}

		switch (op) {
			case isc_sdl_eoc:
				goteoc=true;
				break;
			case isc_sdl_struct:
				if (gotstruct) {
					return setError("duplicate "
							"SDL struct clause");
				}
				if (!parseElementDescriptor()) {
					return false;
				}
				gotstruct=true;
				break;
			case isc_sdl_schema:
				if (!parseName(&schemaname)) {
					return false;
				}
				break;
			case isc_sdl_relation:
				if (!parseName(&relationname)) {
					return false;
				}
				break;
			case isc_sdl_field:
				if (!parseName(&fieldname)) {
					return false;
				}
				break;
			case isc_sdl_rid:
				if (!parseId(&relationid,&relationidset)) {
					return false;
				}
				break;
			case isc_sdl_fid:
				if (!parseId(&fieldid,&fieldidset)) {
					return false;
				}
				break;
			case isc_sdl_do1:
			case isc_sdl_do2:
			case isc_sdl_do3:
				if (gotelement) {
					return setError("SDL loop after "
							"element clause");
				}
				if (!parseLoop(op)) {
					return false;
				}
				break;
			case isc_sdl_element:
				if (gotelement) {
					return setError("duplicate "
							"SDL element clause");
				}
				if (!parseElement()) {
					return false;
				}
				gotelement=true;
				break;
			default:
				// isc_sdl_literal, the comparison, logical,
				// loop, and label opcodes, isc_sdl_begin,
				// isc_sdl_end, isc_sdl_negate, and the
				// arithmetic opcodes.  nothing that generates
				// SDL emits any of them.
				return setError("unsupported SDL opcode");
		}
	}

	// anything past the end-of-clauses byte is junk
	if (ptr!=end) {
		return setError("trailing bytes after SDL");
	}

	if (!gotstruct) {
		return setError("missing SDL struct clause");
	}
	if (!dimensioncount) {
		return setError("missing SDL loop clause");
	}
	if (!gotelement) {
		return setError("missing SDL element clause");
	}
	return true;
}

bool sqlrfirebirdsdl::parseElementDescriptor() {

	// arrays of structures aren't supported, so the struct count must be 1
	byte_t	count;
	if (!getByte(&count)) {
		return setError("truncated SDL");
	}
	if (count!=1) {
		return setError("unsupported SDL struct count");
	}

	// the element is described by a blr type code, and then, depending on
	// the type, a character set/collation word, a scale byte, or a length
	// word.  (firebird's sdl_desc().)
	if (!getByte(&elementtype)) {
		return setError("truncated SDL");
	}

	bool		texttype=false;
	bool		scaletype=false;
	uint16_t	charsetandcollation=0;
	switch (elementtype) {
		case blr_text2:
		case blr_cstring2:
		case blr_varying2:
			// the character set and collation come first
			if (!getWord(&charsetandcollation)) {
				return setError("truncated SDL");
			}
			texttype=true;
			break;
		case blr_text:
		case blr_cstring:
		case blr_varying:
			texttype=true;
			break;
		case blr_short:
		case blr_long:
		case blr_quad:
		case blr_int64:
		case blr_int128:
			scaletype=true;
			break;
		case blr_float:
		case blr_double:
		case blr_d_float:
		case blr_dec64:
		case blr_dec128:
		case blr_sql_date:
		case blr_sql_time:
		case blr_sql_time_tz:
		case blr_ex_time_tz:
		case blr_timestamp:
		case blr_timestamp_tz:
		case blr_ex_timestamp_tz:
		case blr_bool:
			break;
		default:
			return setError("unsupported SDL element type");
	}

	if (scaletype) {
		byte_t	scale;
		if (!getByte(&scale)) {
			return setError("truncated SDL");
		}
		elementscale=(int8_t)scale;
	} else if (texttype) {
		// the declared length in characters.  for blr_varying it
		// doesn't count the 2-byte length that precedes the data, and
		// for blr_cstring it doesn't count the terminator.
		if (!getWord(&elementlength)) {
			return setError("truncated SDL");
		}
		if (!elementlength) {
			return setError("invalid SDL element length");
		}
	}
	return true;
}

bool sqlrfirebirdsdl::parseName(char **name) {

	// a byte count, and then that many bytes, unterminated
	byte_t	len;
	if (!getByte(&len)) {
		return setError("truncated SDL");
	}
	if (ptr+len>end) {
		return setError("truncated SDL");
	}

	delete[] *name;
	*name=charstring::duplicate((const char *)ptr,len);
	ptr+=len;
	return true;
}

bool sqlrfirebirdsdl::parseId(uint16_t *id, bool *idset) {
	if (!getWord(id)) {
		return setError("truncated SDL");
	}
	*idset=true;
	return true;
}

bool sqlrfirebirdsdl::parseLoop(byte_t op) {

	if (dimensioncount==SQLRFIREBIRDSDL_MAX_DIMENSIONS) {
		return setError("too many SDL dimensions");
	}

	// which loop variable this dimension uses
	byte_t	variable;
	if (!getByte(&variable)) {
		return setError("truncated SDL");
	}

	// isc_sdl_do1 leaves the lower bound out, and it defaults to 1
	int32_t	lower=1;
	if (op!=isc_sdl_do1 && !parseLiteral(&lower)) {
		return false;
	}

	int32_t	upper;
	if (!parseLiteral(&upper)) {
		return false;
	}

	// isc_sdl_do3 adds an increment.  nothing generates it, and an
	// increment other than 1 would mean the slice skips elements, which
	// isn't something the element walk below can describe.
	if (op==isc_sdl_do3) {
		int32_t	increment;
		if (!parseLiteral(&increment)) {
			return false;
		}
		if (increment!=1) {
			return setError("unsupported SDL loop increment");
		}
	}

	bounds[dimensioncount].variable=variable;
	bounds[dimensioncount].lower=lower;
	bounds[dimensioncount].upper=upper;
	dimensioncount++;
	return true;
}

bool sqlrfirebirdsdl::parseElement() {

	// arrays of structures aren't supported, so the element count must be 1
	byte_t	count;
	if (!getByte(&count)) {
		return setError("truncated SDL");
	}
	if (count!=1) {
		return setError("unsupported SDL element count");
	}

	// the element is a scalar reference into the array
	byte_t	op;
	if (!getByte(&op)) {
		return setError("truncated SDL");
	}
	if (op!=isc_sdl_scalar) {
		return setError("unsupported SDL element expression");
	}
	return parseScalar();
}

bool sqlrfirebirdsdl::parseScalar() {

	// which struct member the scalar refers to.  arrays of structures
	// aren't supported, so it must be the first one.
	byte_t	member;
	if (!getByte(&member)) {
		return setError("truncated SDL");
	}
	if (member) {
		return setError("unsupported SDL scalar member");
	}

	// one subscript per dimension
	byte_t	count;
	if (!getByte(&count)) {
		return setError("truncated SDL");
	}
	if (count!=dimensioncount) {
		return setError("SDL scalar/loop dimension mismatch");
	}

	// each subscript is the loop variable of the dimension it indexes
	for (byte_t i=0; i<count; i++) {

		byte_t	op;
		if (!getByte(&op)) {
			return setError("truncated SDL");
		}
		if (op!=isc_sdl_variable) {
			return setError("unsupported SDL scalar subscript");
		}

		byte_t	variable;
		if (!getByte(&variable)) {
			return setError("truncated SDL");
		}
		if (variable!=bounds[i].variable) {
			return setError("SDL scalar/loop variable mismatch");
		}
	}
	return true;
}

bool sqlrfirebirdsdl::parseLiteral(int32_t *value) {

	byte_t	op;
	if (!getByte(&op)) {
		return setError("truncated SDL");
	}

	switch (op) {
		case isc_sdl_tiny_integer:
			{
			byte_t	tiny;
			if (!getByte(&tiny)) {
				return setError("truncated SDL");
			}
			*value=(int32_t)(int8_t)tiny;
			return true;
			}
		case isc_sdl_short_integer:
			{
			uint16_t	shrt;
			if (!getWord(&shrt)) {
				return setError("truncated SDL");
			}
			*value=(int32_t)(int16_t)shrt;
			return true;
			}
		case isc_sdl_long_integer:
			{
			uint32_t	lng;
			if (!getLong(&lng)) {
				return setError("truncated SDL");
			}
			*value=(int32_t)lng;
			return true;
			}
		default:
			// a bound built out of arithmetic opcodes, or out of
			// the dead isc_sdl_literal.  nothing generates either.
			return setError("unsupported SDL bound expression");
	}
}

const char *sqlrfirebirdsdl::getSchemaName() const {
	return schemaname;
}

const char *sqlrfirebirdsdl::getRelationName() const {
	return relationname;
}

const char *sqlrfirebirdsdl::getFieldName() const {
	return fieldname;
}

bool sqlrfirebirdsdl::getRelationIdSet() const {
	return relationidset;
}

uint16_t sqlrfirebirdsdl::getRelationId() const {
	return relationid;
}

bool sqlrfirebirdsdl::getFieldIdSet() const {
	return fieldidset;
}

uint16_t sqlrfirebirdsdl::getFieldId() const {
	return fieldid;
}

byte_t sqlrfirebirdsdl::getElementType() const {
	return elementtype;
}

int8_t sqlrfirebirdsdl::getElementScale() const {
	return elementscale;
}

uint16_t sqlrfirebirdsdl::getElementLength() const {
	return elementlength;
}

uint16_t sqlrfirebirdsdl::getDimensionCount() const {
	return dimensioncount;
}

const sqlrfirebirdsdlbound *sqlrfirebirdsdl::getBound(uint16_t dim) const {
	if (dim>=dimensioncount) {
		return NULL;
	}
	return &bounds[dim];
}

int32_t sqlrfirebirdsdl::getLowerBound(uint16_t dim) const {
	if (dim>=dimensioncount) {
		return 0;
	}
	return bounds[dim].lower;
}

int32_t sqlrfirebirdsdl::getUpperBound(uint16_t dim) const {
	if (dim>=dimensioncount) {
		return 0;
	}
	return bounds[dim].upper;
}

uint64_t sqlrfirebirdsdl::getElementCount() const {

	if (!dimensioncount) {
		return 0;
	}

	uint64_t	count=1;
	for (uint16_t i=0; i<dimensioncount; i++) {

		// an inverted range is an empty one
		if (bounds[i].upper<bounds[i].lower) {
			return 0;
		}

		uint64_t	dimcount=(uint64_t)
					(bounds[i].upper-bounds[i].lower)+1;

		// bail rather than wrap around
		if (dimcount && count>((uint64_t)-1)/dimcount) {
			return 0;
		}
		count=count*dimcount;
	}
	return count;
}

bool sqlrfirebirdsdl::getSubscripts(uint64_t index,
					int32_t *subscripts) const {

	if (!subscripts || index>=getElementCount()) {
		return false;
	}

	// row-major, so the last dimension varies fastest
	uint64_t	remainder=index;
	uint16_t	i=dimensioncount;
	while (i) {
		i--;
		uint64_t	dimcount=(uint64_t)
					(bounds[i].upper-bounds[i].lower)+1;
		subscripts[i]=bounds[i].lower+(int32_t)(remainder%dimcount);
		remainder=remainder/dimcount;
	}
	return true;
}

bool sqlrfirebirdsdl::getArrayIndex(const int32_t *subscripts,
					const int32_t *arraylower,
					const int32_t *arrayupper,
					uint64_t *index) const {

	if (!subscripts || !arraylower || !arrayupper ||
					!index || !dimensioncount) {
		return false;
	}

	// row-major again, so each dimension's stride is the product of the
	// dimensions to its right
	uint64_t	result=0;
	for (uint16_t i=0; i<dimensioncount; i++) {

		if (arrayupper[i]<arraylower[i] ||
			subscripts[i]<arraylower[i] ||
			subscripts[i]>arrayupper[i]) {
			return false;
		}

		uint64_t	dimcount=(uint64_t)
					(arrayupper[i]-arraylower[i])+1;
		result=result*dimcount+
			(uint64_t)(subscripts[i]-arraylower[i]);
	}

	*index=result;
	return true;
}
