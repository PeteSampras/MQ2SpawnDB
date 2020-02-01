#include <array>
#include <cstring>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <type_traits>
#include <vector>

#if __has_include(<string.h>)
#  include <string.h>
#endif


/* Internal helpers for string conversion.
 *
 * Do not include this header directly.  The libpqxx headers do it for you.
 */
namespace pqxx::internal
{
/// Convert a number in [0, 9] to its ASCII digit.
inline constexpr char number_to_digit(int i) noexcept
{
  return static_cast<char>(i + '0');
}


/// Like strlen, but allowed to stop at @c max bytes.
inline size_t shortcut_strlen(const char text[], [[maybe_unused]] size_t max)
{
  // strnlen_s is in C11, but not (yet) in C++'s "std" namespace.
  // This may change, so don't qualify explicitly.
  using namespace std;

#if defined(PQXX_HAVE_STRNLEN_S)

  return strnlen_s(text, max);

#elif defined(PQXX_HAVE_STRNLEN)

  return strnlen(text, max);

#else

  return strlen(text);

#endif
}


/// Summarize buffer overrun.
/** Don't worry about the exact parameter types: the sizes will be reasonably
 * small, and nonnegative.
 */
std::string PQXX_LIBEXPORT
state_buffer_overrun(int have_bytes, int need_bytes);


template<typename HAVE, typename NEED>
inline std::string state_buffer_overrun(HAVE have_bytes, NEED need_bytes)
{
  return state_buffer_overrun(
    static_cast<int>(have_bytes), static_cast<int>(need_bytes));
}


/// Throw exception for attempt to convert null to given type.
[[noreturn]] PQXX_LIBEXPORT void
throw_null_conversion(const std::string &type);


template<typename T> PQXX_LIBEXPORT extern std::string to_string_float(T);


/// Generic implementation for into_buf, on top of to_buf.
template<typename T>
inline char *generic_into_buf(char *begin, char *end, const T &value)
{
  const zview text{string_traits<T>::to_buf(begin, end, value)};
  const auto space =
    check_cast<size_t>(end - begin, "floating-point conversion to string");
  // Include the trailing zero.
  const auto len = text.size() + 1;
  if (len > space)
    throw conversion_overrun{"Not enough buffer space to insert " +
                             type_name<T> + ".  " +
                             state_buffer_overrun(space, len)};
  std::memmove(begin, text.data(), len);
  return begin + len;
}


/// String traits for builtin integral types (though not bool).
template<typename T> struct integral_traits
{
  static PQXX_LIBEXPORT T from_string(std::string_view text);
  static PQXX_LIBEXPORT zview to_buf(char *begin, char *end, const T &value);
  static PQXX_LIBEXPORT char *into_buf(char *begin, char *end, const T &value);

  static constexpr size_t size_buffer(const T &) noexcept
  {
    /** Includes a sign if needed; the number of base-10 digits which the type;
     * can reliably represent; the one extra base-10 digit which the type can
     * only partially represent; and the terminating zero.
     */
    return std::is_signed_v<T> + std::numeric_limits<T>::digits10 + 1 + 1;
  }
};


/// String traits for builtin floating-point types.
template<typename T> struct float_traits
{
  static PQXX_LIBEXPORT T from_string(std::string_view text);
  static PQXX_LIBEXPORT zview to_buf(char *begin, char *end, const T &value);
  static PQXX_LIBEXPORT char *into_buf(char *begin, char *end, const T &value);

  static constexpr size_t size_buffer(const T &) noexcept
  {
    /** Includes a sign if needed; a possible leading zero before the decimal
     * point; the full number of base-10 digits which may be needed; a decimal
     * point if needed; and the terminating zero.
     */
    return 1 + 1 + std::numeric_limits<T>::max_digits10 + 1 + 1;
  }
};
} // namespace pqxx::internal


