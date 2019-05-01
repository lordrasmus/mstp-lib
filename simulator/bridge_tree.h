
#pragma once
#include "object.h"
#include "win32/com_ptr.h"

class bridge;

using edge::object;
using edge::uint32_p;
using edge::temp_string_p;
using edge::property_change_args;

extern const edge::NVP bridge_priority_nvps[];
extern const char bridge_priority_type_name[];
using bridge_priority_p = edge::enum_property<uint32_t, bridge_priority_type_name, bridge_priority_nvps, true>;

class bridge_tree : public object
{
	using base = object;

	bridge* const _parent;
	size_t const _tree_index;

	SYSTEMTIME _last_topology_change;
	uint32_t _topology_change_count;

	friend class bridge;

	void on_topology_change (unsigned int timestamp);
	static void on_bridge_property_changing (void* arg, object* obj, const property_change_args& args);
	static void on_bridge_property_changed (void* arg, object* obj, const property_change_args& args);

public:
	bridge_tree (bridge* parent, size_t tree_index);

	uint32_t bridge_priority() const;
	void set_bridge_priority (uint32_t priority);

	std::array<unsigned char, 36> root_priorty_vector() const;
	std::string root_bridge_id() const;
	std::string GetExternalRootPathCost() const;
	std::string GetRegionalRootBridgeId() const;
	std::string GetInternalRootPathCost() const;
	std::string GetDesignatedBridgeId() const;
	std::string GetDesignatedPortId() const;
	std::string GetReceivingPortId() const;

	uint32_t topology_change_count() const { return _topology_change_count; }

	uint32_t hello_time() const;
	uint32_t max_age() const;
	uint32_t bridge_forward_delay() const;
	uint32_t message_age() const;
	uint32_t remaining_hops() const;

	static const bridge_priority_p bridge_priority_property;
	static const temp_string_p root_bridge_id_property;
	static const uint32_p      topology_change_count_property;
	static const uint32_p      hello_time_property;
	static const uint32_p      max_age_property;
	static const uint32_p      forward_delay_property;
	static const uint32_p      message_age_property;
	static const uint32_p      remaining_hops_property;
	static const edge::property* const _properties[];
	static const edge::xtype<bridge_tree> _type;
	const edge::type* type() const override;
};

