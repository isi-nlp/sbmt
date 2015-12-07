// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved


#ifndef _SERIALIZER_BASIC_TYPES_H_
#define _SERIALIZER_BASIC_TYPES_H_

#include <string>
#include <vector>
#include <map>
#include <utility>

namespace LW {
	class ISerializer;
	class OSerializer;

	void read(ISerializer& ser, int& n);
	void write(ISerializer& ser, int n);

	void read(ISerializer& ser, unsigned int& n);
	void write(ISerializer& ser, unsigned int n);

	void read(ISerializer& ser, float& n);
	void write(ISerializer& ser, float n);

	void read(ISerializer& ser, double& n);
	void write(ISerializer& ser, double n);

	void read(ISerializer& ser, std::string& s);
	void write(OSerializer& ser, const std::string& s);

	void read(ISerializer& ser, std::pair<std::string, std::string>& p);
	void write(OSerializer& ser, const std::pair<std::string, std::string>& p);

	void read(ISerializer& ser, std::pair<std::string, unsigned int>& p);
	void write(OSerializer& ser, const std::pair<std::string, unsigned int>& p);

	void read(ISerializer& ser, std::pair<std::string, float>& p);
	void write(OSerializer& ser, const std::pair<std::string, float>& p);

	void read(ISerializer& ser, std::pair<unsigned int, float>& p);
	void write(OSerializer& ser, const std::pair<unsigned int, float>& p);

	void read(ISerializer& ser, std::pair < std::vector<int>, std::vector<int> >& p);
	void write(OSerializer& ser, const std::pair < std::vector<int>, std::vector<int> >& p);

	void read(ISerializer& ser, std::map<int, float>& p);
	void write(OSerializer& ser, const std::map<int, float>& p);

	void read(ISerializer& ser, std::vector < std::vector <int> >& p);
	void write(OSerializer& ser, const std::vector < std::vector <int> >& p);

	void read(ISerializer& ser, std::vector <int>& p);
	void write(OSerializer& ser, const std::vector <int>& p);
} // namespace LW

#endif // _SERIALIZER_BASIC_TYPES_H_
