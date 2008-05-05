/**************************************************************************

	xapbsc.c

	Functions needed to communicate under the xAP BSC schema

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

	04/15/07 by Daniel Berenguer : Type of endpoint added
	03/09/07 by Daniel Berenguer : first version.

***************************************************************************/

// Include section:

#include "globalxap.h"

// Define section:

// Global functions section:

static short int getBSCvalFormat(char *strValue);

// Global variables section:


//*************************************************************************
// xapReadBscBody
//
//	Inspects the xAP BSC message body pointed by pBody
//
//	'fdSocket'		UDP socket
//	'pBody'			Pointer to the beginning of the body section within the
//						xAP message. This pointer should be recovered from a
//						previous call to 'xapReadBscBody' or 'xapReadHead'
//	'header'			Header struct obtained from a previous call to xapReadHead
//	'endp'			Pointer to the xAP endpoint struct to write the results into
//
// Returns:
//		Pointer to the next message body (if exists)
//		NULL if BSC.query message, no more bodies or any error
//*************************************************************************

char *xapReadBscBody(int fdSocket, char *pBody, xaphead header, xAPendp *endp)
{
	char *pStr1, *pStr2;					// Pointers to strings
	char strKey[SIZEXAPKEY];			// Keyword string
	char strValue[SIZEXAPADDR];		// Value string
	int intLen;
	char chr;
	BYTE flgDataCheck = 0;				// Checks the consistency of the message body


	callBodyTimes++;		// Increase the number of studied bodies

	// Inspect the message body
	pStr1 = pBody;		// Place the pointer at the beginning of the xAP body

	// Verify the correct beginning of the body depending of the type of xAP BSC message
	// If xAP BSC command
	if (!strcasecmp(header.class, "xAPBSC.cmd"))
	{
		// Body tittle to expect
		sprintf(strValue, "output.state.%d\n{\n", callBodyTimes);
		// Verify the body tittle
		if (!strncasecmp(pStr1, strValue, strlen(strValue)))
			pStr1 += strlen(strValue);			// Place the pointer inside the body section
		else
			return NULL;
	}
	// If xAP BSC event or info
	else if (!strcasecmp(header.class, "xAPBSC.event") || !strcasecmp(header.class, "xAPBSC.info"))
	{
		//Verify the body tittle
		if (!strncasecmp(pStr1, "input.state\n{\n", 14))
		{
			endp->type = 0;		// input
			pStr1 += 14;			// Place the pointer inside the body section
		}
		else if (!strncasecmp(pStr1, "output.state\n{\n", 15))
		{
			endp->type = 1;		// output
			pStr1 += 15;			// Place the pointer inside the body section
		}
		else
			return NULL;
	}
	else
		return NULL;

	// Up to the end of the body section
	while (pStr1[0] != '}')
	{
		// Capture the key string
		if ((pStr2 = strchr(pStr1, '=')) == NULL)
			return NULL;
		if ((intLen = pStr2 - pStr1) >= sizeof(strKey))			// Only within the limits od strKey
			return NULL;
		strncpy(strKey, pStr1, intLen);								// Copy the key string into strKey
		strKey[intLen] = 0;												// Terminate the string
		pStr1 = pStr2 + 1;												// Move the pointer to the position of the value

		// Capture the value string
		if ((pStr2 = strchr(pStr1, '\n')) == NULL)
			return NULL;
		if ((intLen = pStr2 - pStr1) >= sizeof(strValue))		// Only within the limits od strValue
			return NULL;
		strncpy(strValue, pStr1, intLen);							// Copy the key string into strValue
		strValue[intLen] = 0;											// Terminate the string
		pStr1 = pStr2 + 1;												// Move the pointer to the next line

		// Evaluate the key/value pair
		chr = strKey[0];													// Take strKey first character
		if (isupper((int)chr))											// Convert strKey first character to lower case
			chr = tolower((int)chr);									// if necessary

		switch (chr)														// Consider strKey first character
		{
			case 'i':
				// id of the targeted endpoint?
				if (!strcasecmp(strKey, "id"))
				{
					if (!strcmp(strValue, "*"))	// Wildcard?
						endp->UIDsub = -1;			// Any endpoint
					else
						// Convert the byte to its numeric value (hex)
						endp->UIDsub = strtol(strValue, NULL, 16);
					flgDataCheck |= 0x01;						// First bit on
				}
				break;
			case 't':
				// text?
				if (!strcasecmp(strKey, "text"))
				{
					if (strlen(strValue) >= sizeof(endp->value))
						return NULL;
					strcpy(endp->value, strValue);			// Store the value in the structure
					flgDataCheck |= 0x02;						// second bit on
				}
				break;
			case 'l':
				// level?
				if (!strcasecmp(strKey, "level"))
				{
					if (strlen(strValue) >= sizeof(endp->value))
						return NULL;
					// "level" is only taken into account if there is no "text" in the body
					if (!(flgDataCheck & 0x02))				// If "text" has not been previously detected
					{
						strcpy(endp->value, strValue);		// Store the value in the struct
						flgDataCheck |= 0x04;					// Third bit on
					}
				}
				break;
			case 's':
				// state?
				if (!strcasecmp(strKey, "state"))
				{
					if (strlen(strValue) >= sizeof(endp->state))
						return NULL;
					// Store "state" as value only if there is no "text" nor "level" in the body
					if (!(flgDataCheck & 0x06))				// If "text" nor "level" have not been previously detected
						strcpy(endp->value, strValue);		// Store the state as value in the struct
					// Store "state" as state
					strcpy(endp->state, strValue);

					flgDataCheck |= 0x08;						// Fourth bit on
				}
				break;
			default:
				break;
		}
	}

	// If the body contains a "text"/"level" field
	if (flgDataCheck & 0x06)
		endp->type += 2;	// 0=binary input; 1=binary output; 2=level/text input; 3=level/text output

	// If xAP BSC command and no "id" found in the body
	if (!strcasecmp(header.class, "xAPBSC.cmd") && !(flgDataCheck & 0x01))
		return NULL;

	// Only for BSC commands, return the pointer to the next body if exists
	if (pStr1[1] == '\n' && pStr1[2] != 0 && !strcasecmp(header.class, "xAPBSC.cmd"))
		return pStr1+2;	// Return pointer to the next body
	else
		return NULL;
}

