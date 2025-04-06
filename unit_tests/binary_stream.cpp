#include "ouly/serializers/binary_stream.hpp"
#include "catch2/catch_all.hpp"
#include "catch2/catch_test_macros.hpp"
#include <sstream>
// NOLINTBEGIN
// Define a simple struct for serialization tests
struct TestStruct
{
  int   x;
  float y;
  bool  operator==(const TestStruct& other) const
  {
    return x == other.x && y == other.y;
  }
};

// Define serialization for TestStruct
namespace ouly
{
inline void write(binary_output_stream& stream, const TestStruct& value)
{
  stream.write(reinterpret_cast<const std::byte*>(&value.x), sizeof(value.x));
  stream.write(reinterpret_cast<const std::byte*>(&value.y), sizeof(value.y));
}

inline void read(binary_input_stream& stream, TestStruct& value)
{
  stream.read(reinterpret_cast<std::byte*>(&value.x), sizeof(value.x));
  stream.read(reinterpret_cast<std::byte*>(&value.y), sizeof(value.y));
}

inline void write(binary_ostream& stream, const TestStruct& value)
{
  stream.write(reinterpret_cast<const std::byte*>(&value.x), sizeof(value.x));
  stream.write(reinterpret_cast<const std::byte*>(&value.y), sizeof(value.y));
}

inline void read(binary_istream& stream, TestStruct& value)
{
  stream.read(reinterpret_cast<std::byte*>(&value.x), sizeof(value.x));
  stream.read(reinterpret_cast<std::byte*>(&value.y), sizeof(value.y));
}
} // namespace ouly

TEST_CASE("binary_output_stream operations", "[binary_stream]")
{
  SECTION("Default construction")
  {
    ouly::binary_output_stream stream;
    REQUIRE(stream.size() == 0);
  }

  SECTION("Writing data")
  {
    ouly::binary_output_stream stream;
    int                        testData = 42;
    stream.write(reinterpret_cast<const std::byte*>(&testData), sizeof(testData));

    REQUIRE(stream.size() == sizeof(testData));
    REQUIRE(*reinterpret_cast<const int*>(stream.data()) == testData);
  }

  SECTION("Get string view")
  {
    ouly::binary_output_stream stream;
    int                        testData = 42;
    stream.write(reinterpret_cast<const std::byte*>(&testData), sizeof(testData));

    auto view = stream.get_string();
    REQUIRE(view.size() == sizeof(testData));
    REQUIRE(*reinterpret_cast<const int*>(view.data()) == testData);
  }

  SECTION("Release stream")
  {
    ouly::binary_output_stream stream;
    int                        testData = 42;
    stream.write(reinterpret_cast<const std::byte*>(&testData), sizeof(testData));

    auto binary = stream.release();
    REQUIRE(binary.size() == sizeof(testData));
    REQUIRE(*reinterpret_cast<const int*>(binary.data()) == testData);

    // Original stream should be empty after release
    REQUIRE(stream.size() == 0);
  }

  SECTION("Stream out serialization")
  {
    ouly::binary_output_stream stream;
    TestStruct                 test{123, 45.67f};
    stream.stream_out(test);

    REQUIRE(stream.size() == sizeof(TestStruct));
  }
}

