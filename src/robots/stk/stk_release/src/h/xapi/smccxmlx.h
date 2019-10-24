/** HEADER FILE PROLOGUE *********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      smccxmlx.h                                       */
/** Description:    XAPI client XML parser component common          */
/**                 header file.                                     */
/**                                                                  */
/**                 Public definitions used in the CDK XML parser.   */
/**                                                                  */
/**                 The XMLPARSE is the main structure from which    */
/**                 all other parse structures are anchored.  These  */
/**                 other parse structures are:                      */
/**                 XMLRAWIN: Describes raw input to the parser.     */
/**                 XMLCNVIN: Describes a converted XML blob to the  */
/**                   parser.  The XMLRAWIN may be ASCII, EBCDIC,    */
/**                   etc.  But the corresponding XMLCNVIN will      */
/**                   always be ASCII.                               */
/**                 XMLDECL:  Describes an XML declaration.  There   */
/**                   may be more than one in the chain.  XML        */
/**                   declarations, if present, will be the first    */
/**                   objects found in the XML blob.  We allow more  */
/**                   than one, although true XML does not.          */
/**                 XMLCOMM:  Describes an XML comment.  There may   */
/**                   be more than one in the chain. XML comments    */
/**                   may be found anywhere in the XML blob.         */
/**                   Because they are not particularly useful as    */
/**                   part of the XML data hierarchy, we chain them  */
/**                   together from the XMLPARSE, and do not try to  */
/**                   maintain their spatial representation.         */
/**                 XMLELEM:  Form the leaves of the XML data        */
/**                   hierarchy.  These represent XML elements.      */
/**                   They have a name, and optional content.        */
/**                   They also have pointers to parent, oldest      */
/**                   child, and older, and younger siblings.        */
/**                 XMLATTR:  Describe and XML attribute.            */
/**                   Attributes are associated with elements, and   */
/**                   are a keyword=value pair.  Elements are        */
/**                   optional.  If present. they will be pointed to */
/**                   to from their corresponding XMLELEM.  There    */
/**                   may be more than one attribute for an element. */
/**                                                                  */
/** Example of XMLPARSE structures:                                  */
/**                                                                  */
/**                 XMLPARSE                                         */
/**                    |                                             */
/**           +--------+--------+-------------+--------+             */
/**           |        |        |             |        |             */
/**           |        |        |             |        |             */
/**           V        v        v (1)         v        v             */
/**        XMLRAWIN XMLCNVIN XMLELEM       XMLDECL  XMLCOMM          */
/**                             ^             |        |             */
/**                             |             v        v             */
/**                    +--------+          XMLDECL  XMLCOMM          */
/**                    |                                             */
/**                    v (2)         (3)          (4)                */
/**                 XMLELEM<--->XMLELEM<---->XMLELEM---->XMLATTR     */
/**                    ^                        ^                    */
/**                    |                        |                    */
/**                    |                        |                    */
/**            +-------+                +-------+                    */
/**            |                        |                            */
/**            |                        |                            */
/**            v (5)          (6)       v (7)                        */
/**         XMLELEM<---->XMLELEM     XMLELEM---->XMLATTR---->XMLATTR */
/**                                                                  */
/** In the above example, XMLELEM (1) is the parent of XMLELEM (2)   */
/** through (4).  XMLELEM (2) has two children, XMLELEM (5) and (6). */
/** XMLELEM (4) has 1 child, XMLELEM (7).  XMLELEM (4) had one       */
/** attribute=value pair, while XMLELEM (7) had two attribute-value  */
/** pairs.                                                           */
/**                                                                  */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     05/11/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/** END PROLOGUE *****************************************************/

#ifndef SMCCXMLX_HEADER
#define SMCCXMLX_HEADER


/*********************************************************************/
/* Include srvcommon.h if not already included.                      */
/*********************************************************************/
#ifndef SRVCOMMON_HEADER
    #include "srvcommon.h"
#endif


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#define MAX_COMMAND_LEN                4096
#define TOKEN_MAX_LEN                  79
#define TOKEN_WIDTH                    TOKEN_MAX_LEN + 1
#define TOKEN_MAX_NUM                  255

#define TOKEN_BEGQUOTE_ALIAS           16
#define TOKEN_ENDQUOTE_ALIAS           17
#define TOKEN_PAREN_MARKER             29

#define TOKEN_RETURNED                 0
#define TOKEN_IS_NULL                  4
#define TOKEN_TRUNCATED                8
#define TOKEN_END_OF_LINE              -1

#define TOKEN_BEGLIST                  "LEFTPAREN"
#define TOKEN_ENDLIST                  "RIGHTPAREN"

#define XML_TAG_MAX_LEN                32


/*********************************************************************/
/* XML function names                                                */
/*********************************************************************/
#define FN_ADD_ATTR_TO_HIERARCHY               smccxcs3
#define FN_ADD_ELEM_TO_HIERARCHY               smccxcs1
#define FN_GENERATE_XML_FROM_HIERARCHY         smccxcs2 
#define FN_FREE_HIERARCHY_STORAGE              smccxcs5

#define FN_XML_ASCII_TO_ASCII                  smccxcv1
#define FN_XML_EBCDIC_TO_ASCII                 smccxcv2

#define FN_FIND_ELEMENT_BY_NAME                smccxfe1
#define FN_FIND_NEXT_CHILD_ELEMENT             smccxfe2
#define FN_FIND_ATTRIBUTE_BY_KEYWORD           smccxfe3
#define FN_FIND_NEXT_SIBLING_BY_NAME           smccxfe5