//*************************************************************************
// getBSCvalFormat
//
// Returns the format of a BSC value (level or text)
//
//	'strValue'		Value string (units included)
//
//	Returns:
//		1 : the value must be contained in a "level" field
//		2 : the value must be contained in a "text" field
//		0 : in case of error
//*************************************************************************
static short int getBSCvalFormat(char *strValue)
{
	char *pStr, strBuf[EPVALULEN];
	int intVal1, intVal2;

	if (strlen(strValue) >= sizeof(strBuf))
		return 0;

	strcpy(strBuf, strValue);

	// Level format (value/total)?
	if ((pStr = strchr(strBuf, '/')) != NULL)
	{
		pStr[0] = 0;
		intVal1 = atoi(strBuf);		// value
		intVal2 = atoi(pStr+1);		// total
		// value <= total and total > 0?
		if (intVal1 <= intVal2 && intVal2 > 0)
			return 1;	// Level format
	}

	return 2;	// Text format
}

//*************************************************************************
// xapSendBSCevn
//
// This functions sends a xAPBSC.event message notifying a value change
//
//	'fdSocket'		UDP socket
//	'strSource'		Source address
//	'strUID'			Device UID
//	'strValue'		endpoint value (BSC text or level field). NULL if n.a.
//	'strState'		endpoint state (BSC state field)
//	'flgType'		Type of device: 0=input, 1=output
//
//	Returns:
//		1 if the functions sends the message successfully
//		0 otherwise
//*************************************************************************
short int xapSendBSCevn(int fdSocket, char *strSource, char *strUID, char *strValue, char *strState, BYTE flgType)
{
	char strMessage[SIZEXAPMESS];		// Outgoing UDP message
	char strBody[SIZEXAPMESS];			// Body string

	sckxapAddr.sin_family = AF_INET;						// Same familly than the socket
	sckxapAddr.sin_addr.s_addr	= INADDR_BROADCAST;	// Broadcast over 255.255.255.255
	sckxapAddr.sin_port = htons(XAP_PORT);				// xAP communication port
	memset(&(sckxapAddr.sin_zero), '\0', 8); 			// Clean the rest of the structure

	// If the endpoint is an input
	if (!flgType)
		sprintf(strBody, "input.state\n{\nstate=%s\n", strState);
	// an output
	else
		sprintf(strBody, "output.state\n{\nstate=%s\n", strState);

	// Text or level field
	if (strValue != NULL)
	{
		switch (getBSCvalFormat(strValue))
		{
			case 1:		// Level format
				sprintf(strBody, "%slevel=%s\n", strBody, strValue);
				break;
			case 2:		// Text format
				sprintf(strBody, "%stext=%s\n", strBody, strValue);
				break;
			default:
				return 0;
				break;
		}
	}
	strcat(strBody, "}\n");

	// Assemble the "xAPBSC.event" message
	sprintf(strMessage, "xap-header\n{\nv=%d\nhop=1\nuid=%s\nclass=xAPBSC.event\nsource=%s\n}\n%s",
		XAP_VERSION, strUID, strSource, strBody);

	// Send the message
	if (sendto(fdSocket, strMessage, strlen(strMessage), 0, (struct sockaddr*)&sckxapAddr, sizeof(sckxapAddr)) < 0)
		return 0;

	return 1;
}

