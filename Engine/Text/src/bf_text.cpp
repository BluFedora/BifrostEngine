/******************************************************************************/
/*!
 * @file   bf_text.cpp
 * @author Shareef Raheem (http://blufedora.github.io)
 *
 * @brief
 *  The main API for the BluFedora bitmap text library.
 * 
 *  Notes For Myself:
 *    - UTF8 Should be preferred as it seems to have the right balance of complexity to compact representation + compat w/ Ascii.
 *    - UTF32 is a good middle man encoding since each codeunit maps directly to a code unit.
 *    - DO NOT USE Window's "IsTextUnicode" [https://en.wikipedia.org/wiki/Bush_hid_the_facts]
 *    - Line Endings: Windows/DOS CRLF (\r\n, 0x0D 0x0A), Unix LF (\n, 0x0A), Early Mac (no reason to support) CR (\r, 0x0D)
 *      - TODO(SR): Research |Unicode Line Separator (LS)| and |Unicode Paragraph Separator (PS)|
 *    - Plane 1:
 *      - Private Use Area(U+E000 - U+F8FF, 6400): Area to be used for any reason but is not valid as an official unicode codepoint.
 *        - PUAs in other planes: (15, 65534, U+F0000 - U+FFFFD), (16, 65534, U+100000 - U+10FFFD)
 *      - Specials(U+FFF0 - U+FFFF):         '0xFFFD' is for invalid codepoint
 *    - Full unicode support is 21bit (0-10FFFF), but the plane3+ are uncommon, so 18bit would met most of the needs.
 *    - UTF-16 has surrogate pairs so must reserve the range (U+D800 - U+DFFF, 2048)
 * 
 *  Useful Online Tools:
 *    [https://onlineunicodetools.com/generate-unicode-range]
 *
 * @version 1.0.0
 * @date    2020-08-10
 *
 * @copyright Copyright (c) 2020 Shareef Abdoul-Raheem
 */
/******************************************************************************/
#include "bf/text/bf_text.hpp"

#include "bf/MemoryUtils.h"
#include "bf/StlAllocator.hpp"

static void* STBTT_mallocImpl(std::size_t size, void* user_data);
static void  STBTT_freeImpl(void* ptr, void* user_data);

#define STBTT_malloc STBTT_mallocImpl
#define STBTT_free STBTT_freeImpl

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION
#undef STBTT_STATIC

#include <algorithm>  // sort
#include <cstdio>     // File IO
#include <tuple>      // tie
#include <utility>    // pair
#include <vector>     // vector<T>

namespace bf
{
  // Constants

  static constexpr int             k_NumPlanes           = 17;          //!< The number of planes defined in the Unicode standard.
  static constexpr std::uint32_t   k_NumCharsPerPlane    = 0xFFFF + 1;  //!< The max number of characters contained in a single Unicode plane.
  static constexpr std::uint32_t   k_NumGlyphSetPerPlane = 0xFF + 1;    //!< The number of sets each plane is separated into.
  static constexpr std::uint32_t   k_NumCharsPerGlyphSet = 0xFF + 1;
  static constexpr ImageSizeCoords k_InitialImageSize    = 512;
  static constexpr ImageSizeCoords k_ImageGrowAmount     = 512;
  static constexpr float           k_PtToPixels          = -1.3281472327365f;  //!< Negative since that is how em is signified in this library.
  static constexpr float           k_ScaleFactorToPixels = 0.75292857248934f;  //!< This is the conversion from pixels -> pt.

  // Helper Structs

  namespace
  {
    struct Rectangle final
    {
      ImageSizeCoords2 min;
      ImageSizeCoords2 max;

      Rectangle() = default;

      Rectangle(ImageSizeCoords x, ImageSizeCoords y, ImageSizeCoords w, ImageSizeCoords h) :
        min{x, y},
        max{ImageSizeCoords(x + w), ImageSizeCoords(y + h)}
      {
      }

      ImageSizeCoords left() const { return min.x; }
      ImageSizeCoords right() const { return max.x; }
      ImageSizeCoords top() const { return min.y; }
      ImageSizeCoords bottom() const { return max.y; }
      int             width() const { return right() - left(); }
      int             height() const { return bottom() - top(); }
      int             area() const { return width() * height(); }

#if 0
      void            setLeft(ImageSizeCoords value) { min.x = value; }
      void            setRight(ImageSizeCoords value) { max.x = value; }
      void            setTop(ImageSizeCoords value) { min.y = value; }
      void            setBottom(ImageSizeCoords value) { max.y = value; }
#endif
    };
#if 0
    // [http://blackpawn.com/texts/lightmaps/]
    // [https://github.com/TeamHypersomnia/rectpack2D#algorithm]
    class RectanglePacker final
    {
     public:
      std::vector<Rectangle, StlAllocator<Rectangle>> m_Freelist;
      ImageSizeCoords2                                m_OldSize;

