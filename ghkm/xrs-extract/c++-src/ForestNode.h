// Abstract node class for forest (no only trees).
#ifndef _ForestNode_H_
#define _ForestNode_H_

namespace mtrule {

//! Abstract node class for forest (no only trees).
class ForestNode {
public:
	typedef enum { UNDEF, OR, AND}  NodeType;

	ForestNode()  : m_nodeT(UNDEF) {}
	virtual ~ForestNode() {}

	//! By default, the node is undefined.
	virtual NodeType type() const { return m_nodeT;}
	//! Set the node type.
	virtual void setType(NodeType t) { m_nodeT = t;}

	ForestNode& operator=(const ForestNode& other) {
		setType(other.type());
		return *this;
	}

private:
	NodeType m_nodeT;
};

} // namespace mtrule



#endif