#define FN_PARSE_XML                           smccxml1

#define FN_MOVE_XML_ELEMS_TO_STRUCT            smccxmv1
#define FN_MOVE_XML_ATTRS_TO_STRUCT            smccxmv2
#define FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT      smccxmv3

#define FN_CREATE_XMLPARSE                     smccxps1
#define FN_CREATE_XMLRAWIN                     smccxps2
#define FN_CREATE_XMLCNVIN                     smccxps3
#define FN_CREATE_XMLDECL                      smccxps4
#define FN_CREATE_XMLCOMM                      smccxps5
#define FN_CREATE_XMLELEM                      smccxps6
#define FN_CREATE_XMLATTR                      smccxps7

#define FN_GENERATE_HIERARCHY_FROM_XML         smccxtk1           
#define FN_PARSE_DECL                          smccxtk2
#define FN_PARSE_COMM                          smccxtk3
#define FN_PARSE_ELEM                          smccxtk4
#define FN_PARSE_ATTR                          smccxtk5
#define FN_PARSE_NEXT_STRING                   smccxtk6
#define FN_PARSE_NEXT_NONBLANK                 smccxtk7
#define FN_XML_RESTORE_SPECIAL_CHARS           smccxtk8

#define FN_CONVERT_ASCII_TO_ASCII              smcsasc1
#define FN_CONVERT_ASCII_TO_EBCDIC             smcsasc2
#define FN_CONVERT_EBCDIC_TO_ASCII             smcsasc3

#define FN_CONVERT_CHARHEX_TO_FULLWORD         smcsfmt1
#define FN_CONVERT_CHARHEX_TO_DOUBLEWORD       smcsfmt2
#define FN_CONVERT_CHARDEVADDR_TO_HEX          smcsfmt3
#define FN_CONVERT_BINARY_TO_CHARHEX           smcsfmt5
#define FN_CONVERT_CHARHEX_TO_BINARY           smcsfmt6
#define FN_CONVERT_DIGITS_TO_FULLWORD          smcsfmt7

#define FN_TOKEN_CONVERT_DELIMITERS_IN_QUOTES  smcstok1
#define FN_TOKEN_TEST_MISMATCHED_PARENS        smcstok2
#define FN_TOKEN_REMOVE_PAREN_STRINGS          smcstok3
#define FN_TOKEN_RETURN_NEXT                   smcstok4


/*********************************************************************/
/* Special XAPI-UUI tag names.                                       */
/*===================================================================*/
/* NOTE: While the CDK common XML parser attempts to be a stateless  */
/* parser in the manner of expat, the parsing logic within smccxtk   */
/* will look for the following tags and allow "<" and ">" signs      */
/* within their text content.  This is not correct under the         */
/* applicable RFC but is allowed for XAPI compatibility.             */
/*********************************************************************/
#define STR_UUI_TEXT                   "uui_text"
#define SIZEOF_STR_UUI_TEXT            (sizeof(STR_UUI_TEXT) - 1)

#define STR_REASON                     "reason"
#define SIZEOF_STR_REASON              (sizeof(STR_REASON) - 1)


/*********************************************************************/
/* XML String Equates, Strings, And Sizes                            */
/*===================================================================*/
/* NOTE: Only a subset of XML is supported.                          */
/* XSL, DCL tags are not supported.                                  */
/* NOTE: We use SIZEOF due to C's sizeof() adding the string         */
/* terminator.                                                       */
/*********************************************************************/
#define XML_NOTE_STARTTAG              1
#define STR_NOTE_STARTTAG              "<!NOTATION "
#define SIZEOF_STR_NOTE_STARTTAG       (sizeof(STR_NOTE_STARTTAG) - 1)

#define XML_ENTITY_STARTTAG            2
#define STR_ENTITY_STARTTAG            "<!ENTITY "
#define SIZEOF_STR_ENTITY_STARTTAG     (sizeof(STR_ENTITY_STARTTAG) - 1)

#define XML_PUBLIC                     3
#define STR_PUBLIC                     " PUBLIC "
#define SIZEOF_STR_PUBLIC              (sizeof(STR_PUBLIC) -1)

#define XML_SYSTEM                     4
#define STR_SYSTEM                     " SYSTEM "
#define SIZEOF_STR_SYSTEM              (sizeof(STR_SYSTEM) -1)

#define XML_START_CDATA                5
#define STR_START_CDATA                "[CDATA["
#define SIZEOF_STR_START_CDATA         (sizeof(STR_START_CDATA) - 1)

#define XML_EQU_APOST                  6
#define STR_EQU_APOST                  "&apos;"
#define SIZEOF_STR_EQU_APOST           (sizeof(STR_EQU_APOST) - 1)

#define XML_EQU_QUOTE                  7
#define STR_EQU_QUOTE                  "&quot;"
#define SIZEOF_STR_EQU_QUOTE           (sizeof(STR_EQU_QUOTE) - 1)

#define XML_EQU_AMPER                  8
#define STR_EQU_AMPER                  "&amp;"
#define SIZEOF_STR_EQU_AMPER           (sizeof(STR_EQU_AMPER) - 1)

#define XML_EQU_GREATTHAN              9
#define STR_EQU_GREATTHAN              "&gt;"
#define SIZEOF_STR_EQU_GREATTHAN       (sizeof(STR_EQU_GREATTHAN) - 1)

