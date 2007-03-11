/**************************************************************************

	xapx10.c

	Functions needed to communicate under the xAP X10 schema

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

	03/09/07 by Daniel Berenguer : first version.

***************************************************************************/

// Include section:

#include "globalxap.h"

// Define section:

// Global functions section:

// Global variables section:


//*************************************************************************
// xapReadX10Body
//
//	Inspects the xAP X10 message body pointed by pBody
//
//	'fdSocket'		UDP socket
//	'pBody'			Pointer to the beginning of the body section within the
//						xAP message. This pointer should be recovered from a
//						previous call to 'xapReadHead'
//	'header'			Header struct obtained from a previous call to xapReadHead
//	'endp'			Pointer to the xAP endpoint struct to write the results into
//
// Returns:
//		1 if the function completes successfully
//		0 in case of error
//*************************************************************************

short int xapReadX10Body(int fdSocket, char *pBody, xaphead header, char *devlist, char *command, BYTE *level)
{
	char strDevList[50];					// List of devices pointed by the message
	char strCommand[EPVALULEN];		// Command string
	char *pStr1, *pStr2;					// Pointers to strings
	char strKey[SIZEXAPKEY];			// Keyword string
	char strValue[SIZEXAPADDR];		// Value string
	int intLen, intLevel = 0;
	char chr;
	BYTE flgDataCheck = 0;				// Checks the consistency of the message body


	// Inspect the message body
	pStr1 = pBody;		// Place the pointer at the beginning of the xAP body

	// Verify the correct beginning of the body depending of the type of xAP BSC message
	// If xap-x10.request
	if (!strcasecmp(header.class, "xap-x10.request"))
	{
		// Verify the body tittle
		if (!strncasecmp(pStr1, "xap-x10.request\n{\n", 18))
			pStr1 += 18;			// Place the pointer inside the body section
		else
			return 0;
	}
	// If xap-x10.event
	else if (!strcasecmp(header.class, "xap-x10.event"))
	{
		//Verify the body tittle
		if (!strncasecmp(pStr1, "xap-x10.event\n{\n", 16))
			pStr1 += 16;			// Place the pointer inside the body section
		else
			return 0;
	}
	else
		return 0;

	// Up to the end of the body section
	while (pStr1[0] != '}')
	{
		// Capture the key string
		if ((pStr2 = strchr(pStr1, '=')) == NULL)
			return 0;
		if ((intLen = pStr2 - pStr1) >= sizeof(strKey))			// Only within the limits od strKey
			return 0;
		strncpy(strKey, pStr1, intLen);								// Copy the key string into strKey
		strKey[intLen] = 0;												// Terminate the string
		pStr1 = pStr2 + 1;												// Move the pointer to the position of the value

		// Capture the value string
		if ((pStr2 = strchr(pStr1, '\n')) == NULL)
			return 0;
		if ((intLen = pStr2 - pStr1) >= sizeof(strValue))		// Only within the limits od strValue
			return 0;
		strncpy(strValue, pStr1, intLen);							// Copy the key string into strValue
		strValue[intLen] = 0;											// Terminate the string
		pStr1 = pStr2 + 1;												// Move the pointer to the next line

		// Evaluate the key/value pair
		chr = strKey[0];													// Take strKey first character
		if (isupper((int)chr))											// Convert strKey first character to lower case
			chr = tolower((int)chr);									// if necessary

		switch (chr)														// Consider strKey first character
		{
			case 'c':
				// command to be executed?
				if (!strcasecmp(strKey, "command"))
				{
					if (strlen(strValue) > sizeof(strCommand))
						return 0;
					//Only allowed commands
					if (strcasecmp(strValue, "ON") && strcasecmp(strValue, "OFF") && strcasecmp(strValue, "DIM") &&
						strcasecmp(strValue, "BRIGHT") && strcasecmp(strValue, "ALL_LIGHTS_ON") &&
						strcasecmp(strValue, "ALL_LIGHTS_OFF") && strcasecmp(strValue, "ALL_UNITS_OFF"))
						return 0;
					strcpy(strCommand, strValue);						// Store the command string
					flgDataCheck |= 0x01;								// First bit on
				}
				// Count (only for dim and bright commands)?
				else if (!strcasecmp(strKey, "count"))
				{
					if (strlen(strValue) > 3)
						return 0;
					intLevel = atoi(strValue);							// Absolute Dim/Bright level
					flgDataCheck |= 0x08;								// Fourth bit on
				}
				break;
			case 'd':
				// Device?
				if (!strcasecmp(strKey, "device"))
				{
					if (strlen(strValue) > sizeof(strDevList))
						return 0;
					strcpy(strDevList, strValue);						// Store the list of devices
					flgDataCheck |= 0x02;								// Second bit on
				}
				break;
			case 'e':
				// Event?
				if (!strcasecmp(strKey, "event"))
				{
					if (strlen(strValue) > sizeof(strCommand))
						return 0;
					strcpy(strCommand, strValue);						// Store the command string
					flgDataCheck |= 0x04;								// Third bit on
				}
				break;
			default:
				break;
		}
	}

	// Verify the consistency of the message body
	switch(flgDataCheck)
	{
		case 0x06:	// "device" and "event" fields
			// The "event" field is only allowed for xap-x10.event
			if (strcasecmp(header.class, "xap-x10.event"))
				return 0;
			break;
		case 0x03:	// "device" and "command" fields
			break;
		case 0x0E:	// "device", "event" and "count" fields
			// The "event" field is only allowed for xap-x10.event
			if (strcasecmp(header.class, "xap-x10.event"))
				return 0;
		case 0x0B:	// "device", "command" and "count" fields
			// The "count" field is only allowed for dim/bright commands
			if (strcasecmp(strCommand, "dim") && strcasecmp(strCommand, "bright"))
				return 0;
			break;
		default:		// Bad format
			return 0;
			break;
	}

	// Verify the correct end of the body
	if (pStr1[1] != '\n')
		return 0;

	// Return values
	strcpy(command, strCommand);
	strcpy(devlist, strDevList);
	*level = (BYTE)intLevel;

	return 1;
}


