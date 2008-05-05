/**************************************************************************

	globalxap.h

	Header file for the xAP library

   Copyright (c) 2007 Daniel Berenguer <dberenguer@usapiens.com>

	This file is part of the OPNODE project.

	OPNODE is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	OPNODE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with OPNODE; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


	Last changes:

	04/06/08 by Daniel Berenguer : web.service schema
	04/22/07 by Daniel Berenguer : SMS functions added
	04/15/07 by Daniel Berenguer : Type of endpoint added
	04/06/07 by Daniel Berenguer : EPVALULEN = 92 (Ready to support LCD values)
	03/09/07 by Daniel Berenguer : first version.

***************************************************************************/

//*************************************************************************
//*************************************************************************
// 									INCLUDE SECTION
//*************************************************************************
//*************************************************************************

#ifndef _GLOBALXAP_H
#define _GLOBALXAP_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

//*************************************************************************
//*************************************************************************
// 									DEFINES SECTION
//*************************************************************************
//*************************************************************************

#define BYTE				unsigned char
#define FALSE 0
#define TRUE 1

#define DEVLENGTH			20		// Maximum length for device names and locations
#define EPVALULEN			92		// Maximum length for value strings

#define XAP_PORT			3639	// xAP UDP port

#define SIZEXAPMESS		512	// Maximum length of xAP messages
#define SIZEXAPKEY		36		// Maximum length of xAP keywords
#define SIZEXAPADDR		136	// Maximum length of xAP addresses
#define SIZEXAPUID		40		// Maximum length of xAP UID's
#define FRECXAPHBEAT		60		// Frequency of heartbeat sends (in seconds)
#define XAP_VERSION		12		// Version of the xAP specification
#define SIZEXAPNAME		32		// Maximum length of xAP application names

// Supported xAP classes
#define XAP_HBEAT			0		// xAP heartbeat
#define BSC_COMMAND		1		// xAP BSC command
#define BSC_QUERY			2		// xAP BSC query
#define BSC_EVENT			3		// xAP BSC event
#define BSC_INFO			4		// xAP BSC info
#define X10_REQUEST		5		// xAP X10 command
#define X10_EVENT			6		// xAP X10 event
#define XAP_SHUTDWN		7		// xAP shutdown
#define SMS_MESSAGE		8		// xAP SMS message
#define WEB_REQUEST		9		// Web service request
#define WEB_START			10		// Web service start
#define WEB_STOP			11		// Web service stop

//*************************************************************************
//*************************************************************************
// 									GLOBAL VARIABLES
//*************************************************************************
//*************************************************************************

struct sockaddr_in sckxapAddr;				// Address structure for the xAP communications
char strIncXapBuf[SIZEXAPMESS];				// Stores incoming xAP messages and keeps them during their treatment
unsigned short callBodyTimes;					// Number of studied bodies after inspecting the message header
														// (Number of times xapReadBscBody is called after calling xapReadHead)

// xAP header structure
typedef struct {
	int v;											// xAP version
	int hop;											// hop-count
	char uid[SIZEXAPUID];						// Unique identifier (UID)
	char class[40];								// Message class
	char source[SIZEXAPADDR];					// Message source
	char target[SIZEXAPADDR];					// Message target
	int interval;									// Heartbeat interval
} xaphead;

// xAP endpoint structure
typedef struct {
	char name[DEVLENGTH];						// name
	char location[DEVLENGTH];					// Location
	char value[EPVALULEN];						// Value
	char state[7];									// on, off, toggle
	int UIDsub;										// UID subaddress number (2-digit hexadecimal code)
	BYTE type;										// Type of endpoint: 0=binary input; 1=binary output; 2=level/text input; 3=level/text output
	BYTE nu1;
	BYTE nu2;
	BYTE nu3;
} xAPendp;


//*************************************************************************
//*************************************************************************
// 									GLOBAL FUNCTIONS
//*************************************************************************
//*************************************************************************

// xapcommon.c functions:
//-------------------------
short int xapSetup(char *strIPaddr);
short int xapRead(int fdSocket, char *xapMessage);
char *xapReadHead(int fdSocket, xaphead *header);
short int xapEvalTarget(char *strTarget, char *strVendor, char *strDevice, char *strInstance, xAPendp *endp);
short int xapSendHbeat(int fdSocket, char *strSource, char *strUID, int hbeatFreq);
short int xapSendHbShutdown(int fdSocket, char *strSource, char *strUID, int hbeatFreq);

// xapbsc.c functions:
//-------------------------
char *xapReadBscBody(int fdSocket, char *pBody, xaphead header, xAPendp *endp);
short int xapSendBSCevn(int fdSocket, char *strSource, char *strUID, char *strValue, char *strState, BYTE flgType);
short int xapSendBSCinf(int fdSocket, char *strSource, char *strUID, char *strValue, char *strState, BYTE flgType);
short int xapSendBSCcmd(int fdSocket, char *strSource, char *strUID, char *strTarget, char *strValue, char *strState);
short int xapSendBSCqry(int fdSocket, char *strSource, char *strUID, char *strTarget);

// xapx10.c functions:
//-------------------------
short int xapReadX10Body(int fdSocket, char *pBody, xaphead header, char *devlist, char *command, BYTE *level);
short int xapSendX10evn(int fdSocket, char *strDevice, char *strUID, char *device, char *command, BYTE level);
short int xapSendX10req(int fdSocket, char *strDevice, char *strUID, char *strTarget, char *device, char *command, BYTE level);

// xapsms.c functions:
//-------------------------
short int xapReadSMSBody(int fdSocket, char *pBody, char *phoneNum, char *SMStext, BYTE *flgIncoming);
short int xapSendSMSreceipt(int fdSocket, char *strSource, char *strUID, char *phoneNum, char *SMStext, BYTE flgSent, char *strError);
short int xapSendSMSinbound(int fdSocket, char *strSource, char *strUID, char *phoneNum, char *SMStext);
short int xapSendSMSoutbound(int fdSocket, char *strSource, char *strUID, char *strTarget, char *phoneNum, char *SMStext);

// xapintranet.c functions:
//-------------------------
short int xapReadWebBody(int fdSocket, char *pBody, char *name, char *desc, char *pc, char *icon, char *url, BYTE *type);
short int xapSendWebService(int fdSocket, char *strSource, char *strUID, char *strName, char *strDesc, char *strPC, char *strIcon, char *strURL, char* type);
short int xapSendWebRequest(int fdSocket, char *strSource, char *strUID);

#endif
