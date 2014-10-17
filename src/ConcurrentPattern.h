#ifndef CONCURRENTPATTERN_H
#define CONCURRENTPATTERN_H

namespace ConcurrentPsi {

class ConcurrentPattern {

public:

	enum PatternsEnum { PATTERN_REDUCE };

	enum PatternOpsEnum { PATTERN_NOOP, PATTERN_SUM};

	ConcurrentPattern(PatternsEnum p, PatternOpsEnum o = PATTERN_NOOP)
	{}
}; // class ConcurrentPattern

} // namespace ConcurrentPsi

#endif // CONCURRENTPATTERN_H