#define XML_EQU_LESSTHAN               10
#define STR_EQU_LESSTHAN               "&lt;"
#define SIZEOF_STR_EQU_LESSTHAN        (sizeof(STR_EQU_LESSTHAN) - 1)

#define XML_CDATA_ENDTAG               11
#define STR_CDATA_ENDTAG               "]]>"
#define SIZEOF_STR_CDATA_ENDTAG        (sizeof(STR_CDATA_ENDTAG) - 1)

#define XML_ELEMEND_STARTTAG           12
#define STR_ELEMEND_STARTTAG           "</"
#define SIZEOF_STR_ELEMEND_STARTTAG    (sizeof(STR_ELEMEND_STARTTAG) - 1)

#define XML_EMPTYELEM_ENDTAG           13
#define STR_EMPTYELEM_ENDTAG           "/>"
#define SIZEOF_STR_EMPTYELEM_ENDTAG    (sizeof(STR_EMPTYELEM_ENDTAG) - 1)

#define XML_COMMENT_STARTTAG           14
#define STR_COMMENT_STARTTAG           "<!"
#define SIZEOF_STR_COMMENT_STARTTAG    (sizeof(STR_COMMENT_STARTTAG) - 1)

#define XML_DECL_STARTTAG              15
#define STR_DECL_STARTTAG              "<?"
#define SIZEOF_STR_DECL_STARTTAG       (sizeof(STR_DECL_STARTTAG) - 1)

#define XML_DECL_ENDTAG                16
#define STR_DECL_ENDTAG                "?>"
#define SIZEOF_STR_DECL_ENDTAG         (sizeof(STR_DECL_ENDTAG) - 1)

#define XML_STARTTAG                   17
#define STR_STARTTAG                   "<"
#define SIZEOF_STR_STARTTAG            (sizeof(STR_STARTTAG) - 1)

#define XML_ENDTAG                     18
#define STR_ENDTAG                     ">"
#define SIZEOF_STR_ENDTAG              (sizeof(STR_ENDTAG) - 1)

#define XML_EQUALSIGN                  19
#define STR_EQUALSIGN                  "="
#define SIZEOF_STR_EQUALSIGN           (sizeof(STR_EQUALSIGN) - 1)

#define XML_QUOTE                      20
#define STR_QUOTE                      "\""
#define SIZEOF_STR_QUOTE               (sizeof(STR_QUOTE) - 1)

#define XML_APOST                      21
#define STR_APOST                      "'"
#define SIZEOF_STR_APOST               (sizeof(STR_APOST) - 1)

#define XML_COMMA                      22
#define STR_COMMA                      ","
#define SIZEOF_STR_COMMA               (sizeof(STR_COMMA) - 1)

#define XML_BLANK                      98
#define STR_BLANK                      " "
#define SIZEOF_STR_BLANK               (sizeof(STR_BLANK) - 1)

#define XML_PLAIN_TEXT                 99
#define SIZEOF_STR_PLAIN_TEXT          1


/*********************************************************************/
/* XML Error Codes:                                                  */
/*===================================================================*/
/* Set in XMLPARSE.errorCode                                         */
/*********************************************************************/
#define ERR_MALFORMED_INPUT            1
#define ERR_ALL_BLANK                  2
#define ERR_NO_START_TAG               3
#define ERR_INPUT_CALC_OVERFLOW        4
#define ERR_NAKED_XML_STRING           5
#define ERR_MISMATCHED_TAGS            6
#define ERR_UNEXPECTED_STARTTAG        7
#define ERR_UNEXPECTED_ENDTAG          8
#define ERR_UNEXPECTED_CDATA           9
#define ERR_UNEXPECTED_DECL            10
#define ERR_NO_DECL_END                11
#define ERR_NO_COMM_END                12
#define ERR_NO_ELEM_END                13
#define ERR_NO_ELEM_START_NAME_END     14
#define ERR_NO_ELEM_END_NAME_END       15
#define ERR_NO_ELEM_CONTENT_END        16
#define ERR_NO_ATTR_END                17
#define ERR_NO_ATTR_NAME_END           18
#define ERR_NO_ATTR_VALUE_END          19
#define ERR_STARTTAG_INSIDE_DECL       20
#define ERR_STARTTAG_INSIDE_COMM       21
#define ERR_STARTTAG_INSIDE_START_NAME 22
#define ERR_STARTTAG_INSIDE_END_NAME   23
#define ERR_XMLTAG_INSIDE_ATTR_NAME    24
#define ERR_XMLTAG_INSIDE_ATTR_VALUE   25
#define ERR_NO_XMLELEM                 26
#define ERR_NO_LIBTRANS_DATA           27
#define ERR_UNKNOWN_LIBTRANS_VALUE     28
#define ERR_ROOT_NOT_LIBTRANS          29
#define ERR_ROOT_NOT_LIBREPLY          30
#define ERR_NO_LIBREPLY_DATA           31
#define ERR_NO_ERRORCODE_DATA          32
#define ERR_TAG_TOO_LONG               33
#define ERR_CONTENT_TOO_LONG           34
#define ERR_ATTR_TOO_LONG              35
#define ERR_ATTR_VALUE_TOO_LONG        36
#define ERR_CHAR_COUNT_LOGIC           98
#define ERR_UNSUPPORTED_FUNC           99


/*********************************************************************/
/* Structures:                                                       */
/*********************************************************************/

