Bureau 14 Open Sources Libraries
==================================

Open source libraries by Bureau 14.

License
-------

Copyright (c) 2012, Bureau 14 All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
   * Neither the name of Bureau 14 nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BUREAU 14 BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Compose
=======

Purpose
-------

Compose is used to compose an arbitrary number of functions or functors into a single functor.

Requirements
------------

This library is header-only and written in [C++11][http://en.wikipedia.org/wiki/C%2B%2B11]. It uses the following C++11 features:

    * [Rvalue references][http://en.wikipedia.org/wiki/Rvalue_references#Rvalue_references_and_move_constructors]
    * [Decltype][http://en.wikipedia.org/wiki/Decltype]
    * [Static assertions][http://en.wikipedia.org/wiki/Rvalue_references#Static_assertions]

It makes intensive usage of the Boost libraries, especially [Boost.MPL][http://www.boost.org/doc/libs/1_52_0/libs/mpl/doc/index.html] and [Boost.Fusion][http://www.boost.org/doc/libs/1_52_0/libs/fusion/doc/html/].

It has been tested against [MSVC][http://msdn.microsoft.com/fr-fr/vstudio/hh388567.aspx] 11, [clang][http://clang.llvm.org/] 3.1 and [gcc][http://gcc.gnu.org/] 4.6.

Introductory example
--------------------

 Provided the following functions:

    int f(int x)
    {
        return x +1;
    }

    int g(int x)
    {
        return x * 1;
    }

    int h(int x)
    {
        return x - 2;
    }

One can use compose to create a composed functor as this:

    auto r = compose(&f, &g, &h);

    int v = r(3); // equivalent to (h(g(f(3)));

Compose can be used with functors as well:

    struct f
    {
        int operator()(int x) const
        {
            return x + 1;
        }
    };

    struct g
    {
        int operator()(int x) const
        {
            return x * 1;
        }
    };

    struct h
    {
        int operator()(int x) const
        {
            return x - 2;
        }
    };

    auto r = compose(f(), g(), h());

    int v = r(3); // equivalent to (h(g(f(3)));

Usage
-----

Compose is header-only. The boost [MPL][http://www.boost.org/doc/libs/1_52_0/libs/mpl/doc/index.html], [Fusion][http://www.boost.org/doc/libs/1_52_0/libs/fusion/doc/html/] and [Preprocessor][http://www.boost.org/doc/libs/1_52_0/libs/preprocessor/doc/index.html] libraries are also required, these libraries being header-only as well Compose does not impose any new linking dependencies.

Compose builds a composed functor out of the provided parameters. One can indifferently mix pointer to functions and functors. Thanks to its full rvalue reference support, it is possible to use non copyable functors provided that they offer move semantics.

Compose also support tuples and random access Boost.Fusion containers. However, full move semantics support is currently unavailable through this interface. Example:

    // valid only if f, g and h are copyable.
    auto r = compose(std::forward_tuple(f(), g(), h())); 
    int v = r(3);

Composed functions need to have compatible return types, either directly or via implicit construction.  For example, this is valid:

    struct f
    {
        double operator()(int x) const
        {
            return static_cast<double>(x) * 1.3;
        };
    };

    struct g
    {
        int operator()(double x) const
        {
            return static_cast<int>(x) << 1;
        }

    };

    auto r = compose(f(), g());


And this is not :

    struct f
    {
        std::string operator()(char c) const
        {
            return std::string(10, c);
        }
    };

    struct g
    {
        int operator()(int x) const
        {
            return x + 1;
        }
    };

    // cannot compile, f outputs a std::string and g takes an int
    auto r = compose(f(), g());

Performance
-----------

There is no performance penalty when functions/functors can be inlined.

Limitations
-----------

This library only supports composition up to BOOST_MPL_LIMIT_VECTOR_SIZE (currently set to 20) functions. Rvalue references are not fully supported via the container interface. Standard tuples support requires variadic templates support by the compiler. Boost tuples don't suffer this limitation.