//*************************************************************************
// xapSendX10evn
//
// This functions sends a xap-x10.event message notifying a value change
//
//	'fdSocket'		UDP socket
//	'device'			Device X10 code or list of devices
//	'command'		Executed X10 command
//	'level'			Dim/Bright level
//	'strSource'		Source address (Vendor.Device.Instance)
//	'strUID'			Device UID
//
//	Returns:
//		1 if the functions sends the message successfully
//		0 in case of error
//*************************************************************************
short int xapSendX10evn(int fdSocket, char *strSource, char *strUID, char *device, char *command, BYTE level)
{
	char strMessage[SIZEXAPMESS];		// Outgoing UDP message
	char strBody[SIZEXAPMESS];			// Body string

	sckxapAddr.sin_family = AF_INET;						// Same familly than the socket
	sckxapAddr.sin_addr.s_addr	= INADDR_BROADCAST;	// Broadcast over 255.255.255.255
	sckxapAddr.sin_port = htons(XAP_PORT);				// xAP communication port
	memset(&(sckxapAddr.sin_zero), '\0', 8); 			// Clean the rest of the structure

	if (!strcasecmp(command, "dim") || !strcasecmp(command, "bright"))
		sprintf(strBody, "{\nevent=%s\ndevice=%s\ncount=%d\n}\n", command, device, level);
	else if (!strcasecmp(command, "on") || !strcasecmp(command, "off") || !strcasecmp(command, "all_lights_off") ||
				!strcasecmp(command, "all_lights_on") || !strcasecmp(command, "all_units_off"))
		sprintf(strBody, "{\nevent=%s\ndevice=%s\n}\n", command, device);

	// Assemble the "xap-x10.event" message
	sprintf(strMessage, "xap-header\n{\nv=%d\nhop=1\nuid=%s\nclass=xap-x10.event\nsource=%s\n}\nxap-x10.event\n%s",
		XAP_VERSION, strUID, strSource, strBody);

	// Send the message
	if (sendto(fdSocket, strMessage, strlen(strMessage), 0, (struct sockaddr*)&sckxapAddr, sizeof(sckxapAddr)) < 0)
		return 0;

	return 1;
}

//*************************************************************************
// xapSendX10req
//
// This functions sends a xap-x10.request message
//
//	'fdSocket'		UDP socket
//	'device'			Device X10 code, list of devices or house code
//	'command'		Executed X10 command
//	'level'			Dim/Bright level
//	'strSource'		Source address (Vendor.Device.Instance)
//	'strTarget'		Target address (Vendor.Device.Instance)
//	'strUID'			Device UID
//
//	Returns:
//		1 if the functions sends the message successfully
//		0 in case of error
//*************************************************************************
short int xapSendX10req(int fdSocket, char *strSource, char *strUID, char *strTarget, char *device, char *command, BYTE level)
{
	char strMessage[SIZEXAPMESS];		// Outgoing UDP message
	char strBody[SIZEXAPMESS];			// Body string

	sckxapAddr.sin_family = AF_INET;						// Same familly than the socket
	sckxapAddr.sin_addr.s_addr	= INADDR_BROADCAST;	// Broadcast over 255.255.255.255
	sckxapAddr.sin_port = htons(XAP_PORT);				// xAP communication port
	memset(&(sckxapAddr.sin_zero), '\0', 8); 			// Clean the rest of the structure

	if (!strcasecmp(command, "dim") || !strcasecmp(command, "bright"))
		sprintf(strBody, "{\ncommand=%s\ndevice=%s\ncount=%d\n}\n", command, device, level);
	else if (!strcasecmp(command, "on") || !strcasecmp(command, "off") || !strcasecmp(command, "all_lights_off") ||
				!strcasecmp(command, "all_lights_on") || !strcasecmp(command, "all_units_off"))
		sprintf(strBody, "{\ncommand=%s\ndevice=%s\n}\n", command, device);

	// Assemble the "xap-x10.event" message
	sprintf(strMessage, "xap-header\n{\nv=%d\nhop=1\nuid=%s\nclass=xap-x10.request\nsource=%s\ntarget=%s\n}\nxap-x10.request\n%s",
		XAP_VERSION, strUID, strSource, strTarget, strBody);

	// Send the message
	if (sendto(fdSocket, strMessage, strlen(strMessage), 0, (struct sockaddr*)&sckxapAddr, sizeof(sckxapAddr)) < 0)
		return 0;

	return 1;
}
