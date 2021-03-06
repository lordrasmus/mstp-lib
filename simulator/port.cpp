
// This file is part of the mstp-lib library, available at https://github.com/adigostin/mstp-lib
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#include "pch.h"
#include "port.h"
#include "bridge.h"
#include "win32/utility_functions.h"
#include "win32/d2d_window.h"
#include "win32/text_layout.h"

using namespace D2D1;
using namespace edge;

const char admin_p2p_type_name[] = "admin_p2p";
const nvp admin_p2p_nvps[] =
{
	{ "ForceTrue", (int)STP_ADMIN_P2P_FORCE_TRUE },
	{ "ForceFalse", (int)STP_ADMIN_P2P_FORCE_FALSE },
	{ "Auto", (int)STP_ADMIN_P2P_AUTO },
	{ 0, 0 },
};

const char port_speed_type_name[] = "port_speed";
const char port_speed_unknown_str[] = "(none)";
const nvp port_speed_nvps[] = {
	{  "10M",    10 },
	{ "100M",   100 },
	{   "1G",  1000 },
	{  "10G", 10000 },
	{ 0, 0 },
};

port::port (class bridge* bridge, size_t port_index, edge::side side, float offset)
	: _bridge(bridge), _port_index(port_index), _side(side), _offset(offset)
{
	// No need to call remove_handler since a bridge and its bridge_trees are deleted at the same time.
	_bridge->property_changing().add_handler(&on_bridge_property_changing, this);
	_bridge->property_changed().add_handler(&on_bridge_property_changed, this);

	for (size_t treeIndex = 0; treeIndex < bridge->trees().size(); treeIndex++)
	{
		auto tree = std::unique_ptr<port_tree>(new port_tree(this, treeIndex));
		_trees.push_back (std::move(tree));
	}
}

void port::on_bridge_property_changing (void* arg, object* obj, const property_change_args& args)
{
	auto bt = static_cast<port*>(arg);
	if (args.property == &bridge::stp_enabled_property)
	{
		// Currently no port property needs to change when STP is enabled/disabled.
	}
}

void port::on_bridge_property_changed (void* arg, object* obj, const property_change_args& args)
{
	auto bt = static_cast<port*>(arg);
	if (args.property == &bridge::stp_enabled_property)
	{
		// Currently no port property needs to change when STP is enabled/disabled.
	}
}

D2D1_POINT_2F port::GetCPLocation() const
{
	if (_side == side::left)
		return { _bridge->left() - ExteriorHeight, _bridge->top() + _offset };

	if (_side == side::right)
		return { _bridge->right() + ExteriorHeight, _bridge->top() + _offset };

	if (_side == side::top)
		return { _bridge->left() + _offset, _bridge->top() - ExteriorHeight };

	// _side == side::bottom
	return { _bridge->left() + _offset, _bridge->bottom() + ExteriorHeight };
}

Matrix3x2F port::GetPortTransform() const
{
	if (_side == side::left)
	{
		//portTransform = Matrix3x2F::Rotation (90, Point2F (0, 0)) * Matrix3x2F::Translation (bridgeRect.left, bridgeRect.top + port->GetOffset ());
		// The above calculation is correct but slow. Let's assign the matrix members directly.
		return { 0, 1, -1, 0, _bridge->left(), _bridge->top() + _offset};
	}
	else if (_side == side::right)
	{
		//portTransform = Matrix3x2F::Rotation (270, Point2F (0, 0)) * Matrix3x2F::Translation (bridgeRect.right, bridgeRect.top + port->GetOffset ());
		return { 0, -1, 1, 0, _bridge->right(), _bridge->top() + _offset };
	}
	else if (_side == side::top)
	{
		//portTransform = Matrix3x2F::Rotation (180, Point2F (0, 0)) * Matrix3x2F::Translation (bridgeRect.left + port->GetOffset (), bridgeRect.top);
		return { -1, 0, 0, -1, _bridge->left() + _offset, _bridge->top() };
	}
	else //if (_side == side::bottom)
	{
		//portTransform = Matrix3x2F::Translation (bridgeRect.left + port->GetOffset (), bridgeRect.bottom);
		return { 1, 0, 0, 1, _bridge->left() + _offset, _bridge->bottom() };
	}
}