//*************************************************************************
// xapSendBSCinf
//
// This functions sends a xAPBSC.info message
//
//	'fdSocket'		UDP socket
//	'strSource'		Source address
//	'strUID'			Device UID
//	'strValue'		endpoint value (BSC text or level field). NULL if n.a.
//	'strState'		endpoint state (BSC state field)
//	'flgType'		Type of device: 0=input, 1=output

//
//	Returns:
//		1 if the functions sends the message successfully
//		0 otherwise
//*************************************************************************
short int xapSendBSCinf(int fdSocket, char *strSource, char *strUID, char *strValue, char *strState, BYTE flgType)
{
	char strMessage[SIZEXAPMESS];		// Outgoing UDP message
	char strBody[SIZEXAPMESS];			// Body string


	sckxapAddr.sin_family = AF_INET;						// Same familly than the socket
	sckxapAddr.sin_addr.s_addr	= INADDR_BROADCAST;	// Broadcast over 255.255.255.255
	sckxapAddr.sin_port = htons(XAP_PORT);				// xAP communication port
	memset(&(sckxapAddr.sin_zero), '\0', 8); 			// Clean the rest of the structure

	// If the endpoint is an input
	if (!flgType)
		sprintf(strBody, "input.state\n{\nstate=%s\n", strState);
	// an output
	else
		sprintf(strBody, "output.state\n{\nstate=%s\n", strState);

	// Text or level field
	if (strValue != NULL)
	{
		switch (getBSCvalFormat(strValue))
		{
			case 1:		// Level format
				sprintf(strBody, "%slevel=%s\n", strBody, strValue);
				break;
			case 2:		// Text format
				sprintf(strBody, "%stext=%s\n", strBody, strValue);
				break;
			default:
				return 0;
				break;
		}
	}
	strcat(strBody, "}\n");

	// Assemble the "xAPBSC.info" message
	sprintf(strMessage, "xap-header\n{\nv=%d\nhop=1\nuid=%s\nclass=xAPBSC.info\nsource=%s\n}\n%s",
		XAP_VERSION, strUID, strSource, strBody);

	// Send the message
	if (sendto(fdSocket, strMessage, strlen(strMessage), 0, (struct sockaddr*)&sckxapAddr, sizeof(sckxapAddr)) < 0)
		return 0;

	return 1;
}

