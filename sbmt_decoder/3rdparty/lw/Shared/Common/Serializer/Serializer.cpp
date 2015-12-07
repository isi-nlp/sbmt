// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include <Common/stldefs.h>
#include "Serializer.h"

#include <assert.h>
#ifndef assert
#define assert(x)
#endif

#include "Common/ErrorCode.h"

using namespace std;


namespace LW {

template<class T>
inline void rawRead(istream* pStream, T& pVal) {
	pStream->read((char*) &pVal, sizeof(T));

	if (pStream->gcount() != sizeof(T)) {
		char buffer[100];
		sprintf_s(buffer, 100, "EOF found while reading value at offset %d", (int) pStream->tellg());
        SerializerException e;
        e.set(ERR_IO, buffer);
		throw e;
	}

//	cerr << pStream->tellg() << endl;
}

template<class T>
inline void rawWrite(ostream* pStream, T& pVal) {
	pStream->write((char*) &pVal, sizeof(T));
}


ISerializer::ISerializer(istream* pIn) {
	assert(pIn);

	m_pIn = pIn;
}

bool ISerializer::eof()
{
	if (m_pIn->eof()) {
		return true;
	}

	m_pIn->get();
	bool bEof = m_pIn->eof();
	if (!bEof) {
		m_pIn->unget();
	}

	return bEof;
}

void ISerializer::read(bool& bValue)
{
	LW::rawRead<bool>(m_pIn, bValue);
}

void ISerializer::read(int& pnValue) {
	LW::rawRead<int>(m_pIn, pnValue);
}

void ISerializer::read(unsigned int& pnValue) {
	LW::rawRead<unsigned int>(m_pIn, pnValue);
}


void ISerializer::read(float& pfValue) {
	LW::rawRead<float>(m_pIn, pfValue);
}

void ISerializer::read(double& pdValue) {
	LW::rawRead<double>(m_pIn, pdValue);
}

void ISerializer::read(std::string& psValue) {
	psValue = "";

	char buffer[4096];
	unsigned int nSize = 0;
	// Read the size of the string first
	read(nSize);

	unsigned int nRemaining = nSize;

	while(nRemaining > 0) {
		unsigned int nReadCount;
		if (nRemaining > sizeof(buffer)) {
			nReadCount = sizeof(buffer);
		}
		else {
			nReadCount = nRemaining;
		}

		m_pIn->read(buffer, nReadCount);
		if ((unsigned)m_pIn->gcount() != nReadCount) {
            SerializerException e;
            e.set(ERR_IO, "EOF found while reading string value");
			throw e;
		}

		psValue.append(buffer, nReadCount);

		nRemaining -= nReadCount;
	}
}

void ISerializer::read(char* sValue, size_t nSize)
{
    m_pIn->read(sValue, static_cast<std::streamsize>(nSize));
    if ((size_t)m_pIn->gcount() != nSize) {
        SerializerException e;
        e.set(ERR_IO, "EOF found while reading string value");
		throw e;
	}
}


void ISerializer::readMagicNumber(unsigned int nExpectedMagicNumber) 
{
	unsigned int nMagicNumber;
	read(nMagicNumber);
	if (nMagicNumber != nExpectedMagicNumber) {
		char szError[100];
		sprintf_s(szError, 100,
				"Failed to verify magic number. Expected: %d; Found: %d; Stream offset: %d", 
				nExpectedMagicNumber,
				nMagicNumber,
				(int) m_pIn->tellg());
        SerializerException e;
        e.set(ERR_IO, szError);
		throw e;
	}
}

OSerializer::OSerializer(ostream* pOut) {
	assert(pOut);

	m_pOut = pOut;
}

void OSerializer::write(bool bValue)
{
	LW::rawWrite<bool>(m_pOut, bValue);
}

void OSerializer::write(int nValue) {
	LW::rawWrite<int>(m_pOut, nValue);
}

void OSerializer::write(unsigned int nValue) {
	LW::rawWrite<unsigned int>(m_pOut, nValue);
}


void OSerializer::write(float fValue) {
	LW::rawWrite<float>(m_pOut, fValue);
}

void OSerializer::write(double dValue) {
	LW::rawWrite<double>(m_pOut, dValue);
}

void OSerializer::write(const std::string& sValue) {
	// Write the size
	unsigned int nSize = static_cast<unsigned int>(sValue.length());

	write(nSize);
	m_pOut->write(sValue.c_str(), nSize);
}

void OSerializer::write(const char* sValue, size_t nSize)
{
    m_pOut->write(sValue, static_cast<std::streamsize>(nSize));
}

void OSerializer::writeMagicNumber(unsigned int nMagicNumber)
{
	write(nMagicNumber);
}


IOSerializer::IOSerializer(std::istream* pIn)
:ISerializer(pIn), OSerializer(NULL)
{
}

IOSerializer::IOSerializer(std::ostream* pOut)
:ISerializer(NULL), OSerializer(pOut)
{
}

bool IOSerializer::isReading() 
{
	return (m_pIn != NULL);
}

void IOSerializer::exchange(int& nValue)
{
	if (m_pIn) {
		read(nValue);
	}
	else {
		assert(m_pOut);
		write(nValue);
	}
}

void IOSerializer::exchange(unsigned int& nValue)
{
	if (m_pIn) {
		read(nValue);
	}
	else {
		assert(m_pOut);
		write(nValue);
	}
}

void IOSerializer::exchange(float& fValue)
{
	if (m_pIn) {
		read(fValue);
	}
	else {
		assert(m_pOut);
		write(fValue);
	}
}

void IOSerializer::exchange(double& dValue)
{
	if (m_pIn) {
		read(dValue);
	}
	else {
		assert(m_pOut);
		write(dValue);
	}
}

void IOSerializer::exchange(std::string& sValue)
{
	if (m_pIn) {
		read(sValue);
	}
	else {
		assert(m_pOut);
		write(sValue);
	}
}

void IOSerializer::exchange(char* sValue, size_t nSize)
{
	if (m_pIn) {
		read(sValue, nSize);
	}
	else {
		assert(m_pOut);
		write(sValue, nSize);
	}
}


void IOSerializer::exchangeMagicNumber(unsigned int& nMagicNumber)
{
	if (m_pIn){
		readMagicNumber(nMagicNumber);
	}
	else {
		assert(m_pOut);
		writeMagicNumber(nMagicNumber);
	}
}

} // namespace LW

