#ifndef __PushdownAutomaton_h__
#define __PushdownAutomaton_h__
#include <deque>
#include <vector>

using namespace std;

namespace sbmt {

template<class INPUT_ALPHABET, class STACK_ALPHABET>
class PushdownAutomaton {
public:
    PushdownAutomaton(const deque<INPUT_ALPHABET>& input);
    PushdownAutomaton();
    virtual ~PushdownAutomaton() {}
    void setInput(const deque<INPUT_ALPHABET>& input);
protected:

    deque<INPUT_ALPHABET>  m_input;
    //! Use vector for efficiency purpose.
    vector<STACK_ALPHABET>       m_stack;
    // the input posit of the elments in the above m_stacks.
    vector<int>       m_positStack;
    //! The top of the stack. m_top == 0 means the stack is empty.
    //! the top element is accessed via m_stack[m_top -1 ].
    int m_top; 

    virtual void transit() = 0;
    virtual void reset();
    size_t stackSize() const;
    const STACK_ALPHABET& stackTop() const ;
    void dequeue();
    void enqueue(const STACK_ALPHABET& elem);
    void printStack() const;
};


} 

#include "sbmt/dependency_lm/PushdownAutomaton.cc"


#endif