TEST_CASE("binary_input_stream operations", "[binary_stream]")
{
  SECTION("Construction from pointer and size")
  {
    int                       testData = 42;
    ouly::binary_input_stream stream(reinterpret_cast<const std::byte*>(&testData), sizeof(testData));

    REQUIRE(stream.size() == sizeof(testData));
    REQUIRE(*reinterpret_cast<const int*>(stream.data()) == testData);
  }

  SECTION("Construction from binary_stream_view")
  {
    ouly::binary_stream binary;
    int                 testData = 42;
    binary.resize(sizeof(testData));
    std::memcpy(binary.data(), &testData, sizeof(testData));

    ouly::binary_stream_view  view(binary);
    ouly::binary_input_stream stream(view);

    REQUIRE(stream.size() == sizeof(testData));
    REQUIRE(*reinterpret_cast<const int*>(stream.data()) == testData);
  }

  SECTION("Reading data")
  {
    int                       sourceData = 42;
    ouly::binary_input_stream stream(reinterpret_cast<const std::byte*>(&sourceData), sizeof(sourceData));

    int targetData = 0;
    stream.read(reinterpret_cast<std::byte*>(&targetData), sizeof(targetData));

    REQUIRE(targetData == sourceData);
    REQUIRE(stream.size() == 0); // Stream should be empty after reading
  }

  SECTION("Skipping data")
  {
    int                       values[2] = {42, 84};
    ouly::binary_input_stream stream(reinterpret_cast<const std::byte*>(values), sizeof(values));

    stream.skip(sizeof(int));
    REQUIRE(stream.size() == sizeof(int));
    REQUIRE(*reinterpret_cast<const int*>(stream.data()) == 84);
  }

  SECTION("Get string")
  {
    int                       testData = 42;
    ouly::binary_input_stream stream(reinterpret_cast<const std::byte*>(&testData), sizeof(testData));

    auto binary = stream.get_string();
    REQUIRE(binary.size() == sizeof(testData));
  }

  SECTION("Stream in deserialization")
  {
    TestStruct                 sourceData{123, 45.67f};
    ouly::binary_output_stream outStream;
    outStream.stream_out(sourceData);

    ouly::binary_input_stream inStream(outStream.get_string());
    TestStruct                targetData{0, 0.0f};
    inStream.stream_in(targetData);

    REQUIRE(targetData == sourceData);
  }
}

TEST_CASE("binary_ostream operations", "[binary_stream]")
{
  SECTION("Writing data to std::ostream")
  {
    std::ostringstream   oss;
    ouly::binary_ostream stream(oss);

    int testData = 42;
    stream.write(reinterpret_cast<const std::byte*>(&testData), sizeof(testData));

    REQUIRE(oss.str().size() == sizeof(testData));
    REQUIRE(*reinterpret_cast<const int*>(oss.str().data()) == testData);
  }

  SECTION("Stream out serialization")
  {
    std::ostringstream   oss;
    ouly::binary_ostream stream(oss);

    TestStruct test{123, 45.67f};
    stream.stream_out(test);

    REQUIRE(oss.str().size() == sizeof(TestStruct));

    // Verify the data was written correctly
    TestStruct result;
    std::memcpy(&result, oss.str().data(), sizeof(TestStruct));
    REQUIRE(result == test);
  }
}

TEST_CASE("binary_istream operations", "[binary_stream]")
{
  SECTION("Reading data from std::istream")
  {
    int                  sourceData = 42;
    std::string          buffer(reinterpret_cast<const char*>(&sourceData), sizeof(sourceData));
    std::istringstream   iss(buffer);
    ouly::binary_istream stream(iss);

    int targetData = 0;
    stream.read(reinterpret_cast<std::byte*>(&targetData), sizeof(targetData));

    REQUIRE(targetData == sourceData);
  }

  SECTION("Skipping data")
  {
    int                  values[2] = {42, 84};
    std::string          buffer(reinterpret_cast<const char*>(values), sizeof(values));
    std::istringstream   iss(buffer);
    ouly::binary_istream stream(iss);

    stream.skip(sizeof(int));

    int targetData = 0;
    stream.read(reinterpret_cast<std::byte*>(&targetData), sizeof(targetData));
    REQUIRE(targetData == 84);
  }

  SECTION("Stream in deserialization")
  {
    TestStruct           sourceData{123, 45.67f};
    std::string          buffer(reinterpret_cast<const char*>(&sourceData), sizeof(sourceData));
    std::istringstream   iss(buffer);
    ouly::binary_istream stream(iss);

    TestStruct targetData{0, 0.0f};
    stream.stream_in(targetData);

    REQUIRE(targetData == sourceData);
  }
}
// NOLINTEND