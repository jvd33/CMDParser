/*---------------------------------------------------------------------------
  parser.c
  This implements a simple RPI command line parser
  12/12/2014 - Initial version
--------------------------------------------------------------------------*/

//#include "stdafx.h"
//#include <afx.h>
#include <cmpe240.h>

/*---------------------------------------------------------------------------
 This global table contains the function parsing abstraction.  This maps
 each command line to a processing function and provides other key information
 about the parsing data.  See the structure definition for the full
 description.                  txt    callback  num       max    min evenHex
---------------------------------------------------------------------------*/
PARSEDATA parseData[6] = { { "type", &typeCall, 2, MAX_PARM_LEN, 1, 0, },
                           { "size", &sizeCall, 2, MAX_PARM_LEN, 1, 0, },
                           { "hex",  &hexCall,  2, MAX_PARM_LEN, 2, 1, },
                           { "fmul", &fmulCall, 3,            8, 8, 1, },
                           { "fadd", &faddCall, 3,            8, 8, 1, },
                           { "fenc", &fencCall, 3,            8, 8, 1, }, };


/*-------------------------------------------------------------------------
   Use this for your fatRead() IO read buffer
-------------------------------------------------------------------------*/
uint8_t readBuffer [6*1024*1024];
extern FileHandle HDDimage;
int strcmp(const char *a, const char *b);


/*---------------------------------------------------------------------------
  This function parses the command line inCmdLine, verifies the syntax
  using the data in the global data structure PARSEDATA parseData[]
  and then executes it.
  This function will return 0 for success, non-zero for failure.
  A blank line should not be reported as a failure.
---------------------------------------------------------------------------*/
uint32_t parseCmdLine(char *InCmdLine)
{
	CMDPARM parms[MAX_PARMS+1];

	int i = 0;
	int rc = 0;
	char *ptr;
	for(int z = 0; z < MAX_PARMS+1 && *InCmdLine!='\0'; z++) {
		ptr = parseSingleItem(InCmdLine, parms+z);
		InCmdLine = ptr;
		i++;
	}


	if(i > 0) {
		for(int x=0; x < 6; x++) { //parse command logic
			if(!strcmp(parms[0].parameter, parseData[x].ParmCmdStr)) {

				if(countParms(parms) < parseData[x].NumParms) {
					rc=1; goto ret; //not enough args
				}
				for(int n=1; n < parseData[x].NumParms; n++) { //now validate parameters
					if( ((parms[n].len > parseData[x].MaxParmLen+1) || parms[n].len < parseData[x].MinParmLen) && parms[n].parameter[0] != '\0' )  {
						rc=2; goto ret; //invalid argument size
					}
					else if(parseData[x].EvenHexOnly == 1) {
						if(verifyHex(parms[n].parameter) == 0 || verifyHex(parms[n].parameter) % 2 !=0) { rc=3;goto ret; } //invalid hex
					}
				}
				rc = parseData[x].callBack(parms); //validated and parsed
				goto ret;

			}
			else if(parms[0].len == 0) { rc=0; goto ret;} else { rc=10; continue; } //command not yet found, iterate again

		}
	}

	ret: return rc;
}

int strcmp(const char *a,const char *b){ //need strcmp
  if (! (*a | *b)) return 0;
  return (*a!=*b) ? *a-*b : strcmp(++a,++b);
}



/*---------------------------------------------------------------------------
  Description:  This function searches inStringP for the first
  non-blank character and then copies all the subsequent non-blank
  characters into singlePamr->parameter while keeping track of the
  number of bytes copied and saving the result in cmdParm->len.

  The function will return a pointer to the next blank character or a 0x00
  if all the characters in the string have been processed.
  Note: The string pointer returned is only valid for non-zero lengths.

  Where: char *inStringP     - pointer to the nul terminated string to parse
         CMDPARM *singleParm - pointer to the resulting parsed object

  Returns: 0x00  if no parameter is found or
           pointer to the next byte to be processed

  e.g.  Given:   inStringP = "file  a.txt" then
        singleParm->parameters = "file" (nul terminated)
        singleParm->len        = 4
        and the pointer returned would be: "  a.txt"
---------------------------------------------------------------------------*/
char *parseSingleItem(char *inStringP, CMDPARM *singleParm)
{
	int n = 0x00;
	int spaces = 0;
	int i =0;
	int found = 0;
	while(*inStringP != '\0' && i < MAX_PARM_LEN+1) {
		if(*inStringP != 32) {
			found = 1;
			singleParm->parameter[n] = *inStringP; //otherwise append to str
			i++;
			n++;
			++inStringP;
			continue;
		}
		else if(*inStringP==32 && found==1 ) {  //if space and we've found a char
			break;
		}
		else if(*inStringP==32 && found==0){
			spaces++; //otherwise it's leading spaces that we can ignore
			i++;
			n = i - spaces;
			++inStringP;
			continue;
		}
	}

	if(found && *inStringP != '\0') { //if we've read chars and curr is not null
		singleParm->parameter[n]='\0'; //null terminate
		singleParm->len=n;
		return inStringP; //return pointer to curr char
	} else { // else we've read nothing at all, or hit end of string
		if(n==0) { singleParm->parameter[n] = '\0'; singleParm->len = n;} else { singleParm->parameter[n+1] = '\0'; singleParm->len = n+1; }
		return 0x00; //return empty pointer
	}

} // End parseSingleItem


