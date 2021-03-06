
// This file is part of the mstp-lib library, available at https://github.com/adigostin/mstp-lib
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#ifndef MSTP_LIB_BASE_TYPES_H
#define MSTP_LIB_BASE_TYPES_H

#include "../stp.h"
#include <assert.h>
#include <string.h>
#include <stdint.h>

enum PortIndex { };
inline PortIndex operator++(PortIndex& x, int) { PortIndex res = x; x = (PortIndex) (x + 1); return res; }

enum TreeIndex { };
inline TreeIndex operator++(TreeIndex& x, int) { TreeIndex res = x; x = (TreeIndex) (x + 1); return res; }

static const TreeIndex CIST_INDEX = (TreeIndex)0;

struct STP_BRIDGE;

// ============================================================================

// 16-bit value in network byte order (big-endian); since it consists of chars only, the compiler should give it byte alignment.
struct uint16_nbo
{
private:
	unsigned char vh;
	unsigned char vl;

public:
	void operator= (unsigned short value)
	{
		vh = value >> 8;
		vl = (unsigned char) value;
	}

	bool operator== (const uint16_nbo& rhs) const
	{
		return ((this->vh == rhs.vh) && (this->vl == rhs.vl));
	}

	operator uint16_t() const
	{
		return (vh << 8) | vl;
	}
};

// ============================================================================

// 32-bit value in network byte order (big-endian); since it consists of chars only, the compiler should give it byte alignment.
struct uint32_nbo
{
private:
	unsigned char vhh;
	unsigned char vhl;
	unsigned char vlh;
	unsigned char vll;

public:
	operator uint32_t() const
	{
		return (vhh << 24) | (vhl << 16) | (vlh << 8) | vll;
	}

	void operator= (unsigned int newValue)
	{
		vhh = (unsigned char) (newValue >> 24);
		vhl = (unsigned char) (newValue >> 16);
		vlh = (unsigned char) (newValue >> 8);
		vll = (unsigned char) newValue;
	}

	const uint32_nbo& operator += (unsigned int rhs)
	{
		*this = *this + rhs;
		return *this;
	}

	bool operator== (const uint32_nbo& rhs) const
	{
		return (this->vhh == rhs.vhh)
			&& (this->vhl == rhs.vhl)
			&& (this->vlh == rhs.vlh)
			&& (this->vll == rhs.vll);
	}
};

// ============================================================================

// Eight-byte BridgeId structure as defined in the STP standard, not aligned in memory.
struct BRIDGE_ID
{
private:
	uint16_nbo         _priorityAndMstid;
	STP_BRIDGE_ADDRESS _address;

public:
	bool operator== (const BRIDGE_ID& rhs) const
	{
		return ((this->_priorityAndMstid == rhs._priorityAndMstid) && (this->_address == rhs._address));
	}

	bool operator != (const BRIDGE_ID& rhs) const
	{
		return !this->operator== (rhs);
	}

	void SetPriorityAndMstid (uint16_t settablePriorityComponent, uint16_t mstid)
	{
		assert ((settablePriorityComponent & 0x0FFF) == 0);

		_priorityAndMstid = settablePriorityComponent | mstid;
	}

	void SetAddress (const unsigned char address[6])
	{
		_address.bytes[0] = address[0];
		_address.bytes[1] = address[1];
		_address.bytes[2] = address[2];
		_address.bytes[3] = address[3];
		_address.bytes[4] = address[4];
		_address.bytes[5] = address[5];
	}

	uint16_t GetPriorityAndMstid() const { return _priorityAndMstid; }

	uint16_t GetPriorityWithoutMstid() const { return _priorityAndMstid & 0xF000; }

	uint16_t GetMstid() const { return _priorityAndMstid & 0x0FFF; }

	const STP_BRIDGE_ADDRESS& GetAddress() const { return _address; }
};

// ============================================================================

struct PORT_ID
{
private:
	unsigned char _high;
	unsigned char _low;
	// Valid Port Numbers are in the range 1 through 4095. Value zero in _low means that the structure contains uninitialized data.

public:
	bool IsInitialized () const { return _low != 0; }