/*********************************************************************/
/* XMLRAWIN: XML Raw Input Structure                                 */
/*                                                                   */
/* Used in XMLPARSE below.                                           */
/*===================================================================*/
/* Describe raw input to the XML parser.                             */
/*********************************************************************/
struct XMLRAWIN
{
    struct XMLRAWIN   *pNextXmlrawin;  /* Addr of next XMLRAWIN      */
    int                xmlrawinSize;   /* Size of this XMLRAWIN      */
    int                dataLen;        /* Length of data that follows*/
    /*****************************************************************/
    /* The following field is variable length, and is the number     */
    /* of raw input bytes for "this" piece of the input.             */
    /*****************************************************************/
    char               pData[2];       /* Data                       */
};


/*********************************************************************/
/* XMLCNVIN: XML Converted Input Structure                           */
/*                                                                   */
/* Used in XMLPARSE below.                                           */
/*===================================================================*/
/* Describe converted input to the XML parser.                       */
/*********************************************************************/
struct XMLCNVIN
{
    struct XMLCNVIN   *pNextXmlcnvin;  /* Addr of next XMLCNVIN      */
    int                xmlcnvinSize;   /* Size of this XMLCNVIN      */
    int                dataLen;        /* Length of data that follows*/
    int                nonTextCount;   /* Num or non-text characters */
    /*                                    ...found after conversion  */
    struct XMLCNVIN   *pNextTryXmlcnvin;   /* Addr of next XMLCNVIN  */
    /*                                    ...to try best conversion  */
    void              *pRoutine;       /* Addr of conversion routine */
    char               inputEncoding;  /* Input encoding type flag   */
    char               _f0[3];         /* Reserved                   */
    /*****************************************************************/
    /* The following field is variable length, and is the number     */
    /* of converted input bytes for "this" piece of the input.       */
    /*****************************************************************/
    char               pData[2];       /* Data                       */
};


/*********************************************************************/
/* XMLLEAF: XML Generic Data Hierarchy Structure Header              */
/*===================================================================*/
/* Describe the 1st 8 bytes of XMLDECL, XMLCOMM, XMLELEM, XMLATTR    */
/* structures.                                                       */
/*********************************************************************/
struct XMLLEAF
{
    char               type[4];        /* Structure type literal     */
    int                sequence;       /* Absolute sequence number   */
};


/*********************************************************************/
/* XMLDECL: XML Declaration Structure                                */
/*                                                                   */
/* Used in XMLPARSE below.                                           */
/*===================================================================*/
/* Describe a declaration statement found in the XML blob.           */
/* An XML blob may have from 0 to n declaration statements.          */
/* Between decl start tag and end tag is just free form text.        */
/* We do not allow any imbedded naked XML start tags or end tags.    */
/*********************************************************************/
struct XMLDECL
{
    char               type[4];        /* Type "DECL"                */
    int                sequence;       /* Absolute sequence number   */
    struct XMLDECL    *pNextXmldecl;   /* Addr of next XMLDECL       */
    char               flag;           /* Declaration processing flag*/
#define DECL_START     '\x80'          /* ...Declaration started     */
#define DECL_END       '\x40'          /* ...Declaration ended       */
    char               _f0[3];         /* Reserved                   */
    int                declLen;        /* Length of declaration      */
    char              *pDeclaration;   /* Addr of declaration        */
    int                _f1[2];         /* Reserved                   */
};


/*********************************************************************/
/* XMLCOMM: XML Comment Structure                                    */
/*                                                                   */
/* Used in XMLPARSE below.                                           */
/*===================================================================*/
/* Describe a comment statement found in the XML blob.               */
/* An XML blob may have from 0 to n comments.                        */
/* Between comment start tag and end tag is just free form text.     */
/* We do not allow any imbedded naked XML start tags or end tags.    */
/*********************************************************************/
struct XMLCOMM
{
    char               type[4];        /* Type "COMM"                */
    int                sequence;       /* Absolute sequence number   */
    int                parentSequence; /* Parent sequence number     */
    struct XMLCOMM    *pNextXmlcomm;   /* Addr of next XMLCOMM       */
    char               flag;           /* Comment processing flag    */
#define COMM_START     '\x80'          /* ...Comment started         */
#define COMM_END       '\x40'          /* ...Comment ended           */
    char               _f0[3];         /* Reserved                   */
    int                commLen;        /* Length of comment          */
    char              *pComment;       /* Addr of comment            */
    int                _f1;            /* Reserved                   */
};


/*********************************************************************/
/* XMLATTR: XML Attribute Structure                                  */
/*                                                                   */
/* Used in XMLELEM below.                                            */
/*===================================================================*/
/* Describe an attribute of an XML element.                          */
/* An XML element may have from 0 to n attributes.                   */
/*********************************************************************/
struct XMLATTR
{
    char               type[4];        /* Type "ATTR"                */
    int                sequence;       /* Absolute sequence number   */
    struct XMLELEM    *pParentXmlelem; /* Addr of parent XMLELEM     */
    struct XMLATTR    *pNextXmlattr;   /* Addr of next XMLATTR       */
    char               flag;           /* Attribute processing flag  */
#define ATTR_START     '\x80'          /* ...Attribute started       */
#define ATTR_END       '\x40'          /* ...Attribute ended         */
#define VALUE_START    '\x20'          /* ...Attribute started       */
#define VALUE_END      '\x10'          /* ...Attribute ended         */
    char               valueDelimiter; /* Value delimiter (' or ")   */
    char               _f0[2];         /* Reserved                   */
    int                keywordLen;     /* Length of keyword          */
    int                valueLen;       /* Length of value            */
    char              *pKeyword;       /* Addr of keyword            */
    char              *pValue;         /* Addr of value              */
    int                _f1[3];         /* Reserved                   */
};


