
#include "pch.h"
#include "EditState.h"
#include "Bridge.h"

using namespace std;
using namespace D2D1;

class CreateBridgeES : public EditState
{
	typedef EditState base;
	bool _completed = false;
	unique_ptr<Bridge> _bridge;

public:
	using base::base;

	virtual void OnMouseMove (const MouseLocation& location) override final
	{
		if (_bridge == nullptr)
		{
			unsigned int portCount = 4;
			unsigned int mstiCount = 4;
			size_t macAddressesToReserve = std::max ((size_t) 1 + portCount, (size_t) 16);
			auto macAddress = _pw->GetProject()->AllocMacAddressRange(macAddressesToReserve);
			_bridge.reset (new Bridge (_pw->GetProject(), portCount, mstiCount, macAddress.bytes));
		}

		_bridge->SetLocation (location.w.x - _bridge->GetWidth() / 2, location.w.y - _bridge->GetHeight() / 2);
		::InvalidateRect (_pw->GetEditArea()->GetHWnd(), nullptr, FALSE);
	}

	virtual void OnMouseUp (MouseButton button, UINT modifierKeysDown, const MouseLocation& location) override final
	{
		if (_bridge != nullptr)
		{
			struct CreateAction : public EditAction
			{
				IProject* const _project;
				unique_ptr<Bridge> _bridge;
				size_t _insertIndex;
				CreateAction (IProject* project, unique_ptr<Bridge>&& bridge) : _project(project), _bridge(move(bridge)) { }
				virtual void Redo() override final { _insertIndex = _project->GetBridges().size(); _project->InsertBridge(_insertIndex, move(_bridge), nullptr); }
				virtual void Undo() override final { _bridge = _project->RemoveBridge(_insertIndex, nullptr); }
				virtual std::string GetName() const override final { return "Create Bridge"; }
			};

			Bridge* b = _bridge.get();
			_actionList->PerformAndAddUserAction (make_unique<CreateAction>(_pw->GetProject(), move(_bridge)));
			_selection->Select(b);
			STP_StartBridge (b->GetStpBridge(), GetTimestampMilliseconds());
		}

		_completed = true;
	}

	virtual std::optional<LRESULT> OnKeyDown (UINT virtualKey, UINT modifierKeys) override final
	{
		if (virtualKey == VK_ESCAPE)
		{
			_completed = true;
			::InvalidateRect (_pw->GetEditArea()->GetHWnd(), nullptr, FALSE);
			return 0;
		}

		return nullopt;
	}

	virtual void Render (ID2D1RenderTarget* rt) override final
	{
		if (_bridge != nullptr)
		{
			D2D1_MATRIX_3X2_F oldtr;
			rt->GetTransform(&oldtr);
			rt->SetTransform(_pw->GetEditArea()->GetZoomTransform());
			_bridge->Render (rt, _pw->GetEditArea()->GetDrawingObjects(), _pw->GetSelectedVlanNumber(), ColorF(ColorF::LightGreen));
			rt->SetTransform(&oldtr);
		}
	}

	virtual bool Completed() const override final { return _completed; }
};

unique_ptr<EditState> CreateStateCreateBridge (const EditStateDeps& deps) { return unique_ptr<EditState>(new CreateBridgeES(deps)); }