namespace pqxx
{
/// The built-in arithmetic types do not have inherent null values.
template<typename T>
struct nullness<T, std::enable_if_t<std::is_arithmetic_v<T>>> : no_null<T>
{};


template<> struct string_traits<short> : internal::integral_traits<short>
{};
template<>
struct string_traits<unsigned short>
        : internal::integral_traits<unsigned short>
{};
template<> struct string_traits<int> : internal::integral_traits<int>
{};
template<> struct string_traits<unsigned> : internal::integral_traits<unsigned>
{};
template<> struct string_traits<long> : internal::integral_traits<long>
{};
template<>
struct string_traits<unsigned long> : internal::integral_traits<unsigned long>
{};
template<>
struct string_traits<long long> : internal::integral_traits<long long>
{};
template<>
struct string_traits<unsigned long long>
        : internal::integral_traits<unsigned long long>
{};
template<> struct string_traits<float> : internal::float_traits<float>
{};
template<> struct string_traits<double> : internal::float_traits<double>
{};
template<>
struct string_traits<long double> : internal::float_traits<long double>
{};


template<> struct string_traits<bool>
{
  static PQXX_LIBEXPORT bool from_string(std::string_view text);

  static constexpr zview to_buf(char *, char *, const bool &value) noexcept
  {
    return zview{value ? "true" : "false"};
  }

  static char *into_buf(char *begin, char *end, const bool &value)
  {
    return pqxx::internal::generic_into_buf(begin, end, value);
  }

  static constexpr size_t size_buffer(const bool &) noexcept { return 6; }
};


template<typename T> struct nullness<std::optional<T>>
{
  static constexpr bool has_null = true;
  static constexpr bool is_null(const std::optional<T> &v) noexcept
  {
    return ((not v.has_value()) or pqxx::is_null(*v));
  }
  static constexpr std::optional<T> null() { return std::optional<T>{}; }
};


template<typename T> struct string_traits<std::optional<T>>
{
  static char *into_buf(char *begin, char *end, const std::optional<T> &value)
  {
    return string_traits<T>::into_buf(begin, end, *value);
  }

  zview to_buf(char *begin, char *end, const std::optional<T> &value)
  {
    if (value.has_value())
      return to_buf(begin, end, *value);
    else
      return zview{};
  }

  static std::optional<T> from_string(std::string_view text)
  {
    return std::optional<T>{std::in_place,
                            string_traits<T>::from_string(text)};
  }

  static size_t size_buffer(const std::optional<T> &value)
  {
    return string_traits<T>::size_buffer(value.value());
  }
};


template<typename T> inline T from_string(const std::stringstream &text)
{
  return from_string<T>(text.str());
}


template<> struct string_traits<std::nullptr_t>
{
  static constexpr zview
  to_buf(char *, char *, const std::nullptr_t &) noexcept
  {
    return zview{};
  }
};


template<> struct nullness<const char *>
{
  static constexpr bool has_null = true;
  static constexpr bool is_null(const char *t) noexcept
  {
    return t == nullptr;
  }
  static constexpr const char *null() noexcept { return nullptr; }
};


/// String traits for C-style string ("pointer to const char").
template<> struct string_traits<const char *>
{
  static const char *from_string(std::string_view text) { return text.data(); }

  static zview to_buf(char *begin, char *end, const char *const &value)
  {
    if (value == nullptr)
      return zview{};
    char *const next{string_traits<const char *>::into_buf(begin, end, value)};
    // Don't count the trailing zero, even though into_buf does.
    return zview{begin, static_cast<size_t>(next - begin - 1)};
  }

  static char *into_buf(char *begin, char *end, const char *const &value)
  {
    const auto space = end - begin;
    // Count the trailing zero, even though std::strlen() and friends don't.
    const auto len =
      pqxx::internal::shortcut_strlen(value, static_cast<size_t>(space)) + 1;
    if (space < ptrdiff_t(len))
      throw conversion_overrun{
        "Could not copy string: buffer too small.  " +
        pqxx::internal::state_buffer_overrun(space, len)};
    std::memmove(begin, value, len);
    return begin + len;
  }

  static size_t size_buffer(const char *const &value)
  {
    return std::strlen(value) + 1;
  }
};


template<> struct nullness<char *>
{
  static constexpr bool has_null = true;
  static constexpr bool is_null(const char *t) { return t == nullptr; }
  static constexpr const char *null() { return nullptr; }
};


/// String traits for non-const C-style string ("pointer to char").
template<> struct string_traits<char *>
{
  static char *into_buf(char *begin, char *end, char *const &value)
  {
    return string_traits<const char *>::into_buf(begin, end, value);
  }
  static zview to_buf(char *begin, char *end, char *const &value)
  {
    return string_traits<const char *>::to_buf(begin, end, value);
  }
  static size_t size_buffer(char *const &value)
  {
    return string_traits<const char *>::size_buffer(value);
  }

  // Don't allow conversion to this type since it breaks const-safety.
};


template<size_t N> struct nullness<char[N]> : no_null<char[N]>
{};


/// String traits for C-style string constant ("array of char").
/** @warning This assumes that every array-of-char is a C-style string.  So,
 * it must include a trailing zero.
 */
template<size_t N> struct string_traits<char[N]>
{
  static char *into_buf(char *begin, char *end, const char (&value)[N])
  {
    if (static_cast<size_t>(end - begin) < size_buffer(value))
      throw conversion_overrun{
        "Could not convert char[] to string: too long for buffer."};
    std::memcpy(begin, value, N);
    return begin + N;
  }
  static constexpr size_t size_buffer(const char (&)[N]) noexcept { return N; }

  // Don't allow conversion to this type since it breaks const-safety.
};


template<> struct nullness<std::string> : no_null<std::string>
{};


template<> struct string_traits<std::string>
{
  static std::string from_string(std::string_view text)
  {
    return std::string{text};
  }

  static char *into_buf(char *begin, char *end, const std::string &value)
  {
    if (value.size() >= std::size_t(end - begin))
      throw conversion_overrun{
        "Could not convert string to string: too long for buffer."};
    // Include the trailing zero.
    const auto len = value.size() + 1;
    value.copy(begin, len);
    return begin + len;
  }

  static zview to_buf(char *begin, char *end, const std::string &value)
  {
    char *const next = into_buf(begin, end, value);
    // Don't count the trailing zero, even though into_buf() does.
    return zview{begin, static_cast<size_t>(next - begin - 1)};
  }

  static size_t size_buffer(const std::string &value)
  {
    return value.size() + 1;
  }
};


/// There's no real null for @c std::string_view.
/** I'm not sure how clear-cut this is: a @c string_view may have a null
 * data pointer, which is analogous to a null @c char pointer.
 */
template<> struct nullness<std::string_view> : no_null<std::string_view>
{};


/// String traits for `string_view`.
template<> struct string_traits<std::string_view>
{
  // Don't allow conversion to this type; it has nowhere to store its contents.

  static constexpr size_t size_buffer(const std::string_view &value) noexcept
  {
    return value.size() + 1;
  }
};


template<> struct nullness<zview> : no_null<zview>
{};


/// String traits for `zview`.
template<> struct string_traits<zview>
{
  // Don't allow conversion to this type; it has nowhere to store its contents.

  static constexpr size_t size_buffer(const std::string_view &value) noexcept
  {
    return value.size() + 1;
  }
};


template<> struct nullness<std::stringstream> : no_null<std::stringstream>
{};


template<> struct string_traits<std::stringstream>
{
  static std::stringstream from_string(std::string_view text)
  {
    std::stringstream stream;
    stream.write(text.data(), std::streamsize(text.size()));
    return stream;
  }
};


template<> struct nullness<std::nullptr_t>
{
  static constexpr bool has_null = true;
  static constexpr bool is_null(std::nullptr_t) noexcept { return true; }
  static constexpr std::nullptr_t null() { return nullptr; }
};


template<typename T> struct nullness<std::unique_ptr<T>>
{
  static constexpr bool has_null = true;
  static constexpr bool is_null(const std::unique_ptr<T> &t) noexcept
  {
    return not t or pqxx::is_null(*t);
  }
  static constexpr std::unique_ptr<T> null() { return std::unique_ptr<T>{}; }
};


template<typename T> struct string_traits<std::unique_ptr<T>>
{
  static std::unique_ptr<T> from_string(std::string_view text)
  {
    return std::make_unique<T>(string_traits<T>::from_string(text));
  }

  static char *
  into_buf(char *begin, char *end, const std::unique_ptr<T> &value)
  {
    return string_traits<T>::into_buf(begin, end, *value);
  }

  static zview to_buf(char *begin, char *end, const std::unique_ptr<T> &value)
  {
    if (value)
      return to_buf(begin, end, *value);
    else
      return zview{};
  }

  static size_t size_buffer(const std::unique_ptr<T> &value)
  {
    return string_traits<T>::size_buffer(*value.get());
  }
};


template<typename T> struct nullness<std::shared_ptr<T>>
{
  static constexpr bool has_null = true;
  static constexpr bool is_null(const std::shared_ptr<T> &t) noexcept
  {
    return not t or pqxx::is_null(*t);
  }
  static constexpr std::shared_ptr<T> null() { return std::shared_ptr<T>{}; }
};


template<typename T> struct string_traits<std::shared_ptr<T>>
{
  static std::shared_ptr<T> from_string(std::string_view text)
  {
    return std::make_shared<T>(string_traits<T>::from_string(text));
  }

  static zview to_buf(char *begin, char *end, const std::shared_ptr<T> &value)
  {
    return string_traits<T>::to_buf(begin, end, *value);
  }
  static char *
  into_buf(char *begin, char *end, const std::shared_ptr<T> &value)
  {
    return string_traits<T>::into_buf(begin, end, *value);
  }
  static size_t size_buffer(const std::shared_ptr<T> &value)
  {
    return string_traits<T>::size_buffer(*value);
  }
};
} // namespace pqxx


