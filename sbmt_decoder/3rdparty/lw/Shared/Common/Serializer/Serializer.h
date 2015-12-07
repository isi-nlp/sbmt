// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved


#ifndef _SERIALIZER_H_
#define _SERIALIZER_H_

#include <string>
#include <iostream>
#include <vector>

#include <Common/ErrorCode.h>
#include "SerializerBasicTypes.h"

namespace LW {

    class SerializerException : public Exception {};

	class ISerializer {
	public:
		ISerializer(std::istream* pIn);
	public:
		bool eof();
	public:
		void read(bool& bValue);
		void read(int& nValue);
		void read(unsigned int& nValue);
		void read(float& fValue);
		void read(double& dValue);
		void read(std::string& sValue);
		void read(char* sValue, size_t nSize);
	public:
		void readMagicNumber(unsigned int nExpectedMagicNumber);
	protected:
		std::istream* m_pIn;
	};

	class OSerializer {
	public:
		OSerializer(std::ostream* pOut);
	public:
		void write(bool bValue);
		void write(int nValue);
		void write(unsigned int nValue);
		void write(float fValue);
		void write(double dValue);
		void write(const std::string& sValue);
		void write(const char* sValue, size_t nSize);
	public:
		void writeMagicNumber(unsigned int nMagicNumber);
	protected:
		std::ostream* m_pOut;
	};

	class IOSerializer: public ISerializer, public OSerializer {
	public:
		/// Creates an input serializer
		IOSerializer(std::istream* pIn);
		/// Creates an output serializer
		IOSerializer(std::ostream* pOut);
	public:
		bool isReading();
	public:
		void exchange(int& nValue);
		void exchange(unsigned int& nValue);
		void exchange(float& fValue);
		void exchange(double& dValue);
		void exchange(std::string& sValue);
		void exchange(char* sValue, size_t nSize);
	public:
		void exchangeMagicNumber(unsigned int& nMagicNumber);
	};

	template <class T> 
	class CustomSerializer {
	public:
		static void readVector(ISerializer& in, std::vector<T>& vect) {
			vect.clear();

			// Read the size
			unsigned int nSize;
			in.read(nSize);
			// Read each element, until the size of the vector reaches nSize
			while (vect.size() < nSize) {
				T t;
				// This function must be defined for each class T
				// we want to serialize
				read(in, t);
				vect.push_back(t);
			}
		};

		static void writeVector(OSerializer& out, const std::vector<T>& vect) {
			// Write the size of the vector
			out.write(static_cast<unsigned int>(vect.size()));

			// Read vector elements
			for(size_t i = 0; i < vect.size(); i++){ 
				// This function must be defined for every class T we want
				// to serialize
				write(out, vect[i]);
			}
		}

		static void exchangeVector(IOSerializer& serializer, std::vector<T>& vect) {
			if (serializer.isReading()) {
				readVector(serializer, vect);
			}
			else {
				writeVector(serializer, vect);
			}
		}

	};
} // namespace LW

#endif // _SERIALIZER_H_
