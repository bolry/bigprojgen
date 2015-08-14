// Copyright © 2015 Bo Rydberg


#include <cctype>
#include <cerrno>
#include <clocale>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <chrono>
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

char const cmakeListName[] { "CMakeLists.txt" };
char const headerExt[] { ".h" };
char const libNamePostfix[] { "core" };
char const srcExt[] { ".cpp" };
int const filePrefixLen { 5 };
int const headerExtLen = sizeof headerExt - 1;
ios_base::iostate const osExceptions { ios_base::badbit | ios_base::eofbit | ios_base::failbit };

std::string MODULES;
std::vector<std::string> Enums;
std::vector<std::string> Includes;
std::vector<std::string> NameBases;

int GetCurrentYear()
{
	static int currentYear { };
	if (currentYear == 0) {
		using std::chrono::system_clock;
		auto const now = system_clock::now();
		auto const tt = system_clock::to_time_t(now);
		struct std::tm tp;
		if (localtime_r(&tt, &tp) == nullptr) {
			throw std::runtime_error("problem with localtime_r");
		}
		currentYear = tp.tm_year + 1900;
	}
	return currentYear;
}

int RandInt(int low, int high)
{
	static std::default_random_engine re { };
	using Dist = std::uniform_int_distribution<int>;
	static Dist uid { };
	return uid(re, Dist::param_type { low, high });
}

std::string mkIncludeGuard(std::string const & fname)
{
	std::string incguard{fname};
	for (auto & c : incguard) {
		c = std::toupper(c);
	}
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
	std::string const incguard{mkIncludeGuard(fname)};
	os << "#ifndef " << incguard << "\n"
	      "#define " << incguard << "\n";
	os << "// Copyright © " << GetCurrentYear() << " Bo Rydberg\n";
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

void mksources(std::string const & dirbase, std::string const & namebase,
                int const fileNr, std::vector<std::string> & cppfiles)
{
	std::string const fname{baseFilename(namebase, fileNr)};
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(dirbase + "/" + fname + srcExt);
	os << "// Copyright © " << GetCurrentYear() << " Bo Rydberg\n";
	for (auto const & s : Includes) {
		os << "#include \"" << s << "\"\n";
	};
	os << '\n';
	std::string const className("K" + fname.substr(filePrefixLen));
	os << className << "::" << className << "() :\n"
	      "\t\tm_" << fname.substr(filePrefixLen) <<"()\n"
	      "{\n";
	for (auto const & s : Includes) {
		auto const len = s.length() - filePrefixLen - headerExtLen;
		os << "\tm_" << fname.substr(filePrefixLen) << " += EnumValue_"
		   << s.substr(filePrefixLen, len) << ";\n";
	};
	os << "}\n"
	      "\n"
	      "void " << className << "::Work_" << fname.substr(filePrefixLen) << "()\n"
	      "{\n"
	      "\t++m_" << fname.substr(filePrefixLen) << ";\n"
	      "}\n";
	cppfiles.push_back(fname + srcExt);
}

void mkCMakeLists(std::string const & dirbase, std::string const & namebase,
                std::vector<std::string> const & cppfiles)
{
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(dirbase + "/" + cmakeListName);
	os << "project(Prg" << namebase << ")\n"
			"add_library(" << namebase << libNamePostfix << "\n";
	for (auto const & fname : cppfiles) {
		os << "\t" << fname << "\n";
	};
	os << ")\n";
	os << "target_include_directories(" << namebase << libNamePostfix
	                << " PUBLIC \"$<BUILD_INTERFACE:${Prg" << namebase
	                << "_SOURCE_DIR}>\"\n";
	for(auto const & nb : NameBases) {
		os << "\t\"$<BUILD_INTERFACE:${Prg" << nb << "_SOURCE_DIR}>\"\n";
	}
	os << ")\n";
	NameBases.push_back(namebase);
}

void mkfiles(std::string const & dirbase, std::string const & namebase, int const nrFiles)
{
	std::vector<std::string> cppfiles;
	for (int i { }; i != nrFiles; ++i) {
		mkheader(dirbase, namebase, i);
		mksources(dirbase, namebase, i, cppfiles);
	}
	mkCMakeLists(dirbase, namebase, cppfiles);
	MODULES += " " + dirbase.substr(2);
}

void mkDir(std::string const & dirLocation)
{
	if (mkdir(dirLocation.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
		auto const e = errno;
		std::ostringstream oss;
		oss << "`mkdir(\"" << dirLocation
		    << "\", S_IRWXU | S_IRWXG | S_IRWXO)' failed with errno " << e;
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
	std::string module;
	while (iss >> module) {
		os << "add_subdirectory(" << module << ")\n";
	}
}

int getDepth(int const argc, char *argv[])
{
	if (argc > 1) {
		std::istringstream iss(argv[1]);
		int depth;
		if (iss >> depth && depth > 0 && iss.eof()) {
			return depth;
		}
	}
	int const defaultDepth = 1;
	std::cerr << "Using default depth of " << defaultDepth << "\n";
	return defaultDepth;
}

char getDirRangeEnd(int const argc, char *argv[])
{
	if (argc > 2) {
		std::istringstream iss(argv[2]);
		char theEnd;
		if (iss >> theEnd && 'a' <= theEnd && theEnd <= 'z') {
			return theEnd;
		}
	}
	char const defaultEnd = 'a';
	std::cerr << "Using default range of 'a'..'" << defaultEnd << "'\n";
	return defaultEnd;
}

} // namespace

int main(int argc, char *argv[])
{
	auto const depth = getDepth(argc, argv);
	auto const dirRangeEnd = getDirRangeEnd(argc, argv);
	mkDirRange(depth, ".", "", 'a', dirRangeEnd, 100);
	mkMainCMakeListsFile();
	return EXIT_SUCCESS;
}
