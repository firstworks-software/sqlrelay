// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/environment.h>
#include <rudiments/bytestring.h>
#include <rudiments/snooze.h>
#include <rudiments/sys.h>

#include <defines.h>
#include <datatypes.h>
#include <config.h>

#include <ibase.h>

// for struct tm
#include <time.h>

#define MAX_ITEM_BUFFER_SIZE 32768
#define MAX_SELECT_LIST_SIZE 256
#define MAX_BIND_VARS 512
#define MAX_LOB_CHUNK_SIZE 65535

// how big an array a single field is allowed to pull into memory at once
#define MAX_ARRAY_BUFFER_SIZE (64*1024*1024)

// blr type codes.  An ISC_ARRAY_DESC's array_desc_dtype is one of these -
// isc_array_lookup_bounds() copies RDB$FIELDS.RDB$FIELD_TYPE straight into
// it - and so is the element type in the SDL that isc_array_get_slice()
// generates from the descriptor.  ibase.h doesn't pull in firebird's blr.h,
// so the ones used here are defined locally, guarded in case a future
// ibase.h does pull it in.
#ifndef blr_short
	#define blr_short		7
#endif
#ifndef blr_long
	#define blr_long		8
#endif
#ifndef blr_quad
	#define blr_quad		9
#endif
#ifndef blr_float
	#define blr_float		10
#endif
#ifndef blr_d_float
	#define blr_d_float		11
#endif
#ifndef blr_sql_date
	#define blr_sql_date		12
#endif
#ifndef blr_sql_time
	#define blr_sql_time		13
#endif
#ifndef blr_text
	#define blr_text		14
#endif
#ifndef blr_text2
	#define blr_text2		15
#endif
#ifndef blr_int64
	#define blr_int64		16
#endif
#ifndef blr_bool
	#define blr_bool		23
#endif
#ifndef blr_dec64
	#define blr_dec64		24
#endif
#ifndef blr_dec128
	#define blr_dec128		25
#endif
#ifndef blr_int128
	#define blr_int128		26
#endif
#ifndef blr_double
	#define blr_double		27
#endif
#ifndef blr_sql_time_tz
	#define blr_sql_time_tz		28
#endif
#ifndef blr_timestamp_tz
	#define blr_timestamp_tz	29
#endif
#ifndef blr_ex_time_tz
	#define blr_ex_time_tz		30
#endif
#ifndef blr_ex_timestamp_tz
	#define blr_ex_timestamp_tz	31
#endif
#ifndef blr_timestamp
	#define blr_timestamp		35
#endif
#ifndef blr_varying
	#define blr_varying		37
#endif
#ifndef blr_varying2
	#define blr_varying2		38
#endif
#ifndef blr_cstring
	#define blr_cstring		40
#endif
#ifndef blr_cstring2
	#define blr_cstring2		41
#endif

// isc_dsql_prepare's length parameter is an unsigned short, so a longer
// query's length would wrap
#define MAX_STATEMENT_SIZE 65535

// widest text rendering of a firebird 4 wide decimal (SQL_INT128,
// SQL_DEC16, SQL_DEC34), which get fetched as text since there's no
// native buffer for them here: a 38-digit DECIMAL/NUMERIC/INT128 needs up
// to 41 characters (sign, up to 38 digits, decimal point), a DECFLOAT(34)
// needs up to 43 (sign, digit, point, 33 digits, E, sign, up to 4-digit
// exponent); leave some margin
#define FIREBIRD_WIDEDECIMAL_TEXTLEN 46

// widest text rendering of a firebird 3+ BOOLEAN ("FALSE"), which gets
// fetched as text the same way, since there's no native buffer for it here
#define FIREBIRD_BOOLEAN_TEXTLEN 8

// fb_interpret (firebird 2.0+) supersedes the deprecated isc_interprete,
// whose sizeless buffer walk can overflow msg
static ISC_LONG fbInterpret(char *msg, unsigned int msgsize,
					const ISC_STATUS **pvector) {
#ifdef HAVE_FB_INTERPRET
	return fb_interpret(msg,msgsize,pvector);
#else
	// isc_interprete takes a non-const ISC_STATUS**; it only advances the
	// walking pointer, so dropping const is safe
	return isc_interprete(msg,(ISC_STATUS **)pvector);
#endif
}

// int64's are weird.  To the left of the decimal point is the
// value/10^scale, to the right is value%10^scale
static ssize_t firebirdFormatScaledInt64(char *buffer, size_t buffersize,
						ISC_INT64 v, short sqlscale) {

	ISC_SHORT	scale=-sqlscale;

	// Firebird allows 18 digits of scale, and 10^10 already
	// overflows an int, so the divisor is built with integer
	// math rather than with pow().
	ISC_INT64	p=1;
	for (ISC_SHORT i=0; i<scale; i++) {
		p*=10;
	}

	// Integer division truncates toward zero and the remainder
	// carries the sign, so formatting the halves separately would
	// put a sign on each of them, and lose it entirely when the
	// integer part is zero.
	ISC_INT64	whole=v/p;
	ISC_INT64	frac=v%p;
	const char	*sign=(v<0)?"-":"";
	if (whole<0) {
		whole=-whole;
	}
	if (frac<0) {
		frac=-frac;
	}
	return charstring::printf(buffer,buffersize,
					"%s%lld.%0*lld",
					sign,(int64_t)whole,
					scale,(int64_t)frac);
}

// How many bytes one element of an array takes up in the buffer that
// isc_array_get_slice() fills.  Firebird works this out in sdl_desc()
// (common/sdl.cpp) from the same blr type and length that the descriptor
// carries, and that's the stride its array walk uses.  Note that a varying
// element is described there as a cstring of length+2 bytes, not as a
// varying, so it's stored null-terminated rather than length-prefixed.
// Returns 0 for a type this module can't stride over.
static uint32_t firebirdArrayElementSize(byte_t dtype, uint16_t length) {
	switch (dtype) {
		case blr_text:
		case blr_text2:
		case blr_cstring:
		case blr_cstring2:
			return length;
		case blr_varying:
		case blr_varying2:
			return length+sizeof(uint16_t);
		case blr_bool:
			return 1;
		case blr_short:
			return 2;
		case blr_long:
		case blr_float:
		case blr_sql_date:
		case blr_sql_time:
			return 4;
		case blr_sql_time_tz:
			return 6;
		case blr_int64:
		case blr_quad:
		case blr_double:
		case blr_d_float:
		case blr_dec64:
		case blr_timestamp:
		case blr_ex_time_tz:
			return 8;
		case blr_timestamp_tz:
			return 10;
		case blr_ex_timestamp_tz:
			return 12;
		case blr_dec128:
		case blr_int128:
			return 16;
		default:
			return 0;
	}
}

// appends one element of an array, read out of the raw bytes
// isc_array_get_slice() returned, to output.  Returns false for an element
// type that has no text rendering here.
static bool firebirdAppendArrayElement(stringbuffer *output,
					const ISC_ARRAY_DESC *desc,
					const byte_t *element,
					uint32_t elementsize) {

	// the elements are packed, so nothing about them is guaranteed to be
	// aligned - each one is copied into a local before it's read
	char	buffer[FIREBIRD_WIDEDECIMAL_TEXTLEN];

	switch (desc->array_desc_dtype) {

		case blr_short:
			{
			ISC_SHORT	v=0;
			bytestring::copy(&v,element,sizeof(v));
			if (desc->array_desc_scale) {
				firebirdFormatScaledInt64(buffer,
						sizeof(buffer),(ISC_INT64)v,
						desc->array_desc_scale);
				output->append(buffer);
			} else {
				output->append((int64_t)v);
			}
			return true;
			}

		case blr_long:
			{
			ISC_LONG	v=0;
			bytestring::copy(&v,element,sizeof(v));
			if (desc->array_desc_scale) {
				firebirdFormatScaledInt64(buffer,
						sizeof(buffer),(ISC_INT64)v,
						desc->array_desc_scale);
				output->append(buffer);
			} else {
				output->append((int64_t)v);
			}
			return true;
			}

		case blr_int64:
			{
			ISC_INT64	v=0;
			bytestring::copy(&v,element,sizeof(v));
			if (desc->array_desc_scale) {
				firebirdFormatScaledInt64(buffer,
						sizeof(buffer),v,
						desc->array_desc_scale);
				output->append(buffer);
			} else {
				output->append((int64_t)v);
			}
			return true;
			}

		case blr_float:
			{
			float	v=0.0;
			bytestring::copy(&v,element,sizeof(v));
			charstring::printf(buffer,sizeof(buffer),
						"%.4f",(double)v);
			output->append(buffer);
			return true;
			}

		case blr_double:
		case blr_d_float:
			{
			double	v=0.0;
			bytestring::copy(&v,element,sizeof(v));
			charstring::printf(buffer,sizeof(buffer),"%.4f",v);
			output->append(buffer);
			return true;
			}

		case blr_bool:
			output->append((element[0])?'1':'0');
			return true;

		case blr_timestamp:
			{
			ISC_TIMESTAMP	v;
			bytestring::copy(&v,element,sizeof(v));
			tm	ts;
			#ifdef SQL_TIMESTAMP
			isc_decode_timestamp(&v,&ts);
			#else
			isc_decode_date(&v,&ts);
			#endif
			charstring::printf(buffer,sizeof(buffer),
					"'%d-%02d-%02d %02d:%02d:%02d'",
					ts.tm_year+1900,ts.tm_mon+1,ts.tm_mday,
					ts.tm_hour,ts.tm_min,ts.tm_sec);
			output->append(buffer);
			return true;
			}

		#ifdef SQL_TIMESTAMP
		case blr_sql_date:
			{
			ISC_DATE	v=0;
			bytestring::copy(&v,element,sizeof(v));
			tm	d;
			isc_decode_sql_date(&v,&d);
			charstring::printf(buffer,sizeof(buffer),
					"'%d-%02d-%02d'",
					d.tm_year+1900,d.tm_mon+1,d.tm_mday);
			output->append(buffer);
			return true;
			}

		case blr_sql_time:
			{
			ISC_TIME	v=0;
			bytestring::copy(&v,element,sizeof(v));
			tm	t;
			isc_decode_sql_time(&v,&t);
			charstring::printf(buffer,sizeof(buffer),
					"'%02d:%02d:%02d'",
					t.tm_hour,t.tm_min,t.tm_sec);
			output->append(buffer);
			return true;
			}
		#endif

		case blr_text:
		case blr_text2:
			{
			// a text element is blank padded out to its width
			uint32_t	len=elementsize;
			while (len && element[len-1]==' ') {
				len--;
			}
			output->append('\'');
			output->append((const char *)element,len);
			output->append('\'');
			return true;
			}

		case blr_cstring:
		case blr_cstring2:
		case blr_varying:
		case blr_varying2:
			{
			// firebird's sdl_desc() describes a varying array
			// element as a cstring, so both are stored
			// null-terminated inside the element's width
			uint32_t	len=0;
			while (len<elementsize && element[len]) {
				len++;
			}
			output->append('\'');
			output->append((const char *)element,len);
			output->append('\'');
			return true;
			}

		default:
			return false;
	}
}

// strips the quotes firebirdAppendArrayElement() put around a text, date, or
// time element, if they're there
static void firebirdTrimArrayElementQuotes(const char **text,
						uint32_t *textlen) {
	if (*textlen>=2 && (*text)[0]=='\'' && (*text)[*textlen-1]=='\'') {
		(*text)++;
		(*textlen)=(*textlen)-2;
	}
}

// the inverse of firebirdFormatScaledInt64() - reads a decimal number out of
// text and scales it back up to the int64 firebird stores
static bool firebirdParseScaledInt64(const char *text, uint32_t textlen,
					short sqlscale, ISC_INT64 *value) {

	ISC_SHORT	scale=-sqlscale;

	uint32_t	pos=0;
	bool		negative=false;
	if (pos<textlen && (text[pos]=='-' || text[pos]=='+')) {
		negative=(text[pos]=='-');
		pos++;
	}

	// the digits to the left of the decimal point
	ISC_INT64	v=0;
	bool		gotdigit=false;
	while (pos<textlen && text[pos]>='0' && text[pos]<='9') {
		v=v*10+(text[pos]-'0');
		gotdigit=true;
		pos++;
	}

	// the digits to the right of it, padded out to the scale with zeros,
	// or truncated to it if the value came in with more precision than
	// the column has
	ISC_SHORT	fracdigits=0;
	if (pos<textlen && text[pos]=='.') {
		pos++;
		while (pos<textlen && text[pos]>='0' && text[pos]<='9') {
			if (fracdigits<scale) {
				v=v*10+(text[pos]-'0');
				fracdigits++;
			}
			gotdigit=true;
			pos++;
		}
	}
	while (fracdigits<scale) {
		v=v*10;
		fracdigits++;
	}

	if (!gotdigit || pos!=textlen) {
		return false;
	}

	*value=(negative)?-v:v;
	return true;
}

// reads count decimal digits out of text at *pos
static bool firebirdParseArrayDigits(const char *text, uint32_t textlen,
					uint32_t *pos, uint8_t count,
					int32_t *value) {
	int32_t	v=0;
	for (uint8_t i=0; i<count; i++) {
		if (*pos>=textlen || text[*pos]<'0' || text[*pos]>'9') {
			return false;
		}
		v=v*10+(text[*pos]-'0');
		(*pos)++;
	}
	*value=v;
	return true;
}

// the inverse of the date/time/timestamp rendering in
// firebirdAppendArrayElement() - reads yyyy-mm-dd, hh:mm:ss, or both
static bool firebirdParseArrayDateTime(const char *text, uint32_t textlen,
					bool wantdate, bool wanttime, tm *ts) {

	firebirdTrimArrayElementQuotes(&text,&textlen);

	bytestring::zero(ts,sizeof(tm));
	ts->tm_mday=1;
	ts->tm_isdst=-1;

	uint32_t	pos=0;

	if (wantdate) {
		int32_t	year=0;
		int32_t	month=0;
		int32_t	day=0;
		if (!firebirdParseArrayDigits(text,textlen,&pos,4,&year) ||
			pos>=textlen || text[pos]!='-') {
			return false;
		}
		pos++;
		if (!firebirdParseArrayDigits(text,textlen,&pos,2,&month) ||
			pos>=textlen || text[pos]!='-') {
			return false;
		}
		pos++;
		if (!firebirdParseArrayDigits(text,textlen,&pos,2,&day)) {
			return false;
		}
		ts->tm_year=year-1900;
		ts->tm_mon=month-1;
		ts->tm_mday=day;
	}

	if (wantdate && wanttime) {
		// a space or a T between the two halves
		if (pos<textlen && (text[pos]==' ' || text[pos]=='T')) {
			pos++;
		} else if (pos==textlen) {
			// a date with no time of day is midnight
			return true;
		} else {
			return false;
		}
	}

	if (wanttime) {
		int32_t	hour=0;
		int32_t	minute=0;
		int32_t	second=0;
		if (!firebirdParseArrayDigits(text,textlen,&pos,2,&hour) ||
			pos>=textlen || text[pos]!=':') {
			return false;
		}
		pos++;
		if (!firebirdParseArrayDigits(text,textlen,&pos,2,&minute) ||
			pos>=textlen || text[pos]!=':') {
			return false;
		}
		pos++;
		if (!firebirdParseArrayDigits(text,textlen,&pos,2,&second)) {
			return false;
		}
		ts->tm_hour=hour;
		ts->tm_min=minute;
		ts->tm_sec=second;

		// a fractional second, if there is one, is dropped - the
		// rendering that this parses back never writes one
		if (pos<textlen && text[pos]=='.') {
			pos++;
			while (pos<textlen && text[pos]>='0' &&
						text[pos]<='9') {
				pos++;
			}
		}
	}

	return (pos==textlen);
}

// convertToInteger()/convertToFloat() both return 0 for text that isn't a
// number at all, the same as they'd return for a literal "0" - so a 0 result
// can't be trusted on its own.  This checks that the conversion actually
// consumed the text (rather than stopping at the first character) and that
// nothing but trailing whitespace is left over, so garbage input is caught
// instead of silently becoming zero.
static bool firebirdValidArrayNumber(const char *buffer, const char *endptr) {
	if (endptr==buffer) {
		return false;
	}
	while (*endptr==' ' || *endptr=='\t') {
		endptr++;
	}
	return (*endptr=='\0');
}

// The inverse of firebirdAppendArrayElement() - converts one element's text
// into the raw bytes isc_array_put_slice() expects, at the stride
// firebirdArrayElementSize() reports.  Returns false for an element type
// with no text rendering here, or for text that doesn't parse, so that a
// value that can't be converted fails the bind rather than getting written
// as garbage.  The types covered are exactly the ones
// firebirdAppendArrayElement() renders - the wide decimal types (dec64,
// dec128, int128) and the timezone-bearing types are not among them.
static bool firebirdParseArrayElement(const ISC_ARRAY_DESC *desc,
					byte_t *element,
					uint32_t elementsize,
					const char *text,
					uint32_t textlen) {

	// the elements are packed, so nothing about them is guaranteed to be
	// aligned - each one is built in a local and copied out
	char	buffer[FIREBIRD_WIDEDECIMAL_TEXTLEN];

	// the numeric types all parse out of a null-terminated copy
	switch (desc->array_desc_dtype) {
		case blr_short:
		case blr_long:
		case blr_int64:
		case blr_float:
		case blr_double:
		case blr_d_float:
		case blr_bool:
			if (textlen>=sizeof(buffer)) {
				return false;
			}
			charstring::copy(buffer,text,textlen);
			buffer[textlen]='\0';
			break;
		default:
			break;
	}

	switch (desc->array_desc_dtype) {

		case blr_short:
			{
			ISC_INT64	v=0;
			if (desc->array_desc_scale) {
				if (!firebirdParseScaledInt64(text,textlen,
						desc->array_desc_scale,&v)) {
					return false;
				}
			} else {
				const char	*endptr=NULL;
				v=(ISC_INT64)charstring::
					convertToInteger(buffer,&endptr);
				if (!firebirdValidArrayNumber(
							buffer,endptr)) {
					return false;
				}
			}
			ISC_SHORT	s=(ISC_SHORT)v;
			bytestring::copy(element,&s,sizeof(s));
			return true;
			}

		case blr_long:
			{
			ISC_INT64	v=0;
			if (desc->array_desc_scale) {
				if (!firebirdParseScaledInt64(text,textlen,
						desc->array_desc_scale,&v)) {
					return false;
				}
			} else {
				const char	*endptr=NULL;
				v=(ISC_INT64)charstring::
					convertToInteger(buffer,&endptr);
				if (!firebirdValidArrayNumber(
							buffer,endptr)) {
					return false;
				}
			}
			ISC_LONG	l=(ISC_LONG)v;
			bytestring::copy(element,&l,sizeof(l));
			return true;
			}

		case blr_int64:
			{
			ISC_INT64	v=0;
			if (desc->array_desc_scale) {
				if (!firebirdParseScaledInt64(text,textlen,
						desc->array_desc_scale,&v)) {
					return false;
				}
			} else {
				const char	*endptr=NULL;
				v=(ISC_INT64)charstring::
					convertToInteger(buffer,&endptr);
				if (!firebirdValidArrayNumber(
							buffer,endptr)) {
					return false;
				}
			}
			bytestring::copy(element,&v,sizeof(v));
			return true;
			}

		case blr_float:
			{
			const char	*endptr=NULL;
			float	v=(float)charstring::
					convertToFloat(buffer,&endptr);
			if (!firebirdValidArrayNumber(buffer,endptr)) {
				return false;
			}
			bytestring::copy(element,&v,sizeof(v));
			return true;
			}

		case blr_double:
		case blr_d_float:
			{
			const char	*endptr=NULL;
			double	v=charstring::convertToFloat(buffer,&endptr);
			if (!firebirdValidArrayNumber(buffer,endptr)) {
				return false;
			}
			bytestring::copy(element,&v,sizeof(v));
			return true;
			}

		case blr_bool:
			element[0]=(charstring::compare(buffer,"0") &&
					charstring::compareIgnoringCase(
							buffer,"false") &&
					buffer[0])?1:0;
			return true;

		case blr_timestamp:
			{
			tm	ts;
			if (!firebirdParseArrayDateTime(text,textlen,
							true,true,&ts)) {
				return false;
			}
			ISC_TIMESTAMP	v;
			#ifdef SQL_TIMESTAMP
			isc_encode_timestamp(&ts,&v);
			#else
			isc_encode_date(&ts,&v);
			#endif
			bytestring::copy(element,&v,sizeof(v));
			return true;
			}

		#ifdef SQL_TIMESTAMP
		case blr_sql_date:
			{
			tm	d;
			if (!firebirdParseArrayDateTime(text,textlen,
							true,false,&d)) {
				return false;
			}
			ISC_DATE	v=0;
			isc_encode_sql_date(&d,&v);
			bytestring::copy(element,&v,sizeof(v));
			return true;
			}

		case blr_sql_time:
			{
			tm	t;
			if (!firebirdParseArrayDateTime(text,textlen,
							false,true,&t)) {
				return false;
			}
			ISC_TIME	v=0;
			isc_encode_sql_time(&t,&v);
			bytestring::copy(element,&v,sizeof(v));
			return true;
			}
		#endif

		case blr_text:
		case blr_text2:
			{
			// a text element is stored blank padded out to its
			// width, and anything longer than that is truncated
			firebirdTrimArrayElementQuotes(&text,&textlen);
			uint32_t	len=textlen;
			if (len>elementsize) {
				len=elementsize;
			}
			bytestring::copy(element,text,len);
			bytestring::set(element+len,' ',elementsize-len);
			return true;
			}

		case blr_cstring:
		case blr_cstring2:
		case blr_varying:
		case blr_varying2:
			{
			// firebird's sdl_desc() describes a varying array
			// element as a cstring, so both are stored
			// null-terminated inside the element's width
			firebirdTrimArrayElementQuotes(&text,&textlen);
			uint32_t	len=textlen;
			if (len>elementsize-1) {
				len=elementsize-1;
			}
			bytestring::copy(element,text,len);
			bytestring::zero(element+len,elementsize-len);
			return true;
			}

		default:
			return false;
	}
}

