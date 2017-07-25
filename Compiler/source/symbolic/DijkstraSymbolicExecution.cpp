#include "DijkstraSymbolicExecution.h"

using namespace std;

class SymbolicVarSet::TaggedStackUnion
{
    public:
        enum StackFrameType{STATE, VAR};

        TaggedStackUnion(string inState)
        {
            stateName = new char[inState.length() + 1];
            inState.copy(stateName, inState.length());
            stateName[inState.length()] = '\0';
            type = StackFrameType::STATE;
        }

        TaggedStackUnion(DoubleVarPointer sv)
        {
            symVar = sv;
            type = StackFrameType::VAR;
        }

        ~TaggedStackUnion()
        {
            if (type == StackFrameType::STATE) delete stateName;
        }

        StackFrameType getType() const
        {
            return type;
        }

        char *getStateName() const
        {
            return stateName;
        }

        const std::shared_ptr<SymbolicDouble> getSymVar() const
        {
            return symVar;
        }

private:
    StackFrameType type;
    union
    {
        char* stateName;
        DoubleVarPointer symVar;
    };
};

DoubleVarPointer SymbolicVarSet::findVar(std::string name)
{
    unordered_map<string, DoubleVarPointer>::const_iterator it = variables.find(name);
    if (it != variables.cend()) return it->second;

    else //must copy symbolic variable into this 'scope'
    {
        DoubleVarPointer oldSym = parent->findVar(name);
        DoubleVarPointer newSymPointer(new SymbolicDouble(*oldSym));
        variables["name"] = newSymPointer;
        return newSymPointer;
    }
}