/*********************************************************************/
/* XMLELEM: XML Element Structure                                    */
/*                                                                   */
/* Used in XMLPARSE below.                                           */
/*===================================================================*/
/* Describe an XML element leaf within the XML tree structure.       */
/* an XMLELEM contains information about elements, its contents,     */
/* and pointers to position the element within the data tree.  It    */
/* will also contain pointers to the element's attributes (if        */
/* specified).                                                       */
/*********************************************************************/
struct XMLELEM
{
    char               type[4];        /* Type "ELEM"                */
    int                sequence;       /* Absolute sequence number   */
    int                level;          /* Element level              */
    struct XMLELEM    *pParentXmlelem; /* Addr of parent XMLELEM     */
    struct XMLELEM    *pOlderXmlelem;  /* Addr of older sibling      */
    struct XMLELEM    *pYoungerXmlelem;/* Addr younger sibling       */
    struct XMLELEM    *pChildXmlelem;  /* Addr of child element      */
    struct XMLATTR    *pHocXmlattr;    /* Addr of h-o-c XMLATTR      */
    struct XMLATTR    *pCurrXmlattr;   /* Addr of current XMLATTR    */
    char               flag;           /* Element processing flag    */
#define ELEM_START         '\x80'      /* ...Element started         */
#define ELEM_END           '\x40'      /* ...Element ended           */
#define START_NAME_START   '\x20'      /* ...Elem start name started */
#define START_NAME_END     '\x10'      /* ...Elem start name ended   */
#define CONTENT_START      '\x08'      /* ...Content started         */
#define CONTENT_END        '\x04'      /* ...Content ended           */
#define END_NAME_START     '\x02'      /* ...Elem end name started   */
#define END_NAME_END       '\x01'      /* ...Elem end name ended     */
    char               _f0[3];         /* Reserved                   */
    int                startElemLen;   /* Length of start elem name  */
    int                endElemLen;     /* Length of end elem name    */
    int                contentLen;     /* Length of content          */
    char              *pStartElemName; /* Addr of start elem name    */
    char              *pEndElemName;   /* Addr of end elem name      */
    char              *pContent;       /* Addr of content            */
    char              *pTranslatedContent; /* Addr of content with   */
    /*                                    ...special chars translated*/
    int                translatedLen;  /* Length of translated       */
    /*                                    ...content                 */
    int                _f1[2];         /* Reserved                   */
};


/*********************************************************************/
/* XMLDATA: XML Data Hierarchy Persistent Data Copy                  */
/*                                                                   */
/* Used in XMLPARSE below.                                           */
/*===================================================================*/
/* SMC creates data hierarchy trees internally, without real XML     */
/* input, and without XMLRAWIN or XMLCNVIN buffers.  The SMC will do */
/* this to eventually generate its own XML transactions.  Because    */
/* the input data may not be as persistent as the XMLPARSE structure,*/
/* the SMC may need to copy the data in order to generate the XML    */
/* blob.  The XMLDATA is a persistent copy (at least as persistent   */
/* as the XMLPARSE structure to which it is linked) of the element   */
/* name and element content, or attribute keyword and attribute      */
/* value, from which the actual XML blob will later be generated.    */
/*********************************************************************/
struct XMLDATA
{
    char               type[4];        /* Type "DATA"                */
    int                xmldataLen;     /* Length of this XMLDATA     */
    char               flag;           /* Data flag                  */
#define ELEM_DATA      '\x80'          /* ...XMLELEM data            */
#define ATTR_DATA      '\x40'          /* ...XMLATTR data            */
    char               _f0[3];         /* Reserved                   */
    struct XMLDATA    *pNextXmldata;   /* Addr of next XMLDATA       */
    int                data1Len;       /* Elem name/keyword name len */
    int                data2Len;       /* Content/value len          */
    char               data[1];        /* Actual data follows        */
};