// the XSQLDA carries no declared precision for a NUMERIC/DECIMAL column,
// only the storage type firebird chose to fit it (the narrowest of
// SMALLINT/INTEGER/BIGINT), so this reports that storage type's max
// precision as an upper bound rather than the true declared precision
static uint32_t firebirdNumericPrecisionFromSqlType(short sqltype) {
	if (sqltype==SQL_SHORT || sqltype==SQL_SHORT+1) {
		return 4;
	} else if (sqltype==SQL_LONG || sqltype==SQL_LONG+1) {
		return 9;
	}
	#ifdef SQL_INT64
	else if (sqltype==SQL_INT64 || sqltype==SQL_INT64+1) {
		return 18;
	}
	#endif
	#ifdef SQL_INT128
	else if (sqltype==SQL_INT128 || sqltype==SQL_INT128+1) {
		return 38;
	}
	#endif
	return 18;
}

static int firebirdSqlTypeToDatatype(short sqltype,
					short sqlsubtype, short sqlscale) {
	// mirrors the coercion describeResultSet() applies to output
	// columns, minus the buffer assignment
	if (sqltype==SQL_TEXT || sqltype==SQL_TEXT+1) {
		return CHAR_DATATYPE;
	} else if (sqltype==SQL_VARYING || sqltype==SQL_VARYING+1) {
		return VARCHAR_DATATYPE;
	} else if ((sqltype==SQL_SHORT || sqltype==SQL_SHORT+1) && !sqlscale) {
		return SMALLINT_DATATYPE;
	} else if ((sqltype==SQL_SHORT || sqltype==SQL_SHORT+1) && sqlscale) {
		return (sqlsubtype==1)?NUMERIC_DATATYPE:DECIMAL_DATATYPE;
	// Looks like sometimes firebird returns INT64's as SQL_LONG type.
	// These can be identified because the sqlscale gets set too.  Treat
	// SQL_LONG's with an sqlscale as INT64's.
	} else if ((sqltype==SQL_LONG || sqltype==SQL_LONG+1) && !sqlscale) {
		return INTEGER_DATATYPE;
	} else if (
	#ifdef SQL_INT64
			(sqltype==SQL_INT64 || sqltype==SQL_INT64+1) ||
	#endif
			((sqltype==SQL_LONG || sqltype==SQL_LONG+1) &&
								sqlscale)) {
		return (sqlsubtype==1)?NUMERIC_DATATYPE:DECIMAL_DATATYPE;
	#ifdef SQL_INT128
	} else if (sqltype==SQL_INT128 || sqltype==SQL_INT128+1) {
		return (sqlsubtype==1)?NUMERIC_DATATYPE:DECIMAL_DATATYPE;
	#endif
	#ifdef SQL_DEC16
	} else if (sqltype==SQL_DEC16 || sqltype==SQL_DEC16+1) {
		return DOUBLE_PRECISION_DATATYPE;
	#endif
	#ifdef SQL_DEC34
	} else if (sqltype==SQL_DEC34 || sqltype==SQL_DEC34+1) {
		return DOUBLE_PRECISION_DATATYPE;
	#endif
	#ifdef SQL_BOOLEAN
	} else if (sqltype==SQL_BOOLEAN || sqltype==SQL_BOOLEAN+1) {
		return BOOL_DATATYPE;
	#endif
	} else if (sqltype==SQL_FLOAT || sqltype==SQL_FLOAT+1) {
		return FLOAT_DATATYPE;
	} else if (sqltype==SQL_DOUBLE || sqltype==SQL_DOUBLE+1) {
		return DOUBLE_PRECISION_DATATYPE;
	} else if (sqltype==SQL_D_FLOAT || sqltype==SQL_D_FLOAT+1) {
		return D_FLOAT_DATATYPE;
	} else if (sqltype==SQL_ARRAY || sqltype==SQL_ARRAY+1) {
		return ARRAY_DATATYPE;
	} else if (sqltype==SQL_QUAD || sqltype==SQL_QUAD+1) {
		return QUAD_DATATYPE;
	#ifdef SQL_TIMESTAMP
	} else if (sqltype==SQL_TIMESTAMP || sqltype==SQL_TIMESTAMP+1) {
	#else
	} else if (sqltype==SQL_DATE || sqltype==SQL_DATE+1) {
	#endif
		return TIMESTAMP_DATATYPE;
	#ifdef SQL_TIMESTAMP
	} else if (sqltype==SQL_TYPE_TIME || sqltype==SQL_TYPE_TIME+1) {
		return TIME_DATATYPE;
	} else if (sqltype==SQL_TYPE_DATE || sqltype==SQL_TYPE_DATE+1) {
		return DATE_DATATYPE;
	#endif
	} else if (sqltype==SQL_BLOB || sqltype==SQL_BLOB+1) {
		return (sqlsubtype==1)?CLOB_DATATYPE:BLOB_DATATYPE;
	}
	return UNKNOWN_DATATYPE;
}

struct fieldstruct {
	int		sqlrtype;
	short		type;

	short		shortbuffer;
	long		longbuffer;
	float		floatbuffer;
	double		doublebuffer;
	ISC_QUAD	quadbuffer;
	ISC_DATE	datebuffer;
	ISC_TIME	timebuffer;
	ISC_TIMESTAMP	timestampbuffer;
	ISC_INT64	int64buffer;
	char		*textbuffer;
	ISC_QUAD	blobid;
	isc_blob_handle	blobhandle;
	bool		blobisopen;

	// array support.  The descriptor describes the column, so it's
	// looked up once and kept for the whole result set, but the elements
	// belong to the row, so the buffer is dropped every time a new row
	// is fetched.
	ISC_ARRAY_DESC	arraydesc;
	bool		arraydescvalid;
	byte_t		*arraybuffer;
	uint64_t	arraybuffersize;
	uint64_t	arrayelementcount;
	uint32_t	arrayelementsize;
	bool		arraybuffervalid;

	short		nullindicator;
};

struct inbinddescribestruct {
	short		sqltype;
	short		sqlscale;
	short		sqlsubtype;
	short		sqllen;
	// the relation and field the parameter refers to, which is what an
	// array bind has to look the column's shape up by - see
	// inputBindArray()
	short		relnamelength;
	char		relname[sizeof(((XSQLVAR *)NULL)->relname)+1];
	short		sqlnamelength;
	char		sqlname[sizeof(((XSQLVAR *)NULL)->sqlname)+1];
};

struct datebind {
        int16_t         *year;
        int16_t         *month;
        int16_t         *day;
        int16_t         *hour;
        int16_t         *minute;
        int16_t         *second;
        const char      **tz;
	bool		*isnegative;
	ISC_TIMESTAMP	buffer;
};

class firebirdconnection;

class SQLRSERVER_DLLSPEC firebirdcursor : public sqlrservercursor {
	friend class firebirdconnection;
	private:
		firebirdcursor(sqlrserverconnection *conn, uint16_t id);
		~firebirdcursor();
		void		allocateResultSetBuffers(int32_t columncount);
		void		freeResultSetBuffers();
		bool		prepareQuery(const char *query,
						uint32_t size);
		bool		inputBind(const char *variable, 
						uint16_t variablesize,
						const char *value, 
						uint32_t valuesize,
						short *isnull);
		bool		inputBind(const char *variable, 
						uint16_t variablesize,
						int64_t *value);
		bool		inputBind(const char *variable, 
						uint16_t variablesize,
						double *value, 
						uint32_t precision,
						uint32_t scale);
		bool		inputBind(const char *variable,
                                        	uint16_t variablesize,
                                        	int64_t year,
                                        	int16_t month,
                                        	int16_t day,
                                        	int16_t hour,
                                        	int16_t minute,
                                        	int16_t second,
                                        	int32_t microsecond,
                                        	const char *tz,
						bool isnegative,
                                        	int16_t *isnull);
		bool		inputBindBlob(const char *variable,
						uint16_t variablesize,
						const char *value,
						uint32_t valuesize,
						int16_t *isnull);
		bool		inputBindBlob(const char *variable,
						uint16_t variablesize,
						const char *value,
						uint32_t valuesize,
						const uint32_t *segmentlengths,
						uint16_t segmentcount,
						int16_t *isnull);
		bool		inputBindArray(const char *variable,
						uint16_t variablesize,
						const char *value,
						uint32_t valuesize,
						int16_t *isnull);
		bool		inputBindClob(const char *variable,
						uint16_t variablesize,
						const char *value,
						uint32_t valuesize,
						int16_t *isnull);
		bool		inputBindClob(const char *variable,
						uint16_t variablesize,
						const char *value,
						uint32_t valuesize,
						const uint32_t *segmentlengths,
						uint16_t segmentcount,
						int16_t *isnull);
		bool		outputBind(const char *variable,
						uint16_t variablesize,
						char *value, 
						uint32_t valuesize,
						short *isnull);
		bool		outputBind(const char *variable,
						uint16_t variablesize,
						int64_t *value,
						int16_t *isnull);
		bool		outputBind(const char *variable,
						uint16_t variablesize,
						double *value,
						uint32_t *precision,
						uint32_t *scale,
						int16_t *isnull);
		bool		outputBind(const char *variable,
						uint16_t variablesize,
						int16_t *year,
						int16_t *month,
						int16_t *day,
						int16_t *hour,
						int16_t *minute,
						int16_t *second,
						int32_t *microsecond,
						const char **tz,
						bool *isnegative,
						int16_t *isnull);
		bool		outputBindBlob(const char *variable,
						uint16_t variablesize,
						uint16_t index,
						int16_t *isnull);
		bool		outputBindClob(const char *variable,
						uint16_t variablesize,
						uint16_t index,
						int16_t *isnull);
		bool		getLobOutputBindLength(uint16_t index,
							uint64_t *length);
		bool		getLobOutputBindSegment(uint16_t index,
							char *buffer,
							uint64_t buffersize,
							uint64_t offset,
							uint64_t charstoread,
							uint64_t *charsread);
		void		closeLobOutputBind(uint16_t index);
		bool		executeQuery(const char *query,
						uint32_t size);
		bool		describeResultSet();
		void		getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t	*errorcode,
						bool *liveconnection);
		void		checkForTempTable(const char *query,
							uint32_t size);
		bool		queryIsNotSelect();
		bool		queryIsCommitOrRollback();
		uint64_t	getAffectedRows();
		uint32_t	colCount();
		const char	*getColumnName(uint32_t col);
		uint16_t	getColumnNameSize(uint32_t col);
		uint16_t	getColumnType(uint32_t col);
		uint32_t	getColumnSize(uint32_t col);
		uint32_t	getColumnPrecision(uint32_t col);
		uint32_t	getColumnScale(uint32_t col);
		uint16_t	getColumnIsNullable(uint32_t col);
		const char	*getColumnTable(uint32_t col);
		uint16_t	getColumnTableSize(uint32_t col);
		const char	*getColumnField(uint32_t col);
		uint16_t	getColumnFieldSize(uint32_t col);
		bool		noRowsToReturn();
		bool		fetchRow(bool *error);
		void		getField(uint32_t col,
					const char **field,
					uint64_t *fieldsize,
					bool *lob,
					bool *null);
		bool		getLobFieldLength(uint32_t col,
						uint64_t *length);
		bool		getLobFieldSegment(uint32_t col,
					char *buffer, uint64_t buffersize,
					uint64_t offset, uint64_t charstoread,
					uint64_t *charsread);
		void		closeLobField(uint32_t col);
		bool		getArrayFieldDescriptor(uint32_t col,
					const unsigned char **descriptor,
					uint64_t *descriptorsize);
		bool		getArrayFieldSlice(uint32_t col,
					char *buffer, uint64_t buffersize,
					uint64_t offset,
					uint64_t elementstoread,
					uint64_t *elementsread);
		void		closeArrayField(uint32_t col);
		bool		fetchArrayField(uint32_t col);
		void		closeResultSet();
		bool		columnInfoIsValidAfterPrepare();
		uint16_t	getInputBindCountFromPrepare();
		uint16_t	getInputBindType(uint16_t index);
		uint32_t	getInputBindSize(uint16_t index);
		uint32_t	getInputBindScale(uint16_t index);
		uint32_t	getInputBindPrecision(uint16_t index);
		bool		getInputBindIsNullable(uint16_t index);


		isc_stmt_handle	stmt;

		uint16_t	maxbindcount;

		XSQLDA	ISC_FAR	*inbindsqlda;
		ISC_TIMESTAMP	*inbindts;
		ISC_QUAD	*inbindblobid;
		ISC_QUAD	*inbindarrayid;
		isc_blob_handle	*inbindblobhandle;
		inbinddescribestruct	*inbinddescribe;
		uint16_t	inbindcountfromprepare;

		XSQLDA	ISC_FAR	*outbindsqlda;
		ISC_QUAD	*outbindblobid;
		isc_blob_handle	*outbindblobhandle;
		bool		*outbindblobisopen;
		uint16_t	outbindcount;
		datebind	*outdatebind;
		
		XSQLDA	ISC_FAR	*outsqlda;
		byte_t		*outsqldabuffer;
		fieldstruct	*field;
		int32_t		fieldcount;

		ISC_LONG	querytype;

		firebirdconnection	*firebirdconn;

		bool	queryisexecsp;
		bool	bindformaterror;
		bool	querytoolarge;
		bool	resultsetdescribed;

		regularexpression	executeprocedure;
};

class SQLRSERVER_DLLSPEC firebirdconnection : public sqlrserverconnection {
	friend class firebirdcursor;
	public:
		firebirdconnection(sqlrservercontroller *cont);
		~firebirdconnection();
	private:
		void	initDatabaseFeatures();
		void	handleConnectString();
		bool	logIn(const char **error, const char **warning);
		sqlrservercursor	*newCursor(uint16_t id);
		void	deleteCursor(sqlrservercursor *curs);
		void	logOut();
		sqlrtxmodel_t	getNativeTransactionModel();
		bool	setAutoCommitOn();
		bool	setAutoCommitOff();
		bool	setReadOnly(bool readonly);
		bool	setTransactionIsolationLevel(const char *isolevel);
		bool	supportsAutoCommit();
		bool	commit();
		bool	rollback();
		bool	ping();
		void	getError(char *errorbuffer,
					uint32_t errorbuffersize,
					uint32_t *errorsize,
					int64_t	*errorcode,
					bool *liveconnection);
		bool		selectCatalog(const char *catalog);
		char		*getCurrentCatalog();
		const char	*getDbType();
		const char	*getDbVersion();
		const char	*getDbHostName();
		const char	*getCatalogListQuery(
						const char *catalog);
		const char	*getSchemaListQuery(
						const char *catalog,
						const char *schema);
		const char	*getTableTypeListQuery(
						const char *catalog,
						const char *schema,
						const char *tabletypes);
		const char	*getGlobalTempTableListQuery();
		const char	*getTableListQuery(
						const char *catalog,
						const char *schema,
						const char *table,
						uint16_t objecttypes);
		const char	*getTypeInfoListQuery(
						const char *catalog,
						const char *schema,
						const char *type);
		const char	*getColumnListQuery(
						const char *catalog,
						const char *schema,
						const char *table,
						const char *column);
		const char	*getPrimaryKeysListQuery(
						const char *catalog,
						const char *schema,
						const char *table);
		const char	*getKeyAndIndexListQuery(
						const char *catalog,
						const char *schema,
						const char *table);
		const char	*getProcedureListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure);
		const char	*getProcedureParameterListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure);
		const char	*getBindFormat();
		const char	*getNextvalFormat();
		const char	*getCurrentUserQuery();
		const char	*getLastInsertIdQuery();
		bool		setIsolationLevel(const char *isolevel);
		const char	*setIsolationLevelQuery();
		const char	*getIsolationLevelQuery();
		const char	*mapIsolationLevel(
				const char *isolevel,
				sqlrserverisolationlevelformat_t fromformat,
				sqlrserverisolationlevelformat_t toformat);
		const char * const	*getDatabaseFeatures();
		const char	*getNoopQuery();

		// builds a tpb into "buf" for the current isolation level
		// (curtxisolevel) and the given readonly/autocommit flags,
		// and returns its length
		uint16_t	buildTpb(char *buf, bool readonly,
						bool autocommitflag);

		// commits whatever transaction is open and starts a new one
		// with a tpb built from buildTpb()
		bool	restartTransaction(bool readonly, bool autocommitflag);

		char		dpb[256];
		short		dpbsize;
		isc_db_handle	db;
		isc_tr_handle	tr;

		char		*database;
		char		*host;
		unsigned short	dialect;

		const char	*charset;

		bool		droptemptables;

		char		*dbversion;

		char		*lastinsertidquery;

		stringbuffer	schemalistquery;
		stringbuffer	tabletypelistquery;
		stringbuffer	tablelistquery;
		stringbuffer	procedurelistquery;
		stringbuffer	columnlistquery;
		stringbuffer	typeinfolistquery;
		stringbuffer	primarykeyslistquery;
		stringbuffer	keyandindexlistquery;
		stringbuffer	procedureparameterlistquery;

		ISC_STATUS	error[20];

		stringbuffer	errormsg;

		bool		autocommit;
		bool		nextreadonly;

		// the isolation level the currently-open (or about to be
		// (re)started) transaction uses, as a tpb byte - one of
		// isc_tpb_consistency, isc_tpb_concurrency or
		// isc_tpb_read_committed
		char		curtxisolevel;

		char		*maxconnections;
		const char	*databasefeatures[FEATURE_COUNT];
};

firebirdconnection::firebirdconnection(sqlrservercontroller *cont) :
						sqlrserverconnection(cont) {
	dbversion=NULL;
	lastinsertidquery=NULL;
	database=NULL;
	host=NULL;
	autocommit=false;
	nextreadonly=false;
	curtxisolevel=isc_tpb_read_committed;
	initDatabaseFeatures();
}

firebirdconnection::~firebirdconnection() {
	delete[] dbversion;
	delete[] lastinsertidquery;
	delete[] database;
	delete[] host;
	delete[] maxconnections;
}

