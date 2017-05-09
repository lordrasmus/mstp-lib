
#include "pch.h"
#include "Simulator.h"

using namespace std;

class EditActionList : public IEditActionList
{
	vector<function<void(UndoOrRedo which)>> _actions;
	size_t _savePointIndex = 0;
	size_t _editPointIndex = 0;

	virtual void AddAndPerformEditAction (function<void(UndoOrRedo which)>&& action) override final
	{
		throw not_implemented_exception();
	}
};

const EditActionListFactory editActionListFactory = [](auto... params) { return (IEditActionList*) new EditActionList(params...); };