//*************************************************************************
// xapSendBSCcmd
//
// This functions sends a xAPBSC.cmd message
//	Only one body is allowed with this function
//
//	'fdSocket'		UDP socket
//	'strSource'		Source address
//	'strUID'			Device UID
//	'strTarget'		Target address (subaddress included)
//	'strValue'		New value for the endpoint ("text" or "level" field)
//	'strState'		New state for the endpoint ("state" field)
//
//	Returns:
//		1 if the functions sends the message successfully
//		0 otherwise
//*************************************************************************
short int xapSendBSCcmd(int fdSocket, char *strSource, char *strUID, char *strTarget, char *strValue, char *strState)
{
	char strMessage[SIZEXAPMESS];		// Outgoing UDP message
	char strBody[SIZEXAPMESS];			// Body string

	sckxapAddr.sin_family = AF_INET;						// Same familly than the socket
	sckxapAddr.sin_addr.s_addr	= INADDR_BROADCAST;	// Broadcast over 255.255.255.255
	sckxapAddr.sin_port = htons(XAP_PORT);				// xAP communication port
	memset(&(sckxapAddr.sin_zero), '\0', 8); 			// Clean the rest of the structure

	// Assemble message body
	if (strState != NULL)
		sprintf(strBody, "output.state.1\n{\nid=*\nstate=%s\n", strState);
	else
		strcpy(strBody, "output.state.1\n{\nid=*\n");

	// Text or level field
	if (strValue != NULL)
	{
		switch (getBSCvalFormat(strValue))
		{
			case 1:		// Level format
				sprintf(strBody, "%slevel=%s\n", strBody, strValue);
				break;
			case 2:		// Text format
				sprintf(strBody, "%stext=%s\n", strBody, strValue);
				break;
			default:
				return 0;
				break;
		}
	}
	strcat(strBody, "}\n");	// Close BSC message

	// Assemble the "xAPBSC.cmd" message
	sprintf(strMessage, "xap-header\n{\nv=%d\nhop=1\nuid=%s\nclass=xAPBSC.cmd\nsource=%s\ntarget=%s\n}\n%s",
		XAP_VERSION, strUID, strSource, strTarget, strBody);

	// Send the message
	if (sendto(fdSocket, strMessage, strlen(strMessage), 0, (struct sockaddr*)&sckxapAddr, sizeof(sckxapAddr)) < 0)
		return 0;

	return 1;
}

//*************************************************************************
// xapSendBSCqry
//
// This functions sends a xAPBSC.query message
//
//	'fdSocket'		UDP socket
//	'strSource'		Source address
//	'strUID'			Device UID
//	'strTarget'		Target address (subaddress included)
//
//	Returns:
//		1 if the functions sends the message successfully
//		0 otherwise
//*************************************************************************
short int xapSendBSCqry(int fdSocket, char *strSource, char *strUID, char *strTarget)
{
	char strMessage[SIZEXAPMESS];		// Will store the incoming UDP message
	char strBody[SIZEXAPMESS];			// Body string

	sckxapAddr.sin_family = AF_INET;						// Same familly than the socket
	sckxapAddr.sin_addr.s_addr	= INADDR_BROADCAST;	// Broadcast over 255.255.255.255
	sckxapAddr.sin_port = htons(XAP_PORT);				// xAP communication port
	memset(&(sckxapAddr.sin_zero), '\0', 8); 			// Clean the rest of the structure

	// Construct the message body
	strcpy(strBody, "request\n{\n}\n");

	// Assemble the "xAPBSC.query" message
	sprintf(strMessage, "xap-header\n{\nv=%d\nhop=1\nuid=%s\nclass=xAPBSC.query\nsource=%s\ntarget=%s\n}\n%s",
		XAP_VERSION, strUID, strSource, strTarget, strBody);

	// Send the message
	if (sendto(fdSocket, strMessage, strlen(strMessage), 0, (struct sockaddr*)&sckxapAddr, sizeof(sckxapAddr)) < 0)
		return 0;

	return 1;
}