void firebirdconnection::initDatabaseFeatures() {

	maxconnections=
		charstring::parseNumber(cont->getConfig()->getMaxConnections());

	databasefeatures[FEATURE_AGGREGATE_FUNCTIONS]=
		"ALL,AVG,COUNT,DISTINCT,MAX,MIN,SUM";

	databasefeatures[FEATURE_ALL_PROCEDURES_ARE_CALLABLE]=
		"false";

	databasefeatures[FEATURE_ALL_TABLES_ARE_SELECTABLE]=
		"false";

	databasefeatures[FEATURE_ALTER_DOMAIN_CLAUSES]=
		"";

	databasefeatures[FEATURE_ALTER_TABLE_OPERATIONS]=
		"ADD_COLUMN,DROP_COLUMN";

	databasefeatures[FEATURE_ANSI92_SQL_LEVELS]=
		"ENTRY_LEVEL";

	databasefeatures[FEATURE_AUTO_COMMIT_FAILURE_CLOSES_ALL_RESULT_SETS]=
		"false";

	databasefeatures[FEATURE_BATCH_OPERATIONS]=
		"";

	databasefeatures[FEATURE_BATCH_ROW_COUNTS]=
		"";

	databasefeatures[FEATURE_CATALOG_SEPARATOR]=
		"";

	databasefeatures[FEATURE_CATALOG_TERM]=
		"";

	databasefeatures[FEATURE_CATALOG_USAGE]=
		"";

	databasefeatures[FEATURE_COLLATION_SEQ]=
		"";

	databasefeatures[FEATURE_CREATE_ASSERTION_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_CHARACTER_SET_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_COLLATION_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_DOMAIN_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_SCHEMA_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_TABLE_CLAUSES]=
		"CREATE_TABLE,TABLE_CONSTRAINT,"
			"CONSTRAINT_NAME_DEFINITION,COLUMN_CONSTRAINT,"
			"COLUMN_DEFAULT,COLUMN_COLLATION,"
			"GLOBAL_TEMPORARY,COMMIT_DELETE,COMMIT_PRESERVE";

	databasefeatures[FEATURE_CREATE_TRANSLATION_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_VIEW_CLAUSES]=
		"CREATE_VIEW,CHECK_OPTION";

	databasefeatures[FEATURE_DATA_DEFINITION_TRANSACTION_BEHAVIOR]=
		"";

	databasefeatures[FEATURE_DDL_INDEX_OPERATIONS]=
		"CREATE_INDEX,DROP_INDEX";

	databasefeatures[FEATURE_DEFAULT_RESULT_SET_HOLDABILITY]=
		"HOLD_CURSORS_OVER_COMMIT";

	databasefeatures[FEATURE_DELETES_ARE_DETECTED]=
		"";

	databasefeatures[FEATURE_DOES_MAX_ROW_SIZE_INCLUDE_BLOBS]=
		"false";

	databasefeatures[FEATURE_DROP_ASSERTION_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_CHARACTER_SET_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_COLLATION_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_DOMAIN_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_SCHEMA_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_TABLE_CLAUSES]=
		"DROP_TABLE";

	databasefeatures[FEATURE_DROP_TRANSLATION_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_VIEW_CLAUSES]=
		"DROP_VIEW";

	databasefeatures[FEATURE_EXTRA_NAME_CHARACTERS]=
		"$";

	databasefeatures[FEATURE_FOREIGN_KEY_DELETE_RULES]=
		"CASCADE,NO_ACTION,SET_DEFAULT,SET_NULL";

	databasefeatures[FEATURE_FOREIGN_KEY_UPDATE_RULES]=
		"CASCADE,NO_ACTION,SET_DEFAULT,SET_NULL";

	databasefeatures[FEATURE_FORWARD_ONLY_CURSOR_ATTRIBUTES]=
		"";

	databasefeatures[FEATURE_GENERATED_KEY_ALWAYS_RETURNED]=
		"false";

	databasefeatures[FEATURE_GRANT_CLAUSES]=
		"DELETE_TABLE,INSERT_TABLE,"
			"REFERENCES_TABLE,REFERENCES_COLUMN,"
			"SELECT_TABLE,UPDATE_COLUMN,UPDATE_TABLE,"
			"WITH_GRANT_OPTION";

	databasefeatures[FEATURE_GROUP_BY_CLAUSES]=
		"BASIC";

	databasefeatures[FEATURE_IDENTIFIER_CASE_STORAGE]=
		"UPPER";

	databasefeatures[FEATURE_IDENTIFIER_QUOTE_STRING]=
		"\"";

	databasefeatures[FEATURE_INDEX_KEYWORDS]=
		"ASC,DESC";

	databasefeatures[FEATURE_INFO_SCHEMA_VIEWS]=
		"";

	databasefeatures[FEATURE_INSERTS_ARE_DETECTED]=
		"";

	databasefeatures[FEATURE_INSERT_OPERATIONS]=
		"INSERT_LITERALS,INSERT_SEARCHED";

	databasefeatures[FEATURE_ISOLATION_LEVELS]=
		"READ_COMMITTED,REPEATABLE_READ,SERIALIZABLE";

	databasefeatures[FEATURE_IS_CATALOG_AT_START]=
		"false";

	databasefeatures[FEATURE_LOCAL_FILE_USAGE]=
		"";

	databasefeatures[FEATURE_LOCATORS_UPDATE_COPY]=
		"true";

	databasefeatures[FEATURE_LOCK_TYPES]=
		"NO_CHANGE";

	databasefeatures[FEATURE_MAX_BINARY_LITERAL_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_CATALOG_NAME_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_CHAR_LITERAL_LENGTH]=
		"32765";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_GROUP_BY]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_INDEX]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_ORDER_BY]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_SELECT]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_TABLE]=
		"32767";

	databasefeatures[FEATURE_MAX_COLUMN_NAME_LENGTH]=
		"31";

	databasefeatures[FEATURE_MAX_CONNECTIONS]=maxconnections;

	databasefeatures[FEATURE_MAX_CURSOR_NAME_LENGTH]=
		"31";

	databasefeatures[FEATURE_MAX_IDENTIFIER_LENGTH]=
		"31";

	databasefeatures[FEATURE_MAX_INDEX_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_PROCEDURE_NAME_LENGTH]=
		"31";

	databasefeatures[FEATURE_MAX_ROW_SIZE]=
		"65531";

	databasefeatures[FEATURE_MAX_SCHEMA_NAME_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_STATEMENTS]=
		"0";

	// isc_dsql_prepare's length is an unsigned short (see MAX_STATEMENT_SIZE)
	databasefeatures[FEATURE_MAX_STATEMENT_LENGTH]=
		"65535";

	databasefeatures[FEATURE_MAX_TABLES_IN_SELECT]=
		"0";

	databasefeatures[FEATURE_MAX_TABLE_NAME_LENGTH]=
		"31";

	databasefeatures[FEATURE_MAX_USER_NAME_LENGTH]=
		"31";

	databasefeatures[FEATURE_NEED_LONG_DATA_LENGTH]=
		"false";

	databasefeatures[FEATURE_NULL_PLUS_NON_NULL_IS_NULL]=
		"true";

	databasefeatures[FEATURE_NULL_SORT_ORDER]=
		"LOW";

	databasefeatures[FEATURE_NUMERIC_FUNCTIONS]=
		"TAN,MOD,LOG,COS,ROUND,SQRT,ASIN,ATAN2,COT,"
			"POWER,LOG10,ABS,FLOOR,DEGREES,CEILING,ACOS,"
			"RADIANS,PI,SIN,SIGN,EXP,ATAN,TRUNCATE";

	databasefeatures[FEATURE_OPEN_CURSORS_ACROSS]=
		"";

	databasefeatures[FEATURE_OPEN_STATEMENTS_ACROSS]=
		"COMMIT,ROLLBACK";

	databasefeatures[FEATURE_OTHERS_DELETES_ARE_VISIBLE]=
		"";

	databasefeatures[FEATURE_OTHERS_INSERTS_ARE_VISIBLE]=
		"";

	databasefeatures[FEATURE_OTHERS_UPDATES_ARE_VISIBLE]=
		"";

	databasefeatures[FEATURE_OUTER_JOINS]=
		"BASIC,FULL,LIMITED";

	databasefeatures[FEATURE_OWN_DELETES_ARE_VISIBLE]=
		"SCROLL_INSENSITIVE,SCROLL_SENSITIVE";

	databasefeatures[FEATURE_OWN_INSERTS_ARE_VISIBLE]=
		"SCROLL_INSENSITIVE,SCROLL_SENSITIVE";

	databasefeatures[FEATURE_OWN_UPDATES_ARE_VISIBLE]=
		"SCROLL_INSENSITIVE,SCROLL_SENSITIVE";

	databasefeatures[FEATURE_PREDICATES]=
		"BETWEEN,COMPARISON,EXISTS,IN,"
			"ISNOTNULL,ISNULL,LIKE,"
			"QUANTIFIED_COMPARISON";

	databasefeatures[FEATURE_PROCEDURE_TERM]=
		"PROCEDURE";

	databasefeatures[FEATURE_QUOTED_IDENTIFIER_CASE_STORAGE]=
		"SENSITIVE";

	databasefeatures[FEATURE_RELATIONAL_JOIN_OPERATORS]=
		"CROSS_JOIN,FULL_OUTER_JOIN,INNER_JOIN,"
			"LEFT_OUTER_JOIN,RIGHT_OUTER_JOIN";

	databasefeatures[FEATURE_RESULT_SET_CONCURRENCIES]=
		"FORWARD_ONLY/READ_ONLY,FORWARD_ONLY/UPDATABLE,"
				"SCROLL_INSENSITIVE/READ_ONLY,"
				"SCROLL_INSENSITIVE/UPDATABLE,"
				"SCROLL_SENSITIVE/READ_ONLY,"
				"SCROLL_SENSITIVE/UPDATABLE";

	databasefeatures[FEATURE_RESULT_SET_HOLDABILITIES]=
		"HOLD_CURSORS_OVER_COMMIT,CLOSE_CURSORS_AT_COMMIT";

	databasefeatures[FEATURE_RESULT_SET_TYPES]=
		"FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE";

	databasefeatures[FEATURE_REVOKE_CLAUSES]=
		"DELETE_TABLE,GRANT_OPTION_FOR,"
			"INSERT_TABLE,REFERENCES_COLUMN,"
			"REFERENCES_TABLE,SELECT_TABLE,"
			"UPDATE_COLUMN,UPDATE_TABLE";

	databasefeatures[FEATURE_ROW_ID_LIFETIME]=
		"ROWID_UNSUPPORTED";

	databasefeatures[FEATURE_ROW_VALUE_CONSTRUCTOR_EXPRESSIONS]=
		"VALUE_EXPRESSION,NULL,ROW_SUBQUERY";

	databasefeatures[FEATURE_SCHEMA_TERM]=
		"";

	databasefeatures[FEATURE_SCHEMA_USAGE]=
		"";

	databasefeatures[FEATURE_SCROLL_CONCURRENCIES]=
		"READ_ONLY";

	databasefeatures[FEATURE_SEARCH_STRING_ESCAPE]=
		"\\";

	databasefeatures[FEATURE_SQL_GRAMMAR_LEVELS]=
		"MINIMUM,CORE,EXTENDED";

	databasefeatures[FEATURE_SQL_KEYWORDS]=
		"ADD,ADMIN,BIT_LENGTH,CURRENT_CONNECTION,"
			"CURRENT_TRANSACTION,DELETING,GDSCODE,INDEX,"
			"INSERTING,LONG,OFFSET,PLAN,POST_EVENT,"
			"RDB$DB_KEY,RDB$RECORD_VERSION,RECORD_VERSION,"
			"RECREATE,RETURNING_VALUES,ROW_COUNT,SQLCODE,"
			"UPDATING,VARIABLE,VIEW,WHILE";

	databasefeatures[FEATURE_SQL_STATE_TYPE]=
		"2";

	databasefeatures[FEATURE_STATIC_CURSOR_ATTRIBUTES]=
		"";

	databasefeatures[FEATURE_STORED_PROGRAMS]=
		"PROCEDURES";

	databasefeatures[FEATURE_STRING_FUNCTIONS]=
		"CHARACTER_LENGTH,LEFT,REPEAT,CONCAT,SUBSTRING,"
			"LENGTH,UCASE,CHAR,ASCII,SPACE,POSITION,LCASE,"
			"LTRIM,RIGHT,INSERT,CHAR_LENGTH,LOCATE,REPLACE,"
			"OCTET_LENGTH,RTRIM";

	databasefeatures[FEATURE_SUBQUERY_USAGE]=
		"COMPARISONS,EXISTS,INS,QUANTIFIEDS";

	databasefeatures[FEATURE_SUPPORTS_BATCH_UPDATES]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_COLUMN_ALIASING]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_CONVERT]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_CORRELATED_SUBQUERIES]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_DESCRIBE_PARAMETER]=
		"false";

	databasefeatures[FEATURE_SUPPORTS_EXPRESSIONS_IN_ORDER_BY]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_GET_GENERATED_KEYS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_INTEGRITY_ENHANCEMENT_FACILITY]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_LIKE_ESCAPE_CLAUSE]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_RESULT_SETS]=
		"false";

	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_TRANSACTIONS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_NAMED_PARAMETERS]=
		"false";

	databasefeatures[FEATURE_SUPPORTS_NON_NULLABLE_COLUMNS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_ORDER_BY_UNRELATED]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_SAVEPOINTS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_SELECT_FOR_UPDATE]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_TRANSACTIONS]=
		"true";

	databasefeatures[FEATURE_SYSTEM_FUNCTIONS]=
		"DATABASE,IFNULL,USER";

	databasefeatures[FEATURE_TABLE_CORRELATION_NAMES]=
		"BASIC";

	databasefeatures[FEATURE_TABLE_TERM]=
		"table";

	databasefeatures[FEATURE_TIME_DATE_ADD_INTERVALS]=
		"";

	databasefeatures[FEATURE_TIME_DATE_DIFF_INTERVALS]=
		"";

	databasefeatures[FEATURE_TIME_DATE_FUNCTIONS]=
		"DAYOFMONTH,MONTHNAME,MONTH,CURRENT_TIMESTAMP,"
			"HOUR,DAYOFYEAR,TIMESTAMPADD,DAYOFWEEK,QUARTER,"
			"TIMESTAMPDIFF,YEAR,CURTIME,NOW,DAYNAME,MINUTE,"
			"SECOND,CURRENT_DATE,CURRENT_TIME,WEEK,CURDATE,"
			"EXTRACT";

	databasefeatures[FEATURE_TIME_DATE_LITERALS]=
		"";

	databasefeatures[FEATURE_TRANSACTION_DDL_DML]=
		"DDL_AND_DML";

	databasefeatures[FEATURE_UNION_CLAUSES]=
		"UNION,UNION_ALL";

	databasefeatures[FEATURE_UPDATES_ARE_DETECTED]=
		"";

	databasefeatures[FEATURE_VALUE_EXPRESSIONS]=
		"CASE,CAST,COALESCE,NULLIF";

	databasefeatures[FEATURE_WHERE_CURRENT_OF_OPERATIONS]=
		"DELETE,UPDATE";

}

void firebirdconnection::handleConnectString() {

	sqlrserverconnection::handleConnectString();

	// override legacy "database" parameter with modern "db" parameter
	const char	*dbtmp=cont->getConnectStringValue("db");
	if (charstring::isNullOrEmpty(dbtmp)) {
		dbtmp=cont->getConnectStringValue("database");
	}
	database=charstring::duplicate(dbtmp);

	const char	*dialectstr=cont->getConnectStringValue("dialect");
	if (dialectstr) {
		dialect=charstring::convertToInteger(dialectstr);
		if (dialect<1) {
			dialect=1;
		}
		if (dialect>3) {
			dialect=3;
		}
	} else {
		dialect=3;
	}

	charset=cont->getConnectStringValue("charset");

	droptemptables=charstring::isYes(
			cont->getConnectStringValue("droptemptables"));

	cont->addGlobalTempTables(
			cont->getConnectStringValue("globaltemptables"));

	const char	*lastinsertidfunc=
			cont->getConnectStringValue("lastinsertidfunction");
	if (lastinsertidfunc) {
		stringbuffer	liiquery;
		liiquery.append("select id from ");
		liiquery.append(lastinsertidfunc);
		lastinsertidquery=liiquery.detachString();
	}

	// firebird doesn't support multi-row fetches
	cont->setFetchAtOnce(1);
}

bool firebirdconnection::logIn(const char **err, const char **warning) {

	// parse the host name from the database
	const char	*colon=charstring::findFirst(database,':');
	delete[] host;
	if (colon) {
		host=charstring::duplicate(database,colon-database);
	} else {
		host=sys::getHostName();
	}

	// initialize a parameter buffer
	char	*dpbptr=dpb;

	// set the parameter buffer version
	*dpbptr=isc_dpb_version1;
	dpbptr++;

	// no idea what this does, something involving the "cache"
	*dpbptr=isc_dpb_num_buffers;
	dpbptr++;
	*dpbptr=1;
	dpbptr++;
	*dpbptr=90;
	dpbptr++;

	// set the character set
	if (charstring::getLength(charset)) {
		*dpbptr=isc_dpb_lc_ctype;
		dpbptr++;
		*dpbptr=charstring::getLength(charset);
		dpbptr++;
		charstring::copy(dpbptr,charset);
		dpbptr+=charstring::getLength(charset);
	}

	// determine the parameter buffer size
	dpbsize=dpbptr-dpb;

	// handle user/password parameters
	const char	*user=cont->getLoginUser();
	if (user) {
		environment::setValue("ISC_USER",user);
	}
	const char	*password=cont->getLoginPassword();
	if (password) {
		environment::setValue("ISC_PASSWORD",password);
	}

	// attach to the database
	db=0L;
	tr=0L;
	if (isc_attach_database(error,charstring::getLength(database),
						database,&db,dpbsize,dpb)) {
		db=0L;

		errormsg.clear();

		char			msg[512];
		const ISC_STATUS	*errstatus=error;
		bool			first=false;
		while (fbInterpret(msg,sizeof(msg),&errstatus)) {
			if (first) {
				errormsg.append(": ");
			}
			errormsg.append(msg);
			first=true;
		}
		*err=errormsg.getString();
		return false;
	}

	// start a transaction
	char	logintpb[8];
	uint16_t	logintpblen=buildTpb(logintpb,false,false);
	if (isc_start_transaction(error,&tr,1,&db,logintpblen,logintpb)) {

		tr=0L;

		errormsg.clear();

		char			msg[512];
		const ISC_STATUS	*errstatus=error;
		bool			first=false;
		while (fbInterpret(msg,sizeof(msg),&errstatus)) {
			if (first) {
				errormsg.append(": ");
			}
			errormsg.append(msg);
			first=true;
		}
		*err=errormsg.getString();
		return false;
	}

	return true;
}

sqlrservercursor *firebirdconnection::newCursor(uint16_t id) {
	return (sqlrservercursor *)new firebirdcursor(
					(sqlrserverconnection *)this,id);
}

void firebirdconnection::deleteCursor(sqlrservercursor *curs) {
	delete (firebirdcursor *)curs;
}

void firebirdconnection::logOut() {

	// close all result sets, and any lobs they still have open, while
	// the attachment is still live
	cont->closeAllResultSets();

	isc_detach_database(error,&db);
}

sqlrtxmodel_t firebirdconnection::getNativeTransactionModel() {
	return SQLRTXMODEL_IMPLICIT;
}

uint16_t firebirdconnection::buildTpb(char *buf, bool readonly,
						bool autocommitflag) {
	uint16_t	len=0;
	buf[len++]=isc_tpb_version3;
	buf[len++]=(readonly)?isc_tpb_read:isc_tpb_write;
	buf[len++]=curtxisolevel;
	// isc_tpb_rec_version is a read-committed sub-option - it isn't
	// meaningful under consistency (snapshot table stability) or
	// concurrency (snapshot), so only include it under read committed
	if (curtxisolevel==isc_tpb_read_committed) {
		buf[len++]=isc_tpb_rec_version;
	}
	// FIXME: vladimir changed this to isc_tpb_nowait.  why?
	buf[len++]=isc_tpb_wait;
	if (autocommitflag) {
		buf[len++]=isc_tpb_autocommit;
	}
	return len;
}

bool firebirdconnection::restartTransaction(bool readonly,
						bool autocommitflag) {
	char		tpbbuf[8];
	uint16_t	tpblen=buildTpb(tpbbuf,readonly,autocommitflag);
	return !isc_commit_transaction(error,&tr) &&
		!isc_start_transaction(error,&tr,1,&db,tpblen,tpbbuf);
}

bool firebirdconnection::setAutoCommitOn() {
	autocommit=true;
	// consume the read-only hint, so it applies to this transaction only
	bool	ro=nextreadonly;
	nextreadonly=false;
	return restartTransaction(ro,true);
}

bool firebirdconnection::setAutoCommitOff() {
	autocommit=false;
	// consume the read-only hint, so it applies to this transaction only
	bool	ro=nextreadonly;
	nextreadonly=false;
	return restartTransaction(ro,false);
}

bool firebirdconnection::setReadOnly(bool readonly) {
	nextreadonly=readonly;
	return true;
}

bool firebirdconnection::setTransactionIsolationLevel(
						const char *isolevel) {

	// map the native isolation level name to a tpb byte, using the same
	// vocabulary as mapIsolationLevel() below; NULL/empty falls back to
	// read committed, firebird's own default, exactly as if nothing had
	// been requested at all
	char	target=isc_tpb_read_committed;
	if (isolevel && isolevel[0]) {
		if (!charstring::compareIgnoringCase(
				isolevel,"snapshot table stability")) {
			target=isc_tpb_consistency;
		} else if (!charstring::compareIgnoringCase(
				isolevel,"snapshot")) {
			target=isc_tpb_concurrency;
		} else if (!charstring::compareIgnoringCase(
				isolevel,"read committed") ||
			!charstring::compareIgnoringCase(isolevel,
				"read committed no record version") ||
			!charstring::compareIgnoringCase(
				isolevel,"read consistency")) {
			target=isc_tpb_read_committed;
		} else {
			// unrecognized - not honored, the current
			// transaction continues at its current level
			return false;
		}
	}

	// nothing to do if the level isn't actually changing
	if (target==curtxisolevel) {
		return true;
	}

	// there's no query-based lever for this (see
	// setIsolationLevelQuery()'s comment) and no client-visible
	// substitute the way read-only has, so the only way to honor a
	// per-transaction isolation request is to restart the transaction
	// right now, before the caller runs any query against it - use (but
	// don't consume) the pending read-only hint here too, so a
	// readonly() call made moments earlier for the same transaction
	// isn't lost; leave it pending so setAutoCommitOn()/Off() still
	// apply it too, exactly as they did before this transaction ever
	// asked for a specific isolation level
	char	previous=curtxisolevel;
	curtxisolevel=target;
	if (!restartTransaction(nextreadonly,autocommit)) {
		curtxisolevel=previous;
		return false;
	}
	return true;
}

bool firebirdconnection::supportsAutoCommit() {
	return true;
}

bool firebirdconnection::commit() {
	if (autocommit) {
		return !isc_commit_retaining(error,&tr);
	} else {
		return restartTransaction(false,false);
	}
}

bool firebirdconnection::rollback() {
	if (autocommit) {
		return !isc_rollback_retaining(error,&tr);
	} else {
		char		tpbbuf[8];
		uint16_t	tpblen=buildTpb(tpbbuf,false,false);
		return !isc_rollback_transaction(error,&tr) &&
			!isc_start_transaction(error,&tr,1,&db,
							tpblen,tpbbuf);
	}
}

void firebirdconnection::getError(char *errorbuffer,
					uint32_t errorbuffersize,
					uint32_t *errorsize,
					int64_t *errorcode,
					bool *liveconnection) {

	// declare a buffer for the error
	errormsg.clear();

	char			msg[512];
	const ISC_STATUS	*pvector=error;

	// get the status message
	while (fbInterpret(msg,sizeof(msg),&pvector)) {
		errormsg.append(msg)->append(" \n");
	}

	// get the sql code
	ISC_LONG	sqlcode=isc_sqlcode(error);

	// return the detailed status-vector message rather than the
	// generic sqlcode text, which hides the real cause (eg. -901)
	if (errormsg.getStringLength()) {

		// The error buffer is reused for the life of the process and
		// safeCopy leaves it unterminated, so the size has to come
		// from the source string.  Measuring the buffer instead would
		// run off the end of a short message into the previous,
		// longer one.
		*errorsize=errormsg.getStringLength();
		if (*errorsize>=errorbuffersize) {
			*errorsize=(errorbuffersize)?errorbuffersize-1:0;
		}
		charstring::safeCopy(errorbuffer,errorbuffersize,
					errormsg.getString(),*errorsize);
		if (errorbuffersize) {
			errorbuffer[*errorsize]='\0';
		}

	} else {

		// isc_sql_interprete writes nothing at all for an unrecognized
		// sqlcode, so clear the buffer first rather than measuring
		// whatever was left in it
		bytestring::zero(errorbuffer,errorbuffersize);
		isc_sql_interprete(sqlcode,errorbuffer,errorbuffersize);
		errorbuffer[errorbuffersize-1]='\0';
		*errorsize=charstring::getLength(errorbuffer);
	}

	// set return values
	*errorcode=sqlcode;
	*liveconnection=!(charstring::contains(
				errormsg.getString(),
				"Error reading data from the connection") ||
			charstring::contains(
				errormsg.getString(),
				"Error writing data to the connection"));
}