// static
void port::RenderExteriorNonStpPort (ID2D1RenderTarget* dc, const drawing_resources& dos, bool macOperational)
{
	auto& brush = macOperational ? dos._brushForwarding : dos._brushDiscardingPort;
	dc->DrawLine (Point2F (0, 0), Point2F (0, ExteriorHeight), brush, 2);
}

// static
void port::RenderExteriorStpPort (ID2D1RenderTarget* dc, const drawing_resources& dos, STP_PORT_ROLE role, bool learning, bool forwarding, bool operEdge)
{
	static constexpr float circleDiameter = std::min (ExteriorHeight / 2, ExteriorWidth);

	static constexpr float edw = ExteriorWidth;
	static constexpr float edh = ExteriorHeight;

	static constexpr float discardingFirstHorizontalLineY = circleDiameter + (edh - circleDiameter) / 3;
	static constexpr float discardingSecondHorizontalLineY = circleDiameter + (edh - circleDiameter) * 2 / 3;
	static constexpr float learningHorizontalLineY = circleDiameter + (edh - circleDiameter) / 2;

	static constexpr float dfhly = discardingFirstHorizontalLineY;
	static constexpr float dshly = discardingSecondHorizontalLineY;

	static const D2D1_ELLIPSE ellipseFill = { Point2F (0, circleDiameter / 2), circleDiameter / 2 + 0.5f, circleDiameter / 2 + 0.5f };
	static const D2D1_ELLIPSE ellipseDraw = { Point2F (0, circleDiameter / 2), circleDiameter / 2 - 0.5f, circleDiameter / 2 - 0.5f};

	auto oldaa = dc->GetAntialiasMode();

	if (role == STP_PORT_ROLE_DISABLED)
	{
		// disabled
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		dc->DrawLine (Point2F (0, 0), Point2F (0, edh), dos._brushDiscardingPort, 2);
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		dc->DrawLine (Point2F (-edw / 2, edh / 3), Point2F (edw / 2, edh * 2 / 3), dos._brushDiscardingPort);
	}
	else if ((role == STP_PORT_ROLE_DESIGNATED) && !learning && !forwarding)
	{
		// designated discarding
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		dc->FillEllipse (&ellipseFill, dos._brushDiscardingPort);
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		dc->DrawLine (Point2F (0, circleDiameter), Point2F (0, edh), dos._brushDiscardingPort, 2);
		dc->DrawLine (Point2F (-edw / 2, dfhly), Point2F (edw / 2, dfhly), dos._brushDiscardingPort);
		dc->DrawLine (Point2F (-edw / 2, dshly), Point2F (edw / 2, dshly), dos._brushDiscardingPort);
	}
	else if ((role == STP_PORT_ROLE_DESIGNATED) && learning && !forwarding)
	{
		// designated learning
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		dc->FillEllipse (&ellipseFill, dos._brushLearningPort);
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		dc->DrawLine (Point2F (0, circleDiameter), Point2F (0, edh), dos._brushLearningPort, 2);
		dc->DrawLine (Point2F (-edw / 2, learningHorizontalLineY), Point2F (edw / 2, learningHorizontalLineY), dos._brushLearningPort);
	}
	else if ((role == STP_PORT_ROLE_DESIGNATED) && learning && forwarding && !operEdge)
	{
		// designated forwarding
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		dc->FillEllipse (&ellipseFill, dos._brushForwarding);
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		dc->DrawLine (Point2F (0, circleDiameter), Point2F (0, edh), dos._brushForwarding, 2);
	}
	else if ((role == STP_PORT_ROLE_DESIGNATED) && learning && forwarding && operEdge)
	{
		// designated forwarding operEdge
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		dc->FillEllipse (&ellipseFill, dos._brushForwarding);
		static constexpr D2D1_POINT_2F points[] =
		{
			{ 0, circleDiameter },
			{ -edw / 2 + 1, circleDiameter + (edh - circleDiameter) / 2 },
			{ 0, edh },
			{ edw / 2 - 1, circleDiameter + (edh - circleDiameter) / 2 },
		};

		dc->DrawLine (points[0], points[1], dos._brushForwarding, 2);
		dc->DrawLine (points[1], points[2], dos._brushForwarding, 2);
		dc->DrawLine (points[2], points[3], dos._brushForwarding, 2);
		dc->DrawLine (points[3], points[0], dos._brushForwarding, 2);
	}
	else if (((role == STP_PORT_ROLE_ROOT) || (role == STP_PORT_ROLE_MASTER)) && !learning && !forwarding)
	{
		// root or master discarding
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		dc->DrawEllipse (&ellipseDraw, dos._brushDiscardingPort, 2);
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		dc->DrawLine (Point2F (0, circleDiameter), Point2F (0, edh), dos._brushDiscardingPort, 2);
		dc->DrawLine (Point2F (-edw / 2, dfhly), Point2F (edw / 2, dfhly), dos._brushDiscardingPort);
		dc->DrawLine (Point2F (-edw / 2, dshly), Point2F (edw / 2, dshly), dos._brushDiscardingPort);
	}
	else if (((role == STP_PORT_ROLE_ROOT) || (role == STP_PORT_ROLE_MASTER)) && learning && !forwarding)
	{
		// root or master learning
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		dc->DrawEllipse (&ellipseDraw, dos._brushLearningPort, 2);
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		dc->DrawLine (Point2F (0, circleDiameter), Point2F (0, edh), dos._brushLearningPort, 2);
		dc->DrawLine (Point2F (-edw / 2, learningHorizontalLineY), Point2F (edw / 2, learningHorizontalLineY), dos._brushLearningPort);
	}
	else if (((role == STP_PORT_ROLE_ROOT) || (role == STP_PORT_ROLE_MASTER)) && learning && forwarding)
	{
		// root or master forwarding
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		dc->DrawEllipse (&ellipseDraw, dos._brushForwarding, 2);
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		dc->DrawLine (Point2F (0, circleDiameter), Point2F (0, edh), dos._brushForwarding, 2);
	}
	else if ((role == STP_PORT_ROLE_ALTERNATE) && !learning && !forwarding)
	{
		// Alternate discarding
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		dc->DrawLine (Point2F (0, 0), Point2F (0, edh), dos._brushDiscardingPort, 2);
		dc->DrawLine (Point2F (-edw / 2, dfhly), Point2F (edw / 2, dfhly), dos._brushDiscardingPort);
		dc->DrawLine (Point2F (-edw / 2, dshly), Point2F (edw / 2, dshly), dos._brushDiscardingPort);
	}
	else if ((role == STP_PORT_ROLE_ALTERNATE) && learning && !forwarding)
	{
		// Alternate learning
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		dc->DrawLine (Point2F (0, 0), Point2F (0, edh), dos._brushLearningPort, 2);
		dc->DrawLine (Point2F (-edw / 2, learningHorizontalLineY), Point2F (edw / 2, learningHorizontalLineY), dos._brushLearningPort);
	}
	else if ((role == STP_PORT_ROLE_BACKUP) && !learning && !forwarding)
	{
		// Backup discarding
		dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		dc->DrawLine (Point2F (0, 0), Point2F (0, edh), dos._brushDiscardingPort, 2);
		dc->DrawLine (Point2F (-edw / 2, dfhly / 2), Point2F (edw / 2, dfhly / 2), dos._brushDiscardingPort);
		dc->DrawLine (Point2F (-edw / 2, dfhly), Point2F (edw / 2, dfhly), dos._brushDiscardingPort);
		dc->DrawLine (Point2F (-edw / 2, dshly), Point2F (edw / 2, dshly), dos._brushDiscardingPort);
	}
	else
		assert(false); // not implemented

	dc->SetAntialiasMode(oldaa);
}

