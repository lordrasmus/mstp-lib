
#pragma once
#include "object.h"
#include "win32/com_ptr.h"

class Bridge;

extern const edge::NVP bridge_priority_nvps[];
extern const char bridge_priority_type_name[];
using bridge_priority_property = edge::enum_property<uint32_t, bridge_priority_type_name, bridge_priority_nvps>;

struct BridgeTree : edge::object
{
	using base = edge::object;

	Bridge* const _parent;
	unsigned int const _treeIndex;

	BridgeTree (Bridge* parent, unsigned int treeIndex)
		: _parent(parent), _treeIndex(treeIndex)
	{ }

	HRESULT Serialize (IXMLDOMDocument3* doc, edge::com_ptr<IXMLDOMElement>& elementOut) const;
	HRESULT Deserialize (IXMLDOMElement* bridgeTreeElement);

	//std::wstring GetPriorityLabel () const;
	uint32_t GetPriority() const;
	void SetPriority (uint32_t priority);

	std::array<unsigned char, 36> GetRootPV() const;
	std::string GetRootBridgeId() const;
	std::string GetExternalRootPathCost() const;
	std::string GetRegionalRootBridgeId() const;
	std::string GetInternalRootPathCost() const;
	std::string GetDesignatedBridgeId() const;
	std::string GetDesignatedPortId() const;
	std::string GetReceivingPortId() const;

	static const bridge_priority_property Priority;
	static const edge::temp_string_property RootBridgeId;
	static const edge::property* const _properties[];
	static const edge::type_t _type;
	const edge::type_t* type() const override { return &_type; }
};