namespace pqxx::internal
{
/// String traits for SQL arrays.
template<typename Container> struct array_string_traits
{
  static zview to_buf(char *begin, char *end, const Container &value)
  {
    const auto stop{into_buf(begin, end, value)};
    return zview{begin, static_cast<size_t>(stop - begin - 1)};
  }

  static char *into_buf(char *begin, char *end, const Container &value)
  {
    const size_t budget{size_buffer(value)};
    if (static_cast<size_t>(end - begin) < budget)
      throw conversion_overrun{
        "Not enough buffer space to convert array to string."};

    char *here = begin;
    *here++ = '{';

    bool nonempty{false};
    for (const auto &elt : value)
    {
      if (nullness<elt_type>::is_null(elt))
      {
        s_null.copy(here, s_null.size());
        here += s_null.size();
      }
      else if constexpr (is_sql_array<elt_type>)
      {
        // Render nested array in-place.  Then erase the trailing zero.
        here = string_traits<elt_type>::into_buf(here, end, elt) - 1;
      }
      else
      {
        *here++ = '"';
        const auto text{to_string(elt)};
        for (const char c : text)
        {
          if (c == '\\' or c == '"')
            *here++ = '\\';
          *here++ = c;
        }
        *here++ = '"';
      }
      *here++ = array_separator<elt_type>;
      nonempty = true;
    }

    // Erase that last comma, if present.
    if (nonempty)
      here--;

    *here++ = '}';
    *here++ = '\0';

    return here;
  }

  static size_t size_buffer(const Container &value)
  {
    using elt_traits = string_traits<elt_type>;
    return 3 + std::accumulate(
                 std::begin(value), std::end(value), size_t{},
                 [](size_t acc, const elt_type &elt) {
                   // Opening and closing quotes, plus worst-case escaping, but
                   // don't count the trailing zeroes.
                   const size_t elt_size{nullness<elt_type>::is_null(elt) ?
                                           s_null.size() :
                                           elt_traits::size_buffer(elt) - 1};
                   return acc + 2 * elt_size + 2;
                 });
  }

  // We don't yet support parsing of array types using from_string.  Doing so
  // would require a reference to the connection.

private:
  using elt_type = std::remove_const_t<
    std::remove_reference_t<decltype(*std::declval<Container>().begin())>>;

  static constexpr zview s_null{"NULL"};
};
} // namespace pqxx::internal