void port::render (ID2D1RenderTarget* rt, const drawing_resources& dos, unsigned int vlanNumber) const
{
	D2D1_MATRIX_3X2_F oldtr;
	rt->GetTransform(&oldtr);
	rt->SetTransform (GetPortTransform() * oldtr);

	// Draw the exterior of the port.
	float interiorPortOutlineWidth = OutlineWidth;
	auto b = _bridge->stp_bridge();
	auto treeIndex = STP_GetTreeIndexFromVlanNumber(b, vlanNumber);
	if (STP_IsBridgeStarted (b))
	{
		auto role       = STP_GetPortRole (b, (unsigned int)_port_index, treeIndex);
		auto learning   = STP_GetPortLearning (b, (unsigned int)_port_index, treeIndex);
		auto forwarding = STP_GetPortForwarding (b, (unsigned int)_port_index, treeIndex);
		auto operEdge   = STP_GetPortOperEdge (b, (unsigned int)_port_index);
		RenderExteriorStpPort (rt, dos, role, learning, forwarding, operEdge);

		if (STP_GetTxCount(b, (unsigned int)_port_index) == STP_GetTxHoldCount(b))
		{
			auto layout = edge::text_layout_with_metrics (dos._dWriteFactory, dos._smallTextFormat, "txCount=TxHoldCount");
			D2D1_MATRIX_3X2_F old;
			rt->GetTransform(&old);
			rt->SetTransform (Matrix3x2F::Rotation(-90) * old);
			rt->DrawTextLayout ({ -layout.width() - 3, 2 }, layout, dos._brushWindowText);
			rt->SetTransform(&old);
		}

		if (role == STP_PORT_ROLE_ROOT)
			interiorPortOutlineWidth *= 2;
	}
	else
		RenderExteriorNonStpPort(rt, dos, mac_operational());

	// Draw the interior of the port.
	auto portRect = D2D1_RECT_F { -InteriorWidth / 2, -InteriorDepth, InteriorWidth / 2, 0 };
	inflate (&portRect, -interiorPortOutlineWidth / 2);
	rt->FillRectangle (&portRect, mac_operational() ? dos._poweredFillBrush : dos._unpoweredBrush);
	rt->DrawRectangle (&portRect, dos._brushWindowText, interiorPortOutlineWidth);

	std::wstringstream ss;
	ss << std::setfill(L'0') << std::setw(4) << std::hex << STP_GetPortIdentifier(b, (unsigned int)_port_index, treeIndex);
	auto layout = text_layout_with_metrics (dos._dWriteFactory, dos._smallTextFormat, ss.str().c_str());
	DWRITE_LINE_METRICS lineMetrics;
	UINT32 actualLineCount;
	auto hr = layout->GetLineMetrics(&lineMetrics, 1, &actualLineCount); assert(SUCCEEDED(hr));
	rt->DrawTextLayout ({ -layout.width() / 2, -lineMetrics.baseline - OutlineWidth * 2 - 1}, layout, dos._brushWindowText);

	if (_trees[treeIndex]->fdb_flush_text_visible())
	{
		layout = text_layout_with_metrics (dos._dWriteFactory, dos._smallBoldTextFormat, L"Flush");
		hr = layout->GetLineMetrics(&lineMetrics, 1, &actualLineCount); assert(SUCCEEDED(hr));
		D2D1_POINT_2F l = { -layout.width() / 2, -InteriorDepth - lineMetrics.baseline - lineMetrics.height * 0.2f };
		rt->DrawTextLayout (l, layout, dos._brushWindowText);
	}

	rt->SetTransform (&oldtr);
}

