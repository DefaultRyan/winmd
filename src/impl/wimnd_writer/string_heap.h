#pragma once

#include <unordered_map>
#include <vector>
#include <string_view>
#include <string>
#include <memory>

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
            m_records.push_back({ std::make_unique<std::string>(), 0 });
            m_record_indices.insert({ std::string_view{}, 0 });
        }

        string_handle insert(std::string_view str)
        {
            uint32_t record_index = 0;
            if (auto iter = m_record_indices.find(str); iter != m_record_indices.end())
            {
                record_index = iter->second;
            }
            else
            {
                record_index = static_cast<uint32_t>(m_records.size());
                // Add 1 to reserve space for null terminators
                auto const next_offset = save_size();
                auto value = std::make_unique<std::string>(str);
                std::string_view const key = *value;

                m_records.push_back({ std::move(value), next_offset });
                m_record_indices.insert({ key, record_index });
            }
            
            auto const& record = m_records[record_index];
            return string_handle{ *(record.m_str), record.m_offset };
        }

        uint32_t save_size() const noexcept
        {
            auto& back = m_records.back();
            return back.m_offset + static_cast<uint32_t>(back.m_str->size()) + 1;
        }

    private:
        struct string_record
        {
            std::unique_ptr<std::string> m_str;
            uint32_t m_offset;
        };
        std::unordered_map<std::string_view, uint32_t> m_record_indices;
        std::vector<string_record> m_records;
    };
}