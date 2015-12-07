// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include "SerializerBasicTypes.h"

#include <assert.h>
#include "Common/stldefs.h"
#include "Common/ErrorCode.h"
#include "Serializer.h"

using namespace std;

namespace LW {

void read(ISerializer& ser, int& n) {
	ser.read(n);
}

void write(OSerializer& ser, int n) {
	ser.write(n);
}

void read(ISerializer& ser, unsigned int& n) {
	ser.read(n);
}

void write(OSerializer& ser, unsigned int n) {
	ser.write(n);
}

void read(ISerializer& ser, float& n) {
	ser.read(n);
}

void write(OSerializer& ser, float n) {
	ser.write(n);
}

void read(ISerializer& ser, double& n) {
	ser.read(n);
}

void write(OSerializer& ser, double n) {
	ser.write(n);
}

void read(ISerializer& ser, string& s) {
	ser.read(s);
}

void write(OSerializer& ser, const string& s) {
	ser.write(s);
}

void read(ISerializer& ser, pair<string, string>& p) {
	ser.read(p.first);
	ser.read(p.second);
}

void write(OSerializer& ser, const pair<string, string>& p) {
	ser.write(p.first);
	ser.write(p.second);
}

void read(ISerializer& ser, pair<string, unsigned int>& p) {
	ser.read(p.first);
	ser.read(p.second);
}

void write(OSerializer& ser, const pair<string, unsigned int>& p) {
	ser.write(p.first);
	ser.write(p.second);
}

void read(ISerializer& ser, pair<string, float>& p) {
	ser.read(p.first);
	ser.read(p.second);
}

void write(OSerializer& ser, const pair<string, float>& p) {
	ser.write(p.first);
	ser.write(p.second);
}

void read(ISerializer& ser, pair<unsigned int, float>& p) {
	ser.read(p.first);
	ser.read(p.second);
}

void write(OSerializer& ser, const pair<unsigned int, float>& p) {
	ser.write(p.first);
	ser.write(p.second);
}

void read(ISerializer& ser, pair < vector<int>, vector<int> >& p) {
	CustomSerializer<int>::readVector(ser, p.first);
	CustomSerializer<int>::readVector(ser, p.second);
}

void write(OSerializer& ser, const pair < vector<int>, vector<int> >& p) {
	CustomSerializer<int>::writeVector(ser, p.first);
	CustomSerializer<int>::writeVector(ser, p.second);
}

void read(ISerializer& ser, std::map<int, float>& p){
	unsigned int uiSize, idx;
	float f;
	ser.read(uiSize);
	while(uiSize--){
		ser.read(idx);
		ser.read(f);
		p[idx] = f;
	}
}

void write(OSerializer& ser, const std::map<int, float>& p){
	unsigned int uiSize = static_cast<unsigned int>(p.size());
	ser.write(uiSize);
	std::map<int, float>::const_iterator it = p.begin();
	while(it != p.end()){
		ser.write(it->first);
		ser.write(it->second);
		it++;
		uiSize--;
	}
	if(uiSize) ERROUT(ERR_IO, "Problem with map serialization - size left is %d",  uiSize);
}

void read(ISerializer& ser, std::vector < std::vector <int> >& p) {
	CustomSerializer<vector<int> >::readVector(ser, p);
}

void write(OSerializer& ser, const std::vector < std::vector <int> >& p) {
	CustomSerializer<vector<int> >::writeVector(ser, p);
}
	
void read(ISerializer& ser, std::vector <int>& p) {
	CustomSerializer<int>::readVector(ser, p);
}

void write(OSerializer& ser, const std::vector <int>& p) {
	CustomSerializer<int>::writeVector(ser, p);
}
	
} // namespace LW