D2D1_RECT_F port::GetInnerOuterRect() const
{
	auto tl = D2D1_POINT_2F { -InteriorWidth / 2, -InteriorDepth };
	auto br = D2D1_POINT_2F { InteriorWidth / 2, ExteriorHeight };
	auto tr = GetPortTransform();
	tl = tr.TransformPoint(tl);
	br = tr.TransformPoint(br);
	return { std::min(tl.x, br.x), std::min (tl.y, br.y), std::max(tl.x, br.x), std::max(tl.y, br.y) };
}

void port::render_selection (const edge::zoomable_i* zoomable, ID2D1RenderTarget* rt, const drawing_resources& dos) const
{
	auto ir = GetInnerOuterRect();

	auto oldaa = rt->GetAntialiasMode();
	rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

	auto lt = zoomable->pointw_to_pointd ({ ir.left, ir.top });
	auto rb = zoomable->pointw_to_pointd ({ ir.right, ir.bottom });
	rt->DrawRectangle ({ lt.x - 10, lt.y - 10, rb.x + 10, rb.y + 10 }, dos._brushHighlight, 2, dos._strokeStyleSelectionRect);

	rt->SetAntialiasMode(oldaa);
}

bool port::HitTestCP (const edge::zoomable_i* zoomable, D2D1_POINT_2F dLocation, float tolerance) const
{
	auto cpWLocation = GetCPLocation();
	auto cpDLocation = zoomable->pointw_to_pointd(cpWLocation);

	return (abs (cpDLocation.x - dLocation.x) <= tolerance)
		&& (abs (cpDLocation.y - dLocation.y) <= tolerance);
}