      int has_pixels[2];

     public:
      RectanglePacker(IMemoryManager& memory, ImageSizeCoords width, ImageSizeCoords height) :
        m_Freelist{memory},
        m_OldSize{width, height}
      {
        m_Freelist.emplace_back(0u, 0u, width, height);

        has_pixels[0] = width;
        has_pixels[1] = height;
      }

      ImageSizeCoords width() const { return m_OldSize.x; }
      ImageSizeCoords height() const { return m_OldSize.y; }

      void grow(ImageSizeCoords x_amt, ImageSizeCoords y_amt)
      {
        // NOTE(SR):
        //   Since this adds the y first this will give the x growth a greater height.
        //   Swapping these two if's around would make it so the y would have a greater width.
        //   This choice was made because glyphs are typically taller than they are wide.

        /////////////////////////////
        //                         //
        //  ---------------------  //
        //  |             | (2) |  //
        //  | (0) ORGINAL |  X  |  //
        //  |      BLOCK  |  -  |  //
        //  |             | BLO |  //
        //  ---------------  CK |  //
        //  | (1) Y-BLOCK |     |  //
        //  ---------------------  //
        //                         //
        /////////////////////////////

        if (y_amt)  // (1)
        {
          m_Freelist.emplace_back(0u, m_OldSize.y, m_OldSize.x, y_amt);
          m_OldSize.y += y_amt;
        }

        if (x_amt)  // (2)
        {
          m_Freelist.emplace_back(m_OldSize.x, 0u, x_amt, m_OldSize.y);
          m_OldSize.x += x_amt;
        }
      }

      template<typename F>
      void foreachFreeList(F&& f) const
      {
        for (const auto& i : m_Freelist)
        {
          f(i);
        }
      }

      std::pair<bool, ImageSizeCoords2> insert(const ImageSizeCoords2& size_needed)
      {
        assert(size_needed.x > 2 && size_needed.y > 2);

        const auto length = m_Freelist.size();

        for (std::size_t i = length; i-- > 0;)
        {
          const auto candidate = m_Freelist[i];

          if (candidate.width() >= size_needed.x && candidate.height() >= size_needed.y)
          {
            m_Freelist[i] = m_Freelist.back();
            m_Freelist.pop_back();

            if (candidate.width() != size_needed.x || candidate.height() != size_needed.y)
            {
              Rectangle split0, split1;

              if (size_needed.y < size_needed.x)
              {
                split0 =
                 {
                  ImageSizeCoords(candidate.left() + size_needed.x),
                  ImageSizeCoords(candidate.top()),
                  ImageSizeCoords(candidate.width() - size_needed.x),
                  ImageSizeCoords(candidate.height()),
                 };
                split1 =
                 {
                  ImageSizeCoords(candidate.left()),
                  ImageSizeCoords(candidate.top() + size_needed.y),
                  ImageSizeCoords(size_needed.x),
                  ImageSizeCoords(candidate.height() - size_needed.y),
                 };
              }
              else
              {
                split0 =
                 {
                  ImageSizeCoords(candidate.left() + size_needed.x),
                  ImageSizeCoords(candidate.top()),
                  ImageSizeCoords(candidate.width() - size_needed.x),
                  ImageSizeCoords(size_needed.y),
                 };
                split1 =
                 {
                  ImageSizeCoords(candidate.left()),
                  ImageSizeCoords(candidate.top() + size_needed.y),
                  ImageSizeCoords(candidate.width()),
                  ImageSizeCoords(candidate.height() - size_needed.y),
                 };
              }

              if (split1.area() < split0.area())
              {
                std::swap(split0, split1);
              }

              if (split0.area() != 0)
              {
                m_Freelist.push_back(split0);
              }

              if (split1.area() != 0)
              {
                m_Freelist.push_back(split1);
              }
            }

            has_pixels[0] -= size_needed.x;
            has_pixels[1] -= size_needed.y;

            return std::make_pair(true, ImageSizeCoords2{candidate.left(), candidate.top()});
          }
        }

        return std::make_pair(false, ImageSizeCoords2{});
      }
#if 0
      void remove(const Rectangle& rect)
      {
        for (auto& client : m_Freelist)
        {
          if (client.width() == rect.width())
          {
            if (client.bottom() == rect.top())
            {
              client.setBottom(rect.bottom());
              return;
            }

            if (client.top() == rect.bottom())
            {
              client.setTop(rect.top());
              return;
            }
          }

          if (client.height() == rect.height())
          {
            if (client.right() == rect.left())
            {
              client.setRight(rect.right());
              return;
            }

            if (client.left() == rect.right())
            {
              client.setLeft(rect.left());
              return;
            }
          }
        }

        m_Freelist.push_back(rect);
      }
#endif
    };
#else
    // [http://blackpawn.com/texts/lightmaps/]
    // [https://github.com/TeamHypersomnia/rectpack2D#algorithm]
    // This algorithm works best on items sorted by their height.
    class RectanglePacker final
    {
      struct PackRegion final
      {
        const ImageSizeCoords2 region_offset;
        const ImageSizeCoords2 total_area;
        ImageSizeCoords2       current_row_size;
        ImageSizeCoords        current_y;

