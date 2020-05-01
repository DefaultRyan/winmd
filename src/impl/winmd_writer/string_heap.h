#pragma once

namespace winmd::writer
{
    struct string_index
    {
        uint32_t offset;
    };

    struct string_heap
    {
        string_heap()
        {
            // Empty string is always the first entry in the string heap
            offsets.insert({ "", 0 });
            stream.push_back('\0');
        }

        string_index insert(std::string_view str)
        {
            uint32_t offset = 0;
            if (auto iter = offsets.find(str); iter != offsets.end())
            {
                offset = iter->second;
            }
            else
            {
                offset = static_cast<uint32_t>(stream.size());
                stream.insert(stream.end(), str.begin(), str.end());
                stream.push_back('\0');
                offsets.insert({ std::string{str}, offset });
            }

            return { offset };
        }

        uint32_t save_size() const noexcept
        {
            return static_cast<uint32_t>(stream.size());
        }

        std::string_view get_string(string_index index) const noexcept
        {
            if (index.offset >= stream.size())
            {
                impl::throw_invalid("string_index out of range");
            }
            auto const end = stream.data() + stream.size();

            auto const first = stream.data() + index.offset;
            auto const last = std::find(first, end, '\0');
            if (last == end)
            {
                impl::throw_invalid("missing null terminator");
            }

            return { first, static_cast<size_t>(last - first) };
        }

    private:
        std::map<std::string, uint32_t, std::less<>> offsets;
        std::vector<char> stream;
    };
}