bool firebirdconnection::ping() {

	// call isc_database_info to get page_size and num_buffers,
	// this should always be available unless the db is down
	// if we get an error, then return 0, otherwise return 1
	ISC_STATUS	status[20];
	char		dbitems[]={isc_info_page_size,
					isc_info_num_buffers,
					isc_info_end};
	char		resbuffer[40];

	isc_database_info(status,&db,
				sizeof(dbitems),dbitems,
				sizeof(resbuffer),resbuffer);

	return !(status[0]==1 && status[1]);
}

bool firebirdconnection::selectCatalog(const char *catalog) {

	// keep track of the original db and host
	char	*originaldb=this->database;
	char	*originalhost=this->host;

	// reset the db/host
	this->database=charstring::duplicate(catalog);
	this->host=NULL;

	cont->clearError();

	// log out and log back in to the specified database
	logOut();
	const char	*error=NULL;
	const char	*warning=NULL;
	if (!logIn(&error,&warning)) {

		// Set the error, but don't use the error that was returned
		// from logIn() because it will be confusing.  So, we'll
		// just return the generic SQL Relay error for these kinds of
		// things.
		cont->setError(SQLR_ERROR_DBNOTFOUND_STRING,
				SQLR_ERROR_DBNOTFOUND,true);

		// log back in to the original database, we'll assume that works
		delete[] this->database;
		this->database=originaldb;
		this->host=originalhost;
		logOut();
		logIn(&error,&warning);
		return false;
	}

	// clean up
	delete[] originaldb;
	delete[] originalhost;
	return true;
}

char *firebirdconnection::getCurrentCatalog() {
	return charstring::duplicate(database);
}

const char *firebirdconnection::getDbType() {
	return "firebird";
}

const char *firebirdconnection::getDbVersion() {
	ISC_STATUS	status[20];
	char		dbitems[]={isc_info_version,
					isc_info_end};
	char		resbuffer[256];
	if (!isc_database_info(status,&db,
				sizeof(dbitems),dbitems,
				sizeof(resbuffer),resbuffer)) {

		char	*ptr=resbuffer;

		// first byte is isc_info_version
		ptr++;

		// next 2 bytes are size of the isc_info_version data
		ptr=ptr+sizeof(uint16_t);

		// the next byte is the number of lines of text
		stringbuffer	dbvers;
		char	linecount=*ptr;
		ptr++;
		for (char lineindex=0; lineindex<linecount; lineindex++) {

			// the first byte of each line is the size of the line
			char	linelen=*ptr;
			ptr++;

			// then comes the line of text itself
			if (lineindex) {
				dbvers.append('\n');
			}
			dbvers.append(ptr,linelen);
		}

		delete[] dbversion;
		dbversion=dbvers.detachString();
		return dbversion;
	} 
	return "";
}

const char *firebirdconnection::getDbHostName() {
	return host;
}

const char *firebirdconnection::getCatalogListQuery(const char *catalog) {
	// no good way to get a list of catalogs in firebird
	return "select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	trim('') as table_name, "
		"	trim('') as table_type, "
		"	trim('') as remarks, "
		"	null "
		"from "
		"	rdb$database "
		"where "
		"	1=0";
}

const char *firebirdconnection::getSchemaListQuery(const char *catalog,
							const char *schema) {

	// firebird has no schemas
	return "select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	trim('') as table_name, "
		"	trim('') as table_type, "
		"	trim('') as remarks, "
		"	null "
		"from "
		"	rdb$database "
		"where "
		"	1=0";
}

const char *firebirdconnection::getTableTypeListQuery(
						const char *catalog,
						const char *schema,
						const char *tabletypes) {
	tabletypelistquery.clear();

	// select clause
	tabletypelistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	trim('') as table_name, "
		"	trim(table_type), "
		"	trim('') as remarks, "
		"	null ");

	// from clause
	tabletypelistquery.append(
		"from "
		"(select trim('GLOBAL TEMPORARY') "
			"as table_type from rdb$database "
		"union "
		"select trim('SYSTEM TABLE') "
			"as table_type from rdb$database "
		"union "
		"select trim('TABLE') "
			"as table_type from rdb$database "
		"union "
		"select trim('VIEW') "
			"as table_type from rdb$database) ");

	// where clause
	if (!charstring::isNullOrEmpty(tabletypes)) {
		tabletypelistquery.append(
			"where "
			"	trim(table_type) like upper('");
		tabletypelistquery.append(tabletypes);
		tabletypelistquery.append("') ");
	}

	// order by clause
	tabletypelistquery.append(
		"order by "
		"	table_type");

	return tabletypelistquery.getString();
}

const char *firebirdconnection::getGlobalTempTableListQuery() {
	return "select "
		"	trim(rdb$relation_name) "
		"from "
		"	rdb$relations "
		"where "
		"	rdb$system_flag=0 "
		"	and "
		"	rdb$relation_type=4 ";
}

const char *firebirdconnection::getTableListQuery(const char *catalog,
							const char *schema,
							const char *table,
							uint16_t objecttypes) {
	tablelistquery.clear();

	// select clause
	tablelistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	trim(rdb$relation_name) as table_name, "
		"	trim('TABLE') as table_type, "
		"	trim('') as remarks, "
		"	null ");

	// from clause
	tablelistquery.append(
		"from "
		"	rdb$relations ");

	// where clause
	tablelistquery.append(
		"where "
		"	rdb$system_flag=0 ");
	if (schema) {
		tablelistquery.append(
			"	and "
			"	trim(rdb$owner_name) like upper('");
		tablelistquery.append(schema);
		tablelistquery.append("') ");
	}
	if (table) {
		tablelistquery.append(
			"	and "
			"	trim(rdb$relation_name) like upper('");
		tablelistquery.append(table);
		tablelistquery.append("') ");
	}

	// order by clause
	tablelistquery.append(
		"order by "
		"	rdb$owner_name, "
		"	rdb$relation_name");

	return tablelistquery.getString();
}

static const char	*booltype=
			"select "
			"	trim('BOOLEAN') as type_name, "
			"	-7 as data_type, "
			"	1 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('BOOLEAN') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*smallinttype=
			"select "
			"	trim('SMALLINT') as type_name, "
			"	5 as data_type, "
			"	5 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('SMALLINT') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*inttype=
			"select "
			"	trim('INTEGER') as type_name, "
			"	4 as data_type, "
			"	10 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('INTEGER') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*biginttype=
			"select "
			"	trim('BIGINT') as type_name, "
			"	-5 as data_type, "
			"	19 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('BIGINT') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*numerictype=
			"select "
			"	trim('NUMERIC') as type_name, "
			"	2 as data_type, "
			"	18 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('NUMERIC') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*decimaltype=
			"select "
			"	trim('DECIMAL') as type_name, "
			"	3 as data_type, "
			"	18 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('DECIMAL') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*floattype=
			"select "
			"	trim('FLOAT') as type_name, "
			"	6 as data_type, "
			"	7 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('FLOAT') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*doubleprectype=
			"select "
			"	trim('DOUBLE PRECISION') as type_name, "
			"	8 as data_type, "
			"	15 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('DOUBLE PRECISION') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*chartype=
			"select "
			"	trim('CHAR') as type_name, "
			"	1 as data_type, "
			"	32767 as column_size, "
			"	trim('') as literal_prefix, "
			"	trim('') as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('CHAR') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*varchartype=
			"select "
			"	trim('VARCHAR') as type_name, "
			"	12 as data_type, "
			"	32765 as column_size, "
			"	trim('') as literal_prefix, "
			"	trim('') as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('VARCHAR') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*datetype=
			"select "
			"	trim('DATE') as type_name, "
			"	91 as data_type, "
			"	10 as column_size, "
			"	trim('') as literal_prefix, "
			"	trim('') as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('DATE') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*timetype=
			"select "
			"	trim('TIME') as type_name, "
			"	92 as data_type, "
			"	8 as column_size, "
			"	trim('') as literal_prefix, "
			"	trim('') as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('TIME') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*timestamptype=
			"select "
			"	trim('TIMESTAMP') as type_name, "
			"	93 as data_type, "
			"	19 as column_size, "
			"	trim('') as literal_prefix, "
			"	trim('') as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('TIMESTAMP') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*blobtype=
			"select "
			"	trim('BLOB') as type_name, "
			"	-4 as data_type, "
			"	2147483647 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	0 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('BLOB') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*blobsubtexttype=
			"select "
			"	trim('BLOB SUB_TYPE TEXT') as type_name, "
			"	-1 as data_type, "
			"	2147483647 as column_size, "
			"	trim('') as literal_prefix, "
			"	trim('') as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	1 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('BLOB SUB_TYPE TEXT') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

