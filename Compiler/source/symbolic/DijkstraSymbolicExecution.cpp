#include "DijkstraSymbolicExecution.h"

using namespace std;

class SymbolicVarSet::TaggedStackUnion
{
    private:
        StackFrameType type;
        union
        {
            char* stateName;
            SymbolicVarPointer symVar;
        };

    public:
        enum StackFrameType{STATE, VAR};

        TaggedStackUnion(string inState)
        {
            stateName = new char[inState.length() + 1];
            inState.copy(stateName, inState.length());
            stateName[inState.length()] = '\0';
            type = StackFrameType::STATE;
        }

        TaggedStackUnion(SymbolicVarPointer sv)
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

        const std::shared_ptr<SymbolicVariable> &getSymVar() const
        {
            return symVar;
        }
};

SymbolicVarPointer SymbolicVarSet::findVar(std::string name)
{
    unordered_map<string, SymbolicVarPointer>::const_iterator it = variables.find(name);
    if (it != variables.cend()) return it->second;

    else //must copy symbolic variable into this 'scope'
    {
        SymbolicVarPointer oldSym = parent->findVar(name);
        SymbolicVariable newSym(oldSym);

    }
}