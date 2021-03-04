/******************************************************************************/
/*!
 * @file   bf_iasset_importer.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   
 *
 * @version 0.0.1
 * @date    2021-03-03
 *
 * @copyright Copyright (c) 2021
 */
/******************************************************************************/
#ifndef BF_IASSET_IMPORTER_HPP
#define BF_IASSET_IMPORTER_HPP

#include "bf/StringRange.hpp"                         // StringRange
#include "bf/data_structures/bifrost_hash_table.hpp"  // HashTable<K, V>

namespace bf
{
  class IDocument;
  class Engine;
  class IMemoryManager;

  struct AssetImportCtx
  {
    IDocument*      document; // Out parameter.
    StringRange     asset_full_path;
    StringRange     meta_full_path;
    void*           importer_user_data;
    IMemoryManager* asset_memory;
    Engine*         engine;
  };

  using AssetImporterFn = void(*)(AssetImportCtx& ctx);

  struct AssetImporter
  {
    AssetImporterFn callback;
    void*           user_data;

    AssetImporter(AssetImporterFn cb, void* ud) :
      callback{cb},
      user_data{ud}
    {
    }
  };

  // TODO(SR):
  //   Use of String is pretty heavy, a StringRange would be better since just about all
  //   file extensions registered are `const char*` hardcoded in the program.
  using ImportRegistry = HashTable<String, AssetImporter>;
}  // namespace bf

#endif /* BF_IASSET_IMPORTER_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2021 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
