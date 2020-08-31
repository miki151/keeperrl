#ifndef ProgramOptions_hxx_included
#define ProgramOptions_hxx_included

#include <string>
#include <cstring>
#include <algorithm>
#include <utility>
#include <iterator>
#include <cctype>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <vector>
#include <memory>
#include <unordered_map>
#include <type_traits>
#include <sstream>
#include <iostream>

#ifndef ProgramOptions_no_exceptions
	#include <stdexcept>
#endif // !ProgramOptions_no_exceptions

#ifndef NDEBUG
	#define ProgramOptions_debug
#endif // !NDEBUG

#undef ProgramOptions_assert
#ifdef NDEBUG
	#define ProgramOptions_assert( Expression, Message )
#else // NDEBUG
	#ifdef ProgramOptions_no_exceptions
		#define ProgramOptions_assert( Expression, Message ) assert( ( Expression ) && Message );
	#else // ProgramOptions_no_exceptions
		#define ProgramOptions_assert( Expression, Message )\
			do {\
				if( !( Expression ) )\
					throw std::logic_error{ ( "ProgramOptions.hxx:" + std::to_string( __LINE__ ) + ": " ) + ( Message ) };\
			} while( 0 );
	#endif // ProgramOptions_no_exceptions
#endif // NDEBUG

#ifndef ProgramOptions_no_colors
	#if defined( _WIN32 ) || defined( WIN32 )
		#define ProgramOptions_windows
		#ifndef WIN32_LEAN_AND_MEAN
			#define WIN32_LEAN_AND_MEAN 1
		#endif // !WIN32_LEAN_AND_MEAN
		// #define VC_EXTRALEAN
		#ifndef NOMINMAX
			#define NOMINMAX 1
		#endif // !NOMINMAX
		#ifndef STRICT
			#define STRICT 1
		#endif // !STRICT
		#include <windows.h>
	#else
		#define ProgramOptions_ansi
	#endif
#endif // !ProgramOptions_no_colors

namespace po {

	enum color_t {
		black			= 30,
		maroon			= 31,
		green			= 32,
		brown			= 33,
		navy			= 34,
		purple			= 35,
		teal			= 36,
		light_gray 		= 37,
		dark_gray		= -30,
		red				= -31,
		lime			= -32,
		yellow			= -33,
		blue			= -34,
		fuchsia			= -35,
		cyan			= -36,
		white		 	= -37
	};
#ifndef ProgramOptions_no_colors
	class color_resetter;
	color_resetter operator<<( std::ostream& stream, color_t color );
	class color_resetter {
		std::ostream& m_stream;

#ifdef ProgramOptions_windows
		HANDLE m_console;
		WORD m_old_attributes;
#endif // ProgramOptions_windows

		color_resetter( std::ostream& stream, color_t color )
                        : m_stream{ stream } {
#ifdef ProgramOptions_windows
			m_console = GetStdHandle( STD_OUTPUT_HANDLE );
			assert( m_console != INVALID_HANDLE_VALUE );
			CONSOLE_SCREEN_BUFFER_INFO info;
			const bool result = GetConsoleScreenBufferInfo( m_console, &info );
			assert( result );
			( void )result;
			m_old_attributes = info.wAttributes;
			WORD attribute = 0;
			if( color < 0 ) {
				color = static_cast< color_t >( -color );
				attribute |= FOREGROUND_INTENSITY;
			}
			if( color == maroon || color == brown || color == purple || color == light_gray )
				attribute |= FOREGROUND_RED;
			if( color == green || color == brown || color == teal || color == light_gray )
				attribute |= FOREGROUND_GREEN;
			if( color == navy || color == purple || color == teal || color == light_gray )
				attribute |= FOREGROUND_BLUE;
			SetConsoleTextAttribute( m_console, attribute );
#endif // ProgramOptions_windows
#ifdef ProgramOptions_ansi
			m_stream << "\x1B[";
			if( color < 0 ) {
				color = static_cast< color_t >( -color );
				m_stream << "1;";
			}
			m_stream << static_cast< int >( color ) << 'm';
#endif // ProgramOptions_ansi
		}

	public:
		~color_resetter() {
#ifdef ProgramOptions_windows
			SetConsoleTextAttribute( m_console, m_old_attributes );
#endif // ProgramOptions_windows
#ifdef ProgramOptions_ansi
			m_stream << "\x1B[0m";
#endif // ProgramOptions_ansi
		}

		operator std::ostream&() const {
			return m_stream;
		}
		template< typename T >
		std::ostream& operator<<( T&& arg ) {
			return m_stream << std::forward< T >( arg );
		}

		friend color_resetter operator<<( std::ostream& stream, color_t color );
	};
	inline color_resetter operator<<( std::ostream& stream, color_t color ) {
		return { stream, color };
	}
#else // !ProgramOptions_no_colors
	inline std::ostream& operator<<( std::ostream& stream, color_t /* color */ ) { // -Wunused-parameter
		return stream;
	}
#endif // !ProgramOptions_no_colors

#ifdef ProgramOptions_silent
	inline std::ostream& err() {
		struct dummy_buf_t : public std::streambuf {
			int overflow( int c ) {
				return c;
			}
		};
		static dummy_buf_t dummy_buf;
		static std::ostream dummy_str{ &dummy_buf };
		return dummy_str;
	}
#else // ProgramOptions_silent
	inline std::ostream& err() {
		return std::cerr;
	}
#endif // ProgramOptions_silent

	// Compatibility stuff for the lack of C++14 support
	template< typename T, typename... args_t >
	std::unique_ptr< T > make_unique( args_t&&... args ) {
		return std::unique_ptr< T >{ new T{ std::forward< args_t >( args )... } };
	}

	template< typename T, T... i >
	struct integer_sequence {
		using value_type = T;
		static constexpr std::size_t size = sizeof...( i );
	};
	namespace detail {
		template< typename T, T i, bool _0 = ( i == 0 ), bool _1 = ( i == 1 ), typename = typename std::enable_if< ( i >= 0 ) >::type >
		struct make_integer_sequence_impl {
			template< typename = typename make_integer_sequence_impl< T, i / 2 >::type, typename = typename make_integer_sequence_impl< T, i % 2 >::type >
			struct helper {
			};
			template< T... j, T... k >
			struct helper< integer_sequence< T, j... >, integer_sequence< T, k... > > {
				using type = integer_sequence< T, j..., ( j + i / 2 )..., ( k + i - 1 )... >;
			};
			using type = typename helper<>::type;
		};
		template< typename T, T i >
		struct make_integer_sequence_impl< T, i, true, false > {
			using type = integer_sequence< T >;
		};
		template< typename T, T i >
		struct make_integer_sequence_impl< T, i, false, true > {
			using type = integer_sequence< T, 0 >;
		};
	}
	template< typename T, T N >
	using make_integer_sequence = typename detail::make_integer_sequence_impl< T, N >::type;
	template< std::size_t N >
	using make_index_sequence = make_integer_sequence< std::size_t, N >;
	// End of compatibility stuff

	class repeat {
		std::size_t m_count;
		char m_character;

	public:
		explicit repeat( std::size_t count, char character )
			: m_count{ count }, m_character{ character } {
		}

		friend std::ostream& operator<<( std::ostream& stream, repeat const& object ) {
			std::fill_n( std::ostreambuf_iterator< char >( stream ), object.m_count, object.m_character );
			return stream;
		}
	};

	template< typename enum_t >
	typename std::underlying_type< enum_t >::type enum2int( enum_t value ) {
		return static_cast< typename std::underlying_type< enum_t >::type >( value );
	}

