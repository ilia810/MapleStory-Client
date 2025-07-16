//////////////////////////////////////////////////////////////////////////////
// NoLifeNx - Part of the NoLifeStory project                               //
// Copyright © 2013 Peter Atashian                                          //
//                                                                          //
// This program is free software: you can redistribute it and/or modify     //
// it under the terms of the GNU Affero General Public License as           //
// published by the Free Software Foundation, either version 3 of the       //
// License, or (at your option) any later version.                          //
//                                                                          //
// This program is distributed in the hope that it will be useful,          //
// but WITHOUT ANY WARRANTY; without even the implied warranty of           //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            //
// GNU Affero General Public License for more details.                      //
//                                                                          //
// You should have received a copy of the GNU Affero General Public License //
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    //
//////////////////////////////////////////////////////////////////////////////

#include "file_impl.hpp"
#include "node_impl.hpp"
#include <iostream>
#ifdef _WIN32
#  ifdef _MSC_VER
#    include <codecvt>
#  else
#    include <clocale>
#    include <cstdlib>
#  endif
#  ifdef __MINGW32__
#    include <windows.h>
#  else
#    include <Windows.h>
#  endif // __MINGW32__
#else
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <sys/fcntl.h>
#  include <sys/mman.h>
#  include <unistd.h>
#endif
#include <stdexcept>

namespace nl {
    file::file(std::string name) {
        open(name);
    }
    file::~file() {
        close();
    }
    void file::open(std::string name) {
        close();
        m_data = new data();
#ifdef _WIN32
#  ifdef _MSC_VER
        std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
        auto str = convert.from_bytes(name);
#  else
        std::setlocale(LC_ALL, "en_US.utf8");
        auto str = std::wstring(name.size(), 0);
        auto len = std::mbstowcs(const_cast<wchar_t *>(str.c_str()), name.c_str(), str.size());
        str.resize(len);
#  endif
#  if WINAPI_FAMILY == WINAPI_FAMILY_APP
        m_data->file_handle = ::CreateFile2(str.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);
#  else
        m_data->file_handle = ::CreateFileW(str.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);
#  endif
        if (m_data->file_handle == INVALID_HANDLE_VALUE)
            throw std::runtime_error("Failed to open file " + name);
#  if WINAPI_FAMILY == WINAPI_FAMILY_APP
        m_data->map = ::CreateFileMappingFromApp(m_data->file_handle, 0, PAGE_READONLY, 0, nullptr);
#  else
        m_data->map = ::CreateFileMappingW(m_data->file_handle, 0, PAGE_READONLY, 0, 0, nullptr);
#  endif
        if (!m_data->map)
            throw std::runtime_error("Failed to create file mapping of file " + name);
#  if WINAPI_FAMILY == WINAPI_FAMILY_APP
        m_data->base = ::MapViewOfFileFromApp(m_data->map, FILE_MAP_READ, 0, 0);
#  else
        m_data->base = ::MapViewOfFile(m_data->map, FILE_MAP_READ, 0, 0, 0);
#  endif
        if (!m_data->base)
            throw std::runtime_error("Failed to map view of file " + name);
#else
        m_data->file_handle = ::open(name.c_str(), O_RDONLY);
        if (m_data->file_handle == -1)
            throw std::runtime_error("Failed to open file " + name);
        struct stat finfo;
        if (::fstat(m_data->file_handle, &finfo) == -1)
            throw std::runtime_error("Failed to obtain file information of file " + name);
        m_data->size = finfo.st_size;
        m_data->base = ::mmap(nullptr, m_data->size, PROT_READ, MAP_SHARED, m_data->file_handle, 0);
        if (reinterpret_cast<intptr_t>(m_data->base) == -1)
            throw std::runtime_error("Failed to create memory mapping of file " + name);
#endif
        m_data->header = reinterpret_cast<header const *>(m_data->base);
        if (m_data->header->magic != 0x34474B50)
            throw std::runtime_error(name + " is not a PKG4 NX file");
        m_data->node_table = reinterpret_cast<node::data const *>(reinterpret_cast<char const *>(m_data->base) + m_data->header->node_offset);
        m_data->string_table = reinterpret_cast<uint64_t const *>(reinterpret_cast<char const *>(m_data->base) + m_data->header->string_offset);
        m_data->bitmap_table = reinterpret_cast<uint64_t const *>(reinterpret_cast<char const *>(m_data->base) + m_data->header->bitmap_offset);
        m_data->audio_table = reinterpret_cast<uint64_t const *>(reinterpret_cast<char const *>(m_data->base) + m_data->header->audio_offset);
        
        // v92 NX compatibility: handle files with bitmap_count=0 but containing bitmap nodes
        if (m_data->header->bitmap_count == 0) {
            build_synthetic_bitmap_table(name);
        }
    }
    void file::close() {
        if (!m_data) return;
#ifdef _WIN32
        ::UnmapViewOfFile(m_data->base);
        ::CloseHandle(m_data->map);
        ::CloseHandle(m_data->file_handle);
#else
        ::munmap(const_cast<void *>(m_data->base), m_data->size);
        ::close(m_data->file_handle);
#endif
        delete m_data;
        m_data = nullptr;
    }
    node file::root() const {
        return {m_data->node_table, m_data};
    }
    file::operator node() const {
        return root();
    }
    uint32_t file::string_count() const {
        return m_data->header->string_count;
    }
    uint32_t file::bitmap_count() const {
        // For v92 files, return the synthetic bitmap count if available
        if (m_data->v92_mode && !m_data->synthetic_bitmap_table.empty()) {
            return static_cast<uint32_t>(m_data->synthetic_bitmap_table.size());
        }
        return m_data->header->bitmap_count;
    }
    uint32_t file::audio_count() const {
        return m_data->header->audio_count;
    }
    uint32_t file::node_count() const {
        return m_data->header->node_count;
    }
    std::string file::get_string(uint32_t i) const {
        auto const s = reinterpret_cast<char const *>(m_data->base) + m_data->string_table[i];
        return {s + 2, *reinterpret_cast<uint16_t const *>(s)};
    }
    