/*********************************************************************/
/* XMLPARSE: XML Parse Control Structure                             */
/*===================================================================*/
/* Describe XML Parse control structure.                             */
/*********************************************************************/
struct XMLPARSE
{
    struct XMLRAWIN   *pHocXmlrawin;   /* Addr of h-o-c XMLRAWIN     */
    struct XMLRAWIN   *pCurrXmlrawin;  /* Addr of current XMLRAWIN   */
    struct XMLCNVIN   *pHocXmlcnvin;   /* Addr of h-o-c XMLCNVIN     */
    struct XMLCNVIN   *pCurrXmlcnvin;  /* Addr of current XMLCNVIN   */
    void              *pConvRoutine;   /* Addr of data conv routine  */
    char               leafEye[4];     /* Eyecatcher "LEAF"          */
    struct XMLDECL    *pHocXmldecl;    /* Addr of h-o-c XMLDECL      */
    struct XMLDECL    *pCurrXmldecl;   /* Addr of current XMLDECL    */
    struct XMLCOMM    *pHocXmlcomm;    /* Addr of h-o-c XMLCOMM      */
    struct XMLCOMM    *pCurrXmlcomm;   /* Addr of current XMLCOMM    */
    struct XMLELEM    *pHocXmlelem;    /* Addr of h-o-c XMLELEM      */
    struct XMLELEM    *pCurrXmlelem;   /* Addr of curr XMLELEM       */
    struct XMLDATA    *pHocXmldata;    /* Addr of h-o-c XMLDATA      */
    struct XMLDATA    *pCurrXmldata;   /* Addr of current XMLDATA    */
    void              *pCurrLeaf;      /* Current leaf processing    */
    int                currLevel;      /* Current level              */
    int                maxLevel;       /* Highest level              */
    char               bptrEye[4];     /* Eyecatcher "BPTR"          */
    int                charCount;      /* Characters to parse        */
    int                charParsed;     /* Characters already parsed  */
    int                charRemaining;  /* Characters remaining       */
    char              *pBufferBeg;     /* Start of buffer            */
    char              *pBufferEnd;     /* End of buffer              */
    char              *pBufferPos;     /* Current position in buffer */
    char              *pBufferLast;    /* Position of last parse     */
    char               xflgEye[4];     /* Eyecatcher "XFLG"          */
    int                starttagCount;  /* Number of start tags       */
    int                endtagCount;    /* Number of end tags         */
    int                startElemCount; /* Start element count        */
    int                endElemCount;   /* End element count          */
    int                lastParseLen;   /* Length of last parse       */
    char               inputEncoding;  /* Input encoding flag        */
#define ASCII          'A'             /* ...US-ASCII encoding       */
#define EBCDIC         'E'             /* ...IBM-EBCDIC encoding     */
    char               endOfXml;       /* End of input flag          */
    char               _f0[2];         /* Reserved                   */
    char               xerrEye[4];     /* Eyecatcher "XERR"          */
    void              *pErrLeaf;       /* Leaf in error              */
    char              *pErrBufferPos;  /* pBufferPos at error        */
    char              *pErrBufferLast; /* pBufferLast at error       */
    int                errorCode;      /* Error code                 */
    int                reasonCode;     /* Error reason               */
    int                _f1[6];         /* Reserved                   */
};


/*********************************************************************/
/* XMLSTRUCT XML Structure Descriptor                                */
/*===================================================================*/
/* Describe the structure used to control the translation of an      */
/* XMLPARSE data hierarchy to a combination of formatted and         */
/* unformatted structures.                                           */
/*********************************************************************/
struct XMLSTRUCT
{
    char               parentName[36]; /* Parent XMLELEM name        */
    char               elemName[36];   /* XMLELEM name to translate  */
    int                fieldLen;       /* Target field length        */
    char              *pField;         /* Addr of target field       */
    char               fillFlag;       /* Field fill flag            */
#define NOBLANKFILL    0               /* ...Left justify, no fill;  */
    /*                                    ...output field not        */
    /*                                    ...initialized             */
    /*                                    ...(default)               */
#define BLANKFILL      1               /* ...Left justify, blank     */
    /*                                    ...fill; output field      */
    /*                                    ...initialized with " "    */
#define ZEROFILL       2               /* ...Right justify, zero     */
    /*                                    ...fill if len > 0; if any */
    /*                                    ...source, then pad output */
    /*                                    ...with "0" on left        */
#define ZEROFILLALL    3               /* ...Right justify, zero     */
    /*                                    ...filled; output field    */
    /*                                    ...initialized with "0"    */
    char               bitValue;       /* Bit value flag; whether    */
    /*                                    ...to OR output character  */
    /*                                    ...with specified value    */
    /*                                    ...if source is "Y"(ES)    */
#define NOBITVALUE     0               /* ...(default is off)        */ 
    char               _f0[2];         /* Alignment                  */
};


/*********************************************************************/
/* Macros:                                                           */
/*********************************************************************/

/*********************************************************************/
/* XML_BYPASS_INPUT                                                  */
/*===================================================================*/
/* Update the XMLPARSE structure to indicate that the specified      */
/* number of characters in the input are to be bypassed (i.e. they   */
/* are now processed).  If we reach the end of the buffer, then flag */
/* the condition in the return code.                                 */
/*********************************************************************/
#define XML_BYPASS_INPUT(retcodE, pXmlparsE, numberCharS) \
do \
{ \
    if (pXmlparsE->charRemaining < numberCharS) \
    { \
        TRMEM(pXmlparsE->pBufferPos, 16, \
              "Flagging INPUT_CALC_OVERFLOW; charParsed=%i, " \
              "pBufferPos=%08X, data:\n", \
              pXmlparsE->charParsed, pXmlparsE->pBufferPos); \
        pXmlparsE->pErrLeaf = pXmlparsE->pCurrLeaf; \
        pXmlparsE->pErrBufferPos = pXmlparsE->pBufferPos; \
        pXmlparsE->pErrBufferLast = pXmlparsE->pBufferLast; \
        pXmlparsE->errorCode = ERR_INPUT_CALC_OVERFLOW; \
        pXmlparsE->endOfXml = TRUE; \
        retcodE = pXmlparsE->errorCode; \
    } \
    else \
    { \
        pXmlparsE->pBufferPos += numberCharS; \
        pXmlparsE->charParsed += numberCharS; \
        pXmlparsE->charRemaining -= numberCharS; \
        if (pXmlparsE->charRemaining == 0) \
        { \
            pXmlparsE->endOfXml = TRUE; \
        } \
        retcodE = RC_SUCCESS; \
    } \
}while(0)


