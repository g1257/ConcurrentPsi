#ifndef CONCURRENTPATTERN_H
#define CONCURRENTPATTERN_H

namespace ConcurrentPsi {

class ConcurrentPattern {

public:

	enum PatternsEnum { PATTERN_READONLY, PATTERN_REDUCE };

	enum PatternOpsEnum { PATTERN_NOOP, PATTERN_SUM};

	ConcurrentPattern(PatternsEnum p, PatternOpsEnum = PATTERN_NOOP)
	    : pattern_(p)
	{}

	PatternsEnum operator()() const { return pattern_; }

private:

	PatternsEnum pattern_;
}; // class ConcurrentPattern

} // namespace ConcurrentPsi

#endif // CONCURRENTPATTERN_H

