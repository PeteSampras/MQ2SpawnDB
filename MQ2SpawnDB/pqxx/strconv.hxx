/* String conversion definitions.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/stringconv instead.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_STRINGCONV
#define PQXX_H_STRINGCONV

#include "pqxx/compiler-public.hxx"

#include <algorithm>
#include <cstring>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <typeinfo>

#if __has_include(<charconv>)
#  include <charconv>
#endif

#include "pqxx/except.hxx"
#include "pqxx/util.hxx"
#include "pqxx/zview.hxx"


namespace pqxx::internal
{
/// Attempt to demangle @c std::type_info::name() to something human-readable.
PQXX_LIBEXPORT std::string demangle_type_name(const char[]);
} // namespace pqxx::internal


namespace pqxx
{
/**
 * @defgroup stringconversion String conversion
 *
 * The PostgreSQL server accepts and represents data in string form.  It has
 * its own formats for various data types.  The string conversions define how
 * various C++ types translate to and from their respective PostgreSQL text
 * representations.
 *
 * Each conversion is defined by a specialisations of @c string_traits.  It
 * gets complicated if you want top performance, but until you do, all you
 * really need to care about when converting values between C++ in-memory
 * representations such as @c int and the postgres string representations is
 * the @c pqxx::to_string and @c pqxx::from_string functions.
 *
 * If you need to convert a type which is not supported out of the box, you'll
 * need to define your own specialisations for these templates, similar to the
 * ones defined here and in `pqxx/conversions.hxx`.  Any conversion code which
 * "sees" your specialisation will now support your conversion.  In particular,
 * you'll be able to read result fields into a variable of the new type.
 *
 * There is a macro to help you define conversions for individual enumeration
 * types.  The conversion will represent enumeration values as numeric strings.
 */
//@{

/// A human-readable name for a type, used in error messages and such.
/** Actually this may not always be very user-friendly.  It uses
 * @c std::type_info::name().  On gcc-like compilers we try to demangle its
 * output.  Visual Studio produces human-friendly names out of the box.
 *
 * This variable is not inline.  Inlining it gives rise to "memory leak"
 * warnings from asan, the address sanitizer, possibly from use of
 * @c std::type_info::name.
 */
template<typename TYPE>
const std::string type_name{internal::demangle_type_name(typeid(TYPE).name())};


/// Traits describing a type's "null value," if any.
/** Some C++ types have a special value or state which correspond directly to
 * SQL's NULL.
 *
 * The @c nullness traits describe whether it exists, and whether a particular
 * value is null.
 */
template<typename TYPE, typename ENABLE = void> struct nullness
{
  /// Does this type have a null value?
  static bool has_null;

  /// Is @c value a null?
  static bool is_null(const TYPE &value);

  /// Return a null value.
  /** Don't use this in generic code to compare a value and see whether it is
   * null.  Some types may have multiple null values which do not compare as
   * equal, or may define a null value which is not equal to anything including
   * itself, like in SQL.
   */
  static TYPE null();
};


/// Nullness traits describing a type which does not have a null value.
template<typename TYPE> struct no_null
{
  static constexpr bool has_null = false;
  static constexpr bool is_null(const TYPE &) noexcept { return false; }
};


/// Traits class for use in string conversions.
/** Specialize this template for a type for which you wish to add to_string
 * and from_string support.
 */
template<typename TYPE> struct string_traits
{
  /// Return a @c string_view representing value, plus terminating zero.
  /** Produces a @c string_view containing the PostgreSQL string representation
   * for @c value.
   *
   * Uses the space from @c begin to @c end as a buffer, if needed.  The
   * returned string may lie somewhere in that buffer, or it may be a
   * compile-time constant, or it may be null if value was a null value.  Even
   * if the string is stored in the buffer, its @c begin() may or may not be
   * the same as @c begin.
   *
   * The @c string_view is guaranteed to be valid as long as the buffer from
   * @c begin to @c end remains accessible and unmodified.
   *
   * @throws pqxx::conversion_overrun if the provided buffer space may not be
   * enough.  For maximum performance, this is a conservative estimate.  It may
   * complain about a buffer which is actually large enough for your value, if
   * an exact check gets too expensive.
   */
  static inline zview to_buf(char *begin, char *end, const TYPE &value);

  /// Write value's string representation into buffer at @c begin.
  /** Assumes that value is non-null.
   *
   * Writes value's string representation into the buffer, starting exactly at
   * @c begin, and ensuring a trailing zero.  Returns the address just beyond
   * the trailing zero, so the caller could use it as the @c begin for another
   * call to @c into_buf writing a next value.
   */
  static inline char *into_buf(char *begin, char *end, const TYPE &value);

  static inline TYPE from_string(std::string_view text);

  /// Estimate how much buffer space is needed to represent value.
  /** The estimate may be a little pessimistic, if it saves time.
   *
   * The estimate includes the terminating zero.
   */
  static inline size_t size_buffer(const TYPE &value);
};


