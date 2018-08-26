#ifndef _WOBSITE_STATE_H
#define _WOBSITE_STATE_H

enum state
{
	/* States that are used as bitmasks */
	ACCEPT    = 0x10,
	KEEPALIVE = 0x20,
	ENDING    = 0x30,

	/* Actual status for use */
	UNCREATED = 0x00,
	STARTING  = 0x01 | ACCEPT | KEEPALIVE,
	ACTIVE    = 0x02 | ACCEPT | KEEPALIVE,
	SINGLES   = 0x03 | ACCEPT,
	DRAINING  = 0x04          | KEEPALIVE,
	SHUTDOWN  = 0x05,
	JOINED    = 0x06,
};

#endif
