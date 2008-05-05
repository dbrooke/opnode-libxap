/**************************************************************************

	xapcommon.c

	Bottom level functions of the xAP interface. Common functions to any
	xAP schema

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

	10/07/07 by Daniel Berenguer : Allow extended UID's
	03/09/07 by Daniel Berenguer : first version.

***************************************************************************/

// Include section:

#include "globalxap.h"

// Define section:

// Global functions section:

// Global variables section:

char strlocalIPaddr[16];		// Local IP address of the controller

//*************************************************************************
// xapSetup
//
//	Initializes the UDP communication parameters, opens the UDP socket and
//	sets values for the xAP global variables
//
//	'strIPaddr'		Controller IP address
//
// Returns:
//		Socket created for the xAP communications
//		-1 if any error
//*************************************************************************
short int xapSetup(char *strIPaddr)
{
	int on = 1;		// (TRUE)
	int fdUDPsocket;						// UDP socket

	// Copy the IP address into the global variable strlocalIPaddr (common to xAP and xPL)
	strcpy(strlocalIPaddr, strIPaddr);

	// Create UDP socket
	if ((fdUDPsocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;

	// Configure the socket for broadcast communications
	if (setsockopt(fdUDPsocket, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0)
		return -1;

	// Initialize the address structure
	memset((void*)&sckxapAddr,	0,	sizeof(sckxapAddr));

	sckxapAddr.sin_family = AF_INET;				// Same family than the socket
	sckxapAddr.sin_addr.s_addr	= INADDR_ANY;	// Any packet addressed to us
	sckxapAddr.sin_port = htons(XAP_PORT);		// xAP UDP port
	memset(&(sckxapAddr.sin_zero), '\0', 8); 	// Clean the rest of the structure

	// Bind the socket to our address and port
	if (bind(fdUDPsocket, (struct sockaddr*)&sckxapAddr, sizeof(sckxapAddr)) < 0)
		return -1;

	return fdUDPsocket;	// Return the socket descriptor
}

//*************************************************************************
// xapRead
//
//	Reads incoming xAP messages
//
//	'fdSocket'		UDP socket
//	'xapMessage'	xAP message string to be returned
//
// Returns:
//		1 if the function executes succesfully and the message doesn't
//		come from our device
//		0 if the function executes succesfully and the message comes
//		from our device
//		-1 if any error
//*************************************************************************

short int xapRead(int fdSocket, char *xapMessage)
{
	int intAddressLen;					// Size of the origin address
	char strMessage[SIZEXAPMESS];		// Will store the incoming UDP message
	struct sockaddr_in sckInAddress;	// Address structure
	int intRes = 0;

	// Clean the buffer contents
	memset(strMessage, 0, sizeof(strMessage));

	intAddressLen = sizeof(sckInAddress);
	// Read the incoming message
	intRes = recvfrom(fdSocket, strMessage, sizeof(strMessage)-1, 0, (struct sockaddr*)&sckInAddress, &intAddressLen);
	if (intRes == -1)
		return -1;

	// Copy the received message into the target buffer
	strcpy(xapMessage, strMessage);

	// If the received message doesn't come from our device
	if(strcmp(strlocalIPaddr, inet_ntoa(sckInAddress.sin_addr)))
		intRes = 1;

	return intRes;
}

//*************************************************************************
// xapReadHead
//
//	Evaluates the header section from the message contained in the global
// variable 'strIncXapBuf'
//
//	'fdSocket'		UDP socket
//	'header'			Pointer to the header struct to return the results
//
// Returns:
//		Pointer to the message body if the function executes succesfully
//		and the message doesn't come from our device
//		NULL otherwise
//*************************************************************************

char * xapReadHead(int fdSocket, xaphead *header)
{
	char *pStr1, *pStr2;					// Pointers to strings
	char strKey[SIZEXAPKEY];			// Keyword string
	char strValue[SIZEXAPADDR];		// Value string
	int intLen, intRes;
	char chr;
	BYTE flgDataCheck = 0;				// Checks the consistency of the message header
	BYTE flgHbeat = FALSE;				// TRUE if the received message is a xAP heartbeat


	// Initialize the target structure (header)
	header->v = 0;
	header->hop = 0;
	memset(header->uid, 0, sizeof(header->uid));
	memset(header->class, 0, sizeof(header->class));
	memset(header->source, 0, sizeof(header->source));
	memset(header->target, 0, sizeof(header->target));

	callBodyTimes = 0;	// Reset the number of studied bodies (this is a new message)

	// Only if the message doesn't come from our device
	if ((intRes = xapRead(fdSocket, strIncXapBuf)) != 1)
		return NULL;

	// Inspect the message header
	pStr1 = strIncXapBuf;		// Place the pointer at the beginning of the xAP message

	// Skip possible leading <LF>
	if (pStr1[0] == '\n')
		pStr1++;

	// Verify the correct beginning of the message header
	if (!strncasecmp(pStr1, "xap-header\n{\n", 13))
		pStr1 += 13;				// Place the pointer inside the header section
	// Is it a xAP heartbeat
	else if (!strncasecmp(pStr1, "xap-hbeat\n{\n", 12))
	{
		flgHbeat = TRUE;			// it's a xAP heartbeat
		pStr1 += 12;				// Place the pointer inside the header section
	}
	else
		return NULL;

	// Until the end of the header section
	while (pStr1[0] != '}')
	{
		// Capture the key string
		if ((pStr2 = strchr(pStr1, '=')) == NULL)
			return NULL;

		if ((intLen = pStr2 - pStr1) >= sizeof(strKey))			// Only within the limits of strKey
			return NULL;

		strncpy(strKey, pStr1, intLen);								// Copy the key string into strKey
		strKey[intLen] = 0;												// Terminate the string
		pStr1 = pStr2 + 1;												// Move the pointer to the position of the value

		// Capture the value string
		if ((pStr2 = strchr(pStr1, '\n')) == NULL)
			return NULL;

		if ((intLen = pStr2 - pStr1) >= sizeof(strValue))	// Only within the limits of strValue
			return NULL;

		strncpy(strValue, pStr1, intLen);					// Copy the value string into strValue
		strValue[intLen] = 0;									// Terminate the string
		pStr1 = pStr2 + 1;										// Move the pointer to the next line

		// Evaluate the key/value pair
		chr = strKey[0];											// Take strKey first character
		if (isupper((int)chr))									// Convert strKey first character to lower case
			chr = tolower((int)chr);							// if necessary

		switch (chr)												// Consider strKey first character
		{
			case 'v':
				// xAP specification version?
				if (!strcasecmp(strKey, "v"))
				{
					header->v = atoi(strValue);				// Store the version in the struct
					flgDataCheck |= 0x01;						// First bit on
				}
				break;
			case 'h':
				// hop count?
				if (!strcasecmp(strKey, "hop"))
				{
					header->hop = atoi(strValue);				// Store the hop count in the struct
					flgDataCheck |= 0x02;						// Second bit on
				}
				break;
			case 'u':
				// UID?
				if (!strcasecmp(strKey, "uid"))
				{
					if (strlen(strValue) >= sizeof(header->uid))
						return NULL;
					strcpy(header->uid, strValue);			// Store the UID in the struct
					flgDataCheck |= 0x04;						// Third bit on
				}
				break;
			case 'c':
				// Message class?
				if (!strcasecmp(strKey, "class"))
				{
					if (strlen(strValue) > sizeof(header->class))
						return NULL;
					strcpy(header->class, strValue);			// Store the class in the struct
					flgDataCheck |= 0x08;						// Fourth bit on
				}
				break;
			case 's':
				// xAP source?
				if (!strcasecmp(strKey, "source"))
				{
					if (strlen(strValue) > sizeof(header->source))
						return NULL;
					strcpy(header->source, strValue);		// Store the source in the struct
					flgDataCheck |= 0x10;						// Fifth bit on
				}
				break;
			case 't':
				// xAP target?
				if (!strcasecmp(strKey, "target"))
				{
					if (strlen(strValue) > sizeof(header->target))
						return NULL;
					strcpy(header->target, strValue);		// Store the target in the struct
					flgDataCheck |= 0x20;						// Sixth bit on
				}
				break;
			case 'i':
				// Heartbeat interval?
				if (!strcasecmp(strKey, "interval"))
				{
					header->interval = atoi(strValue);		// Store the heartbeat interval in the struct
					flgDataCheck |= 0x40;						// Seventh bit on
				}
				break;
			default:
				break;
		}
	}

	// Verify the consistency of the message header
	// If the message is a xAP heartbeat
	if (flgHbeat)
	{
		// Is the message really not a xAP heartbeat? Consistency error?
		if (strcasecmp(header->class, "xap-hbeat.alive") || flgDataCheck != 0x5F)	// 0x5F = 01011111b
			return NULL;
	}
	// If conventional message
	else
	{
		// Suported schemas

		// If BSC command or query, there should be a target field. Same for xap-x10.request
		if (!strcasecmp(header->class, "xAPBSC.cmd") || !strcasecmp(header->class, "xAPBSC.query") ||
			 !strcasecmp(header->class, "xap-x10.request"))
		{
			if (flgDataCheck != 0x3F)	// 0x3F = 00111111b
				return NULL;
		}
	}

	return pStr1+2;		// Return pointer to the body
}

//*************************************************************************
//	xapEvalTarget
//
//	Evaluates the target field and decides whether the message is addressed
//	to the device or not. It also extracts the target endpoint. This function
//	considers wildcarding.
//	Address format: Vendor.Device.Instance:location.endpointname
//
//	'strTarget'		string containing the target of the xAP message
//	'strVendor'		Vendor name
//	'strDevice'		Device name
//	'strInstance'	Instance name
//	'endp'			Pointer to the xAP endpoint structure where to pass the
//						results
//
// Returns:
//		1 if the message is addressed to our device
//		0 otherwise
//*************************************************************************

short int xapEvalTarget(char *strTarget, char *strVendor, char *strDevice, char *strInstance, xAPendp *endp)
{
	char *pStr1, *pStr2, *pSubAddr;		// Pointers to strings
	BYTE flgGoOn = FALSE;					// If true, parse subaddress string
	BYTE flgWildCard = FALSE;				// If true, ">" wildcard found in the target base address
	char strBuffer[SIZEXAPADDR];

	strcpy(strBuffer, strTarget);

	pSubAddr = NULL;
	pStr1 = strBuffer;		// Pointer at the beginning of the target string

	// Search subaddress
	if ((pStr2 = strchr(pStr1, ':')) != NULL)
	{
		pStr2[0] = 0;					// Terminate base address string
		pSubAddr = pStr2 + 1;		// Pointer to the subaddress string
	}

	// Capture vendor name
	if ((pStr2 = strchr(pStr1, '.')) != NULL)
	{
		pStr2[0] = 0;
		if (!strcasecmp(pStr1, strVendor) || !strcmp(pStr1, "*"))
		{
			pStr1 = pStr2 + 1;
			// Capture device name
			if ((pStr2 = strchr(pStr1, '.')) != NULL)
			{
				pStr2[0] = 0;
				if (!strcasecmp(pStr1, strDevice) || !strcmp(pStr1, "*"))
				{
					pStr1 = pStr2 + 1;
					// Instance name
					if (!strcasecmp(pStr1, strInstance) || !strcmp(pStr1, "*"))
						flgGoOn = TRUE;
					// ">" wildcard?
					else if (!strcmp(pStr1, ">"))
						flgWildCard = TRUE;
				}
			}
			// ">" wildcard?
			else if (!strcmp(pStr1, ">"))
				flgWildCard = TRUE;
		}
	}
	// ">" wildcard?
	else if (!strcmp(pStr1, ">"))
		flgWildCard = TRUE;

	// Parse Subaddress

	if (flgWildCard || flgGoOn)
	{
		// The target matches our address
		// Search the target endpoint within the target subaddress

		// Wilcard subaddress too?
		if (pSubAddr == NULL && flgWildCard)
		{
			strcpy(endp->location, "*");	// For every location
			strcpy(endp->name, "*");		// For every endpoint name
			return 1;
		}

		pStr1 = pSubAddr;

		// Capture the endpoint location
		if ((pStr2 = strchr(pStr1, '.')) != NULL)
		{
			pStr2[0] = 0;
			// Avoid exceeding sizes
			if (strlen(pStr1) < sizeof(endp->location))
			{
				strcpy(endp->location, pStr1);				// Copy the endpoint location
				pStr1 = pStr2 + 1;								// Pointer at the beginning of the endpoint name
				// Capture the endpoint name
				// If the message is adressed to any endpoint within the captured location
				if (!strcmp(pStr1, ">"))
					strcpy(endp->name, "*");					// For every endpoint name
				// else, avoid exceeding sizes
				else if (strlen(pStr1) < sizeof(endp->name))
					strcpy(endp->name, pStr1);					// Copy the endpoint name
			}
		}
		// ">" wildcard?
		else if (!strcmp(pStr1, ">"))
		{
			strcpy(endp->location, "*");	// For every location
			strcpy(endp->name, "*");		// For every endpoint name
		}
		return 1;	// Address match. endp contains the targeted endpoint
	}

	return 0;	// Address does not match
}

//*************************************************************************
// xapSendHbeat
//
// This functions sends a xAP heartbeat
//
//	'fdSocket'		UDP socket
//	'strSource'		Source address
//	'strUID'			xAP UID
//	'hbeatFreq'		xAP heartbeat frequence (in seconds)
//
//	Returns:
//		1 if the functions sends the message successfully
//		0 otherwise
//*************************************************************************

short int xapSendHbeat(int fdSocket, char *strSource, char *strUID, int hbeatFreq)
{
	char strMessage[SIZEXAPMESS];		// Will store the outgoing UDP message

	sckxapAddr.sin_family = AF_INET;						// Same familly than the socket
	sckxapAddr.sin_addr.s_addr	= INADDR_BROADCAST;	// Broadcast over 255.255.255.255
	sckxapAddr.sin_port = htons(XAP_PORT);				// xAP communication port
	memset(&(sckxapAddr.sin_zero), '\0', 8); 			// Clean the rest of the structure

	// Assemble the "xap-hbeat.alive" message
	sprintf(strMessage, "xap-hbeat\n{\nv=%d\nhop=1\nuid=%s\nclass=xap-hbeat.alive\nsource=%s\ninterval=%d\nport=%d\n}\n",
		XAP_VERSION, strUID, strSource, hbeatFreq, XAP_PORT);

	// Send the message
	if (sendto(fdSocket, strMessage, strlen(strMessage), 0, (struct sockaddr*)&sckxapAddr, sizeof(sckxapAddr)) < 0)
		return 0;

	return 1;
}

//*************************************************************************
// xapSendHbShutdown
//
// This functions sends a xAP heartbeat STOP message (xap-hbeat.stopped)
//
//	'fdSocket'		UDP socket
//	'strSource'		Source address
//	'strUID'			xAP UID
//	'hbeatFreq'		xAP heartbeat frequence (in seconds)
//
//	Returns:
//		1 if the functions sends the message successfully
//		0 otherwise
//*************************************************************************

short int xapSendHbShutdown(int fdSocket, char *strSource, char *strUID, int hbeatFreq)
{
	char strMessage[SIZEXAPMESS];		// Will store the outgoing UDP message

	sckxapAddr.sin_family = AF_INET;						// Same familly than the socket
	sckxapAddr.sin_addr.s_addr	= INADDR_BROADCAST;	// Broadcast over 255.255.255.255
	sckxapAddr.sin_port = htons(XAP_PORT);				// xAP communication port
	memset(&(sckxapAddr.sin_zero), '\0', 8); 			// Clean the rest of the structure

	// Assemble the "xap-hbeat.stopped" message
	sprintf(strMessage, "xap-hbeat\n{\nv=%d\nhop=1\nuid=%s\nclass=xap-hbeat.stopped\nsource=%s\ninterval=%d\nport=%d\n}\n",
		XAP_VERSION, strUID, strSource, hbeatFreq, XAP_PORT);

	// Send the message
	if (sendto(fdSocket, strMessage, strlen(strMessage), 0, (struct sockaddr*)&sckxapAddr, sizeof(sckxapAddr)) < 0)
		return 0;

	return 1;
}