const char *firebirdconnection::getTypeInfoListQuery(const char *catalog,
							const char *schema,
							const char *type) {

	if (!charstring::compare(type,"*")) {
		if (!typeinfolistquery.getSize()) {
			typeinfolistquery.append(booltype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(smallinttype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(inttype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(biginttype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(numerictype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(decimaltype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(floattype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(doubleprectype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(chartype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(varchartype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(datetype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(timetype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(timestamptype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(blobtype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(blobsubtexttype);
		}
		return typeinfolistquery.getString();
	} else if (!charstring::compareIgnoringCase(type,"boolean")) {
		return booltype;
	} else if (!charstring::compareIgnoringCase(type,"smallint")) {
		return smallinttype;
	} else if (!charstring::compareIgnoringCase(type,"integer")) {
		return inttype;
	} else if (!charstring::compareIgnoringCase(type,"int")) {
		return inttype;
	} else if (!charstring::compareIgnoringCase(type,"bigint")) {
		return biginttype;
	} else if (!charstring::compareIgnoringCase(type,"numeric")) {
		return numerictype;
	} else if (!charstring::compareIgnoringCase(type,"decimal")) {
		return decimaltype;
	} else if (!charstring::compareIgnoringCase(type,"float")) {
		return floattype;
	} else if (!charstring::compareIgnoringCase(type,"double precision")) {
		return doubleprectype;
	} else if (!charstring::compareIgnoringCase(type,"char")) {
		return chartype;
	} else if (!charstring::compareIgnoringCase(type,"character")) {
		return chartype;
	} else if (!charstring::compareIgnoringCase(type,"varchar")) {
		return varchartype;
	} else if (!charstring::compareIgnoringCase(type,"character varying")) {
		return varchartype;
	} else if (!charstring::compareIgnoringCase(type,"date")) {
		return datetype;
	} else if (!charstring::compareIgnoringCase(type,"time")) {
		return timetype;
	} else if (!charstring::compareIgnoringCase(type,"timestamp")) {
		return timestamptype;
	} else if (!charstring::compareIgnoringCase(type,"blob")) {
		return blobtype;
	} else if (!charstring::compareIgnoringCase(type,"blob sub_type text")) {
		return blobsubtexttype;
	}
	return NULL;
}

const char *firebirdconnection::getColumnListQuery(const char *catalog,
							const char *schema,
							const char *table,
							const char *column) {

	columnlistquery.clear();

	// select clause
	columnlistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	trim(rf.rdb$relation_name) as table_name, "
		"	trim(rf.rdb$field_name) as column_name, "
		"	fd.rdb$field_type as data_type,"
		"	trim(case fd.rdb$field_type "
		"		when 261 then 'BLOB SUB_TYPE BINARY' "
		"		when 14 then 'CHAR' "
		"		when 40 then 'CSTRING' "
		"		when 11 then 'D_FLOAT' "
		"		when 27 then 'DOUBLE PRECISION' "
		"		when 10 then 'FLOAT' "
		"		when 16 then case fd.rdb$field_sub_type "
		"			when 1 then 'NUMERIC' "
		"			when 2 then 'DECIMAL' "
		"			else 'BIGINT' "
		"		end "
		"		when 8 then 'INTEGER' "
		"		when 9 then 'QUAD' "
		"		when 7 then 'SMALLINT' "
		"		when 12 then 'DATE' "
		"		when 13 then 'TIME' "
		"		when 35 then 'TIMESTAMP' "
		"		when 37 then 'VARCHAR' "
		"		else 'UNKNOWN' "
		"	end) as type_name, "
		"	fd.rdb$field_length as column_size, "
		"	fd.rdb$field_length as buffer_length, "
		"	fd.rdb$field_scale as decimal_digits, "
		"	10 as num_prec_radix, "
		"	case rf.rdb$null_flag "
		"		when 1 then 0 "
		"		else 1 "
		"	end as nullable, "
		"	trim(case "
		"		when rf.rdb$identity_type is not null "
		"			then 'auto_increment ' || "
		"				coalesce(rf.rdb$description,'') "
		"		else coalesce(rf.rdb$description,'') "
		"	end) as remarks, "
		"	trim(rf.rdb$default_source) as column_default, "
		"	null as sql_data_type, "
		"	null as sql_datetime_sub, "
		"	fd.rdb$character_length as char_octet_length, "
		"	rf.rdb$field_position as ordinal_position, "
		"	trim(case rf.rdb$null_flag "
		"		when 1 then 'NO' "
		"		else 'YES' "
		"	end) as is_nullable, "
		"	fd.rdb$field_precision as numeric_precision, "
		"	trim(case ck.key_priority "
		"		when 1 then 'PRI' "
		"		when 2 then 'UNI' "
		"		when 3 then 'MUL' "
		"		else null "
		"	end) as column_key, "
		"	trim(case "
		"		when rf.rdb$identity_type is not null then 'YES' "
		"		else 'NO' "
		"	end) as is_autoincrement, "
		"	null ");

	// from clause
	columnlistquery.append(
		"from "
		"	rdb$relation_fields rf "
		"join "
		"	rdb$relations rl "
		"	on "
		"	rl.rdb$relation_name=rf.rdb$relation_name "
		"left join "
		"	rdb$fields fd "
		"	on "
		"	fd.rdb$field_name=rf.rdb$field_source "
		"left join ( "
		"	select "
		"		rc.rdb$relation_name, "
		"		ix.rdb$field_name, "
		"		min(case rc.rdb$constraint_type "
		"			when 'PRIMARY KEY' then 1 "
		"			when 'UNIQUE' then 2 "
		"			when 'FOREIGN KEY' then 3 "
		"		end) as key_priority "
		"	from "
		"		rdb$relation_constraints rc, "
		"		rdb$index_segments ix "
		"	where "
		"		rc.rdb$index_name=ix.rdb$index_name "
		"		and "
		"		rc.rdb$constraint_type in "
		"			('PRIMARY KEY','UNIQUE','FOREIGN KEY') "
		"	group by "
		"		rc.rdb$relation_name, "
		"		ix.rdb$field_name "
		") ck "
		"on "
		"	rf.rdb$relation_name=ck.rdb$relation_name "
		"	and "
		"	rf.rdb$field_name=ck.rdb$field_name ");

	// where clause
	bool	first=true;
	if (!charstring::isNullOrEmpty(schema)) {
		columnlistquery.append(
			"where "
			"	trim(rl.rdb$owner_name) like upper('");
		columnlistquery.append(schema);
		columnlistquery.append("') ");
		first=false;
	}

	if (!charstring::isNullOrEmpty(table)) {
		if (first) {
			columnlistquery.append("where ");
			first=false;
		} else {
			columnlistquery.append("	and ");
		}
		columnlistquery.append(
			"	trim(rf.rdb$relation_name) like upper('");
		columnlistquery.append(table);
		columnlistquery.append("') ");
	}

	if (!charstring::isNullOrEmpty(column)) {
		if (first) {
			columnlistquery.append("where ");
			first=false;
		} else {
			columnlistquery.append("	and ");
		}
		columnlistquery.append(
			"	trim(rf.rdb$field_name) like upper('");
		columnlistquery.append(column);
		columnlistquery.append("') ");
	}

	// order by clause
	columnlistquery.append(
		"order by "
		"	rf.rdb$field_position");

	return columnlistquery.getString();
}

const char *firebirdconnection::getPrimaryKeysListQuery(const char *catalog,
							const char *schema,
							const char *table) {

	primarykeyslistquery.clear();

	// select clause
	primarykeyslistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	trim(rc.rdb$relation_name) as table_name, "
		"	trim(isg.rdb$field_name) as column_name, "
		"	isg.rdb$field_position+1 as key_seq, "
		"	trim(rc.rdb$constraint_name) as pk_name, "
		"	null ");

	// from clause
	primarykeyslistquery.append(
		"from "
		"	rdb$relation_constraints rc, "
		"	rdb$index_segments isg ");

	// where clause
	primarykeyslistquery.append(
		"where "
		"	rc.rdb$constraint_type='PRIMARY KEY' "
		"	and "
		"	rc.rdb$index_name=isg.rdb$index_name ");
	if (!charstring::isNullOrEmpty(table)) {
		primarykeyslistquery.append(
			"	and "
			"	trim(rc.rdb$relation_name) like upper('");
		primarykeyslistquery.append(table);
		primarykeyslistquery.append("') ");
	}

	// order by clause
	primarykeyslistquery.append(
		"order by "
		"	rc.rdb$relation_name, "
		"	isg.rdb$field_position");

	return primarykeyslistquery.getString();
}

const char *firebirdconnection::getKeyAndIndexListQuery(const char *catalog,
							const char *schema,
							const char *table) {

	keyandindexlistquery.clear();

	// select clause
	keyandindexlistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	trim(i.rdb$relation_name) as table_name, "
		"	case i.rdb$unique_flag "
		"		when 1 then 0 "
		"		else 1 "
		"	end as non_unique, "
		"	trim('') as index_qualifier, "
		"	trim(i.rdb$index_name) as index_name, "
		"	3 as type, "
		"	isg.rdb$field_position+1 as ordinal_position, "
		"	trim(isg.rdb$field_name) as column_name, "
		"	trim(case i.rdb$index_type "
		"		when 1 then 'D' "
		"		else 'A' "
		"	end) as asc_or_desc, "
		"	i.rdb$statistics as cardinality, "
		"	null as pages, "
		"	null as filter_condition, "
		"	null ");

	// from clause
	keyandindexlistquery.append(
		"from "
		"	rdb$indices i, "
		"	rdb$index_segments isg ");

	// where clause
	keyandindexlistquery.append(
		"where "
		"	i.rdb$index_name=isg.rdb$index_name ");
	if (!charstring::isNullOrEmpty(table)) {
		keyandindexlistquery.append(
			"	and "
			"	trim(i.rdb$relation_name) like upper('");
		keyandindexlistquery.append(table);
		keyandindexlistquery.append("') ");
	}

	// order by clause
	keyandindexlistquery.append(
		"order by "
		"	i.rdb$relation_name, "
		"	i.rdb$index_name, "
		"	isg.rdb$field_position");

	return keyandindexlistquery.getString();
}

const char *firebirdconnection::getProcedureListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure) {

	procedurelistquery.clear();

	// select clause
	procedurelistquery.append(
		"select "
		"	null as procedure_cat, "
		"	trim(rdb$owner_name) as procedure_schem, "
		"	trim(rdb$procedure_name) as procedure_name, "
		"	0 as num_input_params, "
		"	0 as num_output_params, "
		"	0 as num_result_sets, "
		"	trim(rdb$description) as remarks, "
		"	trim('1') as procedure_type, "
		"	null ");

	// from clause
	procedurelistquery.append(
		"from "
		"	rdb$procedures ");

	// where clause
	if (!charstring::isNullOrEmpty(schema) ||
		!charstring::isNullOrEmpty(procedure)) {

		bool	first=true;
		procedurelistquery.append("where ");
		if (!charstring::isNullOrEmpty(schema)) {
			procedurelistquery.append(
				"trim(rdb$owner_name) like upper('");
			procedurelistquery.append(schema);
			procedurelistquery.append("') ");
			first=false;
		}
		if (!charstring::isNullOrEmpty(procedure)) {
			if (!first) {
				procedurelistquery.append("and ");
			}
			procedurelistquery.append(
				"trim(rdb$procedure_name) like upper('");
			procedurelistquery.append(procedure);
			procedurelistquery.append("') ");
		}
	}

	// order by clause
	procedurelistquery.append(
		"order by "
		"	rdb$owner_name, "
		"	rdb$procedure_name");

	return procedurelistquery.getString();
}

const char *firebirdconnection::getProcedureParameterListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure) {

	procedureparameterlistquery.clear();

	// select clause
	procedureparameterlistquery.append(
		"select "
		"	null as procedure_cat, "
		"	trim('') as procedure_schem, "
		"	trim(pp.rdb$procedure_name) as procedure_name, "
		"	trim(pp.rdb$parameter_name) as column_name, "
		"	case pp.rdb$parameter_type "
		"		when 0 then 1 "
		"		when 1 then 4 "
		"		else 0 "
		"	end as column_type, "
		"	trim('') as data_type, "
		"	trim(case f.rdb$field_type "
		"		when 261 then 'BLOB SUB_TYPE BINARY' "
		"		when 14 then 'CHAR' "
		"		when 40 then 'CSTRING' "
		"		when 11 then 'D_FLOAT' "
		"		when 27 then 'DOUBLE PRECISION' "
		"		when 10 then 'FLOAT' "
		"		when 16 then case f.rdb$field_sub_type "
		"			when 1 then 'NUMERIC' "
		"			when 2 then 'DECIMAL' "
		"			else 'BIGINT' "
		"		end "
		"		when 8 then 'INTEGER' "
		"		when 9 then 'QUAD' "
		"		when 7 then 'SMALLINT' "
		"		when 12 then 'DATE' "
		"		when 13 then 'TIME' "
		"		when 35 then 'TIMESTAMP' "
		"		when 37 then 'VARCHAR' "
		"		else 'UNKNOWN' "
		"	end) as type_name, "
		"	f.rdb$field_length as column_size, "
		"	null as buffer_length, "
		"	f.rdb$field_scale as decimal_digits, "
		"	10 as num_prec_radix, "
		"	1 as nullable, "
		"	trim(pp.rdb$description) as remarks, "
		"	trim(pp.rdb$default_source) as column_def, "
		"	null as sql_data_type, "
		"	null as sql_datetime_sub, "
		"	f.rdb$character_length as char_octet_length, "
		"	pp.rdb$parameter_number+1 as ordinal_position, "
		"	trim('YES') as is_nullable, "
		"	null ");

	// from clause
	procedureparameterlistquery.append(
		"from "
		"	rdb$procedure_parameters pp "
		"left join "
		"	rdb$fields f "
		"on "
		"	pp.rdb$field_source=f.rdb$field_name ");

	// where clause
	if (!charstring::isNullOrEmpty(procedure)) {
		procedureparameterlistquery.append(
			"where "
			"	trim(pp.rdb$procedure_name) like upper('");
		procedureparameterlistquery.append(procedure);
		procedureparameterlistquery.append("') ");
	}

	// order by clause
	procedureparameterlistquery.append(
		"order by "
		"	pp.rdb$procedure_name, "
		"	pp.rdb$parameter_type desc, "
		"	pp.rdb$parameter_number");

	return procedureparameterlistquery.getString();
}

const char *firebirdconnection::getBindFormat() {
	return "?";
}

const char *firebirdconnection::getNextvalFormat() {
	return "next value for %s";
}

const char *firebirdconnection::getCurrentUserQuery() {
	return "select current_user from rdb$database";
}

const char *firebirdconnection::getLastInsertIdQuery() {
	return lastinsertidquery;
}

bool firebirdconnection::setIsolationLevel(const char *isolevel) {

	// the base implementation would run setIsolationLevelQuery() as a
	// query, but "set transaction %s" starts a transaction in firebird
	// rather than altering the current one - the same problem the
	// per-transaction hint below exists to avoid.  Route the connection-
	// wide default through the same mechanism instead, so curtxisolevel
	// (what buildTpb() actually uses) never falls out of sync with what
	// was last requested here.
	if (!charstring::getLength(isolevel)) {
		return false;
	}
	return setTransactionIsolationLevel(isolevel);
}

const char *firebirdconnection::setIsolationLevelQuery() {
	return "set transaction %s";
}

const char *firebirdconnection::getIsolationLevelQuery() {
	return "select "
		"	case mon$isolation_mode "
		"		when 0 then "
		"cast('snapshot table stability' as varchar(24)) "
		"		when 1 then "
		"cast('snapshot' as varchar(8)) "
		"		when 2 then "
		"cast('read committed' as varchar(14)) "
		"		when 3 then "
		"cast('read committed no record version' as varchar(32)) "
		"		when 4 then "
		"cast('read consistency' as varchar(11)) "
		"	end "
		"from "
		"	mon$transactions "
		"where "
		"	mon$transaction_id=current_transaction";
}

const char *firebirdconnection::mapIsolationLevel(
				const char *isolevel,
				sqlrserverisolationlevelformat_t fromformat,
				sqlrserverisolationlevelformat_t toformat) {
	if (fromformat==toformat) {
		return isolevel;
	}
	if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE) {
		if (!charstring::compare(isolevel,
				"TRANSACTION_READ_COMMITTED")) {
			return "read committed";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_REPEATABLE_READ")) {
			return "snapshot";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_SERIALIZABLE")) {
			return "snapshot table stability";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC) {
		if (!charstring::compareIgnoringCase(
				isolevel,"read committed") ||
			!charstring::compareIgnoringCase(
				isolevel,"read committed no record version") ||
			!charstring::compareIgnoringCase(
				isolevel,"read consistency")) {
			return "TRANSACTION_READ_COMMITTED";
		}
		if (!charstring::compareIgnoringCase(
				isolevel,"snapshot")) {
			return "TRANSACTION_REPEATABLE_READ";
		}
		if (!charstring::compareIgnoringCase(
				isolevel,"snapshot table stability")) {
			return "TRANSACTION_SERIALIZABLE";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE) {
		if (!charstring::compare(isolevel,
				"SQL_TXN_READ_COMMITTED")) {
			return "read committed";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_REPEATABLE_READ")) {
			return "snapshot";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_SERIALIZABLE")) {
			return "snapshot table stability";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC) {
		if (!charstring::compareIgnoringCase(
				isolevel,"read committed") ||
			!charstring::compareIgnoringCase(
				isolevel,"read committed no record version") ||
			!charstring::compareIgnoringCase(
				isolevel,"read consistency")) {
			return "SQL_TXN_READ_COMMITTED";
		}
		if (!charstring::compareIgnoringCase(
				isolevel,"snapshot")) {
			return "SQL_TXN_REPEATABLE_READ";
		}
		if (!charstring::compareIgnoringCase(
				isolevel,"snapshot table stability")) {
			return "SQL_TXN_SERIALIZABLE";
		}
	}
	return isolevel;
}

const char * const *firebirdconnection::getDatabaseFeatures() {
	cont->capDatabaseFeatures(databasefeatures);
	return databasefeatures;
}

const char *firebirdconnection::getNoopQuery() {
	return "execute block as begin end";
}

firebirdcursor::firebirdcursor(sqlrserverconnection *conn, uint16_t id) :
						sqlrservercursor(conn,id) {

	firebirdconn=(firebirdconnection *)conn;

	outsqlda=NULL;
	outsqldabuffer=NULL;
	field=NULL;
	fieldcount=0;
	allocateResultSetBuffers(conn->cont->getMaxColumnCount());

	maxbindcount=conn->cont->getConfig()->getMaxBindCount();
	outbindcount=0;

	// set up input binds
	inbindsqlda=(XSQLDA ISC_FAR *)new byte_t[XSQLDA_LENGTH(maxbindcount)];
	inbindsqlda->version=SQLDA_VERSION1;
	inbindsqlda->sqln=maxbindcount;
	inbindts=new ISC_TIMESTAMP[maxbindcount];
	inbindblobid=new ISC_QUAD[maxbindcount];
	inbindarrayid=new ISC_QUAD[maxbindcount];
	inbindblobhandle=new isc_blob_handle[maxbindcount];
	inbinddescribe=new inbinddescribestruct[maxbindcount];
	inbindcountfromprepare=0;


	// set up output binds
	outbindsqlda=(XSQLDA ISC_FAR *)new byte_t[XSQLDA_LENGTH(maxbindcount)];
	outbindsqlda->version=SQLDA_VERSION1;
	outbindsqlda->sqln=maxbindcount;
	outbindblobid=new ISC_QUAD[maxbindcount];
	outbindblobhandle=new isc_blob_handle[maxbindcount];
	outbindblobisopen=new bool[maxbindcount];
	outdatebind=new datebind[maxbindcount];

	querytype=0L;
	stmt=0L;

	queryisexecsp=false;
	bindformaterror=false;
	querytoolarge=false;
	resultsetdescribed=false;

	setCreateTempTablePattern("(create|CREATE)[ 	\n\r]+(global|GLOBAL)[ 	\n\r]+(temporary|TEMPORARY)[ 	\n\r]+(table|TABLE)[ 	\n\r]+");
	executeprocedure.setPattern("(execute|EXECUTE)[ 	\n\r]+(procedure|PROCEDURE)");
	executeprocedure.study();
}

firebirdcursor::~firebirdcursor() {
	delete[] inbindsqlda;
	delete[] inbindts;
	delete[] inbindblobid;
	delete[] inbindarrayid;
	delete[] inbindblobhandle;
	delete[] inbinddescribe;

	delete[] outbindsqlda;
	delete[] outbindblobid;
	delete[] outbindblobhandle;
	delete[] outbindblobisopen;
	delete[] outdatebind;

	freeResultSetBuffers();
}

void firebirdcursor::allocateResultSetBuffers(int32_t columncount) {

	freeResultSetBuffers();

	if (!columncount) {
		outsqldabuffer=new byte_t[XSQLDA_LENGTH(1)];
		bytestring::zero(outsqldabuffer,XSQLDA_LENGTH(1));
		outsqlda=(XSQLDA ISC_FAR *)outsqldabuffer;
		outsqlda->version=SQLDA_VERSION1;
		outsqlda->sqln=1;
	} else {
		outsqldabuffer=new byte_t[XSQLDA_LENGTH(columncount)];
		bytestring::zero(outsqldabuffer,XSQLDA_LENGTH(columncount));
		outsqlda=(XSQLDA ISC_FAR *)outsqldabuffer;
		outsqlda->version=SQLDA_VERSION1;
		outsqlda->sqln=columncount;
		field=new fieldstruct[columncount];
		for (int32_t i=0; i<columncount; i++) {
			field[i].textbuffer=new char[
					conn->cont->getMaxFieldSize()+1];
			// init these so closeResultSet() can scan for open
			// lobs and buffered arrays on a slot
			// describeResultSet() hasn't described yet
			field[i].sqlrtype=UNKNOWN_DATATYPE;
			field[i].blobisopen=false;
			field[i].arraydescvalid=false;
			field[i].arraybuffer=NULL;
			field[i].arraybuffersize=0;
			field[i].arrayelementcount=0;
			field[i].arrayelementsize=0;
			field[i].arraybuffervalid=false;
		}
		fieldcount=columncount;
	}
}

void firebirdcursor::freeResultSetBuffers() {

	delete[] outsqldabuffer;
	outsqldabuffer=NULL;
	outsqlda=NULL;

	for (int32_t i=0; i<fieldcount; i++) {
		delete[] field[i].textbuffer;
		delete[] field[i].arraybuffer;
	}
	delete[] field;
	field=NULL;
	fieldcount=0;
}

bool firebirdcursor::prepareQuery(const char *query, uint32_t size) {

	resultsetdescribed=false;

	// reject queries too large for isc_dsql_prepare
	querytoolarge=false;
	if (size>MAX_STATEMENT_SIZE) {
		querytoolarge=true;
		return false;
	}

	// initialize column count
	outsqlda->sqld=0;

	// are we executing a stored procedure
	queryisexecsp=executeprocedure.match(query);

	// reset the bind format error flag
	bindformaterror=false;

	// free the old statement if it exists
	if (stmt) {
		isc_dsql_free_statement(firebirdconn->error,
						&stmt,DSQL_drop);
		stmt=0L;
	}

	// allocate a cursor handle
	if (isc_dsql_allocate_statement(firebirdconn->error,
					&firebirdconn->db,&stmt)) {
		return false;
	}

	// prepare the cursor
	if (isc_dsql_prepare(firebirdconn->error,
				&firebirdconn->tr,
				&stmt,size,(char *)query,
				firebirdconn->dialect,
				(queryisexecsp)?outbindsqlda:outsqlda)) {
		return false;
	}

	// null output bind sqldata pointers so we can detect
	// which ones were set by outputBind later
	if (queryisexecsp) {
		for (uint16_t i=0; i<outbindsqlda->sqld; i++) {
			outbindsqlda->sqlvar[i].sqldata=NULL;
		}
	}

	// get the cursor type
	char	typeitem[]={isc_info_sql_stmt_type};
	char	resbuffer[1024];
	if (isc_dsql_sql_info(firebirdconn->error,&stmt,
				sizeof(typeitem),typeitem,
				1024,resbuffer)) {
		return false;
	}

	// (modern versions of isc_vax_integer take a const char * parameter,
	// but old versions take char * and this cast works with both)
	ISC_LONG	len=isc_vax_integer((char *)(resbuffer+1),2);
	querytype=isc_vax_integer((char *)(resbuffer+3),len);

	// find bind parameters, if any.  sqln is how many sqlvar entries
	// isc_dsql_describe_bind() may fill in, so it has to be reset to the
	// number allocated first.  The previous statement's prepare left it
	// at that statement's parameter count, and a lower count here makes
	// describe_bind report sqld but quietly leave the sqlvar entries
	// alone, so they'd still describe the previous statement.
	inbindsqlda->sqld=0;
	inbindsqlda->sqln=maxbindcount;
	if (isc_dsql_describe_bind(firebirdconn->error,&stmt,1,inbindsqlda)) {
		return false;
	}

	// a describe into an sqlda with too few slots reports the true
	// bind count in sqld but doesn't fill sqlvar beyond sqln - bail
	// with an error rather than let sqln, inbindcountfromprepare, or
	// the copy loop below run past the maxbindcount-sized allocation
	if (inbindsqlda->sqld>(int32_t)maxbindcount) {
		stringbuffer	err;
		err.append(SQLR_ERROR_MAXBINDCOUNT_STRING);
		err.append(" (")->append(maxbindcount);
		err.append('<')->append(inbindsqlda->sqld)->append(')');
		conn->cont->setError(this,err.getString(),
				SQLR_ERROR_MAXBINDCOUNT,true);
		return false;
	}
	inbindsqlda->sqln=inbindsqlda->sqld;

	// copy the describe out now - inputBind() overwrites inbindsqlda's
	// sqlvar entries in place on its way to execute
	inbindcountfromprepare=inbindsqlda->sqld;
	for (uint16_t i=0; i<inbindcountfromprepare; i++) {
		inbinddescribe[i].sqltype=inbindsqlda->sqlvar[i].sqltype;
		inbinddescribe[i].sqlscale=inbindsqlda->sqlvar[i].sqlscale;
		inbinddescribe[i].sqlsubtype=inbindsqlda->sqlvar[i].sqlsubtype;
		inbinddescribe[i].sqllen=inbindsqlda->sqlvar[i].sqllen;

		// the names too - an array bind needs them to look the
		// column's shape up, and inputBind() blanks them
		short	relnamelength=inbindsqlda->sqlvar[i].relname_length;
		if (relnamelength<0 ||
			relnamelength>(short)
				sizeof(inbindsqlda->sqlvar[i].relname)) {
			relnamelength=0;
		}
		inbinddescribe[i].relnamelength=relnamelength;
		charstring::copy(inbinddescribe[i].relname,
					inbindsqlda->sqlvar[i].relname,
					relnamelength);
		inbinddescribe[i].relname[relnamelength]='\0';

		short	sqlnamelength=inbindsqlda->sqlvar[i].sqlname_length;
		if (sqlnamelength<0 ||
			sqlnamelength>(short)
				sizeof(inbindsqlda->sqlvar[i].sqlname)) {
			sqlnamelength=0;
		}
		inbinddescribe[i].sqlnamelength=sqlnamelength;
		charstring::copy(inbinddescribe[i].sqlname,
					inbindsqlda->sqlvar[i].sqlname,
					sqlnamelength);
		inbinddescribe[i].sqlname[sqlnamelength]='\0';
	}

	// describe the result set now, so the column info is valid straight
	// after the prepare, but skip the statement types that executeQuery()
	// returns early for
	if (!queryIsCommitOrRollback() && !queryisexecsp) {
		if (!describeResultSet()) {
			return false;
		}
		resultsetdescribed=true;
	}

	return true;
}

bool firebirdcursor::inputBind(const char *variable,
					uint16_t variablesize,
					const char *value,
					uint32_t valuesize,
					int16_t *isnull) {

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}
	inbindsqlda->sqlvar[index].sqltype=SQL_TEXT+1;
	inbindsqlda->sqlvar[index].sqlscale=0;
	inbindsqlda->sqlvar[index].sqlsubtype=0;
	inbindsqlda->sqlvar[index].sqllen=valuesize;
	inbindsqlda->sqlvar[index].sqldata=(char *)value;
	inbindsqlda->sqlvar[index].sqlind=isnull;
	inbindsqlda->sqlvar[index].sqlname_length=0;
	inbindsqlda->sqlvar[index].sqlname[0]='\0';
	inbindsqlda->sqlvar[index].relname_length=0;
	inbindsqlda->sqlvar[index].relname[0]='\0';
	inbindsqlda->sqlvar[index].ownname_length=0;
	inbindsqlda->sqlvar[index].ownname[0]='\0';
	inbindsqlda->sqlvar[index].aliasname_length=0;
	inbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::inputBind(const char *variable,
					uint16_t variablesize,
					int64_t *value) {

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}
	inbindsqlda->sqlvar[index].sqltype=SQL_INT64;
	inbindsqlda->sqlvar[index].sqlscale=0;
	inbindsqlda->sqlvar[index].sqlsubtype=0;
	inbindsqlda->sqlvar[index].sqllen=sizeof(int64_t);
	inbindsqlda->sqlvar[index].sqldata=(char *)value;
	inbindsqlda->sqlvar[index].sqlind=(short *)NULL;
	inbindsqlda->sqlvar[index].sqlname_length=0;
	inbindsqlda->sqlvar[index].sqlname[0]='\0';
	inbindsqlda->sqlvar[index].relname_length=0;
	inbindsqlda->sqlvar[index].relname[0]='\0';
	inbindsqlda->sqlvar[index].ownname_length=0;
	inbindsqlda->sqlvar[index].ownname[0]='\0';
	inbindsqlda->sqlvar[index].aliasname_length=0;
	inbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::inputBind(const char *variable,
					uint16_t variablesize,
					double *value,
					uint32_t precision,
					uint32_t scale) {

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}
	inbindsqlda->sqlvar[index].sqltype=SQL_DOUBLE;
	inbindsqlda->sqlvar[index].sqlscale=scale;
	inbindsqlda->sqlvar[index].sqlsubtype=0;
	inbindsqlda->sqlvar[index].sqllen=sizeof(double);
	inbindsqlda->sqlvar[index].sqldata=(char *)value;
	inbindsqlda->sqlvar[index].sqlind=(short *)NULL;
	inbindsqlda->sqlvar[index].sqlname_length=0;
	inbindsqlda->sqlvar[index].sqlname[0]='\0';
	inbindsqlda->sqlvar[index].relname_length=0;
	inbindsqlda->sqlvar[index].relname[0]='\0';
	inbindsqlda->sqlvar[index].ownname_length=0;
	inbindsqlda->sqlvar[index].ownname[0]='\0';
	inbindsqlda->sqlvar[index].aliasname_length=0;
	inbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::inputBind(const char *variable,
					uint16_t variablesize,
					int64_t year,
					int16_t month,
					int16_t day,
					int16_t hour,
					int16_t minute,
					int16_t second,
					int32_t microsecond,
					const char *tz,
					bool isnegative,
					int16_t *isnull) {

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}

	// build an ISC_TIMESTAMP
	tm	t;
	t.tm_sec=second;
	t.tm_min=minute;
	t.tm_hour=hour;
	t.tm_mday=day;
	t.tm_mon=month-1;
	t.tm_year=year-1900;
	isc_encode_timestamp(&t,&(inbindts[index]));

	// low bit set makes firebird consult sqlind
	inbindsqlda->sqlvar[index].sqltype=SQL_TIMESTAMP+1;
	inbindsqlda->sqlvar[index].sqlscale=0;
	inbindsqlda->sqlvar[index].sqlsubtype=0;
	inbindsqlda->sqlvar[index].sqllen=sizeof(ISC_TIMESTAMP);
	inbindsqlda->sqlvar[index].sqldata=(char *)&(inbindts[index]);
	inbindsqlda->sqlvar[index].sqlind=isnull;
	inbindsqlda->sqlvar[index].sqlname_length=0;
	inbindsqlda->sqlvar[index].sqlname[0]='\0';
	inbindsqlda->sqlvar[index].relname_length=0;
	inbindsqlda->sqlvar[index].relname[0]='\0';
	inbindsqlda->sqlvar[index].ownname_length=0;
	inbindsqlda->sqlvar[index].ownname[0]='\0';
	inbindsqlda->sqlvar[index].aliasname_length=0;
	inbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::inputBindBlob(const char *variable,
					uint16_t variablesize,
					const char *value,
					uint32_t valuesize,
					int16_t *isnull) {
	// no segment boundaries to preserve, so write the value in
	// MAX_LOB_CHUNK_SIZE-sized pieces of our own choosing
	return inputBindBlob(variable,variablesize,value,valuesize,
					NULL,0,isnull);
}

bool firebirdcursor::inputBindBlob(const char *variable,
					uint16_t variablesize,
					const char *value,
					uint32_t valuesize,
					const uint32_t *segmentlengths,
					uint16_t segmentcount,
					int16_t *isnull) {

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}

	if (isnull && *isnull==(int16_t)-1) {
		// a null bind needs no blob, just a null indicator
		bytestring::zero(&inbindblobid[index],sizeof(ISC_QUAD));
	} else {

		// create a blob
		bytestring::zero(&inbindblobhandle[index],
					sizeof(isc_blob_handle));
		if (isc_create_blob2(firebirdconn->error,
					&firebirdconn->db,
					&firebirdconn->tr,
					&inbindblobhandle[index],
					&inbindblobid[index],0,NULL)) {
			return false;
		}

		// write the value to the blob - one isc_put_segment() per
		// segment the client wrote, when segment boundaries were
		// given, or MAX_LOB_CHUNK_SIZE bytes at a time otherwise
		uint32_t	bytesput=0;
		uint16_t	seg=0;
		while (bytesput<valuesize) {
			uint32_t	segbytesleft=(segmentlengths)?
						segmentlengths[seg]:
						(valuesize-bytesput);
			uint32_t	segbytesput=0;
			while (segbytesput<segbytesleft) {
				uint16_t	bytestoput=0;
				if (segbytesleft-segbytesput<
						MAX_LOB_CHUNK_SIZE) {
					bytestoput=segbytesleft-segbytesput;
				} else {
					bytestoput=MAX_LOB_CHUNK_SIZE;
				}
				// (modern versions of isc_put_segment take a
				// const char * parameter, but old versions take
				// char * and this cast works with both)
				if (isc_put_segment(firebirdconn->error,
						&inbindblobhandle[index],
						bytestoput,
						(char *)(value+bytesput))) {
					return false;
				}
				bytesput=bytesput+bytestoput;
				segbytesput=segbytesput+bytestoput;
			}
			seg++;
			if (segmentlengths && seg>=segmentcount &&
					bytesput<valuesize) {
				// ran out of segments before running out of
				// bytes - shouldn't happen, but fall back to
				// chunking whatever is left as one more segment
				segmentlengths=NULL;
			}
		}

		// close the blob
		isc_close_blob(firebirdconn->error,&inbindblobhandle[index]);
	}

	inbindsqlda->sqlvar[index].sqltype=SQL_BLOB+1;
	inbindsqlda->sqlvar[index].sqlscale=0;
	inbindsqlda->sqlvar[index].sqlsubtype=0;
	inbindsqlda->sqlvar[index].sqllen=sizeof(ISC_QUAD);
	inbindsqlda->sqlvar[index].sqldata=(char *)&inbindblobid[index];
	inbindsqlda->sqlvar[index].sqlind=isnull;
	inbindsqlda->sqlvar[index].sqlname_length=0;
	inbindsqlda->sqlvar[index].sqlname[0]='\0';
	inbindsqlda->sqlvar[index].relname_length=0;
	inbindsqlda->sqlvar[index].relname[0]='\0';
	inbindsqlda->sqlvar[index].ownname_length=0;
	inbindsqlda->sqlvar[index].ownname[0]='\0';
	inbindsqlda->sqlvar[index].aliasname_length=0;
	inbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::inputBindArray(const char *variable,
					uint16_t variablesize,
					const char *value,
					uint32_t valuesize,
					int16_t *isnull) {

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}

	// A null, a parameter the prepare didn't describe as an array, and a
	// parameter with no relation/field name to look the shape up by all
	// bind the value as plain text instead.  isc_array_lookup_bounds()
	// finds the column by name, so there is nothing else to try, and a
	// shape this module didn't expect is better reported by the backend
	// than guessed at here.
	if ((isnull && *isnull==(int16_t)-1) ||
		index>=(long)inbindcountfromprepare ||
		(inbinddescribe[index].sqltype!=SQL_ARRAY &&
			inbinddescribe[index].sqltype!=SQL_ARRAY+1) ||
		!inbinddescribe[index].relnamelength ||
		!inbinddescribe[index].sqlnamelength) {
		return inputBind(variable,variablesize,
					value,valuesize,(short *)isnull);
	}

	// look up the column's shape
	ISC_ARRAY_DESC	desc;
	bytestring::zero(&desc,sizeof(desc));
	if (isc_array_lookup_bounds(firebirdconn->error,
					&firebirdconn->db,
					&firebirdconn->tr,
					inbinddescribe[index].relname,
					inbinddescribe[index].sqlname,
					&desc)) {
		return false;
	}

	if (desc.array_desc_dimensions<1 ||
		desc.array_desc_dimensions>
			(short)(sizeof(desc.array_desc_bounds)/
				sizeof(desc.array_desc_bounds[0]))) {
		bindformaterror=true;
		return false;
	}

	// how many elements the column holds, and how wide each one is
	uint64_t	elementcount=1;
	for (short d=0; d<desc.array_desc_dimensions; d++) {
		int32_t	lower=desc.array_desc_bounds[d].array_bound_lower;
		int32_t	upper=desc.array_desc_bounds[d].array_bound_upper;
		if (upper<lower) {
			bindformaterror=true;
			return false;
		}
		elementcount=elementcount*(uint64_t)(upper-lower+1);

		// bail rather than let the count wrap around - see
		// fetchArrayField()
		if (elementcount>MAX_ARRAY_BUFFER_SIZE) {
			bindformaterror=true;
			return false;
		}
	}

	uint32_t	elementsize=firebirdArrayElementSize(
					(byte_t)desc.array_desc_dtype,
					desc.array_desc_length);
	if (!elementcount || !elementsize) {
		bindformaterror=true;
		return false;
	}

	uint64_t	buffersize=elementcount*elementsize;
	if (buffersize>MAX_ARRAY_BUFFER_SIZE) {
		bindformaterror=true;
		return false;
	}

	// the value must be a bracketed, comma-separated list - the same
	// rendering getField() produces for an array column - anything else
	// (no braces at all, or text that just happens to start and end with
	// one) is not an array literal and must fail the bind rather than
	// get parsed as if it were one
	if (valuesize<2 || value[0]!='{' || value[valuesize-1]!='}') {
		bindformaterror=true;
		return false;
	}
	const char	*elements=value+1;
	uint32_t	elementslen=valuesize-2;

	// An element the value didn't supply still occupies its slot in the
	// buffer, so the slots start out empty rather than uninitialized -
	// blanks for a text element, since that's how firebird pads one, and
	// zeros for everything else.
	byte_t	*buffer=new byte_t[buffersize];
	if (desc.array_desc_dtype==blr_text ||
		desc.array_desc_dtype==blr_text2) {
		bytestring::set(buffer,' ',(size_t)buffersize);
	} else {
		bytestring::zero(buffer,(size_t)buffersize);
	}

	uint64_t	parsed=0;
	uint32_t	pos=0;
	while (pos<elementslen) {

		// this element runs to the next comma that isn't inside
		// quotes
		uint32_t	start=pos;
		bool		inquotes=false;
		while (pos<elementslen && (inquotes || elements[pos]!=',')) {
			if (elements[pos]=='\'') {
				inquotes=!inquotes;
			}
			pos++;
		}
		uint32_t	len=pos-start;
		if (pos<elementslen) {
			pos++;
		}

		// more elements than the column holds
		if (parsed>=elementcount) {
			delete[] buffer;
			bindformaterror=true;
			return false;
		}

		if (!firebirdParseArrayElement(&desc,
					buffer+parsed*elementsize,
					elementsize,
					elements+start,len)) {
			delete[] buffer;
			bindformaterror=true;
			return false;
		}

		parsed++;
	}

	// A zeroed id makes isc_array_put_slice() create a new array and hand
	// back its id.  The array is temporary until the transaction the
	// statement runs in commits, which is why this has to happen here,
	// at bind time, rather than anywhere further out.
	bytestring::zero(&inbindarrayid[index],sizeof(ISC_QUAD));

	ISC_LONG	slicelength=(ISC_LONG)buffersize;
	if (isc_array_put_slice(firebirdconn->error,
				&firebirdconn->db,
				&firebirdconn->tr,
				&inbindarrayid[index],
				&desc,
				buffer,
				&slicelength)) {
		// the reason is in the connection's status vector.  Unlike
		// the read side, this doesn't fall back to null - a write
		// that failed must not look like a successful insert of an
		// empty array.
		delete[] buffer;
		return false;
	}

	delete[] buffer;

	inbindsqlda->sqlvar[index].sqltype=SQL_ARRAY+1;
	inbindsqlda->sqlvar[index].sqlscale=0;
	inbindsqlda->sqlvar[index].sqlsubtype=0;
	inbindsqlda->sqlvar[index].sqllen=sizeof(ISC_QUAD);
	inbindsqlda->sqlvar[index].sqldata=(char *)&inbindarrayid[index];
	inbindsqlda->sqlvar[index].sqlind=isnull;
	inbindsqlda->sqlvar[index].sqlname_length=0;
	inbindsqlda->sqlvar[index].sqlname[0]='\0';
	inbindsqlda->sqlvar[index].relname_length=0;
	inbindsqlda->sqlvar[index].relname[0]='\0';
	inbindsqlda->sqlvar[index].ownname_length=0;
	inbindsqlda->sqlvar[index].ownname[0]='\0';
	inbindsqlda->sqlvar[index].aliasname_length=0;
	inbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::inputBindClob(const char *variable,
					uint16_t variablesize,
					const char *value,
					uint32_t valuesize,
					int16_t *isnull) {
	return inputBindBlob(variable,variablesize,
				value,valuesize,isnull);
}

bool firebirdcursor::inputBindClob(const char *variable,
					uint16_t variablesize,
					const char *value,
					uint32_t valuesize,
					const uint32_t *segmentlengths,
					uint16_t segmentcount,
					int16_t *isnull) {
	return inputBindBlob(variable,variablesize,value,valuesize,
				segmentlengths,segmentcount,isnull);
}

bool firebirdcursor::outputBind(const char *variable, 
				uint16_t variablesize,
				char *value, 
				uint32_t valuesize, 
				int16_t *isnull) {

	outbindcount++;

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}
	outbindsqlda->sqlvar[index].sqltype=SQL_TEXT+1;
	outbindsqlda->sqlvar[index].sqlscale=0;
	outbindsqlda->sqlvar[index].sqlsubtype=0;
	outbindsqlda->sqlvar[index].sqllen=valuesize;
	outbindsqlda->sqlvar[index].sqldata=value;
	outbindsqlda->sqlvar[index].sqlind=isnull;
	outbindsqlda->sqlvar[index].sqlname_length=0;
	outbindsqlda->sqlvar[index].sqlname[0]='\0';
	outbindsqlda->sqlvar[index].relname_length=0;
	outbindsqlda->sqlvar[index].relname[0]='\0';
	outbindsqlda->sqlvar[index].ownname_length=0;
	outbindsqlda->sqlvar[index].ownname[0]='\0';
	outbindsqlda->sqlvar[index].aliasname_length=0;
	outbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::outputBind(const char *variable,
				uint16_t variablesize,
				int64_t *value,
				int16_t *isnull) {

	outbindcount++;

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}
	outbindsqlda->sqlvar[index].sqltype=SQL_INT64;
	outbindsqlda->sqlvar[index].sqlscale=0;
	outbindsqlda->sqlvar[index].sqlsubtype=0;
	outbindsqlda->sqlvar[index].sqllen=sizeof(int64_t);
	outbindsqlda->sqlvar[index].sqldata=(char *)value;
	outbindsqlda->sqlvar[index].sqlind=isnull;
	outbindsqlda->sqlvar[index].sqlname_length=0;
	outbindsqlda->sqlvar[index].sqlname[0]='\0';
	outbindsqlda->sqlvar[index].relname_length=0;
	outbindsqlda->sqlvar[index].relname[0]='\0';
	outbindsqlda->sqlvar[index].ownname_length=0;
	outbindsqlda->sqlvar[index].ownname[0]='\0';
	outbindsqlda->sqlvar[index].aliasname_length=0;
	outbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::outputBind(const char *variable,
				uint16_t variablesize,
				double *value,
				uint32_t *precision,
				uint32_t *scale,
				int16_t *isnull) {

	outbindcount++;

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}
	outbindsqlda->sqlvar[index].sqltype=SQL_DOUBLE;
	outbindsqlda->sqlvar[index].sqlscale=*scale;
	outbindsqlda->sqlvar[index].sqlsubtype=0;
	outbindsqlda->sqlvar[index].sqllen=sizeof(double);
	outbindsqlda->sqlvar[index].sqldata=(char *)value;
	outbindsqlda->sqlvar[index].sqlind=isnull;
	outbindsqlda->sqlvar[index].sqlname_length=0;
	outbindsqlda->sqlvar[index].sqlname[0]='\0';
	outbindsqlda->sqlvar[index].relname_length=0;
	outbindsqlda->sqlvar[index].relname[0]='\0';
	outbindsqlda->sqlvar[index].ownname_length=0;
	outbindsqlda->sqlvar[index].ownname[0]='\0';
	outbindsqlda->sqlvar[index].aliasname_length=0;
	outbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::outputBind(const char *variable,
				uint16_t variablesize,
				int16_t *year,
				int16_t *month,
				int16_t *day,
				int16_t *hour,
				int16_t *minute,
				int16_t *second,
				int32_t *microsecond,
				const char **tz,
				bool *isnegative,
				int16_t *isnull) {

	// store the pointers
	outdatebind[outbindcount].year=year;
	outdatebind[outbindcount].month=month;
	outdatebind[outbindcount].day=day;
	outdatebind[outbindcount].hour=hour;
	outdatebind[outbindcount].minute=minute;
	outdatebind[outbindcount].second=second;
	outdatebind[outbindcount].tz=tz;
	outdatebind[outbindcount].isnegative=isnegative;

	char	*value=(char *)&(outdatebind[outbindcount].buffer);

	outbindcount++;

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}
	outbindsqlda->sqlvar[index].sqltype=SQL_TIMESTAMP;
	outbindsqlda->sqlvar[index].sqlscale=0;
	outbindsqlda->sqlvar[index].sqlsubtype=0;
	outbindsqlda->sqlvar[index].sqllen=sizeof(ISC_TIMESTAMP);
	outbindsqlda->sqlvar[index].sqldata=value;
	outbindsqlda->sqlvar[index].sqlind=isnull;
	outbindsqlda->sqlvar[index].sqlname_length=0;
	outbindsqlda->sqlvar[index].sqlname[0]='\0';
	outbindsqlda->sqlvar[index].relname_length=0;
	outbindsqlda->sqlvar[index].relname[0]='\0';
	outbindsqlda->sqlvar[index].ownname_length=0;
	outbindsqlda->sqlvar[index].ownname[0]='\0';
	outbindsqlda->sqlvar[index].aliasname_length=0;
	outbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::outputBindBlob(const char *variable,
					uint16_t variablesize,
					uint16_t ind,
					int16_t *isnull) {

	outbindcount++;

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}

	outbindblobisopen[index]=false;

	outbindsqlda->sqlvar[index].sqltype=SQL_BLOB+1;
	outbindsqlda->sqlvar[index].sqlscale=0;
	outbindsqlda->sqlvar[index].sqlsubtype=0;
	outbindsqlda->sqlvar[index].sqllen=sizeof(ISC_QUAD);
	outbindsqlda->sqlvar[index].sqldata=(char *)&outbindblobid[index];
	outbindsqlda->sqlvar[index].sqlind=isnull;
	outbindsqlda->sqlvar[index].sqlname_length=0;
	outbindsqlda->sqlvar[index].sqlname[0]='\0';
	outbindsqlda->sqlvar[index].relname_length=0;
	outbindsqlda->sqlvar[index].relname[0]='\0';
	outbindsqlda->sqlvar[index].ownname_length=0;
	outbindsqlda->sqlvar[index].ownname[0]='\0';
	outbindsqlda->sqlvar[index].aliasname_length=0;
	outbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::outputBindClob(const char *variable,
					uint16_t variablesize,
					uint16_t index,
					int16_t *isnull) {
	return outputBindBlob(variable,variablesize,index,isnull);
}

bool firebirdcursor::getLobOutputBindLength(uint16_t index, uint64_t *length) {

	// open the blob
	outbindblobhandle[index]=0;
	if (isc_open_blob2(firebirdconn->error,
				&firebirdconn->db,
				&firebirdconn->tr,
				&outbindblobhandle[index],
				&outbindblobid[index],0,NULL)) {
		return false;
	}

	bool	retval=true;

	// read blob info
	char	blobitems[]={isc_info_blob_total_length};
	char	resultbuffer[64];
	if (isc_blob_info(firebirdconn->error,
				&outbindblobhandle[index],
				sizeof(blobitems),
				blobitems,
				sizeof(resultbuffer),
				resultbuffer)) {
		retval=false;
	}

	// get the blob length from the result buffer
	for (const char *p=resultbuffer; *p!=isc_info_end;) {

		// get the item type
		char	itemtype=*p;
		p++;

		// get the item length
		// (modern versions of isc_vax_integer take a const char *
		// parameter, but old versions take char * and this cast works
		// with both)
		uint16_t	itemlength=
				(uint16_t)isc_vax_integer((char *)p,2);
		p=p+2;

		// get the lob length
		if (itemtype==isc_info_blob_total_length) {
			// (modern versions of isc_vax_integer take a
			// const char * parameter, but old versions take a
			// char * and this cast works with both)
			*length=isc_vax_integer((char *)p,itemlength);
		}
 
		// move on
		p=p+itemlength;
	}
				
	// close the blob
	isc_close_blob(firebirdconn->error,&outbindblobhandle[index]);

	return retval;
}

bool firebirdcursor::getLobOutputBindSegment(uint16_t index,
					char *buffer, uint64_t buffersize,
					uint64_t offset, uint64_t charstoread,
					uint64_t *charsread) {

	// open the blob, if necessary
	if (!outbindblobisopen[index]) {
		outbindblobhandle[index]=0;
		if (isc_open_blob2(firebirdconn->error,
					&firebirdconn->db,
					&firebirdconn->tr,
					&outbindblobhandle[index],
					&outbindblobid[index],0,NULL)) {
			return false;
		}
		outbindblobisopen[index]=true;
	}

	// read a blob segment, at most MAX_LOB_CHUNK_SIZE bytes at a time
	uint64_t	totalbytesread=0;
	uint64_t	bytestoread=0;
	uint64_t	remainingbytestoread=charstoread;
	ISC_STATUS	status=0;
	for (;;) {

		// figure out how many bytes to read this time
		if (remainingbytestoread<MAX_LOB_CHUNK_SIZE) {
			bytestoread=remainingbytestoread;
		} else {
			bytestoread=MAX_LOB_CHUNK_SIZE;
			remainingbytestoread=remainingbytestoread-
						MAX_LOB_CHUNK_SIZE;
		}
		// read the bytes
		uint16_t	bytesread=0;
		status=isc_get_segment(firebirdconn->error,
					&outbindblobhandle[index],
					&bytesread,
					bytestoread,
					buffer+totalbytesread);

		// bail on error
		if (status && status!=isc_segment) {
			break;
		}

		// update total bytes read
		totalbytesread=totalbytesread+bytesread;

		// bail if we're done reading
		if (bytesread<bytestoread || totalbytesread==charstoread) {
			break;
		}
	}

	// return number of bytes/chars read
	*charsread=totalbytesread;

	return true;
}

void firebirdcursor::closeLobOutputBind(uint16_t index) {

	// close the blob, if necessary
	if (outbindblobisopen[index]) {
		isc_close_blob(firebirdconn->error,&outbindblobhandle[index]);
		outbindblobisopen[index]=false;
	}
}

bool firebirdcursor::executeQuery(const char *query, uint32_t size) {

	// for commit or rollback, execute the API call and return
	if (querytype==isc_info_sql_stmt_commit) {
		return conn->commit();
	} else if (querytype==isc_info_sql_stmt_rollback) {
		return conn->rollback();
	} else if (queryisexecsp) {

		// handle stored procedures...

		// allocate dummy buffers for any unbound output
		// params so isc_dsql_execute2 has somewhere to write
		bool		*isdummy=NULL;
		memorypool	*bindpool=conn->cont->getBindPool(this);
		for (uint16_t i=0; i<outbindsqlda->sqld; i++) {

			if (outbindsqlda->sqlvar[i].sqldata) {
				continue;
			}

			if (!isdummy) {
				isdummy=(bool *)bindpool->allocate(
					sizeof(bool)*outbindsqlda->sqld);
				bytestring::zero(isdummy,
					sizeof(bool)*outbindsqlda->sqld);
			}
			isdummy[i]=true;

			uint16_t	len=outbindsqlda->sqlvar[i].sqllen;
			outbindsqlda->sqlvar[i].sqldata=
					(char *)bindpool->allocate((len)?len:1);
		}

		// execute the stored procedure
		bool	retval=!isc_dsql_execute2(firebirdconn->error,
							&firebirdconn->tr,
							&stmt,1,
							inbindsqlda,
							outbindsqlda);

		// null-out dummy sqldata pointers
		if (isdummy) {
			for (uint16_t i=0; i<outbindsqlda->sqld; i++) {
				if (isdummy[i]) {
					outbindsqlda->sqlvar[i].sqldata=NULL;
				}
			}
		}

		for (uint16_t i=0; i<outbindsqlda->sqld; i++) {

			// skip unregistered output params
			if (!outbindsqlda->sqlvar[i].sqldata) {
				continue;
			}

			if (outbindsqlda->sqlvar[i].
					sqltype==SQL_TEXT+1) {

				// null-terminate strings
				outbindsqlda->sqlvar[i].
					sqldata[outbindsqlda->sqlvar[i].
								sqllen-1]=0;

			} else if (outbindsqlda->sqlvar[i].
					sqltype==SQL_TIMESTAMP) {

				// copy out date bind data
				tm	t;
				isc_decode_timestamp((ISC_TIMESTAMP *)
					outbindsqlda->sqlvar[i].sqldata,&t);
				*(outdatebind[i].year)=t.tm_year+1900;
				*(outdatebind[i].month)=t.tm_mon+1;
				*(outdatebind[i].day)=t.tm_mday;
				*(outdatebind[i].hour)=t.tm_hour;
				*(outdatebind[i].minute)=t.tm_min;
				*(outdatebind[i].second)=t.tm_sec;
				*(outdatebind[i].tz)=NULL;
				*(outdatebind[i].isnegative)=false;
			}
		}
		return retval;
	}

	// handle non-stored procedures...

	// check for create temp table query
	if (querytype==isc_info_sql_stmt_ddl) {
		checkForTempTable(query,size);
	}

	// describe the result set, unless the prepare already did
	if (!resultsetdescribed && !describeResultSet()) {
		return false;
	}

	// Execute the query
	return !isc_dsql_execute(firebirdconn->error,&firebirdconn->tr,
							&stmt,1,inbindsqlda);
}

bool firebirdcursor::describeResultSet() {

	// get the max column count and field size
	uint32_t	maxcolumncount=conn->cont->getMaxColumnCount();
	uint32_t	maxfieldsize=conn->cont->getMaxFieldSize();

	// when the column count isn't capped, grow the buffers to the count
	// isc_dsql_prepare reported
	if (!maxcolumncount && outsqlda->sqld>fieldcount) {
		allocateResultSetBuffers(outsqlda->sqld);
	}

	// describe the cursor
	if (isc_dsql_describe(firebirdconn->error,&stmt,1,outsqlda)) {
		return false;
	}

	// A describe into an sqlda with too few slots fills in the column
	// count and nothing else, so if the count came back higher than the
	// buffers hold, grow them and describe again.
	if (!maxcolumncount && outsqlda->sqld>fieldcount) {
		allocateResultSetBuffers(outsqlda->sqld);
		if (isc_dsql_describe(firebirdconn->error,&stmt,1,outsqlda)) {
			return false;
		}
	}

	// maxcolumncount capped the buffers below the select list's size -
	// sqlvar wasn't filled in at all in that case (see above), so bail
	// with an error rather than process it as if it were
	if (maxcolumncount && outsqlda->sqld>(int32_t)maxcolumncount) {
		stringbuffer	err;
		err.append(SQLR_ERROR_MAXCOLUMNCOUNTTOOSMALL_STRING);
		err.append(" (")->append(maxcolumncount);
		err.append('<')->append(outsqlda->sqld)->append(')');
		conn->cont->setError(this,err.getString(),
				SQLR_ERROR_MAXCOLUMNCOUNTTOOSMALL,true);
		return false;
	}

	for (uint16_t i=0; i<outsqlda->sqld; i++) {

		// save the actual field type
		field[i].type=outsqlda->sqlvar[i].sqltype;

		// handle the null indicator
		outsqlda->sqlvar[i].sqlind=&field[i].nullindicator;

		// coerce the datatypes and point where the data should go
		if (outsqlda->sqlvar[i].sqltype==SQL_TEXT || 
				outsqlda->sqlvar[i].sqltype==SQL_TEXT+1) {
			outsqlda->sqlvar[i].sqldata=field[i].textbuffer;
			field[i].sqlrtype=CHAR_DATATYPE;
			if ((uint32_t)outsqlda->sqlvar[i].sqllen>
							maxfieldsize) {
				outsqlda->sqlvar[i].sqllen=maxfieldsize;
			}
		} else if (outsqlda->sqlvar[i].sqltype==SQL_VARYING ||
				outsqlda->sqlvar[i].sqltype==SQL_VARYING+1) {
			outsqlda->sqlvar[i].sqldata=field[i].textbuffer;
			field[i].sqlrtype=VARCHAR_DATATYPE;
			if ((uint32_t)outsqlda->sqlvar[i].sqllen>
							maxfieldsize) {
				outsqlda->sqlvar[i].sqllen=maxfieldsize;
			}
		} else if (outsqlda->sqlvar[i].sqltype==SQL_SHORT ||
				outsqlda->sqlvar[i].sqltype==SQL_SHORT+1) {
			// sqllen is already 2 bytes here, matching
			// shortbuffer exactly, so a scaled column (firebird
			// stores a 1-4 digit NUMERIC/DECIMAL as a SMALLINT)
			// needs no buffer widening, just the right type
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].shortbuffer;
			if (outsqlda->sqlvar[i].sqlscale) {
				field[i].sqlrtype=
					(outsqlda->sqlvar[i].sqlsubtype==1)?
					NUMERIC_DATATYPE:DECIMAL_DATATYPE;
			} else {
				field[i].sqlrtype=SMALLINT_DATATYPE;
			}

		// Looks like sometimes firebird returns INT64's as
		// SQL_LONG type.  These can be identified because
		// the sqlscale gets set too.  Treat SQL_LONG's with
		// an sqlscale as INT64's.
		} else if ((outsqlda->sqlvar[i].sqltype==SQL_LONG ||
				outsqlda->sqlvar[i].sqltype==SQL_LONG+1) &&
				!outsqlda->sqlvar[i].sqlscale) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].longbuffer;
			field[i].sqlrtype=INTEGER_DATATYPE;
		} else if (
		#ifdef SQL_INT64
				(outsqlda->sqlvar[i].sqltype==SQL_INT64 ||
				outsqlda->sqlvar[i].sqltype==SQL_INT64+1) ||
		#endif
				((outsqlda->sqlvar[i].sqltype==SQL_LONG ||
				outsqlda->sqlvar[i].sqltype==SQL_LONG+1) &&
				outsqlda->sqlvar[i].sqlscale)) {
		#ifdef SQL_INT64
			// the sqlvar is still describing this as a 4-byte
			// SQL_LONG, but it's bound to the 8-byte int64buffer
			// below.  isc_dsql_fetch only writes as many bytes
			// as sqltype says, leaving int64buffer's upper half
			// stale, so re-describe it as SQL_INT64 (preserving
			// the low, nullability, bit) to get a real 8-byte
			// fetch.
			if (outsqlda->sqlvar[i].sqltype==SQL_LONG ||
					outsqlda->sqlvar[i].sqltype==
					SQL_LONG+1) {
				outsqlda->sqlvar[i].sqltype=SQL_INT64|
					(outsqlda->sqlvar[i].sqltype&1);
				outsqlda->sqlvar[i].sqllen=
						sizeof(ISC_INT64);
			}
		#endif
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].int64buffer;
			if (outsqlda->sqlvar[i].sqlsubtype==1) {
				field[i].sqlrtype=NUMERIC_DATATYPE;
			} else {
				field[i].sqlrtype=DECIMAL_DATATYPE;
			}
		} else if (outsqlda->sqlvar[i].sqltype==SQL_FLOAT ||
			outsqlda->sqlvar[i].sqltype==SQL_FLOAT+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].floatbuffer;
			field[i].sqlrtype=FLOAT_DATATYPE;
		} else if (outsqlda->sqlvar[i].sqltype==SQL_DOUBLE ||
			outsqlda->sqlvar[i].sqltype==SQL_DOUBLE+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].doublebuffer;
			field[i].sqlrtype=DOUBLE_PRECISION_DATATYPE;
		} else if (outsqlda->sqlvar[i].sqltype==SQL_D_FLOAT ||
			outsqlda->sqlvar[i].sqltype==SQL_D_FLOAT+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].doublebuffer;
			field[i].sqlrtype=D_FLOAT_DATATYPE;
		} else if (outsqlda->sqlvar[i].sqltype==SQL_ARRAY || 
				outsqlda->sqlvar[i].sqltype==SQL_ARRAY+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].quadbuffer;
			field[i].sqlrtype=ARRAY_DATATYPE;
			// this column's shape has to be looked up again -
			// the cursor may have run a different query since
			// the descriptor that's cached here was filled in
			field[i].arraydescvalid=false;
			field[i].arraybuffervalid=false;
		} else if (outsqlda->sqlvar[i].sqltype==SQL_QUAD ||
				outsqlda->sqlvar[i].sqltype==SQL_QUAD+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].quadbuffer;
			field[i].sqlrtype=QUAD_DATATYPE;
		#ifdef SQL_TIMESTAMP
		} else if (outsqlda->sqlvar[i].sqltype==SQL_TIMESTAMP || 
				outsqlda->sqlvar[i].
					sqltype==SQL_TIMESTAMP+1) {
		#else
		} else if (outsqlda->sqlvar[i].sqltype==SQL_DATE || 
				outsqlda->sqlvar[i].sqltype==SQL_DATE+1) {
		#endif
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].timestampbuffer;
			field[i].sqlrtype=TIMESTAMP_DATATYPE;
		#ifdef SQL_TIMESTAMP
		} else if (outsqlda->sqlvar[i].sqltype==SQL_TYPE_TIME || 
				outsqlda->sqlvar[i].
					sqltype==SQL_TYPE_TIME+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].timebuffer;
			field[i].sqlrtype=TIME_DATATYPE;
		} else if (outsqlda->sqlvar[i].sqltype==SQL_TYPE_DATE || 
				outsqlda->sqlvar[i].
					sqltype==SQL_TYPE_DATE+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].datebuffer;
			field[i].sqlrtype=DATE_DATATYPE;
		#endif
		} else if (outsqlda->sqlvar[i].sqltype==SQL_BLOB ||
				outsqlda->sqlvar[i].sqltype==SQL_BLOB+1) {
			outsqlda->sqlvar[i].sqldata=(char *)&field[i].blobid;
			outsqlda->sqlvar[i].sqllen=sizeof(ISC_QUAD);
			if (outsqlda->sqlvar[i].sqlsubtype==1) {
				field[i].sqlrtype=CLOB_DATATYPE;
			} else {
				field[i].sqlrtype=BLOB_DATATYPE;
			}
			field[i].blobisopen=false;
	#ifdef SQL_INT128
		// firebird 4's storage for a NUMERIC/DECIMAL of precision
		// 19-38.  There's no native 128-bit buffer here, but
		// firebird will render the value to text for us, so fetch
		// it as a wide-enough SQL_VARYING, the same way the
		// catch-all below does for a type with no dedicated
		// handling, but sized correctly and with the nullability
		// bit preserved
		} else if (outsqlda->sqlvar[i].sqltype==SQL_INT128 ||
				outsqlda->sqlvar[i].sqltype==SQL_INT128+1) {
			outsqlda->sqlvar[i].sqltype=SQL_VARYING|
					(outsqlda->sqlvar[i].sqltype&1);
			outsqlda->sqlvar[i].sqldata=field[i].textbuffer;
			outsqlda->sqlvar[i].sqllen=
					FIREBIRD_WIDEDECIMAL_TEXTLEN;
			if ((uint32_t)outsqlda->sqlvar[i].sqllen>
							maxfieldsize) {
				outsqlda->sqlvar[i].sqllen=maxfieldsize;
			}
			field[i].sqlrtype=
				(outsqlda->sqlvar[i].sqlsubtype==1)?
				NUMERIC_DATATYPE:DECIMAL_DATATYPE;
	#endif
	#ifdef SQL_DEC16
		// firebird 4's DECFLOAT(16), IEEE 754 decimal64; fetched
		// as text, same as SQL_INT128 above
		} else if (outsqlda->sqlvar[i].sqltype==SQL_DEC16 ||
				outsqlda->sqlvar[i].sqltype==SQL_DEC16+1) {
			outsqlda->sqlvar[i].sqltype=SQL_VARYING|
					(outsqlda->sqlvar[i].sqltype&1);
			outsqlda->sqlvar[i].sqldata=field[i].textbuffer;
			outsqlda->sqlvar[i].sqllen=
					FIREBIRD_WIDEDECIMAL_TEXTLEN;
			if ((uint32_t)outsqlda->sqlvar[i].sqllen>
							maxfieldsize) {
				outsqlda->sqlvar[i].sqllen=maxfieldsize;
			}
			field[i].sqlrtype=DOUBLE_PRECISION_DATATYPE;
	#endif
	#ifdef SQL_DEC34
		// firebird 4's DECFLOAT(34), IEEE 754 decimal128; fetched
		// as text, same as SQL_INT128 above
		} else if (outsqlda->sqlvar[i].sqltype==SQL_DEC34 ||
				outsqlda->sqlvar[i].sqltype==SQL_DEC34+1) {
			outsqlda->sqlvar[i].sqltype=SQL_VARYING|
					(outsqlda->sqlvar[i].sqltype&1);
			outsqlda->sqlvar[i].sqldata=field[i].textbuffer;
			outsqlda->sqlvar[i].sqllen=
					FIREBIRD_WIDEDECIMAL_TEXTLEN;
			if ((uint32_t)outsqlda->sqlvar[i].sqllen>
							maxfieldsize) {
				outsqlda->sqlvar[i].sqllen=maxfieldsize;
			}
			field[i].sqlrtype=DOUBLE_PRECISION_DATATYPE;
	#endif
	#ifdef SQL_BOOLEAN
		// firebird 3+'s BOOLEAN; fetched as text, same as
		// SQL_INT128 above
		} else if (outsqlda->sqlvar[i].sqltype==SQL_BOOLEAN ||
				outsqlda->sqlvar[i].sqltype==SQL_BOOLEAN+1) {
			outsqlda->sqlvar[i].sqltype=SQL_VARYING|
					(outsqlda->sqlvar[i].sqltype&1);
			outsqlda->sqlvar[i].sqldata=field[i].textbuffer;
			outsqlda->sqlvar[i].sqllen=
					FIREBIRD_BOOLEAN_TEXTLEN;
			if ((uint32_t)outsqlda->sqlvar[i].sqllen>
							maxfieldsize) {
				outsqlda->sqlvar[i].sqllen=maxfieldsize;
			}
			field[i].sqlrtype=BOOL_DATATYPE;
	#endif
		} else {
			outsqlda->sqlvar[i].sqltype=SQL_VARYING;
			outsqlda->sqlvar[i].sqldata=field[i].textbuffer;
			field[i].sqlrtype=UNKNOWN_DATATYPE;
			if ((uint32_t)outsqlda->sqlvar[i].sqllen>
							maxfieldsize) {
				outsqlda->sqlvar[i].sqllen=maxfieldsize;
			}
		}
	}

	return true;
}

