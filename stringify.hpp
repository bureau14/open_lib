
#pragma once

#include <boost/type_traits/is_signed.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>

namespace qdb
{
namespace tools
{
namespace detail
{
    // I could comment but I don't wanna
    template <typename T, bool b>
    struct select_int_generator
    {
        typedef boost::spirit::karma::int_generator<T> type;
    };

    template <typename T>
    struct select_int_generator<T, false>
    {
        typedef boost::spirit::karma::uint_generator<T> type;
    };

    template <typename T>
    struct integer_generator
    {
        typedef typename select_int_generator<T, boost::is_signed<T>::value>::type type;
    };

    template <typename T, bool b>
    struct select_int_parser
    {
        typedef boost::spirit::qi::int_parser<T> type;
    };

    template <typename T>
    struct select_int_parser<T, false>
    {
        typedef boost::spirit::qi::uint_parser<T> type;
    };

    template <typename T>
    struct integer_parser
    {
        typedef typename select_int_parser<T, boost::is_signed<T>::value>::type type;
    };
}

// very fast int to string and string to int converters (boost lexical cast is slow)
template <typename Integer>
std::string int_to_string(Integer i)
{
    // this will select the signed or unsigned *generator* automagically
    typedef typename detail::integer_generator<Integer>::type integer_generator;
    
    // if you consider that 2^3 ~ 10^1, then you divide by three the number of bits to have the number of 10 based digits
    // note that dividing by 3 is generous, 2^32 is at maximum 9 figures large, not 10 or 11
    // the actual value ln(2)/ln(10) ~ 3.32
    // 
    // max int is 2^64 that's about 10^20... 24 chars is more than enough if you count the sign and want to opt
    // for an easy to align value
    static const size_t local_buffer_size = 24;

    // let's check at compile time nothing fancy happens (we might get a 128-bit integer, who knows!)
    static_assert(local_buffer_size > ((sizeof(Integer) / 3) + 1), "This integer type requires a larger buffer");

    char local[local_buffer_size];
    char * p = local;

#ifdef _DEBUG
    const char * const start = local;
    const char * const end = p + local_buffer_size - 1;
#endif

    boost::spirit::karma::generate(p, integer_generator(), i);

    assert(p > start);
    assert(p < end);
    // ensure we have a 0, we have extra chars that make sure we can't overflow
    p[0] = '\0';

    return local;
}

template <typename Integer>
boost::system::error_code int_from_string(const std::string & str, Integer & res)
{
    // this will select the signed or unsigned *parser* automagically
   typedef typename detail::integer_parser<Integer>::type integer_parser;

    std::string::const_iterator it = str.begin();
    std::string::const_iterator last = str.end();

    const bool great_success = boost::spirit::qi::parse(it, last, integer_parser(), res);

    // we must have parsed successfully and consumed all the input
    return (great_success && (it == last)) ? boost::system::error_code() : make_error_code(boost::system::errc::invalid_argument);
}

}
}
