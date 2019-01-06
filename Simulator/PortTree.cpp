
#include "pch.h"
#include "PortTree.h"
#include "Port.h"
#include "Bridge.h"
#include "stp.h"

using namespace edge;

static const _bstr_t PortTreeString = "PortTree";
static const _bstr_t TreeIndexString = "TreeIndex";
static const _bstr_t PortPriorityString = "PortPriority";

HRESULT PortTree::Serialize (IXMLDOMDocument3* doc, com_ptr<IXMLDOMElement>& elementOut) const
{
	com_ptr<IXMLDOMElement> portTreeElement;
	auto hr = doc->createElement (PortTreeString, &portTreeElement); assert(SUCCEEDED(hr));
	portTreeElement->setAttribute (TreeIndexString, _variant_t(_treeIndex));
	portTreeElement->setAttribute (PortPriorityString, _variant_t(GetPriority()));
	elementOut = std::move(portTreeElement);
	return S_OK;
}

HRESULT PortTree::Deserialize (IXMLDOMElement* portTreeElement)
{
	_variant_t value;
	auto hr = portTreeElement->getAttribute (PortPriorityString, &value);
	if (FAILED(hr))
		return hr;
	if (value.vt == VT_BSTR)
		SetPriority (wcstol (value.bstrVal, nullptr, 10));

	return S_OK;
}

uint32_t PortTree::GetPriority() const
{
	return STP_GetPortPriority (_port->GetBridge()->GetStpBridge(), _port->GetPortIndex(), _treeIndex);
}

void PortTree::SetPriority (uint32_t priority)
{
	if (STP_GetPortPriority (_port->GetBridge()->GetStpBridge(), _port->GetPortIndex(), _treeIndex) != priority)
	{
		assert(false);
		/*
		this->on_property_changing(&Priority);
		STP_SetPortPriority (_port->GetBridge()->GetStpBridge(), _port->GetPortIndex(), _treeIndex, (unsigned char) priority, GetMessageTime());
		this->on_property_changed(&Priority);
		*/
	}
}

const edge::NVP port_priority_nvps[] {
	{ "10 (16 dec)",  0x10 },
	{ "20 (32 dec)",  0x20 },
	{ "30 (48 dec)",  0x30 },
	{ "40 (64 dec)",  0x40 },
	{ "50 (80 dec)",  0x50 },
	{ "60 (96 dec)",  0x60 },
	{ "70 (112 dec)", 0x70 },
	{ "80 (128 dec)", 0x80 },
	{ "90 (144 dec)", 0x90 },
	{ "A0 (160 dec)", 0xA0 },
	{ "B0 (176 dec)", 0xB0 },
	{ "C0 (192 dec)", 0xC0 },
	{ "D0 (208 dec)", 0xD0 },
	{ "E0 (224 dec)", 0xE0 },
	{ "F0 (240 dec)", 0xF0 },
	{ nullptr, 0 },
};

const char port_priority_type_name[] = "PortPriority";

const port_priority_property PortTree::Priority
(
	"PortPriority",
	static_cast<port_priority_property::getter_t>(&GetPriority),
	static_cast<port_priority_property::setter_t>(&SetPriority),
	0x80
);

const edge::property* const PortTree::_properties[] =
{
	&Priority,
};

const edge::type_t PortTree::_type = { "PortTree", &base::_type, _properties };
