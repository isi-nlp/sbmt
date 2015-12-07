#ifndef __PushdownAutomaton_cc__
#define __PushdownAutomaton_cc__
#include <iostream>
#include <deque>
using namespace std;

namespace sbmt {


template<class INPUT_ALPHABET, class STACK_ALPHABET>
PushdownAutomaton<INPUT_ALPHABET,STACK_ALPHABET>::PushdownAutomaton(const deque<INPUT_ALPHABET>& input)
    : m_input(input), m_top(0)
{
    m_input.clear();
    for(size_t i = 0; i < input.size(); ++i){
        m_input.push_back(input[i]);
    }
}
template<class INPUT_ALPHABET, class STACK_ALPHABET>
void PushdownAutomaton<INPUT_ALPHABET,STACK_ALPHABET>::setInput(const deque<INPUT_ALPHABET>& input)
{
    m_input.clear();
    for(size_t i = 0; i < input.size(); ++i){
        m_input.push_back(input[i]);
    }
}

template<class INPUT_ALPHABET, class STACK_ALPHABET>
PushdownAutomaton<INPUT_ALPHABET,STACK_ALPHABET>::PushdownAutomaton()
    : m_top(0)
{
    m_input.clear();
}

template<class INPUT_ALPHABET, class STACK_ALPHABET>
void PushdownAutomaton<INPUT_ALPHABET, STACK_ALPHABET>::reset()
{
}

template<class INPUT_ALPHABET, class STACK_ALPHABET>
void PushdownAutomaton<INPUT_ALPHABET, STACK_ALPHABET>::enqueue(const STACK_ALPHABET& elem) 
{
    m_stack[m_top] = elem;
    ++m_top;
}


template<class INPUT_ALPHABET, class STACK_ALPHABET>
void PushdownAutomaton<INPUT_ALPHABET, STACK_ALPHABET>::dequeue() 
{
    --m_top;
}

template<class INPUT_ALPHABET, class STACK_ALPHABET>
const STACK_ALPHABET& PushdownAutomaton<INPUT_ALPHABET, STACK_ALPHABET>::stackTop() const 
{
    return m_stack[m_top -1 ];
}

template<class INPUT_ALPHABET, class STACK_ALPHABET>
size_t PushdownAutomaton<INPUT_ALPHABET, STACK_ALPHABET>::stackSize()  const {
    return m_top;
}

template<class INPUT_ALPHABET, class STACK_ALPHABET>
void PushdownAutomaton<INPUT_ALPHABET, STACK_ALPHABET>::printStack() const
{
    cout<<">>>STACK\n";
    for(int i = 0; i < m_top; ++i){
        cout<<m_stack[i]<<" ";
    } 
    cout<<"<<STACK\n";
}

}
#endif
