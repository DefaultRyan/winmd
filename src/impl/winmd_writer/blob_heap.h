#pragma once

#include "../winmd_reader/view.h"

namespace winmd::writer
{
    struct blob_index
    {
        uint32_t offset;
    };

    struct blob_heap
    {
        blob_heap()
        {
            // First item in blob heap is always a single empty blob
            stream.push_back(0);
        }

        blob_index insert(reader::byte_view view)
        {
            auto offset = save_size();
            auto const length = view.size();
            if (length < 0x80)
            {
                stream.push_back(static_cast<uint8_t>(length));
            }
            else if (length < 0x4000)
            {
                stream.push_back(0x80 | static_cast<uint8_t>(length >> 8));
                stream.push_back(static_cast<uint8_t>(length & 0xff));
            }
            else if (length < 0x20000000)
            {
                stream.push_back(0xc0 | static_cast<uint8_t>(length >> 24));
                stream.push_back(static_cast<uint8_t>((length >> 16) & 0xff));
                stream.push_back(static_cast<uint8_t>((length >> 8) & 0xff));
                stream.push_back(static_cast<uint8_t>(length & 0xff));
            }
            else
            {
                impl::throw_invalid("blob too long");
            }
            stream.insert(stream.end(), view.begin(), view.end());
            return { offset };
        }

        uint32_t save_size() const noexcept
        {
            return static_cast<uint32_t>(stream.size());
        }

    private:
        // TODO: Fold duplicate blobs
        std::vector<uint8_t> stream;
    };
}