bool port::HitTestInnerOuter (const edge::zoomable_i* zoomable, D2D1_POINT_2F dLocation, float tolerance) const
{
	auto ir = GetInnerOuterRect();
	auto lt = zoomable->pointw_to_pointd ({ ir.left, ir.top });
	auto rb = zoomable->pointw_to_pointd ({ ir.right, ir.bottom });
	return (dLocation.x >= lt.x) && (dLocation.y >= lt.y) && (dLocation.x < rb.x) && (dLocation.y < rb.y);
}

renderable_object::ht_result port::hit_test (const edge::zoomable_i* zoomable, D2D1_POINT_2F dLocation, float tolerance)
{
	if (HitTestCP (zoomable, dLocation, tolerance))
		return { this, HTCodeCP };

	if (HitTestInnerOuter (zoomable, dLocation, tolerance))
		return { this, HTCodeInnerOuter };

	return { };
}

void port::invalidate()
{
	this->event_invoker<invalidate_e>()(this);
}

bool port::IsForwarding (unsigned int vlanNumber) const
{
	auto stpb = _bridge->stp_bridge();
	if (!STP_IsBridgeStarted(stpb))
		return true;

	auto treeIndex = STP_GetTreeIndexFromVlanNumber(stpb, vlanNumber);
	return STP_GetPortForwarding (stpb, (unsigned int)_port_index, treeIndex);
}

void port::SetSideAndOffset (edge::side side, float offset)
{
	if ((_side != side) || (_offset != offset))
	{
		_side = side;
		_offset = offset;
		event_invoker<invalidate_e>()(this);
	}
}

bool port::mac_operational() const { return _actual_speed > 0; }

bool port::auto_edge() const
{
	return STP_GetPortAutoEdge (_bridge->stp_bridge(), (unsigned int)_port_index);
}

void port::set_auto_edge (bool autoEdge)
{
	STP_SetPortAutoEdge (_bridge->stp_bridge(), (unsigned int)_port_index, autoEdge, (unsigned int) GetMessageTime());
}

bool port::admin_edge() const
{
	return STP_GetPortAdminEdge (_bridge->stp_bridge(), (unsigned int)_port_index);
}

void port::set_admin_edge (bool adminEdge)
{
	STP_SetPortAdminEdge (_bridge->stp_bridge(), (unsigned int)_port_index, adminEdge, (unsigned int) GetMessageTime());
}

unsigned int port::GetDetectedPortPathCost() const
{
	return STP_GetDetectedPortPathCost(_bridge->stp_bridge(), (unsigned int)_port_index);
}

unsigned int port::GetAdminExternalPortPathCost() const
{
	return STP_GetAdminExternalPortPathCost (_bridge->stp_bridge(), (unsigned int)_port_index);
}