	void Set (unsigned char priority, unsigned short portNumber);
	void Reset ();
	unsigned char GetPriority () const;
	void SetPriority (unsigned char priority);
	unsigned short GetPortNumber () const;
	unsigned short GetPortIdentifier () const;
	bool IsBetterThan (const PORT_ID& rhs) const;
};

// ============================================================================
// 13.10 and 13.11 in 802.1Q-2018
struct PRIORITY_VECTOR
{
	BRIDGE_ID	RootId;					// a) - used for CIST, zero for MSTIs
	uint32_nbo	ExternalRootPathCost;	// b) - used for CIST, zero for MSTIs
	BRIDGE_ID	RegionalRootId;			// c)
	uint32_nbo	InternalRootPathCost;	// d)
	BRIDGE_ID	DesignatedBridgeId;		// e)
	PORT_ID		DesignatedPortId;		// f)

	bool operator== (const PRIORITY_VECTOR& rhs) const
	{
		return memcmp (this, &rhs, sizeof (*this)) == 0;
	}

	bool operator!= (const PRIORITY_VECTOR& rhs) const
	{
		return memcmp (this, &rhs, sizeof (*this)) != 0;
	}

	bool IsBetterThan (const PRIORITY_VECTOR& rhs) const
	{
		return memcmp (this, &rhs, sizeof (*this)) < 0;
	}

	bool IsBetterThanOrSameAs (const PRIORITY_VECTOR& rhs) const
	{
		return memcmp (this, &rhs, sizeof (*this)) <= 0;
	}

	bool IsWorseThan (const PRIORITY_VECTOR& rhs) const
	{
		return memcmp (this, &rhs, sizeof (*this)) > 0;
	}

	bool IsWorseThanOrSameAs (const PRIORITY_VECTOR& rhs) const
	{
		return memcmp (this, &rhs, sizeof (*this)) >= 0;
	}

	bool IsNotBetterThan (const PRIORITY_VECTOR& rhs) const
	{
		return this->IsWorseThanOrSameAs (rhs);
	}

	// 13.10, page 486 in 802.1Q-2018:
	// A received CIST message priority vector is superior to the port priority vector if, and only if, the message
	// priority vector is better than the port priority vector, or the Designated Bridge Identifier Bridge Address and
	// Designated Port Identifier Port Number components are the same; in which case, the message has been
	// transmitted from the same Designated Port as a previously received superior message
	//
	// 13.11, page 488 in 802.1Q-2018: wording is identical for MSTI priority vectors.
	bool IsSuperiorTo (const PRIORITY_VECTOR& rhs) const
	{
		if (this->IsBetterThan (rhs))
			return true;

		if ((this->DesignatedBridgeId.GetAddress () == rhs.DesignatedBridgeId.GetAddress ())
			&& (this->DesignatedPortId.GetPortNumber () == rhs.DesignatedPortId.GetPortNumber ()))
		{
			return true;
		}

		return false;
	}
};

// ============================================================================

struct TIMES
{
	unsigned short ForwardDelay;
	unsigned short HelloTime;
	unsigned short MaxAge;
	unsigned short MessageAge;
	unsigned char remainingHops;

	bool operator== (const TIMES& other) const;
	bool operator!= (const TIMES& other) const;
};

// ============================================================================

const char* GetPortRoleName (STP_PORT_ROLE role);

enum INFO_IS
{
	INFO_IS_DISABLED,
	INFO_IS_MINE,
	INFO_IS_AGED,
	INFO_IS_RECEIVED,
};

enum RCVD_INFO
{
	RCVD_INFO_UNKNOWN,
	RCVD_INFO_SUPERIOR_DESIGNATED,
	RCVD_INFO_REPEATED_DESIGNATED,
	RCVD_INFO_INFERIOR_DESIGNATED,
	RCVD_INFO_INFERIOR_ROOT_ALTERNATE,
	RCVD_INFO_OTHER,
};

// ============================================================================

#endif
