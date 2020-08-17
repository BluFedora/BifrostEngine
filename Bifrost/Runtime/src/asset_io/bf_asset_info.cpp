#include "bifrost/asset_io/bifrost_asset_info.hpp"

#include "bf/asset_io/bf_file.hpp"
// #include "bf/asset_io/bf_path_manip.hpp"

namespace bf
{
  BaseAssetInfo::BaseAssetInfo(const String& full_path, const size_t length_of_root_path, const BifrostUUID& uuid) :
    bfNonCopyMoveable<BaseAssetInfo>(),
    m_FilePathAbs{full_path},
    m_FilePathRel{m_FilePathAbs.begin() + length_of_root_path + 1, m_FilePathAbs.end()},  // The plus one accouns for the '/'
    m_UUID{uuid},
    m_RefCount{0u},
    m_Tags{},
    m_IsDirty{false},
    m_TypeInfo{nullptr}
  {
  }

  StringRange BaseAssetInfo::filePathExtenstion() const
  {
    return file::extensionOfFile(m_FilePathAbs);
  }
}  // namespace bf