/*---------------------------------------------------------------------------
  This function validates that string only contains valid hex characters and
  returns the count of valid hex characters found or 0 for none.
---------------------------------------------------------------------------*/
uint32_t verifyHex(const char *string)
{
	uint32_t i = 0;
	while(*string != '\0') {
		switch(*string) {
			case '0': i++; break;
			case '1': i++; break;
			case '2': i++; break;
			case '3': i++; break;
			case '4': i++; break;
			case '5': i++; break;
			case '6': i++; break;
			case '7': i++; break;
			case '8': i++; break;
			case '9': i++; break;
			case 'A': i++; break;
			case 'a': i++; break;
			case 'B': i++; break;
			case 'b': i++; break;
			case 'C': i++; break;
			case 'c': i++; break;
			case 'D': i++; break;
			case 'd': i++; break;
			case 'E': i++; break;
			case 'e': i++; break;
			case 'F': i++; break;
			case 'f': i++; break;
			default: i=0; goto exit;

		}
		string++;
	}
	exit: return i;
} // End verifyHex



/*-------------------------------------------------------------------------- -
  This function converts a hex character string up to 8 character
  long into true hexadecimal.  The function does no error checking.
  e.g char2Hex("12");
      char2Hex("12345678");
-------------------------------------------------------------------------- - */
uint32_t char2Hex(const char *string)
{

	uint32_t retval = 0x0;
	int v[8] = {0x10000000, 0x1000000, 0x100000, 0x10000, 0x1000, 0x100, 0x10, 0x1};
	int i = 0;
	int n;

	while(*string != 0x00) {

		n = *string;
		if((n - 0x30) >= 0 && (n - 0x30) <=9 ) {  //if it's a number
			retval += ( (n-0x30)*v[i] );
		}
		else if( (n - 0x41) >= 0 && (n - 0x41) <= 5) { //capital hex letter
			retval = retval << 8; //shift right to make room for letter values
			retval += ( v[i] *n );
		}
		else if((n-0x61) >= 0 && (n-0x61) <=5) { //lowercase hex letter
			retval = retval << 8;
			retval += ( v[i]*n ) ;
		} else { break; }
		i++;
		++string;
	}
	return retval;
} // End char2Hex

/*---------------------------------------------------------------------------
  This function returns the number of parameters in a fully parsed
  command string. e.g. CMDPARM *parms
---------------------------------------------------------------------------*/
uint32_t countParms(CMDPARM *parms)
{
	uint32_t val = 0;
	while(parms[val].parameter[0] != '\0') {
		val++;
	}
	return val;
} // End countParms



/*---------------------------------------------------------------------------
  This function is called when the parser determines the command is a "type"
  command.  It will contain a call to your HDD "type" library.
  Only the first 100 an last 100 bytes need to be displayed.
  This returns 0 for success, non-zero for error

  Typical commands:
  �type TWO.TXT�                  the contents of TWO.TXT
  �type two.txt�                  �syntax error�
  �      type      two.txt     �  �not found�
  � type  �                           �syntax error�
    �type TWO.TXT TWO.TXT�            �syntax error�
---------------------------------------------------------------------------*/
uint32_t typeCall(CMDPARM *parms)
{
	//None of the FAT functions appear to be implemented anywhere, just defined in the header
	//So I have no idea if these work
	/*
	initHDD();
	FileHandle *handle;
	FAT_DirEntry *file = searchDir(parms[1].parameter);
	if(!file) { return 5; } //file not found error
	handle = (FileHandle*) fatOpen(parms[1].parameter, handle);
	fatRead(handle, readBuffer, 100);
	uartPutStr("First 100 bytes:\n\r\0");
	uartPutStr((const char *)readBuffer);
	uartPutStr("\n\r\0");
	int n = 0;
	while(fatRead(handle, readBuffer, 1024*1024*6)) {
		continue;
	}
	while(*readBuffer != '\0') {
		n++;
	}
	uartPutStr("Last 100 bytes:\n\r\0");
	uartPutStr((const char *) readBuffer[n-100]);
	uartPutStr("\n\r\0");*/


    return(0);
} // End typeCall


