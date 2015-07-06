// Copyright © 2015 Bo Rydberg

#include <cctype>
#include <cerrno>
#include <clocale>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using std::ios_base;

char const cmakeListName[] = "CMakeLists.txt";
char const headerExt[] = ".h";
char const libNamePostfix[] = "core";
char const srcExt[] = ".cpp";
int const filePrefixLen = 5;
auto const headerExtLen = sizeof headerExt - 1;
ios_base::iostate const osExceptions = ios_base::badbit | ios_base::eofbit | ios_base::failbit;

std::string MODULES;
std::vector<std::string> Enums;
std::vector<std::string> Includes;

int RandInt(int low, int high)
{
	static std::default_random_engine re { };
	using Dist = std::uniform_int_distribution<int>;
	static Dist uid { };
	return uid(re, Dist::param_type { low, high });
}

std::string mkIncludeGuard(std::string const & fname)
{
	std::string incguard(fname);
	std::transform(std::begin(incguard), std::end(incguard), std::begin(incguard),
			[](char const c) {
				return std::toupper(c);
			});
	incguard += "_H_";
	for (int i = 0; i < 10; ++i) {
		auto const c = RandInt(0, 9 + (('Z' - 'A') + 1));
		if (c < 10) {
			incguard += '0' + c;
		} else {
			incguard += 'A' + c - 10;
		}
	}
	incguard += "_INCLUDED_";
	return incguard;
}

std::string baseFilename(std::string const & namebase, int const fileNr){
	std::ostringstream oss;
	oss.exceptions(osExceptions);
	oss << "file_" << namebase << "_" << std::setw(3) << std::setfill('0') << fileNr;
	return oss.str();
}

void mkheader(std::string const & dirbase, std::string const & namebase, int const fileNr)
{
	std::string const fname(baseFilename(namebase, fileNr));
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(dirbase + "/" + fname + headerExt);
	std::string const incguard(mkIncludeGuard(fname));
	os << "#ifndef " << incguard << "\n"
	      "#define " << incguard << "\n"
	      "// Copyright © 2015 Bo Rydberg\n";
	os << "enum {\n"
	      "\tEnumValue_" << fname.substr(filePrefixLen) << " = 1\n"
	      "};\n";
	std::string const className("K" + fname.substr(filePrefixLen));
	os << "class " << className << " {\n"
	      "public:\n"
	      "\t" << className << "();\n"
	      "\tvoid Work_" << fname.substr(filePrefixLen) << "();\n"
	      "private:\n"
	      "\tint m_" << fname.substr(filePrefixLen) << ";\n"
	      "};\n"
	      "#endif // " << incguard << "\n";
	Includes.push_back(fname + headerExt);
}

void mksource(std::string const & dirbase, std::string const & namebase, int const fileNr)
{
	std::string const fname(baseFilename(namebase, fileNr));
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(dirbase + "/" + fname + srcExt);
	os << "// Copyright © 2015 Bo Rydberg\n";
	std::for_each(Includes.rbegin(), Includes.rend(),
			[&](std::string const & s)
			{
				os << "#include \"" << s << "\"\n";
			});
	os << "\n";
	std::string const className("K" + fname.substr(filePrefixLen));
	os << className << "::" << className << "() :\n"
	      "\t\tm_" << fname.substr(filePrefixLen) <<"()\n"
	      "{\n";
	std::for_each(std::begin(Includes), std::end(Includes),
			[&](std::string const & s)
			{
				auto const len = s.length() - filePrefixLen
				- headerExtLen;
				os << "\tm_" << fname.substr(filePrefixLen) << " += EnumValue_"
				<< s.substr(filePrefixLen, len)
				<< ";\n";
			});
	os << "}\n\n"
	      "void " << className << "::Work_" << fname.substr(filePrefixLen) << "()\n"
	      "{\n"
	      "\t++m_" << fname.substr(filePrefixLen) << ";\n"
	      "}\n";
}

void mkCMakeLists(std::string const & dirbase, std::string const & namebase){
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(dirbase + "/" + cmakeListName);
	os << "project(Prg" << namebase << ")\n"
	      "add_library(" << namebase << libNamePostfix << " f" << namebase << srcExt << ")\n";
}

void mkfiles(std::string const & dirbase, std::string const & namebase, int const nrFiles)
{
	for (auto i = 0; i < nrFiles; ++i) {
		mkheader(dirbase, namebase, i);
		mksource(dirbase, namebase, i);
	}
	mkCMakeLists(dirbase, namebase);
}

void mkDir(std::string const & dirLocation)
{
	if (mkdir(dirLocation.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
		auto const e = errno;
		std::ostringstream oss;
		oss << "`mkdir(\"" << dirLocation
				<< "\", S_IRWXU | S_IRWXG | S_IRWXO)' failed with " << e;
		throw std::runtime_error(oss.str());
	}
}

void mkDirRange(int const depth, std::string const & dirbase, std::string const & namebase,
		char const a, char const z, int const nrFiles)
{
	if (depth <= 0) {
		return mkfiles(dirbase, namebase, nrFiles);
	}
	std::string d(dirbase + "/directory_" + namebase);
	auto const len = d.length();
	for (auto i = a; i <= z; ++i) {
		d.resize(len);
		d += i;
		mkDir(d);
		mkDirRange(depth - 1, d, namebase + i, a, z, nrFiles);
	}
}

void mkMainCMakeListsFile()
{
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(&cmakeListName[0]);
	os << "cmake_minimum_required(VERSION 2.8)\n"
	      "project(BigThing)\n";
	std::istringstream iss(MODULES);
	std::string mod;
	while (iss >> mod) {
		os << "add_subdirectory(" << mod << ")\n";
	}
}

int getDepth(int const argc, char *argv[])
{
	if (1 < argc) {
		std::istringstream iss(argv[1]);
		int dept;
		if (iss >> dept) {
			return dept;
		}
	}
failhandling:
	int const defaultDepth = 1;
	std::cerr << "Using default depth of " << defaultDepth << "\n";
	return defaultDepth;
}

} // namespace

int main(int argc, char *argv[])
{
	auto const depth = getDepth(argc, argv);
	mkDirRange(depth, ".", "", 'a', 'b', 2);
	mkMainCMakeListsFile();
	return EXIT_SUCCESS;
}
