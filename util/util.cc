
#include <algorithm>

#include <sys/stat.h>
#include <execinfo.h>

#include "util.h"

using namespace util;

string util::fmt(const char* fmt, ...)
{
    char buffer[1024];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    buffer[sizeof(buffer)-1] = '\0';
	va_end(ap);

    return string(buffer);
}

std::list<string> util::strsplit(string s, string delim)
{
    std::list<string> l;

    while ( true ) {
        size_t p = s.find("::");

        if ( p == string::npos )
            break;

        l.push_back(s.substr(0, p));

        // FIXME: Don't understand why directly assigning to s doesn't work.
        string t = s.substr(p+2, string::npos);
        s = t;
    }

    l.push_back(s);
    return l;
}

string util::strreplace(const string& s, const string& o, const string& n)
{
    string r = s;

    while ( true ) {
        auto i = r.find(o);

        if ( i == std::string::npos )
            break;

        r.replace(i, o.size(), n);
    }

    return r;
}

string util::strtolower(const string& s)
{
    string t = s;
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    return t;
}

string util::strtoupper(const string& s)
{
    string t = s;
    std::transform(t.begin(), t.end(), t.begin(), ::toupper);
    return t;
}

bool util::pathExists(const string& path)
{
    struct stat st;
    return ::stat(path.c_str(), &st) == 0;
}

bool util::pathIsFile(const string& path)
{
    struct stat st;
    if ( ::stat(path.c_str(), &st) < 0 )
        return false;

    return S_ISREG(st.st_mode);
}

bool util::pathIsDir(const string& path)
{
    struct stat st;
    if ( ::stat(path.c_str(), &st) < 0 )
        return false;

    return S_ISDIR(st.st_mode);
}

bool util::endsWith(const string& s, const string& suffix)
{
    size_t i = s.rfind(suffix);

    if ( i == string::npos )
        return false;

    return (i == (s.length() - suffix.size()));
}

string util::pathJoin(const string& p1, const string& p2)
{
    if ( startsWith(p2, "/") )
        return p2;

    string p = p1;

    while ( endsWith(p, "/") )
        p = p.substr(0, p.size() - 1);

    return p + "/" + p2;
}

string util::dirname(const string& path)
{
    size_t i = path.rfind("/");

    if ( i == string::npos )
        return "";

    string dir = path.substr(0, i);

    string p = path;

    while ( endsWith(p, "/") )
        p = p.substr(0, p.size() - 1);

    return p;
}

string util::basename(const string& path)
{
    size_t i = path.rfind("/");

    if ( i == string::npos )
        return path;

    return path.substr(i+1, string::npos);
}


string util::findInPaths(const string& file, const path_list& paths)
{
    for ( auto d : paths ) {
        string path = pathJoin(d, file);
        if ( pathIsFile(path) )
            return path;
    }

    return "";
}

void util::abort_with_backtrace()
{
    fputs("\n--- Aborting\n", stderr);
    void* callstack[128];
    int frames = backtrace(callstack, 128);
    backtrace_symbols_fd(callstack, frames, 2);
    abort();
}