	enum value_type {
		void_,
		string,
		i32,
		i64,
		u32,
		u64,
		f32,
		f64
	};
	static char const* value_type_strings[] = {
		"void",
		"string",
		"i32",
		"i64",
		"u32",
		"u64",
		"f32",
		"f64"
	};
	inline char const* vt2str( value_type type ) {
		return value_type_strings[ enum2int( type ) ];
	}
	namespace detail {
		template< value_type type >
		struct vt2type_impl {
		};
		template<>
		struct vt2type_impl< void_ > {	using type = void;			};
		template<>
		struct vt2type_impl< string > {	using type = std::string;	};
		template<>
		struct vt2type_impl< i32 > {	using type = std::int32_t;	};
		template<>
		struct vt2type_impl< i64 > {	using type = std::int64_t;	};
		template<>
		struct vt2type_impl< u32 > {	using type = std::uint32_t;	};
		template<>
		struct vt2type_impl< u64 > {	using type = std::uint64_t;	};
		template<>
		struct vt2type_impl< f32 > {	using type = float;			};
		template<>
		struct vt2type_impl< f64 > {	using type = double;		};
	}
	template< value_type type >
	using vt2type = typename detail::vt2type_impl< type >::type;

	using void_t = vt2type< void_ >;
	using string_t = vt2type< string >;
	using i32_t = vt2type< i32 >;
	using i64_t = vt2type< i64 >;
	using u32_t = vt2type< u32 >;
	using u64_t = vt2type< u64 >;
	using f32_t = vt2type< f32 >;
	using f64_t = vt2type< f64 >;

	enum class error_code {
		none,
		argument_expected,
		no_argument_expected,
		conversion_error,
		out_of_range
	};

	template< typename T >
	T pow( T base, unsigned exp ) {
		if( exp == 0 )
			return 1;
		T result = pow( base, exp / 2 );
		result *= result;
		if( exp % 2 )
			result *= base;
		return result;
	}
	template< typename T >
	T pow( T base, int exp ) {
		const T result = pow( base, static_cast< unsigned >( std::abs( exp ) ) );
		return exp >= 0 ? result : T{ 1 } / result;
	}

	template< typename T, unsigned i >
	struct pow10 : public std::integral_constant< T, 10 * pow10< T, i - 1 >::value > {
	};
	template< typename T >
	struct pow10< T, 0 > : public std::integral_constant< T, 1 > {
	};

	template< unsigned i, bool = ( i < 10 ) >
	struct log10 : public std::integral_constant< unsigned, 1 + log10< i / 10 >::value > {
	};
	template<>
	struct log10< 0, true >;
	template< unsigned i >
	struct log10< i, true > : public std::integral_constant< unsigned, 0 > {
	};

	template< typename T >
	struct parsing_report {
		error_code error = error_code::none;
		const T value{};

		parsing_report() = default;
		parsing_report( error_code error )
			: error{ error } {
		}
		parsing_report( T const& value )
			: value{ value } {
		}
		parsing_report( T&& value )
			: value{ std::move( value ) } {
		}

		bool good() const {
			return error == error_code::none;
		}
		bool operator!() const {
			return !good();
		}

		operator T const&() const {
			ProgramOptions_assert( good(), "parsing_report: cannot access data of an erroneous report" );
			return value;
		}

		/*
		template< typename U >
		bool operator==( U const& rhs ) const {
			return good() && value == rhs;
		}
		template< typename U >
		bool operator!=( U const& rhs ) const {
			return good() && value != rhs;
		}
		template< typename U >
		bool operator<( U const& rhs ) const {
			return good() && value < rhs;
		}
		template< typename U >
		bool operator<=( U const& rhs ) const {
			return good() && value <= rhs;
		}
		template< typename U >
		bool operator>( U const& rhs ) const {
			return good() && value > rhs;
		}
		template< typename U >
		bool operator>=( U const& rhs ) const {
			return good() && value >= rhs;
		}
		*/
	};

	inline bool is_bin_digit( char c ) {
		return c == '0' || c == '1';
	}
	inline bool is_digit( char c ) {
		// return std::isdigit( c );
		return '0' <= c && c <= '9';
	}
	inline bool is_hex_digit( char c ) {
		// return ( '0' <= c && c <= '9' ) || ( 'a' <= c && c <= 'f' ) || ( 'A' <= c && c <= 'F' );
		return std::isxdigit( c );
	}

	inline int get_bin_digit( char c ) {
		if( is_bin_digit( c ) )
			return c - '0';
		else
			return -1;
	}
	inline int get_digit( char c ) {
		if( is_digit( c ) )
			return c - '0';
		else
			return -1;
	}
	inline int get_hex_digit( char c ) {
		if( '0' <= c && c <= '9' )
			return c - '0';
		else if( 'a' <= c && c <= 'f' )
			return 10 + c - 'a';
		else if( 'A' <= c && c <= 'F' )
			return 10 + c - 'A';
		else
			return -1;
	}

	namespace detail {
		template< typename forward_iterator_t >
		bool expect_impl( forward_iterator_t /* first */, forward_iterator_t /* last */ ) { // -Wunused-parameter
			return true;
		}
		template< typename forward_iterator_t, typename... tail_t >
		bool expect_impl( forward_iterator_t& first, forward_iterator_t last, char head, tail_t const&... tail ) {
			return first != last && std::tolower( *first ) == std::tolower( head ) && expect_impl( ++first, last, tail... );
		}
	}
	template< typename forward_iterator_t, typename... args_t >
	bool expect( forward_iterator_t& first, forward_iterator_t last, args_t&&... args ) {
		forward_iterator_t first_copy{ first };
		const bool result = detail::expect_impl( first_copy, last, std::forward< args_t >( args )... );
		if( result )
			first = first_copy;
		return result;
	}
	namespace detail {
		template< typename forward_iterator_t, std::size_t N, std::size_t... indices >
		bool expect_unpacker( forward_iterator_t& first, forward_iterator_t last, const char( &args )[ N ], integer_sequence< std::size_t, indices... > ) {
			return expect( first, last, args[ indices ]... );
		}
	}
	template< typename forward_iterator_t, std::size_t N >
	bool expect( forward_iterator_t& first, forward_iterator_t last, const char( &args )[ N ] ) {
		return detail::expect_unpacker( first, last, args, make_index_sequence< N - 1 >{} );
	}