void firebirdcursor::getError(char *errorbuffer,
				uint32_t errorbuffersize,
				uint32_t *errorsize,
				int64_t *errorcode,
				bool *liveconnection) {

	// handle bind format errors
	if (bindformaterror) {
		*errorsize=charstring::getLength(
				SQLR_ERROR_INVALIDBINDVARIABLEFORMAT_STRING);
		if (*errorsize>=errorbuffersize) {
			*errorsize=(errorbuffersize)?errorbuffersize-1:0;
		}
		charstring::safeCopy(errorbuffer,
				errorbuffersize,
				SQLR_ERROR_INVALIDBINDVARIABLEFORMAT_STRING,
				*errorsize);
		if (errorbuffersize) {
			errorbuffer[*errorsize]='\0';
		}
		*errorcode=SQLR_ERROR_INVALIDBINDVARIABLEFORMAT;
		*liveconnection=true;
		return;
	}

	// handle queries too large for isc_dsql_prepare
	if (querytoolarge) {
		*errorsize=charstring::getLength(
				SQLR_ERROR_MAXQUERYSIZE_STRING);
		if (*errorsize>=errorbuffersize) {
			*errorsize=(errorbuffersize)?errorbuffersize-1:0;
		}
		charstring::safeCopy(errorbuffer,
				errorbuffersize,
				SQLR_ERROR_MAXQUERYSIZE_STRING,
				*errorsize);
		if (errorbuffersize) {
			errorbuffer[*errorsize]='\0';
		}
		*errorcode=SQLR_ERROR_MAXQUERYSIZE;
		*liveconnection=true;
		return;
	}

	// otherwise fall back to default implementation
	sqlrservercursor::getError(errorbuffer,
					errorbuffersize,
					errorsize,
					errorcode,
					liveconnection);
}

