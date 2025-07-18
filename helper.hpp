#include <vector>
#include <cassert>
#include <string>
#include <fstream>
#include <cstdint>
#include <string.h>

class stream_reader_t
{
private:
    const uint8_t* m_end{};
    const uint8_t* m_current{};

public:
    stream_reader_t(const uint8_t* begin_, const uint8_t* end_) : m_end(end_), m_current(begin_)
    {
    }

    size_t left() const
    {
        return m_end - m_current;
    }

    bool fits(size_t size_) const
    {
        return left() >= size_;
    }

    void advanced(size_t distance_)
    {
        assert(fits(distance_));
        m_current += distance_;
    }

    void read(void* ptr_, size_t size_)
    {
        assert(fits(size_));
        ::memcpy(ptr_, m_current, size_);
        advanced(size_);
    }

    uint8_t read_uint8()
    {
        uint8_t tmp{};
        read(&tmp, sizeof(tmp));
        return tmp;
    }

    uint16_t read_uint16()
    {
        uint16_t tmp{};
        read(&tmp, sizeof(tmp));
        return tmp;
    }

    uint32_t read_uint32()
    {
        uint32_t tmp{};
        read(&tmp, sizeof(tmp));
        return tmp;
    }

    std::vector<uint8_t> read_vector(size_t size_)
    {
        assert(fits(size_));
        std::vector<uint8_t> tmp(size_);
        read(tmp.data(), tmp.size());
        return tmp;
    }
};

class stream_writer_t
{
private:
    uint8_t* m_begin{};
    uint8_t* m_end{};
    uint8_t* m_current{};

public:
    stream_writer_t(uint8_t* begin_, uint8_t* end_) :m_begin(begin_), m_end(end_), m_current(begin_)
    {
    }
    size_t left() const
    {
        return m_end - m_current;
    }
    size_t current_size() const
    {
        return m_current - m_begin;
    }
    void advanced(size_t distance_)
    {
        assert(left() >= distance_);
        m_current += distance_;
    }
    void write(const uint8_t* begin_, const uint8_t* end_)
    {
        const size_t size = end_ - begin_;
        assert(left() >= size);
        ::memcpy(m_current, begin_, size);
        advanced(size);
    }
    void write_uint8(uint8_t byte_)
    {
        write(&byte_, &byte_ + sizeof(byte_));
    }
    void write_vector(const std::vector<uint8_t>& bytes_)
    {
        write(bytes_.data(), bytes_.data() + bytes_.size());
    }
};

struct test_data_t
{
    std::vector<uint8_t> packed_data;
    std::vector<uint8_t> unpacked_data;
};

inline std::vector<test_data_t> load_test_data(const std::string& file_path_)
{
    std::vector<test_data_t> result;

    std::ifstream file(file_path_, std::ios::binary);
    assert(file);
    const auto test_data = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    stream_reader_t reader(test_data.data(), test_data.data() + test_data.size());
    while (reader.left() > 0)
    {
        const std::vector<uint8_t> packed_data = reader.read_vector(reader.read_uint32());
        const std::vector<uint8_t> unpacked_data = reader.read_vector(reader.read_uint32());
        assert(reader.read_uint32() == 0xDEADBEEF); // simple format-ok check
        result.push_back({ packed_data, unpacked_data });
    }

    return result;
}