void port::SetAdminExternalPortPathCost(unsigned int adminExternalPortPathCost)
{
	this->on_property_changing (&admin_external_port_path_cost_property);
	this->on_property_changing (&external_port_path_cost_property);
	STP_SetAdminExternalPortPathCost (_bridge->stp_bridge(), (unsigned int)_port_index, adminExternalPortPathCost, GetMessageTime());
	this->on_property_changed (&external_port_path_cost_property);
	this->on_property_changed (&admin_external_port_path_cost_property);
}

unsigned int port::GetExternalPortPathCost() const
{
	unsigned int treeIndex = 0; // CIST
	return STP_GetExternalPortPathCost (_bridge->stp_bridge(), (unsigned int)_port_index);
}

bool port::detected_p2p() const
{
	return STP_GetDetectedPointToPointMAC(_bridge->stp_bridge(), (unsigned int)_port_index);
}

STP_ADMIN_P2P port::admin_p2p() const
{
	return STP_GetAdminPointToPointMAC(_bridge->stp_bridge(), (unsigned int)_port_index);
}

void port::set_supported_speed (uint32_t value)
{
	if (_supported_speed != value)
	{
		this->on_property_changing (&supported_speed_property);
		_supported_speed = value;
		this->on_property_changed (&supported_speed_property);
	};
}

void port::set_actual_speed (uint32_t value)
{
	if (_actual_speed != value)
	{
		// The actual speed should either be transitioning from zero to non-zero (MAC_Operational becoming True),
		// or it should be transitioning from non-zero to zero (MAC_Operational becoming False).
		assert (_actual_speed ^ value);

		this->on_property_changing (&actual_speed_property);
		this->on_property_changing (&mac_operational_property);
		_actual_speed = value;
		this->on_property_changed (&mac_operational_property);
		this->on_property_changed (&actual_speed_property);
	}
}

void port::set_admin_p2p (STP_ADMIN_P2P admin_p2p)
{
	this->on_property_changing (&admin_p2p_property);
	this->on_property_changing (&oper_p2p_property);
	STP_SetAdminPointToPointMAC (_bridge->stp_bridge(), (unsigned int)_port_index, admin_p2p, ::GetMessageTime());
	this->on_property_changed (&oper_p2p_property);
	this->on_property_changed (&admin_p2p_property);
}

bool port::oper_p2p() const
{
	return STP_GetOperPointToPointMAC(_bridge->stp_bridge(), (unsigned int)_port_index);
}

const side_p port::side_property {
	"Side", nullptr, nullptr, ui_visible::no,
	static_cast<side_p::member_getter_t>(&side),
	static_cast<side_p::member_setter_t>(&set_side),
	side::bottom
};

const edge::float_p port::offset_property {
	"Offset", nullptr, nullptr, ui_visible::no,
	static_cast<float_p::member_getter_t>(&offset),
	static_cast<float_p::member_setter_t>(&set_offset),
};

static const edge::property_group link_group = { -1, "Link" };

const port_speed_p port::supported_speed_property {
	"SupportedSpeed", &link_group, "Maximum supported speed. The Simulator reads this at the instant the link is established (cable connected).", ui_visible::yes,
	static_cast<port_speed_p::member_getter_t>(&port::supported_speed),
	static_cast<port_speed_p::member_setter_t>(&port::set_supported_speed),
	100
};

const port_speed_p port::actual_speed_property {
	"ActualSpeed", &link_group, "Actual speed in megabits per second, calculated when a link is established as a minimum between this port's and remote port's SupportedSpeed properties, and passed to STP for port path cost calculation.", ui_visible::yes,
	static_cast<port_speed_p::member_var_t>(&port::_actual_speed),
	nullptr,
};

const bool_p port::auto_edge_property {
	"AutoEdge",
	nullptr,
	"The administrative value of the Auto Edge Port parameter. "
		"A value of true(1) indicates if the Bridge detection state machine (BDM, 13.31) "
		"is to detect other Bridges attached to the LAN, and set ieee8021SpanningTreeRstpPortOperEdgePort automatically. "
		"The default value is true(1) This is optional and provided only by implementations that support the automatic "
		"identification of edge ports. The value of this object MUST be retained across reinitializations of the management system.",
	ui_visible::yes,
	static_cast<bool_p::member_getter_t>(&auto_edge),
	static_cast<bool_p::member_setter_t>(&set_auto_edge),
	true,
};