    void file::build_synthetic_bitmap_table(const std::string& filename) {
        
        // First pass: count bitmap nodes and check if any exist
        uint32_t bitmap_node_count = 0;
        uint32_t max_bitmap_id = 0;
        
        for (uint32_t i = 0; i < m_data->header->node_count; ++i) {
            const auto& node = m_data->node_table[i];
            if (node.type == nl::node::type::bitmap) {
                bitmap_node_count++;
                if (node.bitmap.index > max_bitmap_id) {
                    max_bitmap_id = node.bitmap.index;
                }
            }
        }
        
        if (bitmap_node_count == 0) {
            return;
        }
        
        
        // Try to find bitmap data block after string data
#ifdef _WIN32
        LARGE_INTEGER file_size;
        if (!GetFileSizeEx(m_data->file_handle, &file_size)) {
            return;
        }
        size_t fileSize = static_cast<size_t>(file_size.QuadPart);
#else
        size_t fileSize = m_data->size;
#endif
        
        // Calculate start of bitmap data (after string data block)
        uint64_t string_data_end = m_data->header->string_offset + (m_data->header->string_count * 8);
        
        // Find the actual end of string data by parsing each string
        uint64_t actual_string_end = m_data->header->string_offset + (m_data->header->string_count * 8);
        for (uint32_t i = 0; i < m_data->header->string_count; ++i) {
            uint64_t str_offset = m_data->string_table[i];
            if (str_offset >= fileSize - 2) continue;
            
            auto str_ptr = reinterpret_cast<char const *>(m_data->base) + str_offset;
            uint16_t str_len = *reinterpret_cast<uint16_t const *>(str_ptr);
            uint64_t str_end = str_offset + 2 + str_len;
            if (str_end > actual_string_end) {
                actual_string_end = str_end;
            }
        }
        
        // Align to 8-byte boundary
        uint64_t bitmap_start = (actual_string_end + 7) & ~7ULL;
        
        
        if (bitmap_start >= fileSize) {
            return;
        }
        
        // Try to build bitmap offset table
        m_data->synthetic_bitmap_table.clear();
        m_data->synthetic_bitmap_table.resize(max_bitmap_id + 1, 0);
        
        uint64_t current_offset = bitmap_start;
        uint32_t images_parsed = 0;
        
        while (current_offset + 4 < fileSize && images_parsed <= max_bitmap_id) {
            // Read length of this image
            auto length_ptr = reinterpret_cast<uint32_t const *>(reinterpret_cast<char const *>(m_data->base) + current_offset);
            uint32_t image_length = *length_ptr;
            
            // Sanity check on length (should be reasonable for compressed image)
            if (image_length == 0 || image_length > 50 * 1024 * 1024) { // Max 50MB per image
                break;
            }
            
            if (current_offset + 4 + image_length > fileSize) {
                break;
            }
            
            // Store offset for this image index
            if (images_parsed < m_data->synthetic_bitmap_table.size()) {
                m_data->synthetic_bitmap_table[images_parsed] = current_offset;
            }
            
            // Move to next image (align to 8-byte boundary)
            current_offset = ((current_offset + 4 + image_length + 7) & ~7ULL);
            images_parsed++;
        }
        
        if (images_parsed > 0) {
            m_data->v92_mode = true;
            // Point bitmap_table to our synthetic table
            m_data->bitmap_table = m_data->synthetic_bitmap_table.data();
        } else {
        }
    }
}
