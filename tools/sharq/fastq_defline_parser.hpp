#ifndef __CFASTQDEFLINEPARSER_HPP__
#define __CFASTQDEFLINEPARSER_HPP__

#include <string_view>
#include <vector>
#include "fastq_read.hpp"
#include "fastq_defline_matcher.hpp"
#include "fastq_error.hpp"
#include <spdlog/spdlog.h>
#include <set>

using namespace std;

//  ============================================================================
class CDefLineParser
//  ============================================================================
{
public:
    CDefLineParser();
    ~CDefLineParser();
    void Reset();
    void Parse(const string_view& defline, CFastqRead& read);
    bool Match(const string_view& defline, bool strict = false);
    void SetMatchAll();
    const string& GetDeflineType() const;
    uint8_t GetPlatform() const;
    const set<string>& AllDeflineTypes() const { return mDeflineTypes;}
private:
    size_t mIndexLastSuccessfulMatch = 0;
    size_t mAllMatchIndex = -1;
    std::vector<std::shared_ptr<CDefLineMatcher>> mDefLineMatchers;
    std::set<string> mDeflineTypes; ///< Set of deflines types processed by this reader
};


CDefLineParser::CDefLineParser() 
{ 
    Reset(); 
    mDefLineMatchers.emplace_back(new CDefLineMatcher_NoMatch);
    mDefLineMatchers.emplace_back(new CDefLineMatcherBgiNew);
    mDefLineMatchers.emplace_back(new CDefLineMatcherBgiOld);
    mDefLineMatchers.emplace_back(new CDefLineMatcherIlluminaNew);
    mDefLineMatchers.emplace_back(new CDefLineMatcherIlluminaNewNoPrefix);
    mDefLineMatchers.emplace_back(new CDefLineMatcherIlluminaNewWithSuffix);
    mDefLineMatchers.emplace_back(new CDefLineMatcherIlluminaNewWithPeriods);
    mDefLineMatchers.emplace_back(new CDefLineMatcherIlluminaNewWithUnderscores);
    mDefLineMatchers.emplace_back(new CDefLineMatcherIlluminaNewDataGroup);
}

CDefLineParser::~CDefLineParser() 
{

}

void CDefLineParser::SetMatchAll()
{
    mDefLineMatchers.emplace_back(new CDefLineMatcher_AllMatch);
    mAllMatchIndex = mDefLineMatchers.size() - 1;
}


void CDefLineParser::Reset() 
{
    mIndexLastSuccessfulMatch = 0;
}

bool CDefLineParser::Match(const string_view& defline, bool strict) 
{
    if (mDefLineMatchers[mIndexLastSuccessfulMatch]->Matches(defline)) {
        return true;
    }
    for (size_t i = 0; i < mDefLineMatchers.size(); ++i) {
        if (i == mIndexLastSuccessfulMatch) {
            continue;
        }
        if (!mDefLineMatchers[i]->Matches(defline)) {
            continue;
        }
        if (strict && i == mAllMatchIndex)
            return false;
        mIndexLastSuccessfulMatch = i;
        mDeflineTypes.insert(mDefLineMatchers[mIndexLastSuccessfulMatch]->Defline());
        //spdlog::info("Current pattern: {}", mDefLineMatchers[mIndexLastSuccessfulMatch]->Defline());
        return true;
    }
    return false;
}


void CDefLineParser::Parse(const string_view& defline, CFastqRead& read) 
{
    if (Match(defline)) {
        mDefLineMatchers[mIndexLastSuccessfulMatch]->GetMatch(read);
        return;
    }
    throw fastq_error(100, "Defline '{}' not recognized", defline);
}

inline
uint8_t CDefLineParser::GetPlatform() const 
{
    return mDefLineMatchers[mIndexLastSuccessfulMatch]->GetPlatform();
}

inline
const string& CDefLineParser::GetDeflineType() const
{
    return mDefLineMatchers[mIndexLastSuccessfulMatch]->Defline();
}



#endif