	template< typename T, typename forward_iterator_t, typename = typename std::enable_if< std::numeric_limits< T >::is_integer && !std::numeric_limits< T >::is_signed >::type >
	parsing_report< T > str2uint( forward_iterator_t first, forward_iterator_t last ) {
		static_assert( std::numeric_limits< T >::digits % 4 == 0, "platform doesn't meet requirements" );

		for( ; first != last && std::isspace( *first ); ++first );
		expect( first, last, '+' );
		if( first == last || !is_digit( *first ) )
			return error_code::conversion_error;
		for( ; first != last && *first == '0'; ++first );
		T result{};
		if( first == last )
			return result;
		if( expect( first, last, 'x' ) ) {
			if( first == last || !is_hex_digit( *first ) )
				return error_code::conversion_error;
			for( ; first != last && *first == '0'; ++first );
			std::size_t digits = 0;
			enum : std::size_t {
				max_digits = std::numeric_limits< T >::digits / 4
			};
			for( int d{}; first != last && digits <= max_digits && ( d = get_hex_digit( *first ) ) >= 0; ++first, ++digits )
				( result <<= 4 ) |= d;
			if( digits > max_digits )
				return error_code::out_of_range;
			// TODO: exponents for hex ints with pP?
		} else if( expect( first, last, 'b' ) ) {
			if( first == last || !is_bin_digit( *first ) )
				return error_code::conversion_error;
			for( ; first != last && *first == '0'; ++first );
			std::size_t digits = 0;
			enum : std::size_t {
				max_digits = std::numeric_limits< T >::digits
			};
			for( int d{}; first != last && digits <= max_digits && ( d = get_bin_digit( *first ) ) >= 0; ++first, ++digits )
				( result <<= 1 ) |= d;
			if( digits > max_digits )
				return error_code::out_of_range;
		} else {
			std::size_t decimals = 0;
			enum : std::size_t {
				max_decimals = 1 + std::numeric_limits< T >::digits10
			};
			for( int d{}; first != last && decimals <= max_decimals && ( d = get_digit( *first ) ) >= 0; ++first, ++decimals )
				result = 10 * result + static_cast< T >( d );
			if( decimals >= max_decimals )
				if( decimals > max_decimals || ( result < pow10< T, max_decimals - 1 >::value ) )
					return error_code::out_of_range;
			if( expect( first, last, 'e' ) ) {
				expect( first, last, '+' );
				if( first == last || !is_digit( *first ) )
					return error_code::conversion_error;
				for( ; first != last && *first == '0'; ++first );
				unsigned exp{};
				std::size_t decimals = 0;
				enum : std::size_t {
					max = std::numeric_limits< T >::digits10,
					max_decimals = log10< max >::value + 1
				};
				for( int d{}; first != last && decimals <= max_decimals && ( d = get_digit( *first ) ) >= 0; ++first, ++decimals )
					exp = 10 * exp + static_cast< unsigned >( d );
				if( decimals >= max_decimals )
					if( decimals > max_decimals || exp > max )
						return error_code::out_of_range;
				const T fac = pow( T{ 10 }, exp );
				const T mant = result;
				result *= fac;
				if( result / fac != mant )
					return error_code::out_of_range;
			}
		}
		for( ; first != last && std::isspace( *first ); ++first );
		if( first != last )
			return error_code::conversion_error;
		return result;
	}
	template< typename T >
	parsing_report< T > str2uint( std::string const& str ) {
		return str2uint< T >( str.begin(), str.end() );
	}
	template< typename T >
	parsing_report< T > str2uint( char const* str ) {
		return str2uint< T >( str, str + std::strlen( str ) );
	}

	template< typename T, typename forward_iterator_t, typename = typename std::enable_if< std::numeric_limits< T >::is_integer && std::numeric_limits< T >::is_signed >::type >
	parsing_report< T > str2int( forward_iterator_t first, forward_iterator_t last ) {
		for( ; first != last && std::isspace( *first ); ++first );
		const bool neg = expect( first, last, '-' );
		if( !neg )
			expect( first, last, '+' );
		if( first == last || !is_digit( *first ) )
			return error_code::conversion_error;
		using unsigned_t = typename std::make_unsigned< T >::type;
		const auto report = str2uint< unsigned_t >( first, last );
		if( !report )
			return report.error;
		unsigned_t max = neg ? static_cast< unsigned_t >( -std::numeric_limits< T >::min() ) : static_cast< unsigned_t >( std::numeric_limits< T >::max() );
		if( report > max )
			return error_code::out_of_range;
		return neg ? -static_cast< T >( report ) : static_cast< T >( report );
	}
	template< typename T >
	parsing_report< T > str2int( std::string const& str ) {
		return str2int< T >( str.begin(), str.end() );
	}
	template< typename T >
	parsing_report< T > str2int( char const* str ) {
		return str2int< T >( str, str + std::strlen( str ) );
	}

	template< typename T, typename forward_iterator_t, typename = typename std::enable_if< std::is_floating_point< T >::value >::type >
	parsing_report< T > str2flt( forward_iterator_t first, forward_iterator_t last ) {
		static_assert( std::numeric_limits< T >::is_iec559, "platform doesn't meet requirements" );
		static_assert( std::numeric_limits< T >::has_quiet_NaN, "type insufficient; doesn't support quiet NaNs" );
		static_assert( std::numeric_limits< T >::has_infinity, "type insufficient; doesn't support infinities" );

		for( ; first != last && std::isspace( *first ); ++first );
		const bool neg = expect( first, last, '-' );
		if( !neg )
			expect( first, last, '+' );
		T result{};
		if( expect( first, last, "nan" ) ) {
			result = std::numeric_limits< T >::quiet_NaN();
		} else if( expect( first, last, "infinity" )
				|| expect( first, last, "inf" ) ) {
			result = std::numeric_limits< T >::infinity();
		} else {
			// TODO: support for hex floats
			bool valid = false;
			if( first != last )
				valid |= is_digit( *first );
			for( int d{}; first != last && ( d = get_digit( *first ) ) >= 0; ++first )
				result = 10 * result + static_cast< T >( d );
			if( expect( first, last, '.' ) ) {
				if( first != last )
					valid |= is_digit( *first );
				T place = 1;
				for( int d{}; first != last && ( d = get_digit( *first ) ) >= 0; ++first )
					result += static_cast< T >( d ) * ( place /= 10 );
			}
			if( !valid )
				return error_code::conversion_error;
			if( expect( first, last, 'e' ) ) {
				const bool neg = expect( first, last, '-' );
				if( !neg )
					expect( first, last, '+' );
				if( first == last || !is_digit( *first ) )
					return error_code::conversion_error;
				int exp{};
				for( int d{}; first != last && ( d = get_digit( *first ) ) >= 0; ++first )
					exp = 10 * exp + static_cast< int >( d );
				if( neg )
					exp = -exp;
				result *= std::pow( T{ 10 }, exp );
			}
		}
		for( ; first != last && std::isspace( *first ); ++first );
		if( first != last )
			return error_code::conversion_error;
		return neg ? -result : result;
	}
	template< typename T >
	parsing_report< T > str2flt( std::string const& str ) {
		return str2flt< T >( str.begin(), str.end() );
	}
	template< typename T >
	parsing_report< T > str2flt( char const* str ) {
		return str2flt< T >( str, str + std::strlen( str ) );
	}

	template< typename T, typename = typename std::enable_if< std::numeric_limits< T >::is_integer >::type >
	std::string int2str( T const& value ) {
		return std::to_string( value );
	}
	template< typename T, typename = typename std::enable_if< !std::numeric_limits< T >::is_integer >::type >
	std::string flt2str( T const& value ) {
		// TODO: provide a more efficient implementation
		std::ostringstream ostrs;
		ostrs << value;
		return ostrs.str();
	}

	struct value {
		string_t string;
		union {
			i32_t i32;
			i64_t i64;
			u32_t u32;
			u64_t u64;
			f32_t f32;
			f64_t f64;
		};

		value() = default;
		explicit value( string_t const& object )
			: string{ object } {
		}
		explicit value( string_t&& object )
			: string{ std::move( object ) } {
		}
	};

	struct value_vector_base {
		virtual ~value_vector_base() = default;

		virtual std::size_t size() const = 0;

		virtual value const* begin() const = 0;
		value* begin() {
			return const_cast< value* >( static_cast< value_vector_base const& >( *this ).begin() );
		}

		value const* end() const {
			return begin() + size();
		}
		value* end() {
			return const_cast< value* >( static_cast< value_vector_base const& >( *this ).end() );
		}

		value const& get() const {
			ProgramOptions_assert( begin() != nullptr, "value_vector_base: cannot access elements of a void_ vector" );
			ProgramOptions_assert( size() != 0, "value_vector_base: cannot access elements of an empty vector" );
			return begin()[ size() - 1 ];
		}
		value& get() {
			return const_cast< value& >( static_cast< value_vector_base const& >( *this ).get() );
		}

