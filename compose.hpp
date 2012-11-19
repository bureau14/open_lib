
#pragma once

// function composition library
// can compose functions and functors
// compose(f1, f2, f3)(x) <=> f3(f2(f1(x)))

// Copyright (c) 2012, Bureau 14
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//    * Neither the name of Bureau 14 nor the
//      names of its contributors may be used to endorse or promote products
//      derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL BUREAU 14 BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// History
//
// EA - Original open source version with rvalue references support 
//      Tested with Boost 1.52.0, gcc 4.6.1, Clang 3.1 and VC11
//      around 200 lines of extreme terror

#include <boost/config.hpp>

#ifdef BOOST_NO_RVALUE_REFERENCES
#error "Rvalue references support is required for this header"
#endif

#include <boost/mpl/vector.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/transform.hpp>

#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/algorithm/transformation/pop_back.hpp>
#include <boost/fusion/include/pop_back.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/sequence/intrinsic/back.hpp>
#include <boost/fusion/include/back.hpp>
#include <boost/fusion/adapted/boost_tuple.hpp>
#include <boost/fusion/include/boost_tuple.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/include/mpl.hpp>

#ifndef BOOST_NO_VARIADIC_TEMPLATES
// Fusion needs variadic templates for Tuples support
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/tuple.hpp>
#endif

#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

namespace wrpme
{

    template <typename Inner, typename Outer>
    class composed 
    {

    public:
        explicit composed(Inner && inner = Inner(), Outer && outer = Outer()) : _inner(std::forward<Inner>(inner)), _outer(std::forward<Outer>(outer)) {}

        // since we define move semantics, we have to define a copy constructor as well
        // note that we define a copy constructor because we don't have yet perfect forwarding with Boost.Fusion
        // once this is available, composed should be non copyable
        composed(const composed & other) : _inner(other._inner), _outer(other._outer) {}

    public:
        // move semantics support via construction and assignment
        composed(composed && other) : _inner(std::move(other._inner)), _outer(std::move(other._outer)) {}

        composed & operator = (composed && other)
        {
            std::swap(_inner, other._inner);
            std::swap(_outer, other._outer);

            return *this;
        }

    public:
        // VC10 crashes when it attempts to compile this method, you need at least VC11
        // don't mock VC, gcc crashes much more often (Clang <3)
        template <typename T>
        auto operator()(T && x) -> decltype(Outer()(Inner()(x)))
        {
            return _outer(_inner(std::forward<T>(x)));
        }

    private:
        Inner _inner;
        Outer _outer;
    };

    template <typename Types>
    struct make_composed
    {
        // the size must be at least 2 or it will not compile
        typedef Types types;

        // we need first and second element
        typedef typename boost::mpl::at<types, boost::mpl::int_<0>>::type first_type;
        typedef typename boost::mpl::at<types, boost::mpl::int_<1>>::type second_type;
        typedef typename boost::mpl::back<types>::type back_type;

        // we need the subset without the first and second element
        typedef typename boost::mpl::pop_front<typename boost::mpl::pop_front<types>::type>::type types_tail;

        typedef typename boost::mpl::fold<types_tail, 
            composed<first_type, second_type>, 
            composed<boost::mpl::_1, boost::mpl::_2>>::type type;
    };

    template <typename Vector, int Size>
    struct make_from_vector : make_composed<Vector>
    {
        typedef typename make_composed<Vector>::type type;
        typedef typename make_composed<Vector>::back_type back_type;

        // Boost.Fusion doesn't support rvalue references fully yet so perfect forwarding will not work through this interface
        type operator()(Vector && v) const
        {
            static_assert(Size > 1, "need at least 2 functions to make a composition");
            typedef typename boost::fusion::result_of::as_vector<typename boost::fusion::result_of::pop_back<Vector>::type>::type head_vector;
            back_type && back = std::forward<back_type>(boost::fusion::back(v));
            return type(make_from_vector<head_vector, head_vector::size::value>()(boost::fusion::pop_back(std::forward<Vector>(v))), std::move(back));
        }
    };

    // recursion termination
    template <typename Vector>
    struct make_from_vector<Vector, 2> : make_composed<Vector>
    {
        typedef typename make_composed<Vector>::type type;
        typedef typename make_composed<Vector>::first_type first_type;
        typedef typename make_composed<Vector>::second_type second_type;

        type operator()(Vector && v) const
        {
            // we do our best to support move semantics
            return type( std::forward<first_type>(boost::fusion::at<boost::mpl::int_<0>>(v)), 
                        std::forward<second_type>(boost::fusion::at<boost::mpl::int_<1>>(v)));
        }
    };

    // composes from a tuple of pointers to functors (or any multi type container)
    // note that Boost.Fusion rvalues references support being incomplete, perfect forwarding will fail through
    // this interface
    template <typename Container>
    typename make_composed<typename boost::fusion::result_of::as_vector<Container>::type>::type compose(Container && c)
    {
        // convert the tuple to a Boost.Fusion vector, as_vector will break perfect forwarding
        typedef typename boost::fusion::result_of::as_vector<Container>::type vector_type;
        return make_from_vector<vector_type, vector_type::size::value>()(boost::fusion::as_vector(std::forward<Container>(c)));
    }

    // compose from two functions 
    // this helper function is the recursion termination
    template <typename Function1, typename Function2>
    composed<Function1, Function2> compose(Function1 && f1, Function2 && f2)
    {
        return composed<Function1, Function2>(std::forward<Function1>(f1), std::forward<Function2>(f2));
    }    

// without variadic templates we can abuse the C++ preprocessor for the same effect
// (actually for a better effect as we will preserve perfect forwarding easily)
// it has to be noted that you will lose two points of humanity if you ever understand how this macro works

#define WRPME_DETAIL_FORWARD_SINGLE(n) std::forward<Function##n>(f##n)
#define WRPME_DETAIL_FORWARD_SINGLE_COMMA(z, n, _) ,WRPME_DETAIL_FORWARD_SINGLE(n) 

#define WRPME_DETAIL_COMPOSE_FUNCTION(z, n, _) \
    template<BOOST_PP_ENUM_PARAMS_Z(z, BOOST_PP_INC(n), typename Function)> \
    typename make_composed<typename boost::mpl::vector<BOOST_PP_ENUM_PARAMS_Z(z, BOOST_PP_INC(n), Function)> >::type \
    compose(BOOST_PP_ENUM_BINARY_PARAMS_Z(z, BOOST_PP_INC(n), Function, && f)) \
    { \
    return compose(compose(WRPME_DETAIL_FORWARD_SINGLE(0) BOOST_PP_REPEAT_FROM_TO(1, n, WRPME_DETAIL_FORWARD_SINGLE_COMMA, nil)), \
                   WRPME_DETAIL_FORWARD_SINGLE(n)); \
    }

// If you need more than BOOST_MPL_LIMIT_VECTOR_SIZE parameters you and I should have a serious conversation
// by default BOOST_MPL_LIMIT_VECTOR_SIZE is 20...
BOOST_PP_REPEAT_FROM_TO(2, BOOST_MPL_LIMIT_VECTOR_SIZE, WRPME_DETAIL_COMPOSE_FUNCTION, nil)

#undef WRPME_DETAIL_FORWARD_SINGLE
#undef WRPME_DETAIL_FORWARD_SINGLE_COMMA
#undef WRPME_DETAIL_COMPOSE_FUNCTION

}
