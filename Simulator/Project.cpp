
#include "pch.h"
#include "Simulator.h"
#include "Wire.h"

using namespace std;

class Project : public IProject
{
	ULONG _refCount = 1;
	vector<ComPtr<Object>> _objects;
	EventManager _em;
	std::array<uint8_t, 6> _nextMacAddress = { 0x00, 0xAA, 0x55, 0xAA, 0x55, 0x80 };

public:
	virtual const vector<ComPtr<Object>>& GetObjects() const override final { return _objects; }

	virtual void Insert (size_t index, Object* object) override final
	{
		if (index > _objects.size())
			throw invalid_argument("index");

		_objects.push_back (ComPtr<Object>(object));
		object->GetInvalidateEvent().AddHandler (&OnObjectInvalidate, this);
		ObjectInsertedEvent::InvokeHandlers (_em, this, index, object);
		ProjectInvalidateEvent::InvokeHandlers (_em, this);
	}

	virtual void Remove(size_t index) override final
	{
		if (index >= _objects.size())
			throw invalid_argument("index");

		ObjectRemovingEvent::InvokeHandlers(_em, this, index, _objects[index]);
		_objects[index]->GetInvalidateEvent().RemoveHandler(&OnObjectInvalidate, this);
		_objects.erase (_objects.begin() + index);
		ProjectInvalidateEvent::InvokeHandlers(_em, this);
	}

	static void OnObjectInvalidate (void* callbackArg, Object* object)
	{
		auto project = static_cast<Project*>(callbackArg);
		ProjectInvalidateEvent::InvokeHandlers (project->_em, project);
	}

	virtual ObjectInsertedEvent::Subscriber GetObjectInsertedEvent() override final { return ObjectInsertedEvent::Subscriber(_em); }
	virtual ObjectRemovingEvent::Subscriber GetObjectRemovingEvent() override final { return ObjectRemovingEvent::Subscriber(_em); }
	virtual ProjectInvalidateEvent::Subscriber GetProjectInvalidateEvent() override final { return ProjectInvalidateEvent::Subscriber(_em); }

	virtual array<uint8_t, 6> AllocMacAddressRange (size_t count) override final
	{
		if (count >= 128)
			throw range_error("count must be lower than 128.");

		auto result = _nextMacAddress;
		_nextMacAddress[5] += (uint8_t)count;
		if (_nextMacAddress[5] < count)
		{
			_nextMacAddress[4]++;
			if (_nextMacAddress[4] == 0)
				throw not_implemented_exception();
		}

		return result;
	}

	virtual pair<Wire*, size_t> GetWireConnectedToPort (const Port* port) const override final
	{
		for (auto& o : _objects)
		{
			if (auto w = dynamic_cast<Wire*>(o.Get()))
			{
				if (holds_alternative<ConnectedWireEnd>(w->GetP0()) && (get<ConnectedWireEnd>(w->GetP0()) == port))
					return { w, 0 };
				else if (holds_alternative<ConnectedWireEnd>(w->GetP1()) && (get<ConnectedWireEnd>(w->GetP1()) == port))
					return { w, 1 };
			}
		}

		return { };
	}

	#pragma region IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override final { return E_NOTIMPL; }

	virtual ULONG STDMETHODCALLTYPE AddRef() override final
	{
		return InterlockedIncrement(&_refCount);
	}

	virtual ULONG STDMETHODCALLTYPE Release() override final
	{
		auto newRefCount = InterlockedDecrement (&_refCount);
		if (newRefCount == 0)
			delete this;
		return newRefCount;
	}
	#pragma endregion
};

extern const ProjectFactory projectFactory = [] { return ComPtr<IProject>(new Project(), false); };
