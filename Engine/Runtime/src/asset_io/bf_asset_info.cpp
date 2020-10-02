#include "bifrost/asset_io/bifrost_asset_info.hpp"

#include "bf/asset_io/bf_file.hpp"
#include "bf/asset_io/bf_path_manip.hpp"

namespace bf
{
  BaseAssetInfo::BaseAssetInfo(const String& full_path, std::size_t length_of_root_path, const BifrostUUID& uuid) :
    bfNonCopyMoveable<BaseAssetInfo>(),
    m_FilePathAbs{full_path},
    m_FilePathRel{m_FilePathAbs.begin() + (length_of_root_path ? length_of_root_path + 1u : 0u), m_FilePathAbs.end()},  // The plus one accounts for the '/'
    m_UUID{uuid},
    m_RefCount{0u},
    m_Tags{},
    m_TypeInfo{nullptr},
    m_SubAssets{&BaseAssetInfo::m_SubAssetListNode},
    m_SubAssetListNode{},
    m_Flags{AssetInfoFlags::DEFAULT}
  {
  }

  void BaseAssetInfo::setDirty(bool value)
  {
    if (value)
    {
      m_Flags |= AssetInfoFlags::IS_DIRTY;
    }
    else
    {
      m_Flags &= ~AssetInfoFlags::IS_DIRTY;
    }
  }

  StringRange BaseAssetInfo::filePathExtenstion() const
  {
    return file::extensionOfFile(m_FilePathAbs);
  }

  StringRange BaseAssetInfo::fileName() const
  {
    return path::name(filePathRel());
  }

  void BaseAssetInfo::addSubAsset(BaseAssetInfo* asset)
  {
    asset->m_Flags |= AssetInfoFlags::IS_SUB_ASSET;
    m_SubAssets.pushBack(*asset);
  }

  void BaseAssetInfo::removeSubAsset(BaseAssetInfo* asset)
  {
    asset->m_Flags &= ~AssetInfoFlags::IS_SUB_ASSET;
    m_SubAssets.erase(m_SubAssets.makeIterator(*asset));
  }
}  // namespace bf