        PackRegion(ImageSizeCoords2 offset, ImageSizeCoords2 size) :
          region_offset{offset},
          total_area{size},
          current_row_size{0, 0},
          current_y{0}
        {
        }

        std::pair<bool, ImageSizeCoords2> insert(const ImageSizeCoords2& size_needed)
        {
          assert(size_needed.x <= total_area.x);
          assert(size_needed.y <= total_area.y);

        retry_fit:
          const auto new_width = current_row_size.x + size_needed.x;

          // The box can fit in this row
          if (new_width <= total_area.x && current_y + size_needed.y <= total_area.y)
          {
            const auto new_height = std::max(current_row_size.y, size_needed.y);

            const ImageSizeCoords2 position = {current_row_size.x, current_y};

            current_row_size.x = new_width;
            current_row_size.y = new_height;

            return std::make_pair(true, region_offset + position);
          }

          current_y += current_row_size.y;
          current_row_size.x = 0;
          current_row_size.y = 0;

          // The box can fit in this 'new' row.
          if (size_needed.y + current_y <= total_area.y)
          {
            goto retry_fit;
          }

          return std::make_pair(false, ImageSizeCoords2{});
        }
      };

     public:
      std::vector<PackRegion, StlAllocator<PackRegion>> m_PackerRegions;
      ImageSizeCoords2                                  m_OldSize;

     public:
      RectanglePacker(IMemoryManager& memory, ImageSizeCoords width, ImageSizeCoords height) :
        m_PackerRegions{memory},
        m_OldSize{width, height}
      {
        m_PackerRegions.emplace_back(ImageSizeCoords2{0, 0}, m_OldSize);
      }

      ImageSizeCoords width() const { return m_OldSize.x; }
      ImageSizeCoords height() const { return m_OldSize.y; }

      ImageSizeCoords2 insert(const ImageSizeCoords2& size_needed)
      {
        std::pair<bool, ImageSizeCoords2> pack_result;
        const auto                        growth_rect_size = ImageSizeCoords2{k_ImageGrowAmount, k_ImageGrowAmount};

        do
        {
          for (PackRegion& region : m_PackerRegions)
          {
            pack_result = region.insert(size_needed);

            if (pack_result.first)
            {
              return pack_result.second;
            }
          }

          // All packing attepts failed, so add more regions

          m_PackerRegions.emplace_back(ImageSizeCoords2{m_OldSize.x, 0}, growth_rect_size);
          m_PackerRegions.emplace_back(ImageSizeCoords2{0, m_OldSize.y}, growth_rect_size);
          m_PackerRegions.emplace_back(m_OldSize, growth_rect_size);

          m_OldSize.x += growth_rect_size.x;
          m_OldSize.y += growth_rect_size.y;

        } while (true);
      }
    };
#endif

  }  // namespace

  // API Struct Definitions

  struct GlyphSet final
  {
    GlyphInfo glyphs[k_NumCharsPerGlyphSet];
  };

  struct TextPlane final
  {
    GlyphSet* sets[k_NumGlyphSetPerPlane];
  };

  struct Typeface
  {
    IMemoryManager* memory;          //!<
    unsigned char*  font_data;       //!<
    long            font_data_size;  //!<
    stbtt_fontinfo  font_info;       //!<
    int             ascent;          //!<
    int             descent;         //!<
    int             line_gap;        //!<
  };

