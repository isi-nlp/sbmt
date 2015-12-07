// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#ifndef _HEADERIOSERIALIZER_H
#define _HEADERIOSERIALIZER_H
#include "Common/Serializer/Serializer.h"

namespace LW {
template<class CLASSTYPE>
class HeaderIOSerializer{
protected:
	HeaderIOSerializer(){};

	bool m_bHeader;
	ISerializer *m_pIS;
	OSerializer *m_pOS;

	/**stores magic number for writes and reads magic number for reads - returns false if there is a problem
	should be called at the beginning of a file - if magic number does not match then we throw.
	version is used for future compatibility
	*/
	bool header(unsigned int magic){
		if(m_pOS){
			m_pOS->writeMagicNumber(magic);
		}
		else{
			m_pIS->readMagicNumber(magic);
		}
		m_bHeader = true;
		return true;
	}
public:
	HeaderIOSerializer(ostream &os){
		m_pOS = new OSerializer(&os);
		m_pIS = NULL;
		m_bHeader = false;
	}
	HeaderIOSerializer(istream &is){
		m_pOS = NULL;
		m_pIS = new ISerializer(&is);
		m_bHeader = false;
	}

	~HeaderIOSerializer(){
		delete m_pOS;
		delete m_pIS;
	};

	/**reads or writes dependent on the stream initialized with the object*/
	void convert(CLASSTYPE &Seg){
		if(!m_bHeader){
			throw Exception(ERR_NOT_INITIALIZED, "HeaderIOSerializer::header() must be called first");
		}
		if(m_pOS) Seg.write(*m_pOS);
		else Seg.read(*m_pIS);
	}
};
}
#endif
