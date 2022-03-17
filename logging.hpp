#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>


namespace logging {

namespace manip {

template <typename NumericT>
struct hex {
    hex(NumericT&& value) :
        value(value)
    {
    }

    NumericT value;
};

template <typename NumericT>
struct oct {
    oct(NumericT&& value) :
        value(value)
    {
    }

    NumericT value;
};


template <typename NumericT>
struct precision {
    precision(const NumericT& value, int precision) :
        value(value),
        n(precision)
    {}
    precision(NumericT&& value, int precision) :
        value(value),
        n(precision)
    {}

    NumericT value;
    int n;
};


template <typename NumericT>
struct fixed {
    fixed(NumericT&& value) :
        value(value)
    {
    }

    NumericT value;
};


template <typename NumericT>
struct scientific {
    scientific(NumericT&& value) :
        value(value)
    {
    }

    NumericT value;
};


template <typename NumericT>
struct width {
    width(const NumericT &value, int n) :
        value(value),
        n(n)
    {
    }
    width(NumericT &&value, int n) :
        value(value),
        n(n)
    {
    }

    NumericT value;
    int n;
};

}  // namespace manip

namespace impl {

namespace detail {

template <typename StreamT>
struct scoped_ios_format {
    // disable copy
    scoped_ios_format(const scoped_ios_format &) = delete;
    scoped_ios_format &operator =(const scoped_ios_format &) = delete;

    scoped_ios_format(StreamT &stream) :
        format(nullptr)
    {
        format.copyfmt(stream);
    }
    ~scoped_ios_format()
    {
        stream.copyfmt(format);
    }

    StreamT &stream;
    StreamT format;
};

template <typename StreamT>
scoped_ios_format<StreamT> make_scoped_ios_format(StreamT &stream) {
    return make_scoped_ios_format(stream);
}

template <typename StreamT, typename ValueT>
struct serializer {
    static void serialize(StreamT& stream, const ValueT& value) {
        stream << value;
    }
};


// bool
template <typename StreamT>
struct serializer<StreamT, bool> {
    static void serialize(StreamT& stream, const bool& value) {
        auto scope = make_scoped_ios_format(stream);
        stream << std::boolalpha << value;
    }
};


// manip::hex
template <typename StreamT, typename NumericT>
struct serializer<StreamT, manip::hex<NumericT>> {
    static void serialize(StreamT& stream, const manip::hex<NumericT>& value) {
        auto scope = make_scoped_ios_format(stream);
        stream << std::hex << std::showbase;
        serializer<StreamT, NumericT>::serialize(stream, value.value);
    }
};


// manip::hex<float>
template <typename StreamT>
struct serializer<StreamT, manip::hex<float>> {
    static void serialize(StreamT& stream, const manip::hex<float>& value) {
        auto scope = make_scoped_ios_format(stream);
        stream << std::hexfloat;
        serializer<StreamT, float>::serialize(stream, value.value);
    }
};
// manip::hex<double>
template <typename StreamT>
struct serializer<StreamT, manip::hex<double>> {
    static void serialize(StreamT& stream, const manip::hex<double>& value) {
        auto scope = make_scoped_ios_format(stream);
        stream << std::hexfloat;
        serializer<StreamT, double>::serialize(stream, value.value);
    }
};


// manip::oct
template <typename StreamT, typename NumericT>
struct serializer<StreamT, manip::oct<NumericT>> {
    static void serialize(StreamT& stream, const manip::oct<NumericT>& value) {
        auto scope = make_scoped_ios_format(stream);
        stream << std::oct << std::showbase;
        serializer<StreamT, NumericT>::serialize(stream, value.value);
    }
};


// manip::precision
template <typename StreamT, typename NumericT>
struct serializer<StreamT, manip::precision<NumericT>> {
    static void serialize(StreamT& stream, const manip::precision<NumericT>& value) {
        auto scope = make_scoped_ios_format(stream);
        stream << std::setprecision(value.n);
        serializer<StreamT, NumericT>::serialize(stream, value.value);
    }
};


// manip::fixed
template <typename StreamT, typename NumericT>
struct serializer<StreamT, manip::fixed<NumericT>> {
    static void serialize(StreamT& stream, const manip::fixed<NumericT>& value) {
        auto scope = make_scoped_ios_format(stream);
        stream << std::fixed;
        serializer<StreamT, NumericT>::serialize(stream, value.value);
    }
};


// manip::scientific
template <typename StreamT, typename NumericT>
struct serializer<StreamT, manip::scientific<NumericT>> {
    static void serialize(StreamT& stream, const manip::scientific<NumericT>& value) {
        auto scope = make_scoped_ios_format(stream);
        stream << std::scientific;
        serializer<StreamT, NumericT>::serialize(stream, value.value);
    }
};


// manip::width
template <typename StreamT, typename NumericT>
struct serializer<StreamT, manip::width<NumericT>> {
    static void serialize(StreamT& stream, const manip::width<NumericT>& value) {
        auto scope = detail::make_scoped_ios_format(stream);
        stream << std::setw(value.n);
        serializer<StreamT, NumericT>::serialize(stream, value.value);
    }
};


}  // namespace detail

template <typename StreamT>
StreamT& serialize(StreamT& stream) {
    return stream;
}

template <typename StreamT, typename Head, typename... Tail>
StreamT& serialize(StreamT& stream, const Head& value, const Tail&... args) {
    detail::serializer<StreamT, Head>::serialize(stream, value);
    return serialize(stream, args...);
}

}  // namespace impl

//
// this function will be obsolete by c++20 std::format
//
#if __cplusplus > 201704L
[[deprecated("please use std::format() function")]]  // obsolete by c++20
#endif // c++20 check
template <typename... Args>
std::string format(const Args&... args) {
    std::ostringstream oss;
    impl::serialize(oss, args...);
    return oss.str();
}


inline std::string current_time() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    const tm *lt = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(lt, "%FT%T");
    return oss.str();
}



