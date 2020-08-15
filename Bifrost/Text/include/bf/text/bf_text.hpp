/******************************************************************************/
/*!
 * @file   bf_text.hpp
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
#ifndef BF_TEXT_HPP
#define BF_TEXT_HPP

#include "bf/IMemoryManager.hpp"  // IMemoryManager

#include <cstdint>  // sized integer types

namespace bf
{
  /*!
   * @brief
   *   The listing of text encodings supported by this library.
  */
  enum class TextEncoding : std::uint8_t
  {
    ASCII,     //!< Normal ascii encoding with the range 0 - 255.
    UTF8,      //!< Utf8 encoding
    UTF16_LE,  //!< Utf16 encoding (Little Endian)
    UTF16_BE,  //!< Utf16 encoding (Big Endian)
    UTF32_LE,  //!< Utf32 encoding (Little Endian)
    UTF32_BE,  //!< Utf32 encoding (Big Endian)
    UNKNOWN,   //!< The encoding of a peice of text could not be determined.
  };

  namespace detail
  {
    template<TextEncoding encoding>
    struct CodeUnitImpl; /* Undefined */

    template<>
    struct CodeUnitImpl<TextEncoding::ASCII>
    {
      using type = char;
    };

    template<>
    struct CodeUnitImpl<TextEncoding::UTF8>
    {
      using type = char;
    };

    template<>
    struct CodeUnitImpl<TextEncoding::UTF16_LE>
    {
      using type = std::uint16_t;
    };

    template<>
    struct CodeUnitImpl<TextEncoding::UTF16_BE>
    {
      using type = std::uint16_t;
    };

    template<>
    struct CodeUnitImpl<TextEncoding::UTF32_LE>
    {
      using type = std::uint32_t;
    };

    template<>
    struct CodeUnitImpl<TextEncoding::UTF32_BE>
    {
      using type = std::uint32_t;
    };
  }  // namespace detail

  template<TextEncoding encoding>
  using CodeUnit        = typename detail::CodeUnitImpl<encoding>::type;  //!< The type of a code unit for a certain encoding.
  using CodePoint       = std::uint32_t;                                  //!< A type large enough holding a single unicode codepoint.
  using ImageSizeCoords = std::uint16_t;                                  //!< The data type used for all font atlas operations.

  /*!
   * @brief
   *   Hold a pair of `ImageSizeCoords` for convienence.
   */
  struct ImageSizeCoords2 final
  {
    ImageSizeCoords x;  //!< x axis offset or extent.
    ImageSizeCoords y;  //!< y axis offset or extent.

    /*!
     * @brief
     *   Adds two `ImageSizeCoords2`s together by added each component respectively.
     * 
     * @param lhs
     *   The left hand side of the addition.
     * 
     * @param rhs
     *   The right hand side of the addition.
     * 
     * @return
     *   ImageSizeCoords2 - The new 'ImageSizeCoords2' with the accumulated componenents.
     */
    friend ImageSizeCoords2 operator+(const ImageSizeCoords2& lhs, const ImageSizeCoords2& rhs)
    {
      return ImageSizeCoords2{ImageSizeCoords(lhs.x + rhs.x), ImageSizeCoords(lhs.y + rhs.y)};
    }

    /*!
     * @brief 
     *   Compares two `ImageSizeCoords2`s to see if they are equavalent.
     * 
     * @param lhs
     *   The left hand side of the comparison.
     * 
     * @param rhs
     *   The right hand side of the comparison.
     * 
     * @return
     *   true  - Both the x and y components match.
     *   false - Either the x or y component does not matvh.
     */
    friend bool operator==(const ImageSizeCoords2& lhs, const ImageSizeCoords2& rhs)
    {
      return lhs.x == rhs.x && lhs.y == rhs.y;
    }
  };

  /*!
   * @brief
   *   Chuck of data needed for drawing a particular glyph.
   */
  struct GlyphInfo final
  {
    ImageSizeCoords2 bmp_box[2];   //!< Bitmap Source Rect {x, y, width, height} (Does not include padding).
    float            uvs[4];       //!< {min.x, min.y, max.x, max.y}.
    float            advance_x;    //!< Base x advance, `fontAdditionalAdvance` should be used to get how much MORE to advance the cursor when drawing.
    float            offset[2];    //!< Offset of the glyph {x, y}, need to be used when drawing.
    int              glyph_index;  //!< Optimization for STB functions.
  };

  /*!
   * @brief
   *   CPU side grid of pixels with all of the currently loaded glyphs.
   *   
   *   To get a particular pixel (x, y):
   *     const PixelMap::Pixel* pixel = x + pixmap->width * y;
   */
  struct PixelMap final
  {
    /*!
     * @brief
     *   A single pixel in this grid.
     */
    struct Pixel final
    {
      std::uint8_t rgba[4];  //!< {R, G, B, A} data with each component in the 0 - 255 range.
    };

    ImageSizeCoords width;      //!< The width of the image.
    ImageSizeCoords height;     //!< The height of the image.
    Pixel           pixels[1];  //!< Variable length array of length: `width * height`

    /*!
     * @brief 
     *   Calculates the size of the `PixelMap::pixels` array in number of bytes.
     * 
     * @return
     *   std::size_t - The size of the `PixelMap::pixels` array in bytes.
     */
    std::size_t sizeInBytes() const
    {
      return width * height * sizeof(Pixel);
    }
  };

  /*!
   * @brief
   *   Opaque data type representing a single font.
   */
  struct Font;

  /*!
   * @brief
   *   Result from a utfXXXCodepoint decoding function.
   */
  template<TextEncoding encoding>
  struct TextEncodingResult final
  {
    CodePoint                 codepoint;  //!< The codepoint calculated by the function.
    const CodeUnit<encoding>* endpos;     //!< The new location of the passed in code unit after reading a single codepoint.
  };

  Font*                                      makeFont(IMemoryManager& memory, const char* filename, float size) noexcept;
  bool                                       isAscii(const char* bytes, std::size_t bytes_size) noexcept;
  TextEncoding                               guessEncodingFromBOM(const char* bytes, std::size_t bytes_size) noexcept;
  TextEncodingResult<TextEncoding::UTF8>     utf8Codepoint(const CodeUnit<TextEncoding::UTF8>* characters) noexcept;
  TextEncodingResult<TextEncoding::UTF16_LE> utf16LECodepoint(const CodeUnit<TextEncoding::UTF16_LE>* characters) noexcept;
  TextEncodingResult<TextEncoding::UTF16_BE> utf16BECodepoint(const CodeUnit<TextEncoding::UTF16_BE>* characters) noexcept;
  TextEncodingResult<TextEncoding::UTF32_LE> utf32LECodepoint(const CodeUnit<TextEncoding::UTF32_LE>* characters) noexcept;
  TextEncodingResult<TextEncoding::UTF32_BE> utf32BECodepoint(const CodeUnit<TextEncoding::UTF32_BE>* characters) noexcept;
  bool                                       isValidUtf8(const CodeUnit<TextEncoding::UTF8>* characters, std::size_t num_characters) noexcept;
  GlyphInfo                                  fontGetGlyphInfo(Font* self, CodePoint codepoint) noexcept;
  bool                                       fontAtlasNeedsUpload(const Font* self) noexcept;
  bool                                       fontAtlasHasResized(const Font* self) noexcept;
  void                                       fontResetAtlasStatus(Font* self) noexcept;
  float                                      fontAdditionalAdvance(const Font* self, CodePoint from, CodePoint to) noexcept;
  float                                      fontNewlineHeight(const Font* self) noexcept;
  const PixelMap*                            fontPixelMap(const Font* self) noexcept;
  void                                       destroyFont(Font* font) noexcept;
}  // namespace bf

#endif /* BF_TEXT_HPP */
