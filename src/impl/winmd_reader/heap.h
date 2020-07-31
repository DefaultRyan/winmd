#pragma once

#include <algorithm>

namespace winmd::writer
{
    struct string_heap
    {
        string_heap()
        {
            put_string(""sv);
        }

        uint32_t put_string(std::string_view const& str)
        {
            auto iter = m_string_to_offset.lower_bound(str);
            if (iter == m_string_to_offset.end() || iter->first != str)
            {
                return insert_new_string(iter, std::string{ str });
            }
            return iter->second;
        }

        uint32_t put_string(std::string&& str)
        {
            auto iter = m_string_to_offset.lower_bound(str);
            if (iter == m_string_to_offset.end() || iter->first != str)
            {
                return insert_new_string(iter, std::move(str));
            }
            return iter->second;
        }

        std::string_view get_string(uint32_t offset) const
        {
            auto iter = m_offset_to_string.find(offset);
            XLANG_ASSERT(iter != m_offset_to_string.end());
            return iter->second;
        }

        uint32_t size() const noexcept
        {
            return m_total_size;
        }

        std::vector<char> write() const noexcept
        {
            std::vector<char> result;
            result.reserve(m_total_size);
            for (auto&& entry : m_offset_to_string)
            {
                XLANG_ASSERT(entry.first == result.size());
                result.insert(result.end(), entry.second.begin(), entry.second.end());
                result.push_back(0);
            }
            XLANG_ASSERT(result.size() == m_total_size);
            return result;
        }

    private:
        uint32_t insert_new_string(std::map<std::string_view, uint32_t>::iterator hint, std::string&& str)
        {
            auto const length = str.size();
            XLANG_ASSERT(length < UINT32_MAX);

            auto const offset = m_total_size;

            auto [iter, success] = m_offset_to_string.emplace(offset, std::move(str));
            XLANG_ASSERT(success);
            m_string_to_offset.emplace_hint(hint, iter->second, offset);

            m_total_size += (static_cast<uint32_t>(length) + 1);

            return iter->first;
        }
        std::map<std::string_view, uint32_t> m_string_to_offset;
        std::map<uint32_t, std::string> m_offset_to_string;
        uint32_t m_total_size = 0;
    };

    struct blob_heap
    {
        blob_heap()
        {
            uint8_t blob[1] = {};
            put_blob({ blob, blob + 1 });
        }

        uint32_t put_blob(reader::byte_view const& blob)
        {
            auto iter = m_blob_to_offset.lower_bound(blob);
            if (iter == m_blob_to_offset.end() || iter->first != blob)
            {
                return insert_new_blob(iter, std::vector<uint8_t>{ blob.begin(), blob.end() });
            }
            return iter->second;
        }
        
        uint32_t put_blob(std::vector<uint8_t>&& blob)
        {
            auto iter = m_blob_to_offset.lower_bound(blob);
            if (iter == m_blob_to_offset.end() || iter->first != blob)
            {
                return insert_new_blob(iter, std::move(blob));
            }
            return iter->second;
        }

        reader::byte_view get_blob(uint32_t offset) const noexcept
        {
            auto iter = m_offset_to_blob.find(offset);
            XLANG_ASSERT(iter != m_offset_to_blob.end());
            return iter->second;
        }

        uint32_t size() const noexcept
        {
            return m_total_size;
        }

        std::vector<uint8_t> write() const
        {
            std::vector<uint8_t> result;
            result.reserve(m_total_size);

            for (auto&& entry : m_offset_to_blob)
            {
                XLANG_ASSERT(entry.first == result.size());
                auto const [preface_length, preface_bytes] = encode_length(entry.second.size());

                result.insert(result.end(), preface_bytes.begin(), preface_bytes.begin() + preface_length);
                result.insert(result.end(), entry.second.begin(), entry.second.end());
            }
            XLANG_ASSERT(result.size() == m_total_size);
            return result;
        }

    private:
        struct comparator
        {
            bool operator()(reader::byte_view const& lhs, reader::byte_view const& rhs) const noexcept
            {
                return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
            }
        };

        uint32_t insert_new_blob(std::map<reader::byte_view, uint32_t, comparator>::iterator hint, std::vector<uint8_t>&& blob)
        {
            XLANG_ASSERT(blob.size() > 0);
            auto const [preface_length, preface_bytes] = encode_length(blob.size());

            auto const offset = m_total_size;

            auto [iter, success] = m_offset_to_blob.emplace(offset, std::move(blob));
            XLANG_ASSERT(success);
            m_blob_to_offset.emplace_hint(hint, iter->second, offset);

            m_total_size += preface_length + static_cast<uint32_t>(iter->second.size());

            return iter->first;
        }

        static std::pair<uint32_t, std::array<uint8_t, 4>> encode_length(size_t blob_size)
        {
            if (blob_size <= 0x7f)
            {
                return { 1, { blob_size & 0xff, 0, 0, 0 } };
            }
            else if (blob_size <= 0x3fff)
            {
                return { 2, { 0x80 | ((blob_size >> 8) & 0x7f), blob_size & 0xff, 0, 0 } };
            }
            else if (blob_size <= 0x01ffffff)
            {
                return { 4, { 0xc0 | ((blob_size >> 24) & 0x3f), (blob_size >> 16) & 0xff, (blob_size >> 8) & 0xff, blob_size & 0xff } };
            }
            else
            {
                impl::throw_invalid("Blob too long");
            }
        }

        std::map<reader::byte_view, uint32_t, comparator> m_blob_to_offset;
        std::map<uint32_t, std::vector<uint8_t>> m_offset_to_blob;
        uint32_t m_total_size = 0;
    };
}