void firebirdcursor::checkForTempTable(const char *query, uint32_t size) {

	// see if the query matches the pattern for a temporary query that
	// creates a temporary table
	const char	*ptr=skipCreateTempTableClause(query);
	if (!ptr) {
		return;
	}

	// get the table name
	stringbuffer	tablename;
	const char	*endptr=query+size;
	while (ptr && *ptr && *ptr!=' ' &&
		*ptr!='\n' && *ptr!='	' && ptr<endptr) {
		tablename.append(*ptr);
		ptr++;
	}

	// look for "on commit preserve rows"
	bool	preserverowsoncommit=containsOnCommitPreserveRows(ptr);

	if (firebirdconn->droptemptables) {

		// if "droptemptables" was specified...
		conn->cont->addTempTableForDrop(tablename.getString());

	} else if (preserverowsoncommit) {

		// If "on commit preserve rows" was specified, then when
		// the commit/rollback is executed at the end of the
		// session, the data won't be truncated.  It needs to
		// be though, so we'll set it up to be truncated manually.
		conn->cont->addTempTableForTrunc(tablename.getString());
	}
}

bool firebirdcursor::queryIsNotSelect() {
	return (querytype!=isc_info_sql_stmt_select);
}

bool firebirdcursor::queryIsCommitOrRollback() {
	return (querytype==isc_info_sql_stmt_commit ||
		querytype==isc_info_sql_stmt_rollback);
}

uint64_t firebirdcursor::getAffectedRows() {

	char	infoitems[]={isc_info_sql_records};
	char	resbuffer[256];

	if (isc_dsql_sql_info(firebirdconn->error,&stmt,
				sizeof(infoitems),infoitems,
				sizeof(resbuffer),resbuffer)) {
		return 0;
	}

	uint64_t	affectedrows=0;

	for (const char *p=resbuffer; *p!=isc_info_end;) {

		char	itemtype=*p;
		p++;

		// (modern versions of isc_vax_integer take a const char *
		// parameter, but old versions take char * and this cast
		// works with both)
		uint16_t	itemlength=
				(uint16_t)isc_vax_integer((char *)p,2);
		p=p+2;

		if (itemtype==isc_info_sql_records) {

			// parse sub-items
			const char	*end=p+itemlength;
			while (p<end && *p!=isc_info_end) {

				char	subtype=*p;
				p++;

				uint16_t	sublength=
					(uint16_t)isc_vax_integer(
							(char *)p,2);
				p=p+2;

				uint64_t	count=
					(uint64_t)isc_vax_integer(
							(char *)p,sublength);
				p=p+sublength;

				switch (subtype) {
					case isc_info_req_insert_count:
					case isc_info_req_update_count:
					case isc_info_req_delete_count:
						affectedrows+=count;
						break;
				}
			}
		} else {
			p=p+itemlength;
		}
	}

	return affectedrows;
}

uint32_t firebirdcursor::colCount() {
	// for exec procedure queries, outsqlda contains output bind values
	// rather than column info and there is no result set, thus no column
	// info
	return (queryisexecsp)?0:outsqlda->sqld;
}

const char *firebirdcursor::getColumnName(uint32_t col) {
	return outsqlda->sqlvar[col].aliasname;
}

uint16_t firebirdcursor::getColumnNameSize(uint32_t col) {
	return outsqlda->sqlvar[col].aliasname_length;
}

uint16_t firebirdcursor::getColumnType(uint32_t col) {
	return field[col].sqlrtype;
}

uint32_t firebirdcursor::getColumnSize(uint32_t col) {
	return outsqlda->sqlvar[col].sqllen;
}

uint32_t firebirdcursor::getColumnPrecision(uint32_t col) {

	switch (field[col].sqlrtype) {
		case CHAR_DATATYPE:
			return outsqlda->sqlvar[col].sqllen;
		case VARCHAR_DATATYPE:
			return outsqlda->sqlvar[col].sqllen;
		case SMALLINT_DATATYPE:
			return 5;
		case INTEGER_DATATYPE:
			return 11;
		case NUMERIC_DATATYPE:
		case DECIMAL_DATATYPE:
			// field[col].type, not sqlvar[col].sqltype -
			// describeResultSet() rewrites sqlvar's sqltype/
			// sqllen for a scaled SQL_LONG, but field[col].type
			// still holds the original describe type
			return firebirdNumericPrecisionFromSqlType(
							field[col].type);
		case FLOAT_DATATYPE:
			return 0;
		case DOUBLE_PRECISION_DATATYPE:
			return 0;
		case D_FLOAT_DATATYPE:
			return 0;
		case ARRAY_DATATYPE:
			// not sure
			return 0;
		case QUAD_DATATYPE:
			// not sure
			return 0;
		case TIMESTAMP_DATATYPE:
			// not sure
			return 0;
		case TIME_DATATYPE:
			return 8;
		case DATE_DATATYPE:
			return 10;
		case BLOB_DATATYPE:
			return outsqlda->sqlvar[col].sqllen;
		default:
			return outsqlda->sqlvar[col].sqllen;
	}
}

uint32_t firebirdcursor::getColumnScale(uint32_t col) {
	return -outsqlda->sqlvar[col].sqlscale;
}

uint16_t firebirdcursor::getColumnIsNullable(uint32_t col) {
	// the low bit of sqltype indicates nullability
	return outsqlda->sqlvar[col].sqltype&1;
}

const char *firebirdcursor::getColumnTable(uint32_t col) {
	return outsqlda->sqlvar[col].relname;
}

uint16_t firebirdcursor::getColumnTableSize(uint32_t col) {
	return outsqlda->sqlvar[col].relname_length;
}

const char *firebirdcursor::getColumnField(uint32_t col) {
	// sqlname is the field's own name in the table it came from -
	// aliasname (which getColumnName() answers) is what the query
	// called it
	return outsqlda->sqlvar[col].sqlname;
}

uint16_t firebirdcursor::getColumnFieldSize(uint32_t col) {
	return outsqlda->sqlvar[col].sqlname_length;
}

bool firebirdcursor::noRowsToReturn() {
	// for exec procedure queries, outsqlda contains output bind values
	// rather than a result set and there is no result set
	return (queryisexecsp)?true:!outsqlda->sqld;
}

bool firebirdcursor::fetchRow(bool *error) {

	*error=false;

	ISC_STATUS	retcode=isc_dsql_fetch(firebirdconn->error,
							&stmt,1,outsqlda);

	// success
	if (!retcode) {
		// whatever array elements were buffered belong to the row
		// that just got left behind
		for (int32_t i=0; i<fieldcount; i++) {
			field[i].arraybuffervalid=false;
		}
		return true;
	}

	// no more rows
	if (retcode==100) {
		return false;
	}

	// error
	*error=true;
	return false;
}