namespace pqxx
{
template<typename T> struct nullness<std::vector<T>> : no_null<std::vector<T>>
{};


template<typename T>
struct string_traits<std::vector<T>>
        : internal::array_string_traits<std::vector<T>>
{};


template<typename T> inline constexpr bool is_sql_array<std::vector<T>>{true};


template<typename T, size_t N>
struct nullness<std::array<T, N>> : no_null<std::array<T, N>>
{};


template<typename T, size_t N>
struct string_traits<std::array<T, N>>
        : internal::array_string_traits<std::array<T, N>>
{};


template<typename T, size_t N>
inline constexpr bool is_sql_array<std::array<T, N>>{true};
} // namespace pqxx


namespace pqxx
{
template<typename T> inline std::string to_string(const T &value)
{
  if (is_null(value))
    throw conversion_error{"Attempt to convert null " + type_name<T> +
                           " to a string."};

  std::string buf;
  // We can't just reserve() data; modifying the terminating zero leads to
  // undefined behaviour.
  buf.resize(string_traits<T>::size_buffer(value) + 1);
  const auto end{
    string_traits<T>::into_buf(buf.data(), buf.data() + buf.size(), value)};
  buf.resize(static_cast<size_t>(end - buf.data() - 1));
  return buf;
}


template<> inline std::string to_string(const float &value)
{
  return internal::to_string_float(value);
}
template<> inline std::string to_string(const double &value)
{
  return internal::to_string_float(value);
}
template<> inline std::string to_string(const long double &value)
{
  return internal::to_string_float(value);
}
template<> inline std::string to_string(const std::stringstream &value)
{
  return value.str();
}


template<typename T> inline void into_string(const T &value, std::string &out)
{
  if (is_null(value))
    throw conversion_error{"Attempt to convert null " + type_name<T> +
                           " to a string."};

  // We can't just reserve() data; modifying the terminating zero leads to
  // undefined behaviour.
  out.resize(string_traits<T>::size_buffer(value) + 1);
  const auto end{
    string_traits<T>::into_buf(out.data(), out.data() + out.size(), value)};
  out.resize(static_cast<size_t>(end - out.data() - 1));
}
} // namespace pqxx