  struct Font final
  {
    Typeface*       type_face;            // TODO(SR): Shared data amoung fonts
    IMemoryManager* memory;               //
    unsigned char*  font_data;            //
    long            font_data_size;       //
    stbtt_fontinfo  font_info;            //
    float           size;                 // In Pixels
    float           scale_size;           //
    TextPlane*      planes[k_NumPlanes];  //
    float           ascent;               // scale_size * raw_ascent
    float           descent;              // scale_size * raw_descent
    float           line_gap;             // scale_size * raw_line_gap
    PixelMap*       atlas;                // CPU-Texture
    RectanglePacker atlas_packer;         //
    bool            atlas_needs_upload;   //
    bool            atlas_resized;        //

    explicit Font(IMemoryManager& memory) :
      type_face{nullptr},
      memory(&memory),
      font_data(nullptr),
      font_data_size(0),
      font_info(),
      size(0),
      scale_size(0),
      planes{},
      ascent(0),
      descent(0),
      line_gap(0),
      atlas(nullptr),
      atlas_packer{memory, k_InitialImageSize, k_InitialImageSize},
      atlas_needs_upload(false),
      atlas_resized(false)
    {
    }
  };

  // Helpers Fwd Declarations

  namespace
  {
    std::pair<unsigned char*, long> loadFileIntoMemory(IMemoryManager& memory, const char* filename);
    std::uint32_t                   codepointToPlaneIndex(CodePoint codepoint);
    std::uint32_t                   codepointToGlyphSetIndex(CodePoint codepoint);
    std::uint32_t                   codepointToGlyphIndex(CodePoint codepoint);
    TextPlane*                      makeTextPlane(IMemoryManager& memory);
    void                            destroyTextPlane(TextPlane* plane, IMemoryManager& memory);
    GlyphSet*                       makeGlyphSet(Font& font, std::uint32_t set_index);
    void                            destroyGlyphSet(GlyphSet* set, IMemoryManager& memory);
    PixelMap*                       makePixelMap(IMemoryManager& memory, ImageSizeCoords width, ImageSizeCoords height);
    void                            destroyPixelMap(IMemoryManager& memory, PixelMap* pix_map);
  }  // namespace

  // API Implementation

  Font* makeFont(IMemoryManager& memory, const char* filename, float size) noexcept
  {
    Font* self = memory.allocateT<Font>(memory);

    if (self)
    {
      stbtt_fontinfo& font_info = self->font_info;
      int             ascent;
      int             descent;
      int             line_gap;

      // self->memory = &memory; Initialized in ctor

      font_info.userdata = &memory;

      std::tie(self->font_data, self->font_data_size) = loadFileIntoMemory(memory, filename);

      const int err = stbtt_InitFont(&font_info, self->font_data, stbtt_GetFontOffsetForIndex(self->font_data, 0));

      assert(err != 0);

      self->size       = size >= 0.0f ? size : k_PtToPixels * size;
      self->scale_size = size >= 0.0f ? stbtt_ScaleForPixelHeight(&font_info, size) : stbtt_ScaleForMappingEmToPixels(&font_info, -size);

      for (auto& plane : self->planes)
      {
        plane = nullptr;
      }

      stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);

      self->ascent   = self->scale_size * float(ascent);
      self->descent  = self->scale_size * float(descent);
      self->line_gap = self->scale_size * float(line_gap);

      self->atlas              = nullptr;
      self->atlas_needs_upload = false;
      self->atlas_resized      = false;
    }