void firebirdcursor::getField(uint32_t col,
				const char **fld, uint64_t *fldsize,
				bool *lob, bool *null) {

	// handle a null field
	if ((outsqlda->sqlvar[col].sqltype & 1) && 
			field[col].nullindicator==-1) {

		*null=true;

	} else

	// handle a non-null field
	if (outsqlda->sqlvar[col].sqltype==SQL_TEXT ||
			outsqlda->sqlvar[col].sqltype==SQL_TEXT+1) {

		size_t	maxlen=outsqlda->sqlvar[col].sqllen;
		size_t	reallen=charstring::getLength(field[col].textbuffer);
		if (reallen>maxlen) {
			reallen=maxlen;
		}
		*fld=field[col].textbuffer;
		*fldsize=reallen;

	} else if (outsqlda->sqlvar[col].
				sqltype==SQL_SHORT ||
			outsqlda->sqlvar[col].
				sqltype==SQL_SHORT+1) {

		if (outsqlda->sqlvar[col].sqlscale) {
			*fldsize=firebirdFormatScaledInt64(
					field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					(ISC_INT64)field[col].shortbuffer,
					outsqlda->sqlvar[col].sqlscale);
		} else {
			*fldsize=charstring::printf(
					field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%hd",field[col].shortbuffer);
		}
		*fld=field[col].textbuffer;

	} else if (outsqlda->sqlvar[col].
				sqltype==SQL_FLOAT ||
			outsqlda->sqlvar[col].
				sqltype==SQL_FLOAT+1) {

		*fldsize=charstring::printf(field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%.4f",(double)field[col].floatbuffer);
		*fld=field[col].textbuffer;

	} else if (outsqlda->sqlvar[col].
				sqltype==SQL_DOUBLE ||
			outsqlda->sqlvar[col].
				sqltype==SQL_DOUBLE+1 ||
			outsqlda->sqlvar[col].
				sqltype==SQL_D_FLOAT ||
			outsqlda->sqlvar[col].
				sqltype==SQL_D_FLOAT+1) {

		*fldsize=charstring::printf(field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%.4f",field[col].doublebuffer);
		*fld=field[col].textbuffer;

	} else if (outsqlda->sqlvar[col].
				sqltype==SQL_VARYING ||
			outsqlda->sqlvar[col].
				sqltype==SQL_VARYING+1) {

		// the first 2 bytes are the size in 
		// an SQL_VARYING field
		int16_t	size;
		bytestring::copy((void *)&size,
				(void *)field[col].textbuffer,
				sizeof(int16_t));
		*fld=field[col].textbuffer+sizeof(int16_t);
		*fldsize=size;

	// Looks like sometimes firebird returns INT64's as
	// SQL_LONG type.  These can be identified because
	// the sqlscale gets set too.  Treat SQL_LONG's with
	// an sqlscale as INT64's.
	} else if ((outsqlda->sqlvar[col].
				sqltype==SQL_LONG ||
			outsqlda->sqlvar[col].
				sqltype==SQL_LONG+1) &&
			!outsqlda->sqlvar[col].sqlscale) {

		*fldsize=charstring::printf(field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%d",field[col].longbuffer);
		*fld=field[col].textbuffer;

	} else if (
	#ifdef SQL_INT64
			(outsqlda->sqlvar[col].
				sqltype==SQL_INT64 ||
			outsqlda->sqlvar[col].
				sqltype==SQL_INT64+1) ||
	#endif
			((outsqlda->sqlvar[col].
				sqltype==SQL_LONG ||
			outsqlda->sqlvar[col].
				sqltype==SQL_LONG+1) &&
			outsqlda->sqlvar[col].sqlscale)) {

		ISC_INT64	v=field[col].int64buffer;
		if (outsqlda->sqlvar[col].sqlscale) {
			*fldsize=firebirdFormatScaledInt64(
					field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					v,outsqlda->sqlvar[col].sqlscale);
		} else {
			*fldsize=charstring::printf(
					field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%lld",(int64_t)v);
		}
		*fld=field[col].textbuffer;

	} else if (outsqlda->sqlvar[col].sqltype==SQL_ARRAY ||
		outsqlda->sqlvar[col].sqltype==SQL_ARRAY+1) {

		// A generic client has no way to ask for an array's elements,
		// so they're rendered here as a bracketed, comma-separated
		// list.  A firebird client talking to the firebird protocol
		// module gets the elements themselves, through
		// getArrayFieldDescriptor()/getArrayFieldSlice().
		if (fetchArrayField(col)) {

			stringbuffer	elements;
			elements.append('{');
			bool	ok=true;
			for (uint64_t i=0;
				i<field[col].arrayelementcount && ok; i++) {
				if (i) {
					elements.append(',');
				}
				ok=firebirdAppendArrayElement(&elements,
					&field[col].arraydesc,
					field[col].arraybuffer+
					i*field[col].arrayelementsize,
					field[col].arrayelementsize);
			}
			elements.append('}');

			if (ok) {
				// an array can easily render longer than a
				// field is allowed to be, and printf() would
				// answer how long it would have been rather
				// than how much of it fit, so the rendering
				// is truncated by hand here
				// (textbuffer is getMaxFieldSize()+1 bytes,
				// so the terminator below always fits)
				uint64_t	len=elements.getStringLength();
				uint32_t	maxfieldsize=
						conn->cont->getMaxFieldSize();
				if (len>maxfieldsize) {
					len=maxfieldsize;
				}
				bytestring::copy(field[col].textbuffer,
						elements.getString(),
						(size_t)len);
				field[col].textbuffer[len]='\0';
				*fldsize=len;
				*fld=field[col].textbuffer;
			} else {
				// an element type with no text rendering -
				// there's nothing useful to show for it
				*null=true;
			}

		} else {
			// isc_array_lookup_bounds() or isc_array_get_slice()
			// failed, and the reason is in the connection's status
			// vector, but getField() has nowhere to report a
			// per-field error, so the field reads null
			*null=true;
		}

	} else if (outsqlda->sqlvar[col].sqltype==SQL_QUAD ||
		outsqlda->sqlvar[col].sqltype==SQL_QUAD+1) {

		// a quad is an internal id rather than a value, and firebird
		// has no user-visible column type that uses one, so there's
		// nothing to render
		*null=true;

	#ifdef SQL_TIMESTAMP
	} else if (outsqlda->sqlvar[col].sqltype==SQL_TIMESTAMP ||
		outsqlda->sqlvar[col].sqltype==SQL_TIMESTAMP+1) {

		// decode the timestamp
		tm	entry_timestamp;
		isc_decode_timestamp(&field[col].timestampbuffer,
						&entry_timestamp);
	#else
	} else if (outsqlda->sqlvar[col].sqltype==SQL_DATE ||
		outsqlda->sqlvar[col].sqltype==SQL_DATE+1) {

		// decode the timestamp
		tm	entry_timestamp;
		isc_decode_date(&field[col].timestampbuffer,
						&entry_timestamp);
	#endif

		// build a string of "yyyy-mm-dd hh:mm:ss" format
		*fldsize=charstring::printf(field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%d-%02d-%02d %02d:%02d:%02d",
					entry_timestamp.tm_year+1900,
					entry_timestamp.tm_mon+1,
					entry_timestamp.tm_mday,
					entry_timestamp.tm_hour,
					entry_timestamp.tm_min,
					entry_timestamp.tm_sec);
		*fld=field[col].textbuffer;

	#ifdef SQL_TIMESTAMP
	} else if (outsqlda->sqlvar[col].sqltype==SQL_TYPE_TIME ||
		outsqlda->sqlvar[col].sqltype==SQL_TYPE_TIME+1) {

		// decode the time
		tm	entry_time;
		isc_decode_sql_time(&field[col].timebuffer,
						&entry_time);
		// build a string of "hh:mm:ss" format
		*fldsize=charstring::printf(field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%02d:%02d:%02d",
					entry_time.tm_hour,
					entry_time.tm_min,
					entry_time.tm_sec);
		*fld=field[col].textbuffer;

	} else if (outsqlda->sqlvar[col].sqltype==SQL_TYPE_DATE ||
		outsqlda->sqlvar[col].sqltype==SQL_TYPE_DATE+1) {

		// decode the date
		tm	entry_date;
		isc_decode_sql_date(&field[col].datebuffer,
						&entry_date);
		// build a string of "yyyy-mm-dd" format
		*fldsize=charstring::printf(field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%d-%02d-%02d",
					entry_date.tm_year+1900,
					entry_date.tm_mon+1,
					entry_date.tm_mday);
		*fld=field[col].textbuffer;

	#endif
	} else if (outsqlda->sqlvar[col].sqltype==SQL_BLOB ||
			outsqlda->sqlvar[col].sqltype==SQL_BLOB+1) {
		*lob=true;
	}
}

bool firebirdcursor::getLobFieldLength(uint32_t col, uint64_t *length) {

	// ignore non-blobs
	if (field[col].sqlrtype!=BLOB_DATATYPE &&
			field[col].sqlrtype!=CLOB_DATATYPE) {
		return false;
	}

	// open the blob
	field[col].blobhandle=0;
	if (isc_open_blob2(firebirdconn->error,
				&firebirdconn->db,
				&firebirdconn->tr,
				&field[col].blobhandle,
				&field[col].blobid,0,NULL)) {
		return false;
	}

	bool	retval=true;

	// read blob info
	char	blobitems[]={isc_info_blob_total_length};
	char	resultbuffer[64];
	if (isc_blob_info(firebirdconn->error,
				&field[col].blobhandle,
				sizeof(blobitems),
				blobitems,
				sizeof(resultbuffer),
				resultbuffer)) {
		retval=false;
	}

	// get the blob length from the result buffer
	for (const char *p=resultbuffer; *p!=isc_info_end;) {

		// get the item type
		char	itemtype=*p;
		p++;

		// get the item length
		// (modern versions of isc_vax_integer take a const char *
		// parameter, but old versions take a char * and this cast
		// works with both)
		uint16_t	itemlength=
				(uint16_t)isc_vax_integer((char *)p,2);
		p=p+2;

		// get the lob length
		if (itemtype==isc_info_blob_total_length) {
			// (modern versions of isc_vax_integer take a
			// const char * parameter, but old versions take a
			// char * and this cast works with both)
			*length=isc_vax_integer((char *)p,itemlength);
		}
 
		// move on
		p=p+itemlength;
	}

	// close the blob
	isc_close_blob(firebirdconn->error,&field[col].blobhandle);

	return retval;
}

bool firebirdcursor::getLobFieldSegment(uint32_t col,
					char *buffer, uint64_t buffersize,
					uint64_t offset, uint64_t charstoread,
					uint64_t *charsread) {

	// ignore non-blobs
	if (field[col].sqlrtype!=BLOB_DATATYPE &&
			field[col].sqlrtype!=CLOB_DATATYPE) {
		return false;
	}

	// open the blob, if necessary
	if (!field[col].blobisopen) {
		field[col].blobhandle=0;
		if (isc_open_blob2(firebirdconn->error,
					&firebirdconn->db,
					&firebirdconn->tr,
					&field[col].blobhandle,
					&field[col].blobid,0,NULL)) {
			return false;
		}
		field[col].blobisopen=true;
	}

	// read a blob segment, at most MAX_LOB_CHUNK_SIZE bytes at a time
	uint64_t	totalbytesread=0;
	uint64_t	bytestoread=0;
	uint64_t	remainingbytestoread=charstoread;
	ISC_STATUS	status=0;
	for (;;) {

		// figure out how many bytes to read this time
		if (remainingbytestoread<MAX_LOB_CHUNK_SIZE) {
			bytestoread=remainingbytestoread;
		} else {
			bytestoread=MAX_LOB_CHUNK_SIZE;
			remainingbytestoread=remainingbytestoread-
						MAX_LOB_CHUNK_SIZE;
		}
		// read the bytes
		uint16_t	bytesread=0;
		status=isc_get_segment(firebirdconn->error,
					&field[col].blobhandle,
					&bytesread,
					bytestoread,
					buffer+totalbytesread);

		// bail on error
		if (status && status!=isc_segment) {
			break;
		}

		// update total bytes read
		totalbytesread=totalbytesread+bytesread;

		// bail if we're done reading
		if (bytesread<bytestoread || totalbytesread==charstoread) {
			break;
		}
	}

	// return number of bytes/chars read
	*charsread=totalbytesread;

	return true;
}

void firebirdcursor::closeLobField(uint32_t col) {

	// ignore non-blobs
	if (field[col].sqlrtype!=BLOB_DATATYPE &&
			field[col].sqlrtype!=CLOB_DATATYPE) {
		return;
	}

	// close the blob, if necessary
	if (field[col].blobisopen) {
		isc_close_blob(firebirdconn->error,&field[col].blobhandle);
		field[col].blobisopen=false;
	}
}

bool firebirdcursor::getArrayFieldDescriptor(uint32_t col,
					const unsigned char **descriptor,
					uint64_t *descriptorsize) {

	// ignore non-arrays
	if (field[col].sqlrtype!=ARRAY_DATATYPE) {
		return false;
	}

	// look the shape up, if we haven't already
	// (it's a property of the column rather than of the row, so one
	// lookup serves the whole result set - and it costs a query against
	// RDB$RELATION_FIELDS/RDB$FIELDS/RDB$FIELD_DIMENSIONS, so it's worth
	// not repeating)
	if (!field[col].arraydescvalid) {

		// isc_array_lookup_bounds() finds the column by name, so a
		// column that isn't a plain field of a table (an expression,
		// say) can't be looked up at all
		if (!outsqlda->sqlvar[col].relname_length ||
			!outsqlda->sqlvar[col].sqlname_length) {
			return false;
		}

		// the names are copied out by their lengths rather than
		// passed along as they are, since the sqlvar carries a length
		// for each of them and isc_array_lookup_bounds() wants
		// null-terminated strings
		char	relname[sizeof(outsqlda->sqlvar[col].relname)+1];
		char	sqlname[sizeof(outsqlda->sqlvar[col].sqlname)+1];
		charstring::copy(relname,outsqlda->sqlvar[col].relname,
					outsqlda->sqlvar[col].relname_length);
		relname[outsqlda->sqlvar[col].relname_length]='\0';
		charstring::copy(sqlname,outsqlda->sqlvar[col].sqlname,
					outsqlda->sqlvar[col].sqlname_length);
		sqlname[outsqlda->sqlvar[col].sqlname_length]='\0';

		bytestring::zero(&field[col].arraydesc,
					sizeof(field[col].arraydesc));

		if (isc_array_lookup_bounds(firebirdconn->error,
					&firebirdconn->db,
					&firebirdconn->tr,
					relname,sqlname,
					&field[col].arraydesc)) {
			// the reason is in the connection's status vector,
			// which whoever fetches the error next will render
			return false;
		}

		field[col].arraydescvalid=true;
	}

	*descriptor=(const unsigned char *)&field[col].arraydesc;
	*descriptorsize=(uint64_t)sizeof(field[col].arraydesc);
	return true;
}

bool firebirdcursor::fetchArrayField(uint32_t col) {

	// ignore non-arrays
	if (field[col].sqlrtype!=ARRAY_DATATYPE) {
		return false;
	}

	// the elements of this row's array are already in hand
	if (field[col].arraybuffervalid) {
		return true;
	}

	// the descriptor says how many elements there are and how wide each
	// one is
	const unsigned char	*descriptor=NULL;
	uint64_t		descriptorsize=0;
	if (!getArrayFieldDescriptor(col,&descriptor,&descriptorsize)) {
		return false;
	}

	ISC_ARRAY_DESC	*desc=&field[col].arraydesc;

	if (desc->array_desc_dimensions<1 ||
		desc->array_desc_dimensions>
			(short)(sizeof(desc->array_desc_bounds)/
				sizeof(desc->array_desc_bounds[0]))) {
		return false;
	}

	uint64_t	elementcount=1;
	for (short d=0; d<desc->array_desc_dimensions; d++) {
		int32_t	lower=desc->array_desc_bounds[d].array_bound_lower;
		int32_t	upper=desc->array_desc_bounds[d].array_bound_upper;
		if (upper<lower) {
			return false;
		}
		elementcount=elementcount*(uint64_t)(upper-lower+1);

		// bail rather than let the count wrap around
		// (an element is at least a byte wide, so a count past the
		// ceiling can't turn into a buffer under it, and testing it
		// here keeps the next dimension's multiply in range)
		if (elementcount>MAX_ARRAY_BUFFER_SIZE) {
			return false;
		}
	}

	uint32_t	elementsize=firebirdArrayElementSize(
					(byte_t)desc->array_desc_dtype,
					desc->array_desc_length);
	if (!elementcount || !elementsize) {
		return false;
	}

	uint64_t	buffersize=elementcount*elementsize;
	if (buffersize>MAX_ARRAY_BUFFER_SIZE) {
		return false;
	}

	// grow the buffer if this array needs more room than the last one did
	if (field[col].arraybuffer && field[col].arraybuffersize<buffersize) {
		delete[] field[col].arraybuffer;
		field[col].arraybuffer=NULL;
		field[col].arraybuffersize=0;
	}
	if (!field[col].arraybuffer) {
		field[col].arraybuffer=new byte_t[buffersize];
		field[col].arraybuffersize=buffersize;
	}

	// isc_array_get_slice() generates a whole-array SDL from the
	// descriptor itself, so there's nothing else to pass it, and it
	// answers how many bytes it actually wrote in slicelength
	ISC_LONG	slicelength=(ISC_LONG)buffersize;
	if (isc_array_get_slice(firebirdconn->error,
				&firebirdconn->db,
				&firebirdconn->tr,
				&field[col].quadbuffer,
				desc,
				field[col].arraybuffer,
				&slicelength)) {
		// the reason is in the connection's status vector
		return false;
	}

	if (slicelength<0 || (uint64_t)slicelength>buffersize) {
		slicelength=(ISC_LONG)buffersize;
	}

	field[col].arrayelementsize=elementsize;
	field[col].arrayelementcount=((uint64_t)slicelength)/elementsize;
	field[col].arraybuffervalid=true;

	return true;
}

bool firebirdcursor::getArrayFieldSlice(uint32_t col,
					char *buffer, uint64_t buffersize,
					uint64_t offset,
					uint64_t elementstoread,
					uint64_t *elementsread) {

	*elementsread=0;

	if (!fetchArrayField(col)) {
		return false;
	}

	// reading past the end isn't an error, it just reads nothing
	if (offset>=field[col].arrayelementcount) {
		return true;
	}

	// take the smallest of what was asked for, what's left, and what fits
	uint64_t	count=field[col].arrayelementcount-offset;
	if (count>elementstoread) {
		count=elementstoread;
	}
	uint64_t	fits=buffersize/field[col].arrayelementsize;
	if (count>fits) {
		count=fits;
	}

	bytestring::copy(buffer,
			field[col].arraybuffer+
				offset*field[col].arrayelementsize,
			(size_t)(count*field[col].arrayelementsize));

	*elementsread=count;

	return true;
}

void firebirdcursor::closeArrayField(uint32_t col) {

	// don't gate this on the column still being an array - a cursor that
	// ran a different query since (where this column isn't an array, or
	// doesn't exist at this index the same way) shouldn't be left holding
	// a stale, possibly large, array buffer.  freeing is harmless when
	// nothing was ever allocated.
	delete[] field[col].arraybuffer;
	field[col].arraybuffer=NULL;
	field[col].arraybuffersize=0;
	field[col].arrayelementcount=0;
	field[col].arrayelementsize=0;
	field[col].arraybuffervalid=false;

	// the descriptor is deliberately kept - it describes the column
	// rather than the row, and re-looking it up costs a query against the
	// system tables.  describeResultSet() drops it when the cursor runs
	// something else.
}

void firebirdcursor::closeResultSet() {

	// the result set buffers are deliberately left alone - the controller
	// caches column names that point into outsqldabuffer and doesn't
	// refresh them when a query is re-executed without being re-prepared
	// (describeResultSet() grows them when a query needs more columns,
	// so nothing is held beyond the widest result set this cursor has
	// returned)

	// close any lobs, and drop any buffered array elements, that were
	// left behind by an abandoned fetch
	for (int32_t i=0; i<fieldcount; i++) {
		closeLobField(i);
		closeArrayField(i);
	}

	outbindcount=0;
	resultsetdescribed=false;
	if (stmt) {
		isc_dsql_free_statement(firebirdconn->error,&stmt,DSQL_close);
	}
}

bool firebirdcursor::columnInfoIsValidAfterPrepare() {
	return true;
}

uint16_t firebirdcursor::getInputBindCountFromPrepare() {
	return inbindcountfromprepare;
}

uint16_t firebirdcursor::getInputBindType(uint16_t index) {
	if (index>=inbindcountfromprepare) {
		return UNKNOWN_DATATYPE;
	}
	return firebirdSqlTypeToDatatype(inbinddescribe[index].sqltype,
					inbinddescribe[index].sqlsubtype,
					inbinddescribe[index].sqlscale);
}

uint32_t firebirdcursor::getInputBindSize(uint16_t index) {
	if (index>=inbindcountfromprepare) {
		return 0;
	}
	return inbinddescribe[index].sqllen;
}

uint32_t firebirdcursor::getInputBindScale(uint16_t index) {
	if (index>=inbindcountfromprepare) {
		return 0;
	}
	return -inbinddescribe[index].sqlscale;
}

uint32_t firebirdcursor::getInputBindPrecision(uint16_t index) {

	if (index>=inbindcountfromprepare) {
		return 0;
	}

	// same mapping as getColumnPrecision(), except that the types it
	// answers 0 for fall through to sqllen here
	switch (getInputBindType(index)) {
		case CHAR_DATATYPE:
		case VARCHAR_DATATYPE:
		case BLOB_DATATYPE:
			return inbinddescribe[index].sqllen;
		case SMALLINT_DATATYPE:
			return 5;
		case INTEGER_DATATYPE:
			return 11;
		case NUMERIC_DATATYPE:
		case DECIMAL_DATATYPE:
			return firebirdNumericPrecisionFromSqlType(
					inbinddescribe[index].sqltype);
		case TIME_DATATYPE:
			return 8;
		case DATE_DATATYPE:
			return 10;
		default:
			return inbinddescribe[index].sqllen;
	}
}

bool firebirdcursor::getInputBindIsNullable(uint16_t index) {
	if (index>=inbindcountfromprepare) {
		return false;
	}
	// the low bit of sqltype indicates nullability
	return inbinddescribe[index].sqltype&1;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrserverconnection *new_firebirdconnection(
						sqlrservercontroller *cont) {
		return new firebirdconnection(cont);
	}
}