/*---------------------------------------------------------------------------
  This function is called when the parser determines the command is a "size"
  command.  It will contain a call to your HDD "type" library.

  �size TWOO.TXT  �           �not found�
  �size TWO.TXT  �            the file size of TWO.TXT: �5762 bytes�
  -------------------------------------------------------------------------*/
uint32_t sizeCall(CMDPARM *parms)
{

	//None of the FAT functions appear to be implemented anywhere, just defined in the header
	//So I have no idea if these work
	/*
	initHDD();
	uint8_t buff[8];
	FAT_DirEntry *file = searchDir(parms[1].parameter);
	if(!file) { return 5; } //file not found error
	decStr(file->FileSize, buff);
	uartPutStr("The file size of \0");
	uartPutStr(parms[1].parameter);
	uartPutStr(": \0");
	uartPutStr((const char *)buff);
	uartPutStr("bytes.\n\r\0");*/

    return(0);
} // End sizeCall


/*---------------------------------------------------------------------------
  This function is called when the parser determines the command is a "hex"
  command.  It will contain a call to your "hex" library.

  �  hex          313233  �           the string : �123�
  �hex   13233  �                 �syntax error�
  �hex qqqq�                      �syntax error�
  �hex �                          �syntax error�
  �hex  1234qqq�                  �syntax error�
---------------------------------------------------------------------------*/
uint32_t hexCall(CMDPARM *parms)
{

  //Implemented this anyway, oops.
	char *p;
	char ans[5];
	p = ans;
	uint32_t curr = 0x0;

	curr = char2Hex(parms[1].parameter);
	uint32_t nibbles;
	int i = 0;

	while(i < 4) {
		nibbles = (curr >> (24-(8 * i))); //shift each byte into position
		if(nibbles==0) { //if 0 try again
			i++;
		} else {
			*p= (char)nibbles;
			i++;
			++p;
		}
	}
	if(i==0) {
		return 3; //invalid hex
	}

	uartPutStr("The string: \0");
	*p = '\0';
	uartPutStr(ans);
	uartPutStr("\n\r\0");
    return(0);
} // End hexCall


/*---------------------------------------------------------------------------
  This function is called when the parser determines the command is a "fmul"
  command.  It will contain a call to your math  "fmul" library.

  �  fmul  41520000    41520000      �    432c4400
  �  fmul 12345678     7654321�       error message : �second hex number invalid�
  �  fmul 1234568     87654321�       error message : �first hex number invalid�
  �  fmul 1234568     7654321�        error message : �both hex numbers invalid�
  �  fmul 1234568     �               �syntax error�
  �  fmul  �                      �syntax error�
---------------------------------------------------------------------------*/
uint32_t fmulCall(CMDPARM *parms)
{
	IEEE_FLT a;
	IEEE_FLT b;
	IEEE_FLT res;
	a = char2Hex(parms[1].parameter);
	b = char2Hex(parms[2].parameter);
	res = IeeeMult(a, b);
	uartHexStrings(res);
	uartPutStr("\n\r\0");
    return(0);
} // End fmulCall


/*---------------------------------------------------------------------------
  This function is called when the parser determines the command is a "fadd"
  command.  It will contain a call to your math "fadd" library.

  �  fadd 41520000  41520000� 41d20000
  �  fadd 12345678     7654321�   error message : �second hex number invalid�
  �  fadd 1234568     87654321�   error message : �first hex number invalid�
  �  fadd 1234568     7654321�    error message : �both hex numbers invalid�
  �  fadd 1234568     �           �syntax error�
  �  fadd  �                  �syntax error�
---------------------------------------------------------------------------*/
uint32_t faddCall(CMDPARM *parms)
{
	IEEE_FLT a;
	IEEE_FLT b;
	IEEE_FLT res;
	a = char2Hex(parms[1].parameter);
	b = char2Hex(parms[2].parameter);
	res = IeeeAdd(a, b);
	uartHexStrings(res);
	uartPutStr("\n\r\0");
    return(0);
} // End faddCmd


/*---------------------------------------------------------------------------
  This function is called when the parser determines the command is a "fenc"
  command.  It will contain a call to your math "fen c" library.

  �  fenc  FFFFFFF3  20000000        C1520000
  �  fenc 12345678     7654321�   error message : �second hex number invalid�
  �  fenc 1234568     87654321�   error message : �first hex number invalid�
  �  fenc 1234568     7654321�    error message : �both hex numbers invalid�
  �  fenc 1234568     �           �syntax error�
  �  fenc  �                  �syntax error�
---------------------------------------------------------------------------*/
uint32_t fencCall(CMDPARM *parms)
{
	INT_FRACT a = {char2Hex(parms[1].parameter), char2Hex(parms[2].parameter)};
	IEEE_FLT res;
	res = IeeeEncode(a);
    uartHexStrings(res);
	uartPutStr("\n\r\0");
    return(0);
} // End fencCmd
