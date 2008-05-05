/**************************************************************************

	xapintranet.c

	Functions needed to communicate under the xAP intranet schema

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

	03/31/08 by Daniel Berenguer : first version.

***************************************************************************/

// Include section:

#include "globalxap.h"

// Define section:

// Global functions section:

// Global variables section:


//*************************************************************************
// xapReadSMSBody
//
//	Analyzes the xAP SMS message body pointed by pBody
//
//	'fdSocket'		UDP socket
//	'pBody'			Pointer to the beginning of the body section within the
//						xAP message. This pointer should be recovered from a
//						previous call to 'xapReadHead'
//	'name'			Name of application (returned value)
//	'desc'			Description of application (returned value)
//	'pc'				Name of PC (returned value)
//	'icon'			URL to the application icon
//	'url'				Application URL
//	'type'			Type of web.service message (returned value)
//
// Returns:
//		1 if the function completes successfully
//		0 in case of error
//*************************************************************************

short int xapReadWebBody(int fdSocket, char *pBody, char *name, char *desc, char *pc, char *icon, char *url, BYTE *type)
{
	char strName[SIZEXAPNAME];		// Name of application
	char strDesc[SIZEXAPADDR];			// Description
	char strPC[SIZEXAPNAME];			// Name of PC
	char strIcon[SIZEXAPADDR];			// icon
	char strURL[SIZEXAPADDR];			// URL
	BYTE flgType = WEB_REQUEST;		// Request message by default
	char *pStr1, *pStr2;					// Pointers to strings
	char strKey[SIZEXAPKEY];			// Keyword string
	char strValue[SIZEXAPADDR];		// Value string
	char chr;
	BYTE flgDataCheck = 0;				// Checks the consistency of the message body


	// Inspect the message body
	pStr1 = pBody;		// Place the pointer at the beginning of the xAP body

	// Verify the correct beginning of the message body
	if (!strncasecmp(pStr1, "request\n{\n", 10))
		pStr1 += 10;			// Place the pointer inside the body section
	else if (!strncasecmp(pStr1, "server.start\n{\n", 15))
	{
		flgType = WEB_START;
		pStr1 += 15;			// Place the pointer inside the body section
	}
	else if (!strncasecmp(pStr1, "server.stop\n{\n", 14))
	{
		flgType = WEB_STOP;
		pStr1 += 14;			// Place the pointer inside the body section
	}
	else
		return 0;

	// Up to the end of the body section
	while (pStr1[0] != '}')
	{
		// Capture the key string
		if ((pStr2 = strchr(pStr1, '=')) == NULL)
			return 0;
		pStr2[0] = 0;
		if (strlen(pStr1) >= sizeof(strKey))						// Only within the limits of strKey
			return 0;
		strcpy(strKey, pStr1);											// Copy the key string into strKey
		pStr1 = pStr2 + 1;												// Move the pointer to the position of the value

		// Capture the value string
		if ((pStr2 = strchr(pStr1, '\n')) == NULL)
			return 0;
		pStr2[0] = 0;
		if (strlen(pStr1) >= sizeof(strValue))						// Only within the limits od strValue
			return 0;
		strcpy(strValue, pStr1);										// Copy the key string into strValue
		pStr1 = pStr2 + 1;												// Move the pointer to the next line

		// Evaluate the key/value pair
		chr = strKey[0];													// Take strKey first character
		if (isupper((int)chr))											// Convert strKey first character to lower case
			chr = tolower((int)chr);									// if necessary

		switch (chr)														// Consider strKey first character
		{
			case 'n':
				// name of application?
				if (!strcasecmp(strKey, "name"))
				{
					if (strlen(strValue) >= sizeof(strName))
						return 0;
					strcpy(strName, strValue);						// Store name
					flgDataCheck |= 0x01;								// First bit on
				}
				break;
			case 'd':
				// description of application?
				if (!strcasecmp(strKey, "desc"))
				{
					if (strlen(strValue) >= sizeof(strDesc))
						return 0;
					strcpy(strDesc, strValue);							// Store description
					flgDataCheck |= 0x02;								// Second bit on
				}
				break;
			case 'p':
				// name of PC?
				if (!strcasecmp(strKey, "pc"))
				{
					if (strlen(strValue) >= sizeof(strPC))
						return 0;
					strcpy(strPC, strValue);							// Store PC name
					flgDataCheck |= 0x04;								// Second bit on
				}
				break;
			case 'i':
				// application icon?
				if (!strcasecmp(strKey, "icon"))
				{
					if (strlen(strValue) >= sizeof(strIcon))
						return 0;
					strcpy(strIcon, strValue);							// Store icon URL
					flgDataCheck |= 0x08;								// Second bit on
				}
				break;
			case 'u':
				// application URL?
				if (!strcasecmp(strKey, "url"))
				{
					if (strlen(strValue) >= sizeof(strURL))
						return 0;
					strcpy(strURL, strValue);							// Store application URL
					flgDataCheck |= 0x10;								// Second bit on
				}
				break;
			default:
				break;
		}
	}

	*type = flgType;

	// Verify the consistency of the message body
	if (flgType == WEB_REQUEST)
	{
		if (flgDataCheck == 0)
			return 1;
	}
	else if (flgDataCheck == 0x1F)
	{
		// Return values
		if (name != NULL)
			strcpy(name, strName);
		if (desc != NULL)
			strcpy(desc, strDesc);
		if (pc != NULL)
			strcpy(pc, strPC);
		if (icon != NULL)
			strcpy(icon, strIcon);
		if (url != NULL)
			strcpy(url, strURL);

		return 1;
	}
	return 0;
}