/*********************************************************************/
/* XML_BYPASS_AND_MARK_ERROR                                         */
/*===================================================================*/
/* Update the XMLPARSE structure to indicate that we encounted the   */
/* specified error.  For instance, we encountered a start            */
/* declaration while in the middle of another start declaration.     */
/* We update the XMLPARSE to bypass the bad input, and update the    */
/* error fields.                                                     */
/*********************************************************************/
#define XML_BYPASS_AND_MARK_ERROR(errorCodE, pXmlparsE, numberCharS) \
do \
{ \
    auto int retcodE; \
    TRMEM(pXmlparsE->pBufferPos, 16, \
          "Flagging errorCode=%i; charParsed=%i, pBufferPos=%08X, data:\n", \
          errorCodE, pXmlparsE->charParsed, pXmlparsE->pBufferPos); \
    pXmlparsE->pErrLeaf = pXmlparsE->pCurrLeaf; \
    pXmlparsE->pErrBufferPos = pXmlparsE->pBufferPos; \
    pXmlparsE->pErrBufferLast = pXmlparsE->pBufferLast; \
    pXmlparsE->errorCode = errorCodE; \
    if (numberCharS > 0) \
    { \
        XML_BYPASS_INPUT(retcodE, pXmlparsE, numberCharS); \
    } \
}while(0)


/*********************************************************************/
/* XML_VERIFY_XMLELEM_IN_XMLPARSE                                    */
/*===================================================================*/
/* Verify that the XMLELEM structure exists in the current XMLPARSE. */
/* We keep following the parent XMLELEM pointers until we reach      */
/* the root XMLELEM.  If this matches the pHocXmlelem, then we       */
/* indicate RC_SUCCESS.                                              */
/*********************************************************************/
#define XML_VERIFY_XMLELEM_IN_XMLPARSE(retcodE, pXmleleM, pXmlparsE) \
do \
{ \
    auto struct XMLELEM *pParentXmleleM = pXmleleM; \
    retcodE = RC_FAILURE; \
    while (pParentXmleleM->pParentXmlelem != NULL) \
    { \
        pParentXmleleM = pParentXmleleM->pParentXmlelem; \
    } \
    if (pParentXmleleM == pXmlparsE->pHocXmlelem) \
    { \
        retcodE = RC_SUCCESS; \
    } \
}while(0)


/*********************************************************************/
/* XML_FIND_LAST_SIBLING_IN_XMLELEM                                  */
/*===================================================================*/
/* Point to the youngest (last) sibling for the specified XMLELEM.   */
/* A NULL value indicates that the XMLELEM is childless.             */
/*********************************************************************/
#define XML_FIND_LAST_SIBLING_IN_XMLELEM(pXmleleM, pLastSiblingXmleleM) \
do \
{ \
    pLastSiblingXmleleM = pXmleleM->pChildXmlelem; \
    if (pLastSiblingXmleleM != NULL) \
    { \
        while (pLastSiblingXmleleM->pYoungerXmlelem != NULL) \
        { \
            pLastSiblingXmleleM = pLastSiblingXmleleM->pYoungerXmlelem; \
        } \
    } \
}while(0)