		virtual void push_back( value const& object ) = 0;
	};

	template< bool is_void, bool is_single >
	class value_vector;
	template<>
	class value_vector< false, false > final : public value_vector_base {
		std::vector< value > m_values;

	public:
		virtual std::size_t size() const override {
			return m_values.size();
		}
		virtual value const* begin() const override {
			return &*m_values.begin();
		}
		virtual void push_back( value const& object ) override {
			m_values.push_back( object );
		}
	};
	template<>
	class value_vector< false, true > final : public value_vector_base {
		bool m_set = false;
		value m_value;

	public:
		virtual std::size_t size() const override {
			return m_set ? 1 : 0;
		}
		virtual value const* begin() const override {
			return &m_value;
		}
		virtual void push_back( value const& object ) override {
			m_value = object;
			m_set = true;
		}
	};
	template<>
	class value_vector< true, false > final : public value_vector_base {
		std::size_t m_counter = 0;

	public:
		virtual std::size_t size() const override {
			return m_counter;
		}
		virtual value const* begin() const override {
			return nullptr;
		}
		virtual void push_back( value const& /* object */ ) override { // -Wunused-parameter
			++m_counter;
		}
	};
	template<>
	class value_vector< true, true > final : public value_vector_base {
		bool m_set = false;

	public:
		virtual std::size_t size() const override {
			return m_set ? 1 : 0;
		}
		virtual value const* begin() const override {
			return nullptr;
		}
		virtual void push_back( value const& /* object */ ) override { // -Wunused-parameter
			m_set = true;
		}
	};

	namespace detail {
		template< typename fun_t, typename... args_t >
		class is_invocable_sfinae {
			using false_t = char[ 1 ];
			using true_t = char[ 2 ];

			template< typename _fun_t, typename... _args_t >
			static false_t& test( ... );
			template< typename _fun_t, typename... _args_t >
			static true_t& test( decltype( std::declval< _fun_t >()( std::declval< _args_t >()... ) )* );

		public:
			static constexpr bool value = sizeof( test< fun_t, args_t... >( 0 ) ) == sizeof( true_t );
		};
	}
	template< typename fun_t, typename... args_t >
	struct is_invocable : public std::integral_constant< bool, detail::is_invocable_sfinae< fun_t, args_t... >::value > {
	};

	struct callback_base {
		virtual ~callback_base() = default;
		virtual void invoke( value const& object ) = 0;
#ifdef ProgramOptions_debug
		virtual bool good() const {
			return true;
		}
#endif // ProgramOptions_debug
	};

	template< typename invocable_t >
	class callback_storage : public callback_base {
	protected:
		typename std::remove_const< typename std::remove_reference< invocable_t >::type >::type m_invocable;

	public:
		template< typename... args_t >
		explicit callback_storage( args_t&&... args )
			: m_invocable{ std::forward< args_t >( args )... } {
		}
	};

	template<
		value_type type,
		typename invocable_t,
		bool with_void = is_invocable< invocable_t >::value,
		bool with_string = is_invocable< invocable_t, string_t >::value,
		bool with_type = is_invocable< invocable_t, vt2type< type > >::value
	>
	struct callback : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& /* object */ ) override { // -Wunused-parameter
			ProgramOptions_assert( false, "callback: incompatible parameter type" );
		}
#ifdef ProgramOptions_debug
		virtual bool good() const override {
			return false;
		}