const bool_p port::admin_edge_property {
	"AdminEdge",
	nullptr,
	"The administrative value of the Edge Port parameter. "
		"A value of true(1) indicates that this port should be assumed as an edge-port, and a value of false(2) indicates "
		"that this port should be assumed as a non-edge-port. Setting this object will also cause the corresponding instance "
		"of dot1dStpPortOperEdgePort to change to the same value. Note that even when this object's value is true, "
		"the value of the corresponding instance of dot1dStpPortOperEdgePort can be false if a BPDU has been received. "
		"The value of this object MUST be retained across reinitializations of the management system",
	ui_visible::yes,
	static_cast<bool_p::member_getter_t>(&admin_edge),
	static_cast<bool_p::member_setter_t>(&set_admin_edge),
	false,
};

const bool_p port::mac_operational_property {
	"MAC_Operational",
	&link_group,
	nullptr,
	ui_visible::yes,
	static_cast<bool_p::member_getter_t>(&mac_operational),
	nullptr,
	false,
};

static const edge::property_group port_path_cost_group = { 5, "Port Path Cost" };

const uint32_p port::detected_port_path_cost_property {
	"DetectedPortPathCost",
	&port_path_cost_group,
	"The Port Path Cost calculated from ActualSpeed, which STP uses for path cost calculation while AdminExternalPortPathCost is zero.",
	ui_visible::yes,
	static_cast<uint32_p::member_getter_t>(&GetDetectedPortPathCost),
	nullptr,
	0,
};

const uint32_p port::admin_external_port_path_cost_property {
	"AdminExternalPortPathCost",
	&port_path_cost_group,
	nullptr,
	ui_visible::yes,
	static_cast<uint32_p::member_getter_t>(&GetAdminExternalPortPathCost),
	static_cast<uint32_p::member_setter_t>(&SetAdminExternalPortPathCost),
	0,
};

const uint32_p port::external_port_path_cost_property {
	"ExternalPortPathCost",
	&port_path_cost_group,
	nullptr,
	ui_visible::yes,
	static_cast<uint32_p::member_getter_t>(&GetExternalPortPathCost),
	nullptr,
	0,
};

const bool_p port::detected_p2p_property {
	"detectedPointToPointMAC",
	&link_group,
	nullptr,
	ui_visible::yes,
	static_cast<bool_p::member_getter_t>(&detected_p2p),
	nullptr,
};

const admin_p2p_p port::admin_p2p_property {
	"adminPointToPointMAC",
	&link_group,
	nullptr,
	ui_visible::yes,
	static_cast<admin_p2p_p::member_getter_t>(&admin_p2p),
	static_cast<admin_p2p_p::member_setter_t>(&set_admin_p2p),
	STP_ADMIN_P2P_AUTO,
};

const edge::bool_p port::oper_p2p_property {
	"operPointToPointMAC",
	&link_group,
	nullptr,
	ui_visible::yes,
	static_cast<bool_p::member_getter_t>(&oper_p2p),
	nullptr,
};

const typed_object_collection_property<port, port_tree> port::trees_property {
	"PortTrees", nullptr, nullptr, ui_visible::no,
	&tree_count, &tree
};

const edge::property* const port::_properties[] =
{
	&side_property,
	&offset_property,
	&supported_speed_property,
	&actual_speed_property,
	&auto_edge_property,
	&admin_edge_property,
	&mac_operational_property,
	&detected_port_path_cost_property,
	&admin_external_port_path_cost_property,
	&external_port_path_cost_property,
	&detected_p2p_property,
	&admin_p2p_property,
	&oper_p2p_property,
	&trees_property,
};

const edge::xtype<port> port::_type = { "Port", &base::_type, _properties, nullptr };