/*********************************************************************/
/* XADD macro simplifies the process of creating a XML hierarchy.    */
/*===================================================================*/
/* Parameters:                                                       */
/* "xmlFielD" is a XMLELEM pointer within the XMLPARSE struct.       */
/* "tagNamE"  is resolved to "TAG_tagNamE".                          */
/* "tagValuE" must reference a character array.                      */
/*********************************************************************/
#define XADD( xmlFielD, tagNamE, tagValuE ) \
      { offsetof( struct XMLPARSE, xmlFielD ), \
        XMLGEN_NUM( TAG_##tagNamE ), tagValuE, sizeof(tagValuE) }


/*********************************************************************/
/* XPOP restores the previous XML index level. Used with XADD.       */
/*********************************************************************/
#define XPOP { 0, 1, "XPOP", 4 }


/*********************************************************************/
/* XOUT macro simplifies the process of identifying XML tags within  */
/* an element hierarchy with the goal of moving XML tag values       */
/* to character fields.                                              */
/*********************************************************************/
#define XOUT( parentTaG, childTaG, fieldNamE, blankFilL ) \
      { XMLGEN_NUM( TAG_##parentTaG ), XMLGEN_NUM( TAG_##childTaG ), \
        sizeof( fieldNamE ), fieldNamE, blankFilL, NOBITVALUE, 0, 0 }


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
extern int FN_ADD_ATTR_TO_HIERARCHY(struct XMLPARSE *pXmlparse,
                                    struct XMLELEM  *pParentXmlelem,
                                    char            *pKeyword,
                                    char            *pValue,
                                    int              inputValueLen);

extern int FN_ADD_ELEM_TO_HIERARCHY(struct XMLPARSE *pXmlparse,
                                    struct XMLELEM  *pParentXmlelem,
                                    char            *pElemName,
                                    char            *pElemContent,
                                    int              elemContentLen);

extern void FN_CONVERT_ASCII_TO_ASCII(char *pInput,
                                      int   length);

extern void FN_CONVERT_ASCII_TO_EBCDIC(char *pInput,
                                       int   length);

extern void FN_CONVERT_EBCDIC_TO_ASCII(char *pInput,
                                       int   length);

extern void FN_CONVERT_BINARY_TO_CHARHEX(char *pInput,
                                         int   inputLen,
                                         char *pOutput);

extern int FN_CONVERT_CHARDEVADDR_TO_HEX(char            charDevAddr[4],
                                         unsigned short *pHexDevAddr);

extern int FN_CONVERT_CHARHEX_TO_BINARY(char *pInput,
                                        int   inputLen,
                                        char *pOutput);

extern int FN_CONVERT_CHARHEX_TO_DOUBLEWORD(char               *pInput,
                                            int                 inputLen,
                                            unsigned long long *pBinaryDoubleword);

extern int FN_CONVERT_CHARHEX_TO_FULLWORD(char         *pInput,
                                          int           inputLen,
                                          unsigned int *pBinaryFullword);

extern int FN_CONVERT_DIGITS_TO_FULLWORD(char         *pInput,
                                         int           inputLen,
                                         unsigned int *pBinaryFullword);

extern int FN_CREATE_XMLATTR(struct XMLPARSE *pXmlparse);

extern int FN_CREATE_XMLCNVIN(struct XMLPARSE *pXmlparse);

extern int FN_CREATE_XMLDECL(struct XMLPARSE *pXmlparse);

extern int FN_CREATE_XMLCOMM(struct XMLPARSE *pXmlparse);

extern int FN_CREATE_XMLELEM(struct XMLPARSE *pXmlparse);

extern struct XMLPARSE *FN_CREATE_XMLPARSE(void);

extern int FN_CREATE_XMLRAWIN(struct XMLPARSE *pXmlparse,
                              char            *pData,
                              int              dataLen);

extern struct XMLATTR *FN_FIND_ATTRIBUTE_BY_KEYWORD(struct XMLPARSE *pXmlparse,
                                                    struct XMLELEM  *pParentXmlelem,
                                                    char            *pKeyword);

extern struct XMLELEM *FN_FIND_ELEMENT_BY_NAME(struct XMLPARSE *pXmlparse,
                                               struct XMLELEM  *pStartXmlelem,
                                               char            *pElemName);

extern struct XMLELEM *FN_FIND_NEXT_CHILD_ELEMENT(struct XMLPARSE *pXmlparse,
                                                  struct XMLELEM  *pParentXmlelem,
                                                  struct XMLELEM  *pCurrChildXmlelem);

extern struct XMLELEM *FN_FIND_NEXT_SIBLING_BY_NAME(struct XMLPARSE *pXmlparse,
                                                    struct XMLELEM  *pParentXmlelem,
                                                    struct XMLELEM  *pCurrChildXmlelem,
                                                    char            *pElemName);

extern void FN_FREE_HIERARCHY_STORAGE(struct XMLPARSE *pXmlparse);

extern int FN_GENERATE_HIERARCHY_FROM_XML(struct XMLPARSE *pXmlparse);

extern struct XMLRAWIN *FN_GENERATE_XML_FROM_HIERARCHY(struct XMLPARSE *pXmlparse);


extern void FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(struct XMLPARSE  *pXmlparse,
                                              struct XMLELEM   *pParentXmlelem,
                                              struct XMLSTRUCT *pStartXmlstruct,
                                              int               xmlstructCount);

extern void FN_MOVE_XML_ATTRS_TO_STRUCT(struct XMLPARSE  *pXmlparse,
                                        struct XMLELEM   *pXmlelem,
                                        struct XMLSTRUCT *pStartXmlstruct,
                                        int               xmlstructEntries);

extern void FN_MOVE_XML_ELEMS_TO_STRUCT(struct XMLPARSE  *pXmlparse,
                                        struct XMLSTRUCT *pStartXmlstruct,
                                        int               xmlstructEntries);

extern int FN_PARSE_ATTR(struct XMLPARSE *pXmlparse);

extern int FN_PARSE_COMM(struct XMLPARSE *pXmlparse);

extern int FN_PARSE_DECL(struct XMLPARSE *pXmlparse);

extern int FN_PARSE_ELEM(struct XMLPARSE *pXmlparse);

extern int FN_PARSE_NEXT_NONBLANK(struct XMLPARSE *pXmlparse);

extern int FN_PARSE_NEXT_STRING(struct XMLPARSE *pXmlparse);

extern struct XMLPARSE *FN_PARSE_XML(char *xmlBlob,
                                     int   xmlBlobSize);

extern int FN_TOKEN_CONVERT_DELIMITERS_IN_QUOTES(char *pUnparsedCommand,
                                                 char *pDelimiters);

extern int FN_TOKEN_REMOVE_PAREN_STRINGS(char  *pCmdCpy,
                                         char  *pCmdStr,
                                         char **pParenPtr,
                                         int    maxParenPtrs, 
                                         int    maxParenDepth,
                                         int    compressBlankFlag);

extern int FN_TOKEN_RETURN_NEXT(char  *pCurrCmdTextPos,
                                int    tokenNum,
                                char  *pCurrCmdToken,
                                char  *pBegDelimiters,
                                char  *pEndDelimiters,
                                short *pBypassChars);

extern int FN_TOKEN_TEST_MISMATCHED_PARENS(char  *pUnparsedCommand,
                                           short  maxParenDepth);

extern struct XMLCNVIN *FN_XML_ASCII_TO_ASCII(struct XMLPARSE *pXmlparse,
                                              char             checkResultsFlag);

extern struct XMLCNVIN *FN_XML_EBCDIC_TO_ASCII(struct XMLPARSE *pXmlparse,
                                               char             checkResultsFlag);

extern void FN_XML_RESTORE_SPECIAL_CHARS(char *xmlString,
                                         int  *pXmlStringLen);


#endif                                           /* SMCCXMLX_HEADER  */


