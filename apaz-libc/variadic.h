#define __FIRST(a, ...) a
#define __SECOND(a, b, ...) b
#define __EMPTY()
#define __TOKCAT(a, b) a##b
#define _END_OF_ARGUMENTS_() 0

#define __EVAL(...) __EVAL1024(__VA_ARGS__)
#define __EVAL1024(...) __EVAL512(__EVAL512(__VA_ARGS__))
#define __EVAL512(...) __EVAL256(__EVAL256(__VA_ARGS__))
#define __EVAL256(...) __EVAL128(__EVAL128(__VA_ARGS__))
#define __EVAL128(...) __EVAL64(__EVAL64(__VA_ARGS__))
#define __EVAL64(...) __EVAL32(__EVAL32(__VA_ARGS__))
#define __EVAL32(...) __EVAL16(__EVAL16(__VA_ARGS__))
#define __EVAL16(...) __EVAL8(__EVAL8(__VA_ARGS__))
#define __EVAL8(...) __EVAL4(__EVAL4(__VA_ARGS__))
#define __EVAL4(...) __EVAL2(__EVAL2(__VA_ARGS__))
#define __EVAL2(...) __EVAL1(__EVAL1(__VA_ARGS__))
#define __EVAL1(...) __VA_ARGS__

#define __DEFER2(m) m __EMPTY __EMPTY()()

#define __IS_PROBE(...) __SECOND(__VA_ARGS__, 0)
#define __PROBE() ~, 1

#define __NOT(x) __IS_PROBE(__TOKCAT(__NOT_, x))
#define __BOOL(x) __NOT(__NOT(x))

#define __IF_ELSE(condition) ___IF_ELSE(__BOOL(condition))
#define ___IF_ELSE(condition) __TOKCAT(__IF_, condition)

#define __IF_1(...) __VA_ARGS__ __IF_1_ELSE
#define __IF_0(...) __IF_0_ELSE
#define __IF_1_ELSE(...)
#define __IF_0_ELSE(...) __VA_ARGS__

#define HAS_ARGS(...) __BOOL(__FIRST(_END_OF_ARGUMENTS_ __VA_ARGS__)())

#define __MAP(m, __FIRST, ...)                                                 \
  m(__FIRST) __IF_ELSE(HAS_ARGS(__VA_ARGS__))(                                 \
      __DEFER2(___MAP)()(m, __VA_ARGS__))(/* Do __NOThing, just terminate */   \
  )
#define ___MAP() __MAP