//*************************************************************************
// xapSendWebService
//
// This functions sends a Web.Service message
//
//	'fdSocket'		UDP socket
//	'strSource'		Source address (Vendor.Device.Instance)
//	'strUID'			Device UID
//	'strName'		Name of application
//	'strDesc'		Description of application
//	'strPC'			Name of PC
//	'strIcon'		Application icon
//	'strURL'			Application URL
//	'type'			"Server.Start" or "Server.Stop"
//
//	Returns:
//		1 if the functions sends the message successfully
//		0 in case of error
//*************************************************************************
short int xapSendWebService(int fdSocket, char *strSource, char *strUID, char *strName, char *strDesc, char *strPC, char *strIcon, char *strURL, char* type)
{
	char strMessage[SIZEXAPMESS];		// Outgoing UDP message
	char strBody[SIZEXAPMESS];			// Body string


	sckxapAddr.sin_family = AF_INET;						// Same familly than the socket
	sckxapAddr.sin_addr.s_addr	= INADDR_BROADCAST;	// Broadcast over 255.255.255.255
	sckxapAddr.sin_port = htons(XAP_PORT);				// xAP communication port
	memset(&(sckxapAddr.sin_zero), '\0', 8); 			// Clean the rest of the structure

	sprintf(strBody, "{\nname=%s\ndesc=%s\npc=%s\nicon=%s\nurl=%s\n}\n",
		strName, strDesc, strPC, strIcon, strURL);

	// Assemble the message
	sprintf(strMessage, "xap-header\n{\nv=%d\nhop=1\nuid=%s\nclass=web.service\nsource=%s\n}\n%s\n%s",
		XAP_VERSION, strUID, strSource, type, strBody);

	// Send message
	if (sendto(fdSocket, strMessage, strlen(strMessage), 0, (struct sockaddr*)&sckxapAddr, sizeof(sckxapAddr)) < 0)
		return 0;

	return 1;
}

//*************************************************************************
// xapSendWebRequest
//
// This functions sends a Web.Service request
//
//	'fdSocket'		UDP socket
//	'strSource'		Source address (Vendor.Device.Instance)
//	'strUID'			Device UID
//
//	Returns:
//		1 if the functions sends the message successfully
//		0 in case of error
//*************************************************************************
short int xapSendWebRequest(int fdSocket, char *strSource, char *strUID)
{
	char strMessage[SIZEXAPMESS];		// Outgoing UDP message


	sckxapAddr.sin_family = AF_INET;						// Same familly than the socket
	sckxapAddr.sin_addr.s_addr	= INADDR_BROADCAST;	// Broadcast over 255.255.255.255
	sckxapAddr.sin_port = htons(XAP_PORT);				// xAP communication port
	memset(&(sckxapAddr.sin_zero), '\0', 8); 			// Clean the rest of the structure

	// Assemble the message
	sprintf(strMessage, "xap-header\n{\nv=%d\nhop=1\nuid=%s\nclass=web.service\nsource=%s\n}\nRequest\n{\n}\n",
		XAP_VERSION, strUID, strSource);

	// Send message
	if (sendto(fdSocket, strMessage, strlen(strMessage), 0, (struct sockaddr*)&sckxapAddr, sizeof(sckxapAddr)) < 0)
		return 0;

	return 1;
}
