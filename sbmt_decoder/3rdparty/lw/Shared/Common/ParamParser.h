// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _PARAM_PARSER_H
#define _PARAM_PARSER_H

#include <vector>
#include <map>
#include <string>

namespace LW {

class ParamParser
{
private:
	struct ParamEntry {
		std::string  m_sValue;
		bool	m_bHasValue;
		bool	m_bFound;
	};
public:
	typedef std::map<std::string, ParamEntry> ParamMap;
	typedef std::vector<std::string> StringVector;
public:
	ParamParser();
	void registerParam(const char* szParamName, bool bHasValue);
	void parse(int argc, char** argv);
	bool getNamedParam(const char* szParamName, std::string& sValue) const;
	bool getNamedParam(const char* szParamName, int& nValue) const;
	bool getNamedParam(const char* szParamName, double& nValue) const;
	bool getNamedParam(const char* szParamName) const;

	void setNamedParam(const char* szParamName, const std::string& sParamValue);

	void removeNamedParam(const char* szParamName);

	void getParams(std::vector<std::string>& values) const;

	std::string getUnnamedParam(int unsigned nIndex) const;
	/// Returns a list of parameter passed on the command line that were not registered (probably invalid)
	const StringVector& getInvalidParams() const;
private:
	ParamMap m_namedParams;
	StringVector	m_unnamedParams;
	StringVector	m_invalidParams;
};

} // namespace LW

#endif // _PARAM_PARSER_H

