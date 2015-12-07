// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include "Common/ParamParser.h"
#include <cstdlib>

using namespace std;

namespace LW {

ParamParser::ParamParser()
{
}

void ParamParser::registerParam(const char* szParamName, bool bHasValue)
{
	ParamEntry paramEntry;
	paramEntry.m_bFound = false;
	paramEntry.m_bHasValue = bHasValue;
	m_namedParams.insert(ParamMap::value_type(szParamName, paramEntry));
}

void ParamParser::parse(int argc, char** argv)
{
	for (int i = 1; i < argc; i++) {
		string sParam(argv[i]);
		// Is it a named param?
		if (sParam[0] == '-') {
			string sName = sParam.substr(1);
			// Has this param been registered?
			ParamMap::iterator it = m_namedParams.find(sName);
			if (m_namedParams.end() == it) {
				// This is an invalid parameter. Just add it to the list in case the 
				// caller is interested
				m_invalidParams.push_back(sName);
			}
			// Is it supposed to have a value?
			else if (it->second.m_bHasValue) {
				if (i >= argc - 1) {
					// Do nothing here, the parameter will just be empty as it was not passed on the command line
				}
				else {
					it->second.m_sValue = argv[i + 1];
					// Skip next parameter, as it was a value
				}
				i++;
			}

			it->second.m_bFound = true;
		}
		else {
			string sValue(argv[i]);
			// Is it an unnamed param
			m_unnamedParams.push_back(sValue);
		}
	}
}

bool ParamParser::getNamedParam(const char* szParamName, string& sValue) const
{
	ParamMap::const_iterator it = m_namedParams.find(string(szParamName));
	bool bReturn = false;
	if (it != m_namedParams.end()) {
		if (it->second.m_bFound) {
			sValue = it->second.m_sValue;
			bReturn = true;
		}
	}
	return bReturn;
}

bool ParamParser::getNamedParam(const char* szParamName, int& nValue) const
{
	string sParam;
	if (!getNamedParam(szParamName, sParam)) {
		// Not found
		return false;
	}
	else {
		// Found
		nValue = atoi(sParam.c_str());
		return true;
	}
}

bool ParamParser::getNamedParam(const char* szParamName, double& dValue) const
{
	string sParam;
	if (!getNamedParam(szParamName, sParam)) {
		// Not found
		return false;
	}
	else {
		// Found
		dValue = atof(sParam.c_str());
		return true;
	}
}


bool ParamParser::getNamedParam(const char* szParamName) const {
	string sDummy;
	return getNamedParam(szParamName, sDummy);
}

void ParamParser::setNamedParam(const char* szParamName, const std::string& sParamValue)
{
	ParamEntry entry;
	entry.m_sValue = sParamValue;
	entry.m_bHasValue = true;
	entry.m_bFound = true;

	m_namedParams[string(szParamName)] = entry;
}

void ParamParser::removeNamedParam(const char* szParamName)
{
	m_namedParams.erase(string(szParamName));
}

void ParamParser::getParams(vector<string>& values) const
{
	ParamMap::const_iterator it;

	for (it = m_namedParams.begin(); it != m_namedParams.end(); it++) {
		if (it->second.m_bFound) {
			values.push_back(string("-") + it->first);
			if (it->second.m_bHasValue) {
				values.push_back(it->second.m_sValue);
			}
		}
	}
}



string ParamParser::getUnnamedParam(unsigned int nIndex) const
{
	if (nIndex >= 0 && nIndex < m_unnamedParams.size()) {
		return m_unnamedParams[nIndex];
	}
	else {
		return "";
	}
}

const ParamParser::StringVector& ParamParser::getInvalidParams() const
{
	return m_invalidParams;
}

} // namespace LW

