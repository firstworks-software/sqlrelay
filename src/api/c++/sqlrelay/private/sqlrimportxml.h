// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information.

	private:
		bool	tagStart(const char *ns, const char *name);
		bool	attributeName(const char *name);
		bool	attributeValue(const char *value);
		bool	text(const char *string);
		char	*unescapeValue(const char *value);
		bool	tagEnd(const char *ns, const char *name);

		bool	tableTagStart();
		bool	columnsTagStart();
		bool	columnTagStart();
		bool	columnTagEnd();
		bool	columnsTagEnd();
		bool	rowsTagStart();
		bool	rowTagStart();
		bool	fieldTagStart();
		bool	rowTagEnd();
		bool	fieldTagEnd();
		bool	rowsTagEnd();
		bool	tableTagEnd();
		bool	sequenceTagStart();
		bool	sequenceTagEnd();

		unsigned short	currenttag;
		char		*currentattribute;
		char		*cname;
		bool		infield;
		char		*fval;
		char		*sequencevalue;

		static const unsigned short	NULLTAG;
		static const unsigned short	TABLETAG;
		static const unsigned short	SEQUENCETAG;
		static const unsigned short	COLUMNSTAG;
		static const unsigned short	COLUMNTAG;
		static const unsigned short	ROWSTAG;
		static const unsigned short	ROWTAG;
		static const unsigned short	FIELDTAG;

		static const unsigned short	NULLATTR;
		static const unsigned short	NAMEATTR;
		static const unsigned short	TYPEATTR;
		static const unsigned short	LENGTHATTR;
		static const unsigned short	PRECISIONATTR;
		static const unsigned short	SCALEATTR;
		static const unsigned short	NULLABLEATTR;
		static const unsigned short	PRIMARYKEYATTR;
		static const unsigned short	UNIQUEATTR;
		static const unsigned short	PARTOFKEYATTR;
		static const unsigned short	UNSIGNEDATTR;
		static const unsigned short	ZEROFILLEDATTR;
		static const unsigned short	BINARYATTR;
		static const unsigned short	AUTOINCREMENTATTR;
