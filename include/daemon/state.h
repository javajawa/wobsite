#ifndef _WOBSITE_STATE_H
#define _WOBSITE_STATE_H

enum state
{
	/* States that are used as bitmasks */
	ACCEPT    = 0x01,
	KEEPALIVE = 0x02,
	ENDING    = 0x04,

	/* Actual status for use */
	UNCREATED = 0x00,
	STARTING  = 0x10 | ACCEPT | KEEPALIVE,
	ACTIVE    = 0x20 | ACCEPT | KEEPALIVE,
	SINGLES   = 0x30 | ACCEPT,
	DRAINING  = 0x40          | KEEPALIVE,
	SHUTDOWN  = 0x50,
	JOINED    = 0x60,
};

#endif
