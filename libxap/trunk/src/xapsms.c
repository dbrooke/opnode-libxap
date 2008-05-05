/**************************************************************************

	xapsms.c

	Functions needed to communicate under the xAP SMS schema

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

	04/22/07 by Daniel Berenguer : first version.

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
//	'phoneNum'		Phone number
//	'SMStext'		SMS text
//	'flgIncoming'	TRUE if the message describes an incoming SMS. FALSE if
//						the controller is requested to send a SMS. Value to be returned
//
// Returns:
//		1 if the function completes successfully
//		0 in case of error
//*************************************************************************

short int xapReadSMSBody(int fdSocket, char *pBody, char *phoneNum, char *SMStext, BYTE *flgIncoming)
{
	char strPhoneNum[20];				// Phone number
	char strSMStext[129];				// Message text to send
	BYTE flgInc = FALSE;					// TRUE if incoming message
	char *pStr1, *pStr2;					// Pointers to strings
	char strKey[SIZEXAPKEY];			// Keyword string
	char strValue[SIZEXAPADDR];		// Value string
	char chr;
	BYTE flgDataCheck = 0;				// Checks the consistency of the message body


	// Inspect the message body
	pStr1 = pBody;		// Place the pointer at the beginning of the xAP body

	// Verify the correct beginning of the message body
	if (!strncasecmp(pStr1, "outbound\n{\n", 11))
		pStr1 += 11;			// Place the pointer inside the body section
	else if (!strncasecmp(pStr1, "inbound\n{\n", 10))
	{
		flgInc = TRUE;
		pStr1 += 10;			// Place the pointer inside the body section
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
			case 'm':
				// text message?
				if (!strcasecmp(strKey, "msg"))
				{
					if (strlen(strValue) >= sizeof(strSMStext))
						return 0;
					strcpy(strSMStext, strValue);						// Store SMS text
					flgDataCheck |= 0x01;								// First bit on
				}
				break;
			case 'n':
				// phone number?
				if (!strcasecmp(strKey, "num"))
				{
					if (strlen(strValue) >= sizeof(strPhoneNum))
						return 0;
					strcpy(strPhoneNum, strValue);					// Store phonenumber
					flgDataCheck |= 0x02;								// Second bit on
				}
				break;
			default:
				break;
		}
	}

	// Verify the consistency of the message body
	if (flgDataCheck != 0x03)
		return 0;

	// Return values
	strcpy(phoneNum, strPhoneNum);
	strcpy(SMStext, strSMStext);
	*flgIncoming = flgInc;

	return 1;
}


//*************************************************************************
// xapSendSMSreceipt
//
// This functions sends a SMS.receipt message notifying the send of a SMS
//
//	'fdSocket'		UDP socket
//	'strSource'		Source address (Vendor.Device.Instance)
//	'strUID'			Device UID
//	'phoneNum'		Phone number
//	'SMStext'		Message sent
//	'flgSent'		TRUE if message successfully sent
//	'strError'		Error message (only if flgSent = FALSE)
//
//	Returns:
//		1 if the functions sends the message successfully
//		0 in case of error
//*************************************************************************
short int xapSendSMSreceipt(int fdSocket, char *strSource, char *strUID, char *phoneNum, char *SMStext, BYTE flgSent, char *strError)
{
	char strMessage[SIZEXAPMESS];		// Outgoing UDP message
	char strBody[SIZEXAPMESS];			// Body string
	char strSent[4];
	char strErr[EPVALULEN];

	// SMS successfully sent?
	if (flgSent)
	{
		strcpy(strSent, "yes");
		strcpy(strErr, "none");
	}
	else
	{
		strcpy(strSent, "no");
		strcpy(strErr, strError);
	}

	sckxapAddr.sin_family = AF_INET;						// Same familly than the socket
	sckxapAddr.sin_addr.s_addr	= INADDR_BROADCAST;	// Broadcast over 255.255.255.255
	sckxapAddr.sin_port = htons(XAP_PORT);				// xAP communication port
	memset(&(sckxapAddr.sin_zero), '\0', 8); 			// Clean the rest of the structure

	sprintf(strBody, "{\nmsg=%s\nnum=%s\nsent=%s\nerror=%s\n}\n", SMStext, phoneNum, strSent, strErr);

	// Assemble the "sms.receipt" message
	sprintf(strMessage, "xap-header\n{\nv=%d\nhop=1\nuid=%s\nclass=sms.receipt\nsource=%s\n}\nreceipt\n%s",
		XAP_VERSION, strUID, strSource, strBody);

	// Send message
	if (sendto(fdSocket, strMessage, strlen(strMessage), 0, (struct sockaddr*)&sckxapAddr, sizeof(sckxapAddr)) < 0)
		return 0;

	return 1;
}

//*************************************************************************
// xapSendSMSinbound
//
// This functions sends a SMS.message inbound message notifying the
//	reception of a SMS
//
//	'fdSocket'		UDP socket
//	'strSource'		Source address (Vendor.Device.Instance)
//	'strUID'			Device UID
//	'phoneNum'		Phone number
//	'SMStext'		Message sent
//
//	Returns:
//		1 if the functions sends the message successfully
//		0 in case of error
//*************************************************************************
short int xapSendSMSinbound(int fdSocket, char *strSource, char *strUID, char *phoneNum, char *SMStext)
{
	char strMessage[SIZEXAPMESS];		// Outgoing UDP message
	char strBody[SIZEXAPMESS];			// Body string


	sckxapAddr.sin_family = AF_INET;						// Same familly than the socket
	sckxapAddr.sin_addr.s_addr	= INADDR_BROADCAST;	// Broadcast over 255.255.255.255
	sckxapAddr.sin_port = htons(XAP_PORT);				// xAP communication port
	memset(&(sckxapAddr.sin_zero), '\0', 8); 			// Clean the rest of the structure

	sprintf(strBody, "{\nmsg=%s\nnum=%s\n}\n", phoneNum, SMStext);

	// Assemble the "sms.message" inbound message
	sprintf(strMessage, "xap-header\n{\nv=%d\nhop=1\nuid=%s\nclass=sms.message\nsource=%s\n}\ninbound\n%s",
		XAP_VERSION, strUID, strSource, strBody);

	// Send message
	if (sendto(fdSocket, strMessage, strlen(strMessage), 0, (struct sockaddr*)&sckxapAddr, sizeof(sckxapAddr)) < 0)
		return 0;

	return 1;
}

//*************************************************************************
// xapSendSMSoutbound
//
// This functions sends a SMS.message outbound message requesting a xAP node
//	to send of a SMS
//
//	'fdSocket'		UDP socket
//	'strSource'		Source address (Vendor.Device.Instance)
//	'strUID'			Device UID
//	'phoneNum'		Phone number
//	'SMStext'		Message sent
//
//	Returns:
//		1 if the functions sends the message successfully
//		0 in case of error
//*************************************************************************
short int xapSendSMSoutbound(int fdSocket, char *strSource, char *strUID, char *strTarget, char *phoneNum, char *SMStext)
{
	char strMessage[SIZEXAPMESS];		// Outgoing UDP message
	char strBody[SIZEXAPMESS];			// Body string


	sckxapAddr.sin_family = AF_INET;						// Same familly than the socket
	sckxapAddr.sin_addr.s_addr	= INADDR_BROADCAST;	// Broadcast over 255.255.255.255
	sckxapAddr.sin_port = htons(XAP_PORT);				// xAP communication port
	memset(&(sckxapAddr.sin_zero), '\0', 8); 			// Clean the rest of the structure

	sprintf(strBody, "{\nmsg=%s\nnum=%s\n}\n", phoneNum, SMStext);

	// Assemble the "sms.message" outbound message
	sprintf(strMessage, "xap-header\n{\nv=%d\nhop=1\nuid=%s\nclass=sms.message\nsource=%s\ntarget=%s\n}\ninbound\n%s",
		XAP_VERSION, strUID, strSource, strTarget, strBody);

	// Send message
	if (sendto(fdSocket, strMessage, strlen(strMessage), 0, (struct sockaddr*)&sckxapAddr, sizeof(sckxapAddr)) < 0)
		return 0;

	return 1;
}
