/** SOURCE FILE PROLOGUE *********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      smcsfmt.c                                        */
/** Description:    XAPI client data conversion service.             */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     05/15/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/** END PROLOGUE *****************************************************/


/*********************************************************************/
/* Includes:                                                         */
/*********************************************************************/
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "smccxmlx.h"
#include "srvcommon.h"
#include "xapi.h"


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: FN_CONVERT_CHARHEX_TO_FULLWORD                    */
/** Description:   Convert up to 8 hex characters into an integer.   */
/**                                                                  */
/** Validate that the specified number of hexadecimal characters     */
/** were entered and convert them into a binary fullword (assumed    */
/** to be type int).                                                 */
/**                                                                  */
/** RC_FAILURE indicates that there were non-hexadecimal             */
/** characters (i.e. 0-9, and A-F) in the input.  Partial            */ 
/** conversion is performed with 0 being substituted for the         */
/** offending non-hexadecimal character.                             */
/**                                                                  */
/** NOTE: Only the first 8 characters of pInput will be converted    */
/** if inputLen is > 8.                                              */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "FN_CONVERT_CHARHEX_TO_FULLWORD"

extern int FN_CONVERT_CHARHEX_TO_FULLWORD(char         *pInput,
                                          int           inputLen,
                                          unsigned int *pBinaryFullword)
{
    int                 convertLen          = inputLen;
    int                 lastRC              = RC_SUCCESS;
    int                 i;
    int                 j;
    int                 workDigit;
    int                 power16;
    unsigned int        runningTotal        = 0;
    char                workCharHex[9];

    if (convertLen > 8)
    {
        TRMSGI(TRCI_ERROR,
               "Input length=%d truncated to length=8\n",
               inputLen);

        convertLen = 8;
    }

    memset(workCharHex, 0, sizeof(workCharHex));

    memcpy(workCharHex, 
           pInput, 
           convertLen);

    for (i = 0;
        i < convertLen; 
        i++)
    {
        switch (pInput[i])
        {
        case '0':
            workDigit = 0;

            break;

        case '1':
            workDigit = 1;

            break;

        case '2':
            workDigit = 2;

            break;

        case '3':
            workDigit = 3;

            break;

        case '4':
            workDigit = 4;

            break;

        case '5':
            workDigit = 5;

            break;

        case '6':
            workDigit = 6;

            break;

        case '7':
            workDigit = 7;

            break;

        case '8':
            workDigit = 8;

            break;

        case '9':
            workDigit = 9;

            break;

        case 'A':
            workDigit = 10;

            break;

        case 'B':
            workDigit = 11;

            break;

        case 'C':
            workDigit = 12;

            break;

        case 'D':
            workDigit = 13;

            break;

        case 'E':
            workDigit = 14;

            break;

        case 'F':
            workDigit = 15;

            break;

        default:
            TRMEMI(TRCI_ERROR, 
                   pInput, inputLen,
                   "Error; non-charhex at location=%i\n",
                   i);

            workDigit = 0;
            lastRC = RC_FAILURE;
        }

        power16 = 1;

        for (j = 1; 
            j < (convertLen - i); 
            j++)
        {
            power16 = power16 * 16;
        }

        runningTotal = runningTotal + ((workDigit) * (power16));
    }

    *pBinaryFullword = runningTotal;

    return lastRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_CONVERT_CHARHEX_TO_DOUBLEWORD                  */
/** Description:   Convert up to 16 hex characters into a doubleword.*/
/**                                                                  */
/** Validate that the specified number of hexadecimal characters     */
/** were entered and convert them into a binary doubleword (assumed  */
/** to be type long long).                                           */
/**                                                                  */
/** RC_FAILURE indicates that there were non-hexadecimal             */
/** characters (i.e. 0-9, and A-F) in the input.  Partial            */ 
/** conversion is performed with 0 being substituted for the         */
/** offending non-hexadecimal character.                             */
/**                                                                  */
/** NOTE: Only the first 16 characters of pInput will be converted   */
/** if inputLen is > 16.                                             */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_CONVERT_CHARHEX_TO_DOUBLEWORD"

extern int FN_CONVERT_CHARHEX_TO_DOUBLEWORD(char               *pInput,
                                            int                 inputLen,
                                            unsigned long long *pBinaryDoubleword)
{
    int                 convertLen          = inputLen;               
    int                 lastRC              = RC_SUCCESS;

    char                workCharHex[17];
    char                workBinary[8];
    char                convCharHexString[17];

    if (convertLen > 16)
    {
        TRMSGI(TRCI_ERROR,
               "Input string length=%i truncated to length=16\n",
               convertLen);

        convertLen = 16;
    }

    memset(workCharHex, 0, sizeof(workCharHex)); 

    if (convertLen < 16)
    {
        memcpy(&(workCharHex[((16 - convertLen) -1)]), 
               pInput,
               convertLen);
    }
    else
    {
        memcpy(workCharHex, 
               pInput,
               convertLen);
    }

    lastRC = FN_CONVERT_CHARHEX_TO_BINARY(workCharHex,
                                          16,
                                          workBinary);

    memcpy((char*) pBinaryDoubleword, 
           workBinary,
           8);

    FORMAT_LONGLONG(*pBinaryDoubleword,
                    convCharHexString);

    return lastRC;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: FN_CONVERT_CHARDEVADDR_TO_HEX                     */
/** Description:   Convert a character device address to binary.     */
/**                                                                  */
/** This function validates and converts a 3 or 4 character          */
/** device address (with either a leading blank, or trailing blank   */
/** or NULL permitted) into a 2 character hexadecimal device         */
/** address (unsigned short).                                        */
/**                                                                  */
/** RC_FAILURE indicates that there were non-hexadecimal             */
/** characters (i.e. 0-9, and A-F) in the input character device     */
/** address and the conversion was not performed.  Partial           */
/** conversion is not possible.                                      */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef  SELF
#define SELF "FN_CONVERT_CHARDEVADDR_TO_HEX"     

extern int FN_CONVERT_CHARDEVADDR_TO_HEX(char            charDevAddr[4],
                                         unsigned short *pHexDevAddr)
{
    char                workCharDevAddr[4];
    int                 i;
    int                 lastRC;

    if ((charDevAddr[3] == ' ') ||
        (charDevAddr[3] == 0))
    {
        workCharDevAddr[0] = '0';

        memcpy(&workCharDevAddr[1],
               charDevAddr,
               3);
    }
    else
    {
        memcpy(workCharDevAddr,
               charDevAddr,
               sizeof(workCharDevAddr));

        if (workCharDevAddr[0] == ' ')
        {
            workCharDevAddr[0] = '0';
        }
    }

    lastRC = FN_CONVERT_CHARHEX_TO_BINARY(workCharDevAddr,
                                          4,
                                          (char*) pHexDevAddr);

    if (lastRC != RC_SUCCESS)
    {
        TRMSGI(TRCI_ERROR,
               "Error converting charDevAddr %.4s to hex\n",
               charDevAddr);

        return lastRC;
    }

    return lastRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_CONVERT_BINARY_TO_CHARHEX                      */
/** Description:   Convert binary into character hexadecimal.        */
/**                                                                  */
/** Convert an 8 bit binary nibble into a 16 bit character           */
/** hexadecimal representation.                                      */
/** Example: Convert X'0A' into X'F0C1' (C'0A').                     */ 
/**                                                                  */
/** NOTE: pOutput must be twice as large as pInput.                  */
/** If caller wants a string, it is their responsibility to          */
/** append the trailing NULL.                                        */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_CONVERT_BINARY_TO_CHARHEX"

extern void FN_CONVERT_BINARY_TO_CHARHEX(char *pInput,
                                         int   inputLen,
                                         char *pOutput)
{
    int                 i;
    int                 j;
    char                nibbles[2];

    for (i = 0; 
        i < inputLen; 
        i++)
    {
        nibbles[0] = pInput[i];
        nibbles[0] = nibbles[0] >> 4;
        nibbles[1] = pInput[i];
        nibbles[1] = nibbles[1] << 4;
        nibbles[1] = nibbles[1] >> 4;

        for (j = 0; 
            j < 2; 
            j++)
        {
            switch (nibbles[j])
            {
            case '\x00':
                pOutput[((2 * i) + j)] = '0';

                break;

            case '\x01':
                pOutput[((2 * i) + j)] = '1';

                break;

            case '\x02':
                pOutput[((2 * i) + j)] = '2';

                break;

            case '\x03':
                pOutput[((2 * i) + j)] = '3';

                break;

            case '\x04':
                pOutput[((2 * i) + j)] = '4';

                break;

            case '\x05':
                pOutput[((2 * i) + j)] = '5';

                break;

            case '\x06':
                pOutput[((2 * i) + j)] = '6';

                break;

            case '\x07':
                pOutput[((2 * i) + j)] = '7';

                break;

            case '\x08':
                pOutput[((2 * i) + j)] = '8';

                break;

            case '\x09':
                pOutput[((2 * i) + j)] = '9';

                break;

            case '\x0A':
                pOutput[((2 * i) + j)] = 'A';

                break;

            case '\x0B':
                pOutput[((2 * i) + j)] = 'B';

                break;

            case '\x0C':
                pOutput[((2 * i) + j)] = 'C';

                break;

            case '\x0D':
                pOutput[((2 * i) + j)] = 'D';

                break;

            case '\x0E':
                pOutput[((2 * i) + j)] = 'E';

                break;

            case '\x0F':
                pOutput[((2 * i) + j)] = 'F';

                break;

            default:
                pOutput[((2 * i) + j)] = '0';
            }
        }
    }

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_CONVERT_CHARHEX_TO_BINARY                      */
/** Description:   Convert character hexadecimal into binary.        */
/**                                                                  */
/** Convert a 16 bit character hexadecilam representation into       */
/** an 8 bit binary nibble.                                          */
/** Example: Convert X'F0C1' (C'0A') into X'0A'.                     */ 
/**                                                                  */
/** RC_WARNING indicates that the inputLen was an odd number, and    */
/** that the conversion was truncated by a character.                */
/**                                                                  */
/** RC_FAILURE indicates that there were non-hexadecimal             */
/** characters (i.e. 0-9, and A-F) in the input.  Partial            */
/** conversion is possible.                                          */
/**                                                                  */
/** NOTE: pInputLen must be an even number.                          */
/** pOutput will be half as large as pInput.                         */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_CONVERT_CHARHEX_TO_BINARY"

extern int FN_CONVERT_CHARHEX_TO_BINARY(char *pInput,
                                        int   inputLen,
                                        char *pOutput)
{
    int                 lastRC              = RC_SUCCESS;
    int                 i;
    int                 j;
    char                nibbles[2];
    char                binaryData;
    short               workData;
    char                nonCharhexTrace     = FALSE;

    /*****************************************************************/
    /* If inputLen is not an even number, subtract 1 to make it even,*/
    /* set a bad return code, but do the conversion anyway.          */
    /*****************************************************************/
    if ((inputLen % 2) != 0)
    {
        lastRC = RC_WARNING;
        inputLen = inputLen - 1;
    }

    for (i = 0; 
        i < inputLen; 
        i += 2)
    {
        nibbles[0] = pInput[i];
        nibbles[1] = pInput[(i + 1)];

        for (j = 0;
            j < 2; 
            j++)
        {
            switch (nibbles[j])
            {
            case '0':
                nibbles[j] = 0;

                break;

            case '1':
                nibbles[j] = 1;

                break;

            case '2':
                nibbles[j] = 2;

                break;

            case '3':
                nibbles[j] = 3;

                break;

            case '4':
                nibbles[j] = 4;

                break;

            case '5':
                nibbles[j] = 5;

                break;

            case '6':
                nibbles[j] = 6;

                break;

            case '7':
                nibbles[j] = 7;

                break;

            case '8':
                nibbles[j] = 8;

                break;

            case '9':
                nibbles[j] = 9;

                break;

            case 'A':
                nibbles[j] = 10;

                break;

            case 'B':
                nibbles[j] = 11;

                break;

            case 'C':
                nibbles[j] = 12;

                break;

            case 'D':
                nibbles[j] = 13;

                break;

            case 'E':
                nibbles[j] = 14;

                break;

            case 'F':
                nibbles[j] = 15;

                break;

            default:
                /*****************************************************/
                /* If we find a non char hex (0-9, A-F) character,   */
                /* then set a bad return code, default the binary    */
                /* character to 0, and continue.                     */
                /*****************************************************/
                if (!(nonCharhexTrace))
                {
                    TRMEMI(TRCI_ERROR, 
                           pInput, inputLen,
                           "Error; non-charhex at location=%i\n",
                           (i+j));
                }

                nibbles[j] = 0;
                lastRC = RC_FAILURE;
            }
        }

        workData = (16 * nibbles[0]) + (nibbles[1]);

        memcpy(nibbles,
               (char*) &workData,
               2);

        binaryData = nibbles[1];
        pOutput[(i / 2)] = binaryData;
    }

    return lastRC;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: FN_CONVERT_DIGITS_TO_FULLWORD                     */
/** Description:   Converter character digits into binary.           */
/**                                                                  */
/** Validate that the specified number of digits was entered and     */
/** convert it to a binary fullword (assumed to be type int).        */
/**                                                                  */
/** This is the same as the C atoi() function except that the input  */
/** does not need to be NULL terminated string.                      */
/**                                                                  */
/** RC_FAILURE indicates that there were non-digit characters        */
/** (i.e. 0-9) in the input.  Partial conversion is performed        */
/** with 0 being substituted for the offending non-digit.            */
/**                                                                  */
/** NOTE: Input limited to 19 digits.                                */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "FN_CONVERT_DIGITS_TO_FULLWORD"

extern int FN_CONVERT_DIGITS_TO_FULLWORD(char         *pInput,
                                         int           inputLen,
                                         unsigned int *pBinaryFullword)
{
    int                 convertLen          = inputLen;
    int                 lastRC              = RC_SUCCESS;
    int                 i;
    int                 j;
    int                 workDigit;
    int                 power10;
    unsigned int        runningTotal        = 0;

    if (convertLen > 19)
    {
        TRMSGI(TRCI_ERROR,
               "Input length=%d truncated to length=19\n",
               inputLen);

        convertLen = 19;
    }

    for (i = 0;
        i < convertLen; 
        i++)
    {
        switch (pInput[i])
        {
        case '0':
            workDigit = 0;

            break;

        case '1':
            workDigit = 1;

            break;

        case '2':
            workDigit = 2;

            break;

        case '3':
            workDigit = 3;

            break;

        case '4':
            workDigit = 4;

            break;

        case '5':
            workDigit = 5;

            break;

        case '6':
            workDigit = 6;

            break;

        case '7':
            workDigit = 7;

            break;

        case '8':
            workDigit = 8;

            break;

        case '9':
            workDigit = 9;

            break;

        default:
            TRMEMI(TRCI_ERROR, 
                   pInput, inputLen,
                   "Error; non-digit at location=%i\n",
                   i);

            workDigit = 0;
            lastRC = RC_FAILURE;
        }

        power10 = 1;

        for (j = 1; 
            j < (convertLen - i); 
            j++)
        {
            power10 = power10 * 10;
        }

        runningTotal = runningTotal + ((workDigit) * (power10));
    }

    *pBinaryFullword = runningTotal;

    return lastRC;
}




