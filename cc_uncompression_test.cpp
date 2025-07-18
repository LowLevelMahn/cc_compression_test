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

struct block_t {
    uint8_t packed_size{};
    bool more_blocks{ false };
    uint16_t unpacked_size{};
};

block_t read_block(stream_reader_t& input_reader_)
{
    uint8_t packed_size = input_reader_.read_uint8();
    const uint8_t flag = input_reader_.read_uint8();
    assert(flag == 0 || flag == 1);
    const uint16_t unpacked_size = input_reader_.read_uint16();
    return { packed_size, flag == 1, unpacked_size };
}

struct tables_t {
    std::vector<uint8_t> table0;
    std::vector<uint8_t> table1;
    std::vector<uint8_t> table3;
    std::vector<uint8_t> table4;
};

constexpr uint8_t END_INDEX = 0;

tables_t read_and_prepare_tables(stream_reader_t& input_reader_, const uint8_t packed_size)
{
    assert(packed_size != 0);

    // read & prepare uncompress-helper tables
    const std::vector<uint8_t> temp_table = input_reader_.read_vector(packed_size); // only needed for initialization, not for later uncompression

    auto read_table = [&](size_t size) {
        std::vector<uint8_t> table(size + 1);
        input_reader_.read(&table[1], size);
        table[0] = 0xFF;
        return table;
    };

    const std::vector<uint8_t> table0 = read_table(packed_size);
    const std::vector<uint8_t> table1 = read_table(packed_size);

    // prepare table3 and table4 based on the read tables
    std::vector<uint8_t> table3(256, 0);
    std::vector<uint8_t> table4(1 + temp_table.size(), 0);

    for (size_t i = 0; i < temp_table.size(); ++i) {
        const uint8_t ofs = temp_table[i]; // 0..n
        //table3 offsets are initial all 0 but filled offsets get then re-used
        const uint8_t index = static_cast<uint8_t>(i + 1); // (0..255)+1
        table4[index] = table3[ofs]; //1+256  [0] ignored, [1-256]
        table3[ofs] = index;
    }
    table4[0] = 0xFF; // index unused, never read

    return { table0, table1, table3, table4 };
}

void uncompress_part1(
    stream_writer_t& output_writer_,
    const std::vector<uint8_t>& table_,
    const uint8_t start_index_,
    const tables_t& tables_
);

void uncompress_part0(stream_writer_t& output_writer_, const tables_t& tables, const uint8_t start_index_)
{
    uncompress_part1(output_writer_, tables.table0, start_index_, tables);
    uncompress_part1(output_writer_, tables.table1, start_index_, tables);
}

enum class Action {
    Next,
    Recurse,
    Finished
};

Action next_action(
    const uint8_t start_index_,
    const uint8_t index_,
    const std::vector<uint8_t>& table4_,
    uint8_t* const next_index_)
{
    if (start_index_ > index_) {
        *next_index_ = index_;
        return Action::Recurse;
    }

    const uint8_t parent = table4_[index_];
    if (parent == END_INDEX) {
        return Action::Finished;
    }

    if (start_index_ >= parent) {
        *next_index_ = parent;
        return Action::Recurse;
    }

    *next_index_ = parent;
    return Action::Next;
}

void uncompress_part1(
    stream_writer_t& output_writer_,
    const std::vector<uint8_t>& table_,
    const uint8_t start_index_,
    const tables_t& tables_
)
{
    const uint8_t table3_index = table_[start_index_];
    uint8_t index = tables_.table3[table3_index];

    while (index != END_INDEX)
    {
        const Action action = next_action(start_index_, index, tables_.table4, &index);

        if (action == Action::Next)
        {
            continue;
        }

        if (action == Action::Recurse)
        {
            uncompress_part0(output_writer_, tables_, index);
            return;
        }

        if (action == Action::Finished)
        {
            break;
        }
    }

    output_writer_.write_uint8(table3_index);
}

bool process_block(stream_reader_t& input_reader_, stream_writer_t& output_writer_)
{
    const block_t block = read_block(input_reader_);

    if (block.packed_size == 0) {
        output_writer_.write_vector(input_reader_.read_vector(block.unpacked_size));
        return block.more_blocks;
    }

    const tables_t tables = read_and_prepare_tables(input_reader_, block.packed_size);
    const std::vector<uint8_t> compressed_data = input_reader_.read_vector(block.unpacked_size);

    for (const uint8_t value : compressed_data)
    {
        const uint8_t index = tables.table3[value];

        if (index == END_INDEX) {
            output_writer_.write_uint8(value);
        }
        else {
            uncompress_part0(output_writer_, tables, index);
        }
    }

    return block.more_blocks;
}

std::vector<uint8_t> uncompress(const std::vector<uint8_t>& packed_data_, const size_t unpacked_size_)
{
    stream_reader_t input_reader(packed_data_.data(), packed_data_.data() + packed_data_.size());

    std::vector<uint8_t> output(unpacked_size_);
    stream_writer_t output_writer(output.data(), output.data() + output.size());

    while (process_block(input_reader, output_writer)) {}

    assert(input_reader.left() == 0);
    assert(output_writer.left() == 0);

    return output;
}

bool test(const std::vector<uint8_t>& compressed_, const std::vector<uint8_t>& uncompressed_reference_)
{
    const std::vector<uint8_t> uncompressed = uncompress(compressed_, uncompressed_reference_.size());
    return uncompressed == uncompressed_reference_;
}

struct test_data_t
{
    std::vector<uint8_t> packed_data;
    std::vector<uint8_t> unpacked_data;
};

std::vector<test_data_t> load_test_data(const std::string& file_path_)
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

int main(int argc, char* argv[])
{
    const std::string file_path = argv[1];

    const auto test_data = load_test_data(file_path);

    int nr = 0;
    for (const auto& td : test_data)
    {
        printf("nr: %i\n", nr++);
        test(td.packed_data, td.unpacked_data);
    }

    return 0;
}