#endif // ProgramOptions_debug
	};
	template< typename invocable_t, bool with_string, bool with_type >
	struct callback< void_, invocable_t, true, with_string, with_type > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& /* object */ ) override { // -Wunused-parameter
			this->m_invocable();
		}
	};
	template< typename invocable_t, bool with_void, bool with_string >
	struct callback< string, invocable_t, with_void, with_string, true > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& object ) override {
			this->m_invocable( object.string );
		}
	};
	template< typename invocable_t, bool with_string >
	struct callback< string, invocable_t, true, with_string, false > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& /* object */ ) override { // -Wunused-parameter
			this->m_invocable();
		}
	};
	template< typename invocable_t, bool with_void, bool with_string >
	struct callback< i32, invocable_t, with_void, with_string, true > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& object ) override {
			this->m_invocable( object.i32 );
		}
	};
	template< typename invocable_t, bool with_void >
	struct callback< i32, invocable_t, with_void, true, false > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& object ) override {
			this->m_invocable( object.string );
		}
	};
	template< typename invocable_t >
	struct callback< i32, invocable_t, true, false, false > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& /* object */ ) override { // -Wunused-parameter
			this->m_invocable();
		}
	};
	template< typename invocable_t, bool with_void, bool with_string >
	struct callback< i64, invocable_t, with_void, with_string, true > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& object ) override {
			this->m_invocable( object.i64 );
		}
	};
	template< typename invocable_t, bool with_void >
	struct callback< i64, invocable_t, with_void, true, false > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& object ) override {
			this->m_invocable( object.string );
		}
	};
	template< typename invocable_t >
	struct callback< i64, invocable_t, true, false, false > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& /* object */ ) override { // -Wunused-parameter
			this->m_invocable();
		}
	};
	template< typename invocable_t, bool with_void, bool with_string >
	struct callback< u32, invocable_t, with_void, with_string, true > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& object ) override {
			this->m_invocable( object.u32 );
		}
	};
	template< typename invocable_t, bool with_void >
	struct callback< u32, invocable_t, with_void, true, false > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& object ) override {
			this->m_invocable( object.string );
		}
	};
	template< typename invocable_t >
	struct callback< u32, invocable_t, true, false, false > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& /* object */ ) override { // -Wunused-parameter
			this->m_invocable();
		}
	};
	template< typename invocable_t, bool with_void, bool with_string >
	struct callback< u64, invocable_t, with_void, with_string, true > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& object ) override {
			this->m_invocable( object.u64 );
		}
	};
	template< typename invocable_t, bool with_void >
	struct callback< u64, invocable_t, with_void, true, false > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& object ) override {
			this->m_invocable( object.string );
		}
	};
	template< typename invocable_t >
	struct callback< u64, invocable_t, true, false, false > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& /* object */ ) override { // -Wunused-parameter
			this->m_invocable();
		}
	};
	template< typename invocable_t, bool with_void, bool with_string >
	struct callback< f32, invocable_t, with_void, with_string, true > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& object ) override {
			this->m_invocable( object.f32 );
		}
	};
	template< typename invocable_t, bool with_void >
	struct callback< f32, invocable_t, with_void, true, false > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& object ) override {
			this->m_invocable( object.string );
		}
	};
	template< typename invocable_t >
	struct callback< f32, invocable_t, true, false, false > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& /* object */ ) override { // -Wunused-parameter
			this->m_invocable();
		}
	};
	template< typename invocable_t, bool with_void, bool with_string >
	struct callback< f64, invocable_t, with_void, with_string, true > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& object ) override {
			this->m_invocable( object.f64 );
		}
	};
	template< typename invocable_t, bool with_void >
	struct callback< f64, invocable_t, with_void, true, false > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& object ) override {
			this->m_invocable( object.string );
		}
	};
	template< typename invocable_t >
	struct callback< f64, invocable_t, true, false, false > : public callback_storage< invocable_t > {
		using callback_storage< invocable_t >::callback_storage;
		virtual void invoke( value const& /* object */ ) override { // -Wunused-parameter
			this->m_invocable();
		}
	};

	template< value_type type, typename underlying_t >
	class value_iterator {
		static_assert( type != void_, "iterating over voids disallowed" );
		static_assert( std::is_same< typename std::iterator_traits< underlying_t >::iterator_category, std::random_access_iterator_tag >::value, "underlying iterator incompatible" );
		underlying_t m_underlying;

	public:
		using difference_type = typename std::iterator_traits< underlying_t >::difference_type;
		using value_type = vt2type< type >;
		using pointer = value_type const*;
		using reference = value_type const&;
		using iterator_category = std::random_access_iterator_tag;

	private:
		reference deref( std::integral_constant< ::po::value_type, string > ) const {
			return m_underlying->string;
		}
		reference deref( std::integral_constant< ::po::value_type, i32 > ) const {
			return m_underlying->i32;
		}
		reference deref( std::integral_constant< ::po::value_type, i64 > ) const {
			return m_underlying->i64;
		}
		reference deref( std::integral_constant< ::po::value_type, u32 > ) const {
			return m_underlying->u32;
		}
		reference deref( std::integral_constant< ::po::value_type, u64 > ) const {
			return m_underlying->u64;
		}
		reference deref( std::integral_constant< ::po::value_type, f32 > ) const {
			return m_underlying->f32;
		}
		reference deref( std::integral_constant< ::po::value_type, f64 > ) const {
			return m_underlying->f64;
		}

	public:
		value_iterator() {
		}
		explicit value_iterator( underlying_t const& underlying )
			: m_underlying{ underlying } {
		}

		reference operator*() const {
			return deref( std::integral_constant< ::po::value_type, type >{} );
		}
		pointer operator->() const {
			return &operator*();
		}

		bool operator==( value_iterator const& rhs ) const {
			return m_underlying == rhs.m_underlying;
		}
		bool operator!=( value_iterator const& rhs ) const {
			return m_underlying != rhs.m_underlying;
		}
		bool operator<( value_iterator const& rhs ) const {
			return m_underlying < rhs.m_underlying;
		}
		bool operator<=( value_iterator const& rhs ) const {
			return m_underlying <= rhs.m_underlying;
		}
		bool operator>( value_iterator const& rhs ) const {
			return m_underlying > rhs.m_underlying;
		}
		bool operator>=( value_iterator const& rhs ) const {
			return m_underlying >= rhs.m_underlying;
		}

		value_iterator& operator++() {
			++m_underlying;
			return *this;
		}
		value_iterator operator++( int ) {
			value_iterator result{ *this };
			++*this;
			return result;
		}
		value_iterator& operator--() {
			--m_underlying;
			return *this;
		}
		value_iterator operator--( int ) {
			value_iterator result{ *this };
			--*this;
			return result;
		}

		value_iterator& operator+=( difference_type n ) {
			m_underlying += n;
			return *this;
		}
		value_iterator& operator-=( difference_type n ) {
			m_underlying -= n;
			return *this;
		}

		difference_type operator-( value_iterator const& rhs ) const {
			return m_underlying - rhs.m_underlying;
		}

		reference operator[]( difference_type n ) const {
			return *( *this + n );
		}
	};

	template< value_type type, typename underlying_t >
	value_iterator< type, underlying_t > operator+( value_iterator< type, underlying_t > object, typename value_iterator< type, underlying_t >::difference_type n ) {
		return object += n;
	}
	template< value_type type, typename underlying_t >
	value_iterator< type, underlying_t > operator+( typename value_iterator< type, underlying_t >::difference_type n, value_iterator< type, underlying_t > object ) {
		return object += n;
	}
	template< value_type type, typename underlying_t >
	value_iterator< type, underlying_t > operator-( value_iterator< type, underlying_t > object, typename value_iterator< type, underlying_t >::difference_type n ) {
		return object -= n;
	}

	class option {
		char m_abbreviation = '\0';
		std::string m_description;
		value_type m_type = void_;
		bool m_multi = false;

		std::unique_ptr< value_vector_base > m_fallback;
		std::unique_ptr< value_vector_base > m_data;

		std::vector< std::unique_ptr< callback_base > > m_callbacks;

#ifdef ProgramOptions_debug
		bool m_mutable = true;

	public:
		void make_immutable() {
			m_mutable = false;
		}

	private:
		void mutable_operation() const {
			ProgramOptions_assert( m_mutable, "cannot change options after parsing" );
		}
#endif // ProgramOptions_debug

		value_vector_base& get_vector() const {
			ProgramOptions_assert( m_data != nullptr || m_fallback != nullptr, "cannot access an option with neither user set value nor fallback" );
			if( m_data != nullptr )
				return *m_data;
			else
				return *m_fallback;
		}

		parsing_report< value > make_value() const {
			if( get_type() == void_ )
				return {};
			else
				return error_code::argument_expected;
		}
		parsing_report< value > make_value( std::string&& str ) const {
			if( get_type() == void_ ) {
				if( str.empty() )
					return {};
				else
					return error_code::no_argument_expected;
			}
			value result;
			switch( get_type() ) {
			case i32: {
					const auto report = str2int< i32_t >( str );
					if( !report )
						return report.error;
					result.i32 = report;
				}
				break;
			case i64: {
					const auto report = str2int< i64_t >( str );
					if( !report )
						return report.error;
					result.i64 = report;
				}
				break;
			case u32: {
					const auto report = str2uint< u32_t >( str );
					if( !report )
						return report.error;
					result.u32 = report;
				}
				break;
			case u64: {
					const auto report = str2uint< u64_t >( str );
					if( !report )
						return report.error;
					result.u64 = report;
				}
				break;
			case f32: {
					const auto report = str2flt< f32_t >( str );
					if( !report )
						return report.error;
					result.f32 = report;
				}
				break;
			case f64: {
					const auto report = str2flt< f64_t >( str );
					if( !report )
						return report.error;
					result.f64 = report;
				}
				break;
			default: // -Wswitch
				;
			}
			result.string = std::move( str );
			return result;
		}
		parsing_report< value > make_value( std::string const& str ) const {
			return make_value( std::string{ str } );
		}
		template< typename T >
		parsing_report< value > make_value( T const& integer, typename std::enable_if< std::is_integral< T >::value >::type* = nullptr ) const {
			value result;
			bool out_of_range = false;
			switch( get_type() ) {
			case string:
				break;
			case i32:
				result.i32 = static_cast< i32_t >( integer );
				out_of_range = ( static_cast< T >( result.i32 ) != integer );
				break;
			case i64:
				result.i64 = static_cast< i64_t >( integer );
				out_of_range = ( static_cast< T >( result.i64 ) != integer );
				break;
			case u32:
				result.u32 = static_cast< u32_t >( integer );
				out_of_range = ( static_cast< T >( result.u32 ) != integer );
				break;
			case u64:
				result.u64 = static_cast< u64_t >( integer );
				out_of_range = ( static_cast< T >( result.u64 ) != integer );
				break;
			case f32:
				result.f32 = static_cast< f32_t >( integer );
				break;
			case f64:
				result.f64 = static_cast< f64_t >( integer );
				break;
			default:
				return error_code::conversion_error;
			}
			if( out_of_range )
				return error_code::out_of_range;
			result.string = int2str( integer );
			return result;
		}
		template< typename T >
		parsing_report< value > make_value( T const& floating, typename std::enable_if< std::is_floating_point< T >::value >::type* = nullptr ) const {
			value result;
			switch( get_type() ) {
			case string:
				break;
			case f32:
				result.f32 = static_cast< f32_t >( floating );
				break;
			case f64:
				result.f64 = static_cast< f64_t >( floating );
				break;
			default:
				return error_code::conversion_error;
			}
			result.string = flt2str( floating );
			return result;
		}

		template< value_type type >
		bool valid_iterator_type() const {
			return type == string || type == get_type();

		}
		template< value_type type >
		void assert_iterator_type() const {
			ProgramOptions_assert( valid_iterator_type< type >(), "" );
		}

		void notify( value const& object ) {
			for( auto&& i : m_callbacks )
				i->invoke( object );
		}

	public:
		// using value_type = value; // there's another crucial type called value_type...
		using const_iterator = value const*;
		using iterator = const_iterator;
		using const_reverse_iterator = std::reverse_iterator< const_iterator >;
		using reverse_iterator = const_reverse_iterator;

		bool was_set() const {
			return m_data != nullptr;
		}
		bool available() const {
			return m_data != nullptr || m_fallback != nullptr;
		}

		std::size_t size() const {
			if( available() )
				return get_vector().size();
			else
				return 0;
		}
		std::size_t count() const {
			return size();
		}

		value const& get() const {
			ProgramOptions_assert( available(), "get: option unavailable" );
			return get_vector().get();
		}
		value const& get( std::size_t i ) const {
			ProgramOptions_assert( available(), "get: option unavailable" );
			ProgramOptions_assert( i < size(), "get: index out of range" );
			return begin()[ i ];
		}
		// TODO: strictly speaking, this is implementation defined
		value const& get_or( value const& def ) const {
			if( available() )
				return get();
			else
				return def;
		}
		value const& get_or( value const& def, std::size_t i ) const {
			if( i < size() )
				return get( i );
			else
				return def;
		}

		iterator begin() const {
			if( available() )
				return get_vector().begin();
			else
				return nullptr;
		}
		const_iterator cbegin() const {
			return begin();
		}
		reverse_iterator rbegin() const {
			return reverse_iterator{ begin() };
		}
		const_reverse_iterator crbegin() const {
			return rbegin();
		}

		iterator end() const {
			if( available() )
				return get_vector().end();
			else
				return nullptr;
		}
		const_iterator cend() const {
			return end();
		}
		reverse_iterator rend() const {
			return reverse_iterator{ end() };
		}
		const_reverse_iterator crend() const {
			return rend();
		}

		template< value_type type >
		value_iterator< type, iterator > begin() const {
			assert_iterator_type< type >();
			return value_iterator< type, iterator >{ begin() };
		}
		template< value_type type >
		value_iterator< type, iterator > cbegin() const {
			return begin< type >();
		}
		template< value_type type >
		std::reverse_iterator< value_iterator< type, iterator > > rbegin() const {
			return std::reverse_iterator< value_iterator< type, iterator > >{ begin< type >() };
		}
		template< value_type type >
		std::reverse_iterator< value_iterator< type, iterator > > crbegin() const {
			return rbegin< type >();
		}

		template< value_type type >
		value_iterator< type, iterator > end() const {
			assert_iterator_type< type >();
			return value_iterator< type, iterator >{ end() };
		}
		template< value_type type >
		value_iterator< type, iterator > cend() const {
			return end< type >();
		}
		template< value_type type >
		std::reverse_iterator< value_iterator< type, iterator > > rend() const {
			return std::reverse_iterator< value_iterator< type, iterator > >{ end< type >() };
		}
		template< value_type type >
		std::reverse_iterator< value_iterator< type, iterator > > crend() const {
			return rend< type >();
		}

		template< value_type type >
		std::vector< vt2type< type > > to_vector() const {
			return std::vector< vt2type< type > >( begin< type >(), end< type >() );
		}

		option& abbreviation( char value ) {
#ifdef ProgramOptions_debug
			mutable_operation();
#endif // ProgramOptions_debug
			ProgramOptions_assert( value == '\0' || ( std::isgraph( value ) && value != '-' ), "abbreviation must either be \'\\0\' or a printable character" );
			m_abbreviation = value;
			return *this;
		}
		option& no_abbreviation() {
			return abbreviation( '\0' );
		}
		char get_abbreviation() const {
			return m_abbreviation;
		}

	private:
		static bool valid_description( std::string const& value ) {
			return std::find_if_not( value.begin(), value.end(), []( char c ){ return std::isprint( c ) || c == '\n'; } ) == value.end();
		}
		static void assert_description( std::string const& value ) {
			ProgramOptions_assert( valid_description( value ), "description may only consist of printable characters and newlines" );
		}

	public:
		option& description( std::string const& value ) {
#ifdef ProgramOptions_debug
			mutable_operation();
#endif // ProgramOptions_debug
			assert_description( value );
			m_description = value;
			return *this;
		}
		option& description( std::string&& value ) {
#ifdef ProgramOptions_debug
			mutable_operation();
#endif // ProgramOptions_debug
			assert_description( value );
			m_description = std::move( value );
			return *this;
		}
		std::string const& get_description() const {
			return m_description;
		}

	private:
		static bool valid_type( value_type type ) {
			switch( type ) { // switch( enum ) ensures that a warning is generated whenever an additional enum value is added
			case void_:
			case string:
			case i32:
			case i64:
			case u32:
			case u64:
			case f32:
			case f64:
				return true;
			default:
				return false;
			}
		}

	public:
		option& type( value_type type ) {
#ifdef ProgramOptions_debug
			mutable_operation();
#endif // ProgramOptions_debug
			ProgramOptions_assert( m_fallback == nullptr && m_data == nullptr && m_callbacks.empty(), "type() must be set prior to: fallback(), callback(), parsing" );
			ProgramOptions_assert( valid_type( type ), "type: invalid value_type" );
			m_type = type;
			return *this;
		}
		value_type get_type() const {
			return m_type;
		}

		option& single() {
#ifdef ProgramOptions_debug
			mutable_operation();
#endif // ProgramOptions_debug
			ProgramOptions_assert( m_fallback == nullptr && m_data == nullptr && m_callbacks.empty(), "single() must be set prior to: fallback(), callback(), parsing" );
			m_multi = false;
			return *this;
		}
		option& multi() {
#ifdef ProgramOptions_debug
			mutable_operation();
#endif // ProgramOptions_debug
			ProgramOptions_assert( m_fallback == nullptr && m_data == nullptr && m_callbacks.empty(), "multi() must be set prior to: fallback(), callback(), parsing" );
			m_multi = true;
			return *this;
		}
		bool is_single() const {
			return !is_multi();
		}
		bool is_multi() const {
			return m_multi;
		}

	private:
		template< typename T >
		parsing_report< value > fallback_parse( T&& arg ) {
			auto result = make_value( std::forward< T >( arg ) );
			ProgramOptions_assert( result.good(), "fallback: cannot convert argument to option's type" );
			return result;
		}

		template< typename head_t, typename... tail_t >
		void fallback_single( head_t&& head, tail_t&&... /* tail */ ) { // -Wunused-parameter
			ProgramOptions_assert( sizeof...( tail_t ) == 0, "fallback: too many arguments for single() option" );
			decltype( m_fallback ) new_fallback = make_unique< value_vector< false, true > >();
			new_fallback->push_back( fallback_parse( std::forward< head_t >( head ) ) );
			m_fallback.swap( new_fallback );
		}

		template< typename head_t, typename... tail_t >
		void fallback_helper( value_vector_base& vector, head_t&& head, tail_t&&... tail ) {
			vector.push_back( fallback_parse( std::forward< head_t >( head ) ) );
			fallback_helper( vector, std::forward< tail_t >( tail )... );
		}
		void fallback_helper( value_vector_base& /* vector */ ) { // -Wunused-parameter
		}
		template< typename... args_t >
		void fallback_multi( args_t&&... args ) {
			decltype( m_fallback ) new_fallback = make_unique< value_vector< false, false > >();
			fallback_helper( *new_fallback, std::forward< args_t >( args )... );
			m_fallback.swap( new_fallback );
		}

	public:
		template< typename... args_t >
		option& fallback( args_t&&... args ) {
#ifdef ProgramOptions_debug
			mutable_operation();
#endif // ProgramOptions_debug
			static_assert( sizeof...( args_t ) != 0, "fallback: no arguments provided" );
			ProgramOptions_assert( get_type() != void_, "fallback: not allowed for options of type void_" );
			if( is_single() )
				fallback_single( std::forward< args_t >( args )... );
			else
				fallback_multi( std::forward< args_t >( args )... );
			return *this;
		}
		option& no_fallback() {
#ifdef ProgramOptions_debug
			mutable_operation();
#endif // ProgramOptions_debug
			m_fallback = nullptr;
			return *this;
		}

		template< typename invocable_t >
		option& callback( invocable_t&& invocable ) {
#ifdef ProgramOptions_debug
			mutable_operation();
#endif // ProgramOptions_debug
			std::unique_ptr< callback_base > new_callback;
			switch( get_type() ) {
			case void_:
				new_callback = make_unique< ::po::callback< void_, typename std::remove_reference< invocable_t >::type > >( std::forward< invocable_t >( invocable ) );
				break;
			case string:
				new_callback = make_unique< ::po::callback< string, typename std::remove_reference< invocable_t >::type > >( std::forward< invocable_t >( invocable ) );
				break;
			case i32:
				new_callback = make_unique< ::po::callback< i32, typename std::remove_reference< invocable_t >::type > >( std::forward< invocable_t >( invocable ) );
				break;
			case i64:
				new_callback = make_unique< ::po::callback< i64, typename std::remove_reference< invocable_t >::type > >( std::forward< invocable_t >( invocable ) );
				break;
			case u32:
				new_callback = make_unique< ::po::callback< u32, typename std::remove_reference< invocable_t >::type > >( std::forward< invocable_t >( invocable ) );
				break;
			case u64:
				new_callback = make_unique< ::po::callback< u64, typename std::remove_reference< invocable_t >::type > >( std::forward< invocable_t >( invocable ) );
				break;
			case f32:
				new_callback = make_unique< ::po::callback< f32, typename std::remove_reference< invocable_t >::type > >( std::forward< invocable_t >( invocable ) );
				break;
			case f64:
				new_callback = make_unique< ::po::callback< f64, typename std::remove_reference< invocable_t >::type > >( std::forward< invocable_t >( invocable ) );
			// 	break;
			// default:
			// 	assert( false );
			}
#ifdef ProgramOptions_debug
			ProgramOptions_assert( new_callback->good(), "callback: incompatible parameter type" );
#endif // ProgramOptions_debug
			m_callbacks.emplace_back( std::move( new_callback ) );
			return *this;
		}
		option& no_callback() {
#ifdef ProgramOptions_debug
			mutable_operation();
#endif // ProgramOptions_debug
			m_callbacks.clear();
			return *this;
		}

	private:
		std::unique_ptr< value_vector_base > make_vector() const {
			switch( ( get_type() == void_ ) | is_single() << 1 ) {
			case false | false << 1:
				return make_unique< value_vector< false, false > >();
			case false | true << 1:
				return make_unique< value_vector< false, true > >();
			case true | false << 1:
				return make_unique< value_vector< true, false > >();
			case true | true << 1:
				return make_unique< value_vector< true, true > >();
			}
			return {}; // -Wreturn-type
		}

	public:
		template< typename... args_t >
		error_code parse( args_t&&... args ) {
			const auto result = make_value( std::forward< args_t >( args )... );
			if( result.good() ) {
				if( m_data == nullptr )
					m_data = make_vector();
				m_data->push_back( result );
				notify( result );
			}
			return result.error;
		}
	};

	class parser {
		using options_t = std::unordered_map< std::string, option >;
		options_t m_options;
		char const* m_program_name = "";

		options_t::iterator find_abbreviation( char value ) {
			auto iter = m_options.begin();
			for( ; iter != m_options.end(); ++iter  )
				if( iter->second.get_abbreviation() == value )
					break;
			return iter;
		}
		template< typename expression_t, typename... args_t >
		bool parse( options_t::iterator option, expression_t const& expression, args_t&&... args ) const {
			const error_code code = option->second.parse( std::forward< args_t >( args )... );
			if( code == error_code::none )
				return true;
			err() << red << "error: ";
			err() << "option \'";
			err() << blue << option->first;
			err() << "\' ";
			switch( code ) {
				case error_code::argument_expected:
				case error_code::conversion_error:
					err() << "expects an argument of type " << vt2str( option->second.get_type() );
					break;
				case error_code::no_argument_expected:
					err() << "doesn't expect any arguments";
					break;
				case error_code::out_of_range:
					err() << "has an argument that caused an out of range error";
                                          break;
				default: // -Wswitch
					;
			}
			err() << "; ignoring \'";
			err() << white << expression;
			err() << "\'\n";
			return false;
		}
		bool dashed_non_option( char* arg ) {
			return arg[ 0 ] == '-'
				&& ( is_digit( arg[ 1 ] ) || arg[ 1 ] == '.' )
				&& find_abbreviation( arg[ 1 ] ) == m_options.end();
		}
		bool extract_argument( options_t::iterator option, int argc, char** argv, int& i, int j ) {
			std::string expression = argv[ i ];
			char const* argument = "";
			// --var...
			if( argv[ i ][ j ] == '\0' ) {
				// --var data
				if( option->second.get_type() != void_ &&
					i + 1 < argc &&
					( argv[ i + 1 ][ 0 ] != '-' || dashed_non_option( argv[ i + 1 ] ) )
				) {
					++i;
					expression += ' ';
					expression += argv[ i ];
					argument = argv[ i ];
				}
			} else if( argv[ i ][ j ] == '=' ) {
				// --var=data
				argument = &argv[ i ][ j + 1 ];
			} else {
				// --vardata
				argument = &argv[ i ][ j ];
			}
			return parse( option, std::move( expression ), argument );
		}

		static void error() {
			err() << red << "error: ";
		}
		template< typename arg_t >
		static void ignoring( arg_t&& arg ) {
			err() << "; ignoring \'";
			err() << white << std::forward< arg_t >( arg );
			err() << "\'\n";
		}
		template< typename arg_t >
		static void error_unrecognized_option( arg_t&& arg ) {
			error();
			err() << "unrecognized option";
			ignoring( std::forward< arg_t >( arg ) );
		}

	public:
		bool wellformed() const {
			for( auto&& i : m_options ) {
				if( !valid_designator( i.first ) )
					return false;
				if( i.first.size() == 0 && i.second.get_abbreviation() != '\0' )
					return false;
				if( i.first.size() == 1 && i.second.get_abbreviation() != i.first[ 0 ] )
					return false;
			}
			std::string abbreviations;
			for( auto&& i : m_options )
				if( char a = i.second.get_abbreviation() )
					abbreviations.push_back( a );
			std::sort( abbreviations.begin(), abbreviations.end() );
			if( std::adjacent_find( abbreviations.begin(), abbreviations.end() ) != abbreviations.end() )
				return false;
			return true;
		}

                bool parseArgs( int argc, char** argv ) {
			ProgramOptions_assert( wellformed(), "cannot parse with an ill-formed parser" );
			ProgramOptions_assert( std::none_of( m_options.begin(), m_options.end(), []( options_t::value_type const& x ){ return x.second.was_set(); } ), "some options were already set" );
			if( argc == 0 )
				return true;
			bool good = true;
			m_program_name = std::max( std::strrchr( argv[ 0 ], '/' ), std::strrchr( argv[ 0 ], '\\' ) );
			if( m_program_name == nullptr )
				m_program_name = argv[ 0 ];
			else
				++m_program_name; // skip the dash
			const auto unnamed = m_options.find( "" );
			const bool has_unnamed = unnamed != m_options.end();
			for( int i = 1; i < argc; ++i ) {
				if( argv[ i ][ 0 ] == '\0' )
					continue;
				if( argv[ i ][ 0 ] != '-' || ( has_unnamed && dashed_non_option( argv[ i ] ) ) ) {
					if( !has_unnamed ) {
						good = false;
						error();
						err() << "unnamed arguments not allowed";
						ignoring( argv[ i ] );
					} else {
						good &= parse( unnamed, argv[ i ], argv[ i ] );
					}
				} else {
					// -...
					if( argv[ i ][ 1 ] == '\0' ) {
						// -
						good = false;
						error();
						err() << "invalid argument; ignoring single hyphen\n";
					} else if( argv[ i ][ 1 ] == '-' ) {
						// --...
						char* first = &argv[ i ][ 2 ];
						char* last = first;
                                                  for( ; is_character_permitted(*last); ++last );
						const auto opt = m_options.find( std::string{ first, last } );
						if( opt == m_options.end() ) {
							good = false;
							error_unrecognized_option( argv[ i ] );
						} else {
							good &= extract_argument( opt, argc, argv, i, last - argv[ i ] );
						}
					} else {
						// -f...
						const auto head = find_abbreviation( argv[ i ][ 1 ] );
						if( head == m_options.end() ) {
							good = false;
							error_unrecognized_option( argv[ i ] );
						} else if( head->second.get_type() == void_ ) {
							// -fgh
							char c;
							for( std::size_t j = 1; ( c = argv[ i ][ j ] ) != '\0'; ++j ) {
								if( !std::isprint( c ) || c == '-' ) {
									good = false;
									err() << "invalid character \'" << c << "\'";
									ignoring( &argv[ i ][ j ] );
									break;
								}
								const auto opt = find_abbreviation( c );
								if( opt == m_options.end() ) {
									good = false;
									error_unrecognized_option( c );
									continue;
								}
								if( opt->second.get_type() != void_ ) {
									good = false;
									err() << "non-void options not allowed in option packs";
									ignoring( c );
									continue;
								}
								good &= parse( opt, c );
							}
						} else {
							good &= extract_argument( head, argc, argv, i, 2 );
						}
					}
				}
			}
#ifdef ProgramOptions_debug
			for( auto&& i : m_options )
				i.second.make_immutable();
#endif // ProgramOptions_debug
			return good;
		}

	private:
                 static bool is_character_permitted(char c) {
                    return std::isalpha( c ) || c == '-' || c== '_';
                 }

		static bool valid_designator( std::string const& designator ) {
			if( designator.empty() )
				return true;
			if( designator[ 0 ] == '-' )
				return false;
                         return std::find_if_not( designator.begin(), designator.end(), []( char c ){ return is_character_permitted(c); } ) == designator.end();
		}
		option& operator_brackets_helper( std::string&& designator ) {
			ProgramOptions_assert( valid_designator( designator ), "operator[]: designator may only consist of letters and hyphens and mustn't start with a hyphen" );
			const bool empty = designator.empty();
			const char initial = designator.size() == 1 ? designator[ 0 ] : '\0';
			const auto result = m_options.emplace( std::move( designator ), option{} );
			if( result.second ) {
				if( initial )
					result.first->second.abbreviation( initial );
				if( empty )
					result.first->second
						.type( string )
						.multi();
			}
			return result.first->second;
		}

	public:
    int count() const {
      return m_options.size();
    }
		option& operator[]( std::string const& designator ) {
			return operator_brackets_helper( std::string{ designator } );
		}
		option& operator[]( std::string&& designator ) {
			return operator_brackets_helper( std::move( designator ) );
		}

		// TODO: inform about options' arguments e.g. -O[u32] or --include-path[=string]
		friend std::ostream& operator<<( std::ostream& stream, parser const& object ) {
			enum : std::size_t {
				console_width = 80 - 1, // -1 because writing until the real end returns the carriage
				left_padding = 2,
				max_abbreviation_width = 2,
				max_separator_width = 2,
				max_verbose_width = 25,
				mid_padding = 2,
				paragraph_indenture = 2
			};
			ProgramOptions_assert( object.wellformed(), "cannot print an ill-formed parser" );
			stream << "Usage:\n  " << object.m_program_name;
			const auto unnamed = object.m_options.find( "" );
			if( unnamed != object.m_options.end() ) {
				stream << " [argument";
				if( unnamed->second.is_multi() )
					stream << "s...";
				stream << ']';
			}
			if( !object.m_options.empty() )
				stream << " [options]";
			stream << "\nAvailable options:\n";
			bool any_abbreviations = false;
			std::size_t max_verbose = 0;
			for( auto&& i : object.m_options ) {
				any_abbreviations |= i.second.get_abbreviation() != '\0';
				max_verbose = std::max( max_verbose, i.first.size() );
			}
			const bool any_verbose = max_verbose > 1;
			if( any_verbose )
				max_verbose += 2; // --
			const std::size_t abbreviation_width = any_abbreviations * max_abbreviation_width;
			const std::size_t separator_width = ( any_abbreviations && any_verbose ) * max_separator_width;
			const std::size_t verbose_start = left_padding + abbreviation_width + separator_width;
			const std::size_t verbose_width = std::min( any_verbose * max_verbose_width, max_verbose );
			const std::size_t description_start = verbose_start + verbose_width + mid_padding;
			for( auto&& i : object.m_options ) {
				if( i.first.empty() )
					continue;
				stream << repeat{ left_padding, ' ' };
				const char abbreviation = i.second.get_abbreviation();
				const bool verbose = i.first.size() > 1;
				if( abbreviation )
					stream << white << '-' << abbreviation;
				else
					stream << repeat{ abbreviation_width, ' ' };
				if( abbreviation && verbose )
					stream << ',' << ' ';
				else
					stream << repeat{ separator_width, ' ' };
				if( verbose ) {
					stream << white << '-' << '-' << i.first;
					const int rem = verbose_width - 2 - i.first.size();
					if( rem >= 0 )
						stream << repeat{ static_cast< std::size_t >( rem ) + mid_padding, ' ' };
					else
						stream << '\n' << repeat{ description_start, ' ' };
				} else {
					stream << repeat{ verbose_width + mid_padding, ' ' };
				}
				std::size_t caret = description_start;
				std::string const& descr = i.second.get_description();
				for( std::size_t i = 0; i < descr.size(); ++i ) {
					const bool last = ( i + 1 < descr.size() ) && ( caret + 1 >= console_width );
					if( last ) {
						if( std::isgraph( descr[ i ] ) && std::isgraph( descr[ i + 1 ] ) ) {
							if( std::isgraph( descr[ i - 1 ] ) ) {
								stream << '-';
							}
							--i;
						} else {
							stream << descr[ i ];
						}
					}
					if( descr[ i ] == '\n' || last ) {
						caret = description_start + paragraph_indenture;
						stream << '\n' << repeat{ caret, ' ' };
						if( std::isblank( descr[ i + 1 ] ) )
							++i;
					} else {
						stream << descr[ i ];
						++caret;
					}
				}
				stream << '\n';
			}
			return stream;
		}
	};
}

#endif // !ProgramOptions_hxx_included