class Logger {
public:
    Logger(const std::string &filepath)
        : m_file(filepath)
    {}
    ~Logger()
    {}

    void debug(const std::string &msg) {
        write("DEBUG", msg);
    }
    void info(const std::string &msg) {
        write("INFO", msg);
    }
    void warning(const std::string &msg) {
        write("WARNING", msg);
    }
    void error(const std::string &msg) {
        write("ERROR", msg);
    }

    void write(const char *type, const std::string &msg) {
        m_file << current_time() << ":" << type << ":" << msg << std::endl;
    }

    std::ofstream m_file;

public:
    static std::unique_ptr<Logger> create(const std::string& filepath) {
        auto logger = std::make_unique<Logger>(filepath);
        if ( ! logger->m_file ) {
            return nullptr;
        }
        return logger;
    }

};

//
// for declaration
//
#define LOGGING_DECL_IMPL(name) std::shared_ptr<logging::Logger> m_p ## name ;

#define LOGGING_DECL() LOGGING_DECL_IMPL(Logger)
#define LOGGING_GLOBAL_DECL() LOGGING_DECL_IMPL(GlobalLogger)

//
// construct logger
//
static constexpr bool bLoggingEnabled = true;
#define LOGGING_CREATE_IMPL(logger, filename) { if ( logging::bLoggingEnabled ) { logger = logging::Logger::create(filename); } }
// core implementation
#define LOGGING_WRITE_IMPL(logger, method, ...) if ( logger ) { logger -> method (logging::format(__VA_ARGS__)); }

//
// raw output
//
#define LOGGING_DEBUG0(logger, ...) LOGGING_WRITE_IMPL(logger, debug, __VA_ARGS__)
#define LOGGING_INFO0(logger, ...) LOGGING_WRITE_IMPL(logger, info, __VA_ARGS__)
#define LOGGING_WARNING0(logger, ...) LOGGING_WRITE_IMPL(logger, warning, __VA_ARGS__)
#define LOGGING_ERROR0(logger, ...) LOGGING_WRITE_IMPL(logger, error, __VA_ARGS__)

//
// Global logger (device independent)
//
#define LOGGING_GLOBAL_LOG_CREATE(filename) LOGGING_CREATE_IMPL(m_pGlobalLogger, filename)

// implementation
#define LOGGING_GLOBAL_LOG_WRITE(method, ...) LOGGING_WRITE_IMPL(m_pGlobalLogger, method, __VA_ARGS__)
// format: YYYY-mm-ddTHH:MM:SS:DEBUG: ...
#define LOGGING_GLOBAL_LOG_DEBUG(...)   LOGGING_GLOBAL_LOG_WRITE(debug, __VA_ARGS__)
// format: YYYY-mm-ddTHH:MM:SS:INFO: ...
#define LOGGING_GLOBAL_LOG_INFO(...)    LOGGING_GLOBAL_LOG_WRITE(info, __VA_ARGS__)
// format: YYYY-mm-ddTHH:MM:SS:WARNING: ...
#define LOGGING_GLOBAL_LOG_WARNING(...) LOGGING_GLOBAL_LOG_WRITE(warning, __VA_ARGS__)
// format: YYYY-mm-ddTHH:MM:SS:ERROR: ...
#define LOGGING_GLOBAL_LOG_ERROR(...)   LOGGING_GLOBAL_LOG_WRITE(error, __VA_ARGS__)


//
// Logger
//
#define LOGGING_CREATE(filename) LOGGING_CREATE_IMPL(m_pLogger, filename)

// implementation
#define LOGGING_WRITE(method, ...) LOGGING_WRITE_IMPL(m_pLogger, method, __VA_ARGS__)
// format: YYYY-mm-ddTHH:MM:SS:DEBUG: ...
#define LOGGING_DEBUG(...)   LOGGING_WRITE(debug, __VA_ARGS__)
// format: YYYY-mm-ddTHH:MM:SS:INFO: ...
#define LOGGING_INFO(...)    LOGGING_WRITE(info, __VA_ARGS__)
// format: YYYY-mm-ddTHH:MM:SS:WARNING: ...
#define LOGGING_WARNING(...) LOGGING_WRITE(warning, __VA_ARGS__)
// format: YYYY-mm-ddTHH:MM:SS:ERROR: ...
#define LOGGING_ERROR(...)   LOGGING_WRITE(error, __VA_ARGS__)

}  // namespace logging

using logging::manip::hex;
using logging::manip::oct;
using logging::manip::precision;
using logging::manip::fixed;
using logging::manip::scientific;
using logging::manip::width;
