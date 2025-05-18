#pragma once

#include <vector>
#include "match-result.h"

// Simple interface for a class that listens for match results.
class IMatchResultListener{
public:
    virtual ~IMatchResultListener() = default;
    virtual void deliver_results(MatchResult result) = 0;
};

class MatchResultRecorder : public IMatchResultListener
{
public:
    MatchResultRecorder();
    void deliver_results(MatchResult result);

};
