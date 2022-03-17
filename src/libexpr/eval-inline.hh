#pragma once

#include "eval.hh"

#define LocalNoInline(f) static f __attribute__((noinline)); f
#define LocalNoInlineNoReturn(f) static f __attribute__((noinline, noreturn)); f

namespace nix {

LocalNoInlineNoReturn(void throwEvalError(const Pos & pos, const char * s))
{
    throw EvalError({
        .msg = hintfmt(s),
        .errPos = pos
    });
}

LocalNoInlineNoReturn(void throwTypeError(const Pos & pos, const char * s, const Value & v))
{
    throw TypeError({
        .msg = hintfmt(s, showType(v)),
        .errPos = pos
    });
}


void EvalState::forceValue(Value & v, const Pos & pos)
{
    forceValue(v, [&]() { return pos; });
}


template<typename Callable>
void EvalState::forceValue(Value & v, Callable getPos)
{
    if (v.isThunk()) {
        Env * env = v.thunk.env;
        Expr * expr = v.thunk.expr;
        try {
            v.mkBlackhole();
            //checkInterrupt();
            expr->eval(*this, *env, v);
        } catch (...) {
            v.mkThunk(env, expr);
            throw;
        }
    }
    else if (v.isApp())
        callFunction(*v.app.left, *v.app.right, v, noPos);
    else if (v.isBlackhole())
        throwEvalError(getPos(), "infinite recursion encountered");
}


inline void EvalState::forceAttrs(Value & v, const Pos & pos, const std::string_view & errorCtx)
{
    forceAttrs(v, [&]() { return pos; }, errorCtx);
}


template <typename Callable>
inline void EvalState::forceAttrs(Value & v, Callable getPos, const std::string_view & errorCtx)
{
    try {
        forceValue(v, noPos);
        if (v.type() != nAttrs) {
            throwTypeError(noPos, "value is %1% while a set was expected", v);
        }
    } catch (Error & e) {
        Pos pos = getPos();
        e.addTrace(pos, errorCtx);
        throw;
    }
}


inline void EvalState::forceList(Value & v, const Pos & pos, const std::string_view & errorCtx)
{
    try {
        forceValue(v, noPos);
        if (!v.isList()) {
            throwTypeError(noPos, "value is %1% while a list was expected", v);
        }
    } catch (Error & e) {
        e.addTrace(pos, errorCtx);
        throw;
    }
}

/* Note: Various places expect the allocated memory to be zeroed. */
inline void * allocBytes(size_t n)
{
    void * p;
#if HAVE_BOEHMGC
    p = GC_MALLOC(n);
#else
    p = calloc(n, 1);
#endif
    if (!p) throw std::bad_alloc();
    return p;
}


}