/// Nullness: Enums do not have an inherent null value.
template<typename ENUM>
struct nullness<ENUM, std::enable_if_t<std::is_enum_v<ENUM>>> : no_null<ENUM>
{};
} // namespace pqxx


namespace pqxx::internal
{
/// Helper class for defining enum conversions.
/** The conversion will convert enum values to numeric strings, and vice versa.
 *
 * To define a string conversion for an enum type, derive a @c string_traits
 * specialisation for the enum from this struct.
 *
 * There's usually an easier way though: the @c PQXX_DECLARE_ENUM_CONVERSION
 * macro.  Use @c enum_traits manually only if you need to customise your
 * traits type in more detail.
 */
template<typename ENUM> struct enum_traits
{
  using impl_type = std::underlying_type_t<ENUM>;
  using impl_traits = string_traits<impl_type>;

  static constexpr zview to_buf(char *begin, char *end, const ENUM &value)
  {
    return impl_traits::to_buf(begin, end, static_cast<impl_type>(value));
  }

  static constexpr char *into_buf(char *begin, char *end, const ENUM &value)
  {
    return impl_traits::into_buf(begin, end, static_cast<impl_type>(value));
  }

  static ENUM from_string(std::string_view text)
  {
    return static_cast<ENUM>(impl_traits::from_string(text));
  }

  static size_t size_buffer(const ENUM &value)
  {
    return impl_traits::size_buffer(static_cast<impl_type>(value));
  }
};
} // namespace pqxx::internal


/// Macro: Define a string conversion for an enum type.
/** This specialises the @c pqxx::string_traits template, so use it in the
 * @c ::pqxx namespace.
 *
 * For example:
 *
 *      #include <iostream>
 *      #include <pqxx/strconv>
 *      enum X { xa, xb };
 *      namespace pqxx { PQXX_DECLARE_ENUM_CONVERSION(x); }
 *      int main() { std::cout << pqxx::to_string(xa) << std::endl; }
 */
#define PQXX_DECLARE_ENUM_CONVERSION(ENUM)                                    \
  template<> struct string_traits<ENUM> : pqxx::internal::enum_traits<ENUM>   \
  {};                                                                         \
  template<> const std::string type_name<ENUM> { #ENUM }


namespace pqxx
{
/// Attempt to convert postgres-generated string to given built-in type.
/** If the form of the value found in the string does not match the expected
 * type, e.g. if a decimal point is found when converting to an integer type,
 * the conversion fails.  Overflows (e.g. converting "9999999999" to a 16-bit
 * C++ type) are also treated as errors.  If in some cases this behaviour
 * should be inappropriate, convert to something bigger such as @c long @c int
 * first and then truncate the resulting value.
 *
 * Only the simplest possible conversions are supported.  Fancy features like
 * hexadecimal or octal, spurious signs, or exponent notation won't work.
 * Whitespace is not stripped away.  Only the kinds of strings that come out of
 * PostgreSQL and out of to_string() can be converted.
 */
template<typename T> inline T from_string(std::string_view text)
{
  return string_traits<T>::from_string(text);
}


/// Attempt to convert postgres-generated string to given built-in object.
/** This is like the single-argument form of the function, except instead of
 * returning the value, it sets @c value.
 *
 * You may find this more convenient in that it infers the type you want from
 * the argument you pass.  But there are disadvantages: it requires an
 * assignment operator, and it may be less efficient.
 */
template<typename T> inline void from_string(std::string_view text, T &value)
{
  value = from_string<T>(text);
}


/// Convert a value to a readable string that PostgreSQL will understand.
/** The conversion does no special formatting, and ignores any locale settings.
 * The resulting string will be human-readable and in a format suitable for use
 * in SQL queries.  It won't have niceties such as "thousands separators"
 * though.
 */
template<typename TYPE> inline std::string to_string(const TYPE &value);


/// Convert a value to a readable string that PostgreSQL will understand.
/** This variant of to_string can sometimes save a bit of time in loops, by
 * re-using a std::string for multiple conversions.
 */
template<typename TYPE>
inline void into_string(const TYPE &value, std::string &out);


/// Is @c value null?
template<typename TYPE> inline bool is_null(const TYPE &value)
{
  if constexpr (nullness<TYPE>::has_null)
  {
    return nullness<TYPE>::is_null(value);
  }
  else
  {
    ignore_unused(value);
    return false;
  }
}


/// Does this type translate to an SQL array?
/** Specialisations may override this to be true for container types.
 *
 * This may not always be a black-and-white choice.  For instance, a
 * @c std::string is a container, but normally it translates to an SQL string,
 * not an SQL array.
 */
template<typename T> inline constexpr bool is_sql_array{false};


/// Element separator between SQL array elements of this type.
template<typename T> inline constexpr char array_separator{','};
//@}
} // namespace pqxx


#include "pqxx/internal/conversions.hxx"
#endif