    return self;
  }

  bool isAscii(const char* bytes, std::size_t bytes_size) noexcept
  {
    const char* bytes_end = bytes + bytes_size;

    while (bytes != bytes_end)
    {
      if (*bytes & 0x80)
      {
        return false;
      }

      ++bytes;
    }

    return true;
  }

  TextEncoding guessEncodingFromBOM(const char* bytes, std::size_t bytes_size) noexcept
  {
    static constexpr auto k_UTF_8_BOM     = "\xEF\xBB\xBF";
    static constexpr auto k_UTF_16_LE_BOM = "\xFF\xFE";
    static constexpr auto k_UTF_16_BE_BOM = "\xFE\xFF";
    static constexpr auto k_UTF_32_LE_BOM = "\xFF\xFE\x00\x00";
    static constexpr auto k_UTF_32_BE_BOM = "\x00\x00\xFE\xFF";

    if (bytes_size >= 2)
    {
      if (memcmp(bytes, k_UTF_16_LE_BOM, 2) == 0)
      {
        return TextEncoding::UTF16_LE;
      }

      if (memcmp(bytes, k_UTF_16_BE_BOM, 2) == 0)
      {
        return TextEncoding::UTF16_BE;
      }

      if (bytes_size >= 3)
      {
        if (memcmp(bytes, k_UTF_8_BOM, 3) == 0)
        {
          return TextEncoding::UTF8;
        }

        if (bytes_size >= 4)
        {
          if (memcmp(bytes, k_UTF_32_LE_BOM, 4) == 0)
          {
            return TextEncoding::UTF32_LE;
          }

          if (memcmp(bytes, k_UTF_32_BE_BOM, 4) == 0)
          {
            return TextEncoding::UTF32_BE;
          }
        }
      }
    }

    return TextEncoding::UNKNOWN;
  }

  TextEncodingResult<TextEncoding::UTF8> utf8Codepoint(const CodeUnit<TextEncoding::UTF8>* characters) noexcept
  {
    static constexpr unsigned int k_InvMask = 0xFFFFFF00;

    CodePoint result;
    int       num_bytes;

    switch (characters[0] & 0xF8)
    {
      case 0xF0:  // 1111 0xxx
      {
        result    = characters[0] & ~(k_InvMask | 0xF0);
        num_bytes = 4;
        break;
      }
      case 0xE0:  // 1110 0xxx
      {
        result    = characters[0] & ~(k_InvMask | 0xE0);
        num_bytes = 3;
        break;
      }
      case 0xC0:  // 1100 0xxx
      {
        result    = characters[0] & ~(k_InvMask | 0xC0);
        num_bytes = 2;
        break;
      }
      default:
      {
        result    = characters[0] & 0xFFFFFF7F;
        num_bytes = 1;
        break;
      }
    }

    for (int i = 1; i < num_bytes; ++i)
    {
      result = (result << 6) | (characters[i] & ~(k_InvMask | 0xC0));
    }

    return TextEncodingResult<TextEncoding::UTF8>{result, characters + num_bytes};
  }

  TextEncodingResult<TextEncoding::UTF16_LE> utf16LECodepoint(const CodeUnit<TextEncoding::UTF16_LE>* characters) noexcept
  {
    if (characters[0] >= 0x0000 && characters[0] <= 0xD7FF || characters[0] >= 0xE000 && characters[0] <= 0xFFFF)
    {
      return TextEncodingResult<TextEncoding::UTF16_LE>{characters[0], characters + 1};
    }

    const CodePoint codepoint = characters[0] | characters[1] << 10;

    return TextEncodingResult<TextEncoding::UTF16_LE>{codepoint, characters + 2};
  }

  TextEncodingResult<TextEncoding::UTF16_BE> utf16BECodepoint(const CodeUnit<TextEncoding::UTF16_BE>* characters) noexcept
  {
    if (characters[0] >= 0x0000 && characters[0] <= 0xD7FF || characters[0] >= 0xE000 && characters[0] <= 0xFFFF)
    {
      return TextEncodingResult<TextEncoding::UTF16_BE>{characters[0], characters + 1};
    }

    const CodePoint codepoint = characters[1] | characters[0] << 10;

    return TextEncodingResult<TextEncoding::UTF16_BE>{codepoint, characters + 2};
  }

  TextEncodingResult<TextEncoding::UTF32_LE> utf32LECodepoint(const CodeUnit<TextEncoding::UTF32_LE>* characters) noexcept
  {
    return TextEncodingResult<TextEncoding::UTF32_LE>{bfBytesReadUint32LE(reinterpret_cast<const bfByte*>(characters)), characters + 1};
  }

  TextEncodingResult<TextEncoding::UTF32_BE> utf32BECodepoint(const CodeUnit<TextEncoding::UTF32_BE>* characters) noexcept
  {
    return TextEncodingResult<TextEncoding::UTF32_BE>{bfBytesReadUint32BE(reinterpret_cast<const bfByte*>(characters)), characters + 1};
  }

  bool isValidUtf8(const CodeUnit<TextEncoding::UTF8>* characters, std::size_t num_characters) noexcept
  {
    const auto character_end = characters + num_characters;

    while (characters != character_end)
    {
      const auto res            = utf8Codepoint(characters);
      const auto num_units_used = res.endpos - characters;

      for (std::ptrdiff_t i = 1; i < num_units_used; ++i)
      {
        if ((characters[i] & 0xC0) != 0x80)
        {
          return false;
        }
      }

      switch (num_units_used)
      {
        case 1:
        {
          if (res.codepoint > 0x7F)
          {
            return false;
          }

          break;
        }
        case 2:
        {
          break;
        }
        case 3:
        {
          if (res.codepoint < 0x0800 || (res.codepoint >> 11) == 0x1B)
          {
            return false;
          }

          break;
        }
        case 4:
        {
          if (res.codepoint < 0x10000 || 0x10FFFF < res.codepoint)
          {
            return false;
          }

          break;
        }
        default:
        {
          return false;
        }
      }

      characters = res.endpos;

      if (characters > character_end)
      {
        // Over indexing
        return false;
      }
    }

    return true;
  }

  GlyphInfo fontGetGlyphInfo(Font* self, CodePoint codepoint) noexcept
  {
    const std::uint32_t text_plane_idx = codepointToPlaneIndex(codepoint);
    const std::uint32_t glyph_set_idx  = codepointToGlyphSetIndex(codepoint);
    const std::uint32_t glyph_idx      = codepointToGlyphIndex(codepoint);

    assert(text_plane_idx < k_NumPlanes);
    assert(glyph_set_idx < k_NumGlyphSetPerPlane);
    assert(glyph_idx < k_NumCharsPerGlyphSet);

    TextPlane*& text_plane = self->planes[text_plane_idx];

    if (!text_plane) { text_plane = makeTextPlane(*self->memory); }

    GlyphSet*& glyph_set = text_plane->sets[glyph_set_idx];

    if (!glyph_set) { glyph_set = makeGlyphSet(*self, glyph_set_idx); }

    return glyph_set->glyphs[glyph_idx];
  }

  bool fontAtlasNeedsUpload(const Font* self) noexcept
  {
    return self->atlas_needs_upload;
  }

  bool fontAtlasHasResized(const Font* self) noexcept
  {
    return self->atlas_resized;
  }

  void fontResetAtlasStatus(Font* self) noexcept
  {
    self->atlas_needs_upload = false;
    self->atlas_resized      = false;
  }

  float fontAdditionalAdvance(const Font* self, CodePoint from, CodePoint to) noexcept
  {
    return float(stbtt_GetGlyphKernAdvance(&self->font_info, from, to));
  }

  float fontNewlineHeight(const Font* self) noexcept
  {
    return (self->ascent - self->descent + self->line_gap) * k_ScaleFactorToPixels;
  }

  const PixelMap* fontPixelMap(const Font* self) noexcept
  {
    return self->atlas;
  }

  void destroyFont(Font* font) noexcept
  {
    if (font->atlas)
    {
      destroyPixelMap(*font->memory, font->atlas);
    }

    for (auto& plane : font->planes)
    {
      if (plane)
      {
        destroyTextPlane(plane, *font->memory);
      }
    }

    font->memory->deallocate(font->font_data, font->font_data_size);
    font->memory->deallocateT(font);
  }

  // Helper Definitions

  namespace
  {
    std::pair<unsigned char*, long> loadFileIntoMemory(IMemoryManager& memory, const char* const filename)
    {
      FILE*          f      = fopen(filename, "rb");  // NOLINT(android-cloexec-fopen)
      unsigned char* buffer = NULL;
      long           file_size;

      if (f)
      {
        fseek(f, 0, SEEK_END);
        file_size = ftell(f);
        fseek(f, 0, SEEK_SET);  //same as rewind(f);
        buffer = static_cast<unsigned char*>(memory.allocate(file_size + std::size_t(1)));

        if (buffer)
        {
          fread(buffer, sizeof(buffer[0]), file_size, f);
          buffer[file_size] = '\0';
        }
        else
        {
          file_size = 0;
        }

        fclose(f);
      }
      else
      {
        file_size = 0;
      }

      return {buffer, file_size};
    }

    //
    // Codepoint -> TextPlane index : Codepoint / k_NumCharsPerPlane
    // Codepoint -> GlyphSet  index : Codepoint % k_NumCharsPerPlane / k_NumGlyphSetPerPlane
    // Codepoint -> GlyphInfo index : codepoint % k_NumCharsPerGlyphSet
    //

    /* Sanity Checks
      const unsigned codepoint = 0x102A2;
      const unsigned i0        = codepoint / k_NumCharsPerPlane;
      const unsigned i1        = codepoint % k_NumCharsPerPlane / k_NumGlyphSetPerPlane;
      const unsigned i2        = codepoint % k_NumCharsPerGlyphSet;
    */

    std::uint32_t codepointToPlaneIndex(CodePoint codepoint)
    {
      return codepoint / k_NumCharsPerPlane;
    }

    std::uint32_t codepointToGlyphSetIndex(CodePoint codepoint)
    {
      return codepoint % k_NumCharsPerPlane / k_NumGlyphSetPerPlane;
    }

    std::uint32_t codepointToGlyphIndex(CodePoint codepoint)
    {
      return codepoint % k_NumCharsPerGlyphSet;
    }

    TextPlane* makeTextPlane(IMemoryManager& memory)
    {
      TextPlane* self = memory.allocateT<TextPlane>();

      if (self)
      {
        for (auto& set : self->sets)
        {
          set = nullptr;
        }
      }

      return self;
    }

    void destroyTextPlane(TextPlane* plane, IMemoryManager& memory)
    {
      for (auto& set : plane->sets)
      {
        if (set)
        {
          destroyGlyphSet(set, memory);
        }
      }

      memory.deallocateT(plane);
    }

    GlyphSet* makeGlyphSet(Font& font, std::uint32_t set_index)
    {
      static constexpr int   k_Padding             = 2;
      static constexpr int   k_HalfPadding         = k_Padding / 2;
      static constexpr float k_SubpixelShiftX      = 0.0f;
      static constexpr float k_SubpixelShiftY      = 0.0f;
      static constexpr float k_SubpixelOversampleX = 1.0f;
      static constexpr float k_SubpixelOversampleY = 1.0f;

      static_assert((k_Padding & 1) == 0, "Padding must be divisible by 2 evenly");

      IMemoryManager& memory = *font.memory;
      GlyphSet*       self   = memory.allocateT<GlyphSet>();

      if (self)
      {
        const float      scale           = font.scale_size;
        auto&            packer          = font.atlas_packer;
        ImageSizeCoords2 largest_bmp     = {0u, 0u};
        const CodePoint  first_codepoint = set_index * k_NumCharsPerGlyphSet;
        GlyphInfo*       sorted_glyphs[k_NumCharsPerGlyphSet];

        // Grab all the sizing info

        for (std::uint32_t i = 0; i < k_NumCharsPerGlyphSet; ++i)
        {
          GlyphInfo&      info      = self->glyphs[i];
          const CodePoint codepoint = first_codepoint + i;
          int             raw_advance;
          int             left_side_bearing;
          int             x0, y0, x1, y1;

          sorted_glyphs[i] = &info;

          info.glyph_index = stbtt_FindGlyphIndex(&font.font_info, codepoint);

          if (!info.glyph_index)
          {
            continue;
          }

          stbtt_GetGlyphHMetrics(&font.font_info, info.glyph_index, &raw_advance, &left_side_bearing);
          stbtt_GetGlyphBitmapBoxSubpixel(&font.font_info, info.glyph_index, scale, scale, k_SubpixelShiftX, k_SubpixelShiftY, &x0, &y0, &x1, &y1);

          info.offset[0]    = float(x0);
          info.offset[1]    = float(y0);
          info.bmp_box[1].x = ImageSizeCoords(x1 - x0);
          info.bmp_box[1].y = ImageSizeCoords(y1 - y0);
          info.advance_x    = float(raw_advance) * scale;

          largest_bmp.x = std::max(largest_bmp.x, info.bmp_box[1].x);
          largest_bmp.y = std::max(largest_bmp.y, info.bmp_box[1].y);
        }

        // Sort from greatest to least height

        std::sort(std::begin(sorted_glyphs), std::end(sorted_glyphs), [](GlyphInfo* a, GlyphInfo* b) {
          return a->bmp_box[1].y > b->bmp_box[1].y;
        });

        // Try to pack into the atlas

        for (auto* const info : sorted_glyphs)
        {
          if (!info->glyph_index || info->bmp_box[1].x == 0 || info->bmp_box[1].y == 0)
          {
            continue;
          }

          const ImageSizeCoords2 padded_size = info->bmp_box[1] + ImageSizeCoords2{k_Padding, k_Padding};

          info->bmp_box[0] = packer.insert(padded_size);
        }

        // Update the atlas with the packed glyphs

        font.atlas_needs_upload = true;

        if (!font.atlas || font.atlas->width != packer.width() || font.atlas->height != packer.height())
        {
          if (font.atlas)
          {
            destroyPixelMap(memory, font.atlas);
          }

          font.atlas         = makePixelMap(memory, packer.width(), packer.height());
          font.atlas_resized = true;
        }

        // Rasterize the packed glyphs into the correct locations

        PixelMap::Pixel*     pixels                = font.atlas->pixels;
        const auto           aux_bmp_buffer_length = std::size_t(largest_bmp.x) * largest_bmp.y * sizeof(unsigned char);
        unsigned char* const aux_bmp_buffer        = (unsigned char*)memory.allocate(aux_bmp_buffer_length);

        for (auto& info : self->glyphs)
        {
          const auto bmp_width  = info.bmp_box[1].x;
          const auto bmp_height = info.bmp_box[1].y;

          info.uvs[0] = float(info.bmp_box[0].x + k_HalfPadding) / float(packer.width());
          info.uvs[1] = float(info.bmp_box[0].y + k_HalfPadding) / float(packer.height());
          info.uvs[2] = info.uvs[0] + float(info.bmp_box[1].x) / float(packer.width());
          info.uvs[3] = info.uvs[1] + float(info.bmp_box[1].y) / float(packer.height());

          if (!info.glyph_index || info.bmp_box[1].x == 0 || info.bmp_box[1].y == 0)
          {
            continue;
          }

          stbtt_MakeGlyphBitmapSubpixel(
           &font.font_info,
           aux_bmp_buffer,
           bmp_width,
           bmp_height,
           bmp_width,
           scale * k_SubpixelOversampleX,
           scale * k_SubpixelOversampleY,
           k_SubpixelShiftX,
           k_SubpixelShiftY,
           info.glyph_index);

          for (ImageSizeCoords y = 0; y < bmp_height; ++y)
          {
            const auto y_src_offset = std::size_t(y) * bmp_width;
            const auto y_dst_offset = (std::size_t(info.bmp_box[0].y) + y + k_HalfPadding) * font.atlas->width;

            for (ImageSizeCoords x = 0; x < bmp_width; ++x)
            {
              const unsigned char*   src_alpha = aux_bmp_buffer + std::size_t(x) + y_src_offset;
              PixelMap::Pixel* const dst_pixel = pixels + (std::size_t(info.bmp_box[0].x) + x + k_HalfPadding) + y_dst_offset;

              // Memory initialized to 0xFF in 'makePixelMap'.
              //   dst_pixel->rgba[0] = 0xFF;
              //   dst_pixel->rgba[1] = 0xFF;
              //   dst_pixel->rgba[2] = 0xFF;
              dst_pixel->rgba[3] = *src_alpha;
            }
          }
        }

        memory.deallocate(aux_bmp_buffer, aux_bmp_buffer_length);
      }

      return self;
    }

    void destroyGlyphSet(GlyphSet* set, IMemoryManager& memory)
    {
      memory.deallocateT(set);
    }

    PixelMap* makePixelMap(IMemoryManager& memory, ImageSizeCoords width, ImageSizeCoords height)
    {
      const std::size_t num_pixels  = std::size_t(width) * height;
      const std::size_t pixels_size = num_pixels * sizeof(PixelMap::Pixel);
      PixelMap* const   self        = (PixelMap*)memory.allocate(offsetof(PixelMap, pixels) + pixels_size);

      if (self)
      {
        PixelMap::Pixel* const pixels = self->pixels;

        self->width  = width;
        self->height = height;

#if 0 /* NOTE(SR): This is so that I can debug individual channels */
        for (std::size_t i = 0; i < num_pixels; ++i)
        {
          pixels[i].rgba[0] = 0x00;
          pixels[i].rgba[1] = 0xFF;
          pixels[i].rgba[2] = 0x00;
          pixels[i].rgba[3] = 0x00;
        }
#else
        std::memset(pixels, 0xFF, pixels_size);
#endif
      }

      return self;
    }

    void destroyPixelMap(IMemoryManager& memory, PixelMap* pix_map)
    {
      memory.deallocateT(pix_map);
    }
  }  // namespace
}  // namespace bf

static void* STBTT_mallocImpl(std::size_t size, void* user_data)
{
  bf::IMemoryManager* const memory     = (bf::IMemoryManager*)user_data;
  std::size_t* const        allocation = (std::size_t*)memory->allocate(sizeof(std::size_t) + size);

  allocation[0] = size;

  return allocation + 1u;
}

static void STBTT_freeImpl(void* ptr, void* user_data)
{
  bf::IMemoryManager* const memory     = (bf::IMemoryManager*)user_data;
  std::size_t* const        allocation = (std::size_t*)ptr - 1u;

  memory->deallocate(allocation, allocation[0]);
}
