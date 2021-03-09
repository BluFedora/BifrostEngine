#include "bf/anim2D/bf_anim2D_api.h"

#include "bf/anim2D/bf_anim2D_network.h"  // bfAnim2DPacketHeader

#include <bf/DenseMap.hpp>        // DenseMap
#include <bf/IMemoryManager.hpp>  // IMemoryManager
#include <bf/MemoryUtils.h>       // bfBytesReadUint*
#include <bf/bf_net.hpp>          // Networking API

#include <assert.h> /* assert              */
#include <stdlib.h> /* realloc, free, NULL */

// Helper Struct Definitions

// NOTE(SR):
//   The two levels of indirection here is kinda gross
//   but is needed for code reuse, maybe that is a bad goal / reason ...
struct CallbackAllocator final : public bf::IMemoryManager
{
  void*             user_data;
  bfAnim2DAllocator alloc_fn;

  CallbackAllocator(void* ud, bfAnim2DAllocator alloc) :
    bf::IMemoryManager(),
    user_data{ud},
    alloc_fn{alloc}
  {
  }

  void* allocate(std::size_t size) override
  {
    return alloc_fn(nullptr, 0u, size, user_data);
  }

  void deallocate(void* ptr, std::size_t old_size) override
  {
    alloc_fn(ptr, old_size, 0u, user_data);
  }
};

// Struct Definitions

struct NetworkingData;

struct bfAnim2DCtx /*!< @copydoc bfAnim2DCtx */
{
  bfAnim2DCreateParams params; /*!< */
  CallbackAllocator    allocator;
  bfSpritesheet*       spritesheet_list; /*!< */
  NetworkingData*      network_module;

  bfAnim2DCtx(const bfAnim2DCreateParams& params) :
    params{params},
    allocator{params.user_data, params.allocator},
    spritesheet_list{nullptr},
    network_module{nullptr}
  {
  }
};

struct NetworkingData final
{
  static constexpr int k_PacketReadSize = 8192 * 4;

  bfNet::RequestURL    url;
  bfNet::Socket        socket;
  bfNet::Address       address;
  bool                 is_connected;
  bf::Array<char>      current_packet;
  bfAnim2DPacketHeader current_packet_header;
  char                 read_buffer[k_PacketReadSize];

  NetworkingData(CallbackAllocator& allocator) :
    url{bfNet::RequestURL::create("localhost", k_bfSRSMServerPort)},
    socket{},
    address{bfNet::makeAddress(bfNet::NetworkFamily::IPv4, url.ip_address, k_bfSRSMServerPort)},
    is_connected{false},
    current_packet{allocator},
    current_packet_header{0u, 0u, {}},
    read_buffer{}
  {
  }

  void establishConnection()
  {
    if (!socket)
    {
      socket = bfNet::createSocket(bfNet::NetworkFamily::IPv4, bfNet::SocketType::TCP);
      (void)socket.makeNonBlocking();
    }

    if (socket)
    {
      if (!is_connected)
      {
        const auto err = socket.connectTo(address);

        is_connected = err.isSuccess() || bfNet::isErrorAlreadyConnected(err.code);
      }
    }
  }

  bool readPackets(bfAnim2DCtx* self, bfAnim2DChangeEvent* out_event);
};

// Helpers Fwd Declarations

template<typename T>
static void bfPrependDoublyLL(T*& head, T* item);
template<typename T>
static void bfRemoveDoublyLL(T*& head, T* item);

static void*         bfDefaultAllocator(void* ptr, size_t old_size, size_t new_size, void* user_data);
static void*         bfAllocate(const bfAnim2DCtx* parent, size_t size);
static void          bfFree(const bfAnim2DCtx* parent, void* ptr, size_t size);
static bfOwnedString bfStringClone(const bfAnim2DCtx* parent, const bfStringSpan& string);
static void          bfStringFree(const bfAnim2DCtx* parent, bfOwnedString string);
static bool          bfLoadUpSpritesheetFromData(bfAnim2DCtx* self, bfSpritesheet* sheet, const uint8_t* srsm_bytes, size_t num_srsm_bytes);

// API Definitions

bfAnim2DCtx* bfAnim2D_new(const bfAnim2DCreateParams* params)
{
  bfAnim2DCreateParams safe_params = *params;

  if (!safe_params.allocator)
  {
    safe_params.allocator = bfDefaultAllocator;
  }

  bfAnim2DCtx* self = new (safe_params.allocator(nullptr, 0u, sizeof(*self), safe_params.user_data)) bfAnim2DCtx(safe_params);

  return self;
}

void* bfAnim2D_userData(const bfAnim2DCtx* self)
{
  return self->params.user_data;
}

bfBool8 bfAnim2D_networkClientUpdate(bfAnim2DCtx* self, bfAnim2DChangeEvent* out_event)
{
  if (out_event)
  {
    if (!self->network_module)
    {
      bfNet::Startup();
      self->network_module = new (bfAllocate(self, sizeof(*self->network_module))) NetworkingData(self->allocator);
    }

    if (self->network_module)
    {
      self->network_module->establishConnection();
      return self->network_module->readPackets(self, out_event);
    }
  }

  return false;
}

bfSpritesheet* bfAnim2D_loadSpritesheet(bfAnim2DCtx* self, bfStringSpan name, const uint8_t* srsm_bytes, size_t num_srsm_bytes)
{
  bfSpritesheet* sheet = new (bfAllocate(self, sizeof(bfSpritesheet))) bfSpritesheet;

  sheet->name           = bfStringClone(self, name);
  sheet->user_data      = nullptr;
  sheet->animations     = nullptr;
  sheet->uvs            = nullptr;
  sheet->num_animations = 0;
  sheet->num_uvs        = 0;
  memset(sheet->guid, '\0', sizeof(sheet->guid));

  bfPrependDoublyLL(self->spritesheet_list, sheet);

  if (!bfLoadUpSpritesheetFromData(self, sheet, srsm_bytes, num_srsm_bytes))
  {
    bfAnim2D_destroySpritesheet(self, sheet);
    sheet = nullptr;
  }

  return sheet;
}

void bfAnim2D_stepFrame(
 bfAnim2DUpdateInfo*   sprites,
 const bfSpritesheet** spritesheets,
 uint16_t              num_sprites,
 float                 delta_time)
{
  for (uint16_t i = 0; i < num_sprites; ++i)
  {
    bfAnim2DUpdateInfo& sprite               = sprites[i];
    const float         playback_speed       = sprite.playback_speed;
    const bool          playback_is_positive = playback_speed >= 0.0f;
    const float         abs_playback_speed   = playback_is_positive ? +playback_speed : -playback_speed;
    const float         playback_delta       = abs_playback_speed * delta_time;
    float               time_left_for_frame  = sprite.time_left_for_frame - playback_delta;
    uint32_t            current_frame        = sprite.current_frame;
    bool                has_finished_playing = false;

    if (time_left_for_frame <= 0.0f)
    {
      const bfAnimation& animation            = spritesheets[sprite.spritesheet_idx]->animations[sprite.animation];
      const uint32_t     num_frames_minus_one = animation.num_frames - 1;
      const uint32_t     last_frame_in_anim   = playback_is_positive ? num_frames_minus_one : 0;

      if (current_frame != last_frame_in_anim)
      {
        current_frame += playback_is_positive ? +1 : -1;
      }
      else
      {
        if (sprite.is_looping)
        {
          current_frame = playback_is_positive ? 0 : num_frames_minus_one;  // Reset to first frame
          // current_frame = num_frames_minus_one - last_frame_in_anim;  // Alternative way to calculate the same thing
        }

        has_finished_playing = true;
      }

      if (current_frame <= num_frames_minus_one)
      {
        time_left_for_frame = animation.frames[current_frame].frame_time;
      }
    }

    // Write results from calculations back to the sprite.
    sprite.time_left_for_frame  = time_left_for_frame;
    sprite.current_frame        = current_frame;
    sprite.has_finished_playing = has_finished_playing;
  }
}

static void bfAnim2D_clearSpritesheet(bfAnim2DCtx* self, bfSpritesheet* spritesheet)
{
  for (uint32_t a = 0; a < spritesheet->num_animations; ++a)
  {
    bfAnimation* const anim = spritesheet->animations + a;

    bfStringFree(self, anim->name);
    bfFree(self, anim->frames, sizeof(*anim->frames) * anim->num_frames);
  }

  bfFree(self, spritesheet->uvs, sizeof(*spritesheet->uvs) * spritesheet->num_uvs);
}

void bfAnim2D_destroySpritesheet(bfAnim2DCtx* self, bfSpritesheet* spritesheet)
{
  bfStringFree(self, spritesheet->name);
  bfAnim2D_clearSpritesheet(self, spritesheet);

  bfRemoveDoublyLL(self->spritesheet_list, spritesheet);
  spritesheet->~bfSpritesheet();
  bfFree(self, spritesheet, sizeof(*spritesheet));
}

void bfAnim2D_delete(bfAnim2DCtx* self)
{
  if (self->network_module)
  {
    self->network_module->~NetworkingData();
    bfFree(self, self->network_module, sizeof(*self->network_module));
    bfNet::Shutdown();
  }

  while (self->spritesheet_list)
  {
    bfAnim2D_destroySpritesheet(self, self->spritesheet_list);
  }

  bfFree(self, self, sizeof(*self));
}

// Helper Definitions

template<typename T>
static void bfPrependDoublyLL(T*& head, T* item)
{
  item->prev = nullptr;
  item->next = head;

  if (head)
  {
    head->prev = item;
  }

  head = item;
}

template<typename T>
static void bfRemoveDoublyLL(T*& head, T* item)
{
  if (item->prev)
  {
    item->prev->next = item->next;
  }
  else
  {
    head = item->next;
  }

  if (item->next)
  {
    item->next->prev = item->prev;
  }
}

static void* bfDefaultAllocator(void* ptr, size_t old_size, size_t new_size, void* user_data)
{
  (void)user_data;
  (void)old_size;

  /*
    NOTE(Shareef):
      "if new_size is zero, the behavior is implementation defined
      (null pointer may be returned in which case the old memory block may or may not be freed),
      or some non-null pointer may be returned that may
      not be used to access storage."
  */
  if (new_size == 0u)
  {
    free(ptr);
    ptr = nullptr;
  }
  else
  {
    void* const new_ptr = realloc(ptr, new_size);

    if (!new_ptr)
    {
      /*
        NOTE(Shareef):
          As to not leak memory, realloc says:
            "If there is not enough memory, the old memory block is not freed and null pointer is returned."
      */
      free(ptr);
    }

    ptr = new_ptr;
  }

  return ptr;
}

static void* bfAllocate(const bfAnim2DCtx* parent, size_t size)
{
  return parent->params.allocator(nullptr, 0u, size, parent->params.user_data);
}

static void bfFree(const bfAnim2DCtx* parent, void* ptr, size_t size)
{
  parent->params.allocator(ptr, size, 0u, parent->params.user_data);
}

static bfOwnedString bfStringClone(const bfAnim2DCtx* parent, const bfStringSpan& string)
{
  bfOwnedString ret;

  ret.str     = static_cast<char*>(bfAllocate(parent, string.str_len + 1));
  ret.str_len = string.str_len;

  std::memcpy(ret.str, string.str, string.str_len);

  ret.str[ret.str_len] = '\0';

  return ret;
}

static void bfStringFree(const bfAnim2DCtx* parent, bfOwnedString string)
{
  bfFree(parent, string.str, string.str_len + 1);
}

bool bfLoadUpSpritesheetFromData(bfAnim2DCtx* self, bfSpritesheet* sheet, const uint8_t* srsm_bytes, size_t num_srsm_bytes)
{
  // TODO(SR): ADD ERROR CHECKS ON EACH ACCESS.

  static constexpr std::size_t k_HeaderSize = sizeof(uint32_t) +  // "SRSM"
                                              sizeof(uint16_t) +  // data_offset
                                              sizeof(uint8_t) +   // version
                                              sizeof(uint8_t) +   // num_chunks
                                              sizeof(uint16_t) +  // atlas_width
                                              sizeof(uint16_t);   // atlas_height

  // Invalid since all srsm files have atleast a header.
  if (num_srsm_bytes < k_HeaderSize)
  {
    return false;
  }

  const bfByte* const bytes_start  = srsm_bytes;
  const bfByte* const bytes_end    = bytes_start + num_srsm_bytes;
  const bfByte*       bytes        = bytes_start;
  uint32_t            header_magic = bfBytesReadUint32LE(bytes);
  bytes += sizeof(uint32_t);

  // Invalid file type missing magic tag.
  if (memcmp(&header_magic, "SRSM", 4) != 0)
  {
    return false;
  }

  uint16_t header_data_offset = bfBytesReadUint16LE(bytes);
  bytes += sizeof(header_data_offset);

  uint8_t header_version = bfBytesReadUint8LE(bytes);
  bytes += sizeof(header_version);

  uint8_t header_num_chunks = bfBytesReadUint8LE(bytes);
  bytes += sizeof(header_num_chunks);

  uint16_t header_atlas_width = bfBytesReadUint16LE(bytes);
  bytes += sizeof(header_atlas_width);

  uint16_t header_atlas_height = bfBytesReadUint16LE(bytes);
  bytes += sizeof(header_atlas_height);

  // TODO(SR): Handle version mismatch? Or do I consider a bump in version to be a breaking change?
  // Version mismatch
  if (header_version != k_bfAnim2DVersion)
  {
    return false;
  }

  // TODO(SR):
  //   I can probably do a smarter allocation scheme,
  //   such as not freeing if there are minimal changes but this is simpler.

  // sheet->name           = bfStringClone(self, name);
  // sheet->user_data      = nullptr;
  bfAnim2D_clearSpritesheet(self, sheet);
  sheet->animations     = nullptr;
  sheet->uvs            = nullptr;
  sheet->num_animations = 0;
  sheet->num_uvs        = 0;
  memset(sheet->guid, '\0', sizeof(sheet->guid));

  const bfByte* const chucks_start = bytes_start + header_data_offset;
  const bfByte*       chucks       = chucks_start;

  for (uint8_t i = 0; i < header_num_chunks; ++i)
  {
    uint8_t chunk_type[4];

    memcpy(chunk_type, chucks, sizeof(chunk_type));
    chucks += sizeof(chunk_type);

    uint32_t chunk_data_length = bfBytesReadUint32LE(chucks);
    chucks += sizeof(chunk_data_length);

    const bfByte* chuck_data = chucks;

    if (memcmp(chunk_type, "FRME", 4) == 0)
    {
      struct FrameData
      {
        uint32_t image_x;       // units = pixels
        uint32_t image_y;       // units = pixels
        uint32_t image_width;   // units = pixels
        uint32_t image_height;  // units = pixels
      };

      const uint32_t num_frames = bfBytesReadUint32LE(chuck_data);
      chuck_data += sizeof(num_frames);

      sheet->uvs     = (bfUVRect*)bfAllocate(self, sizeof(bfUVRect) * num_frames);
      sheet->num_uvs = num_frames;

      for (uint32_t f = 0; f < num_frames; ++f)
      {
        const FrameData* frame_data = (const FrameData*)chuck_data;

        sheet->uvs[f].x      = float(frame_data->image_x) / float(header_atlas_width);
        sheet->uvs[f].y      = float(frame_data->image_y) / float(header_atlas_height);
        sheet->uvs[f].width  = float(frame_data->image_width) / float(header_atlas_width);
        sheet->uvs[f].height = float(frame_data->image_height) / float(header_atlas_height);

        chuck_data += sizeof(FrameData);
      }
    }
    else if (memcmp(chunk_type, "ANIM", 4) == 0)
    {
      const uint32_t num_animations = bfBytesReadUint32LE(chuck_data);
      chuck_data += sizeof(num_animations);

      sheet->animations     = (bfAnimation*)bfAllocate(self, sizeof(bfAnimation) * num_animations);
      sheet->num_animations = num_animations;

      for (uint32_t a = 0; a < num_animations; ++a)
      {
        bfAnimation* const anim = sheet->animations + a;

        bfStringSpan anim_name;
        anim_name.str_len = bfBytesReadUint32LE(chuck_data);
        chuck_data += sizeof(uint32_t);
        anim_name.str = (char*)chuck_data;
        chuck_data += anim_name.str_len + 1;

        const uint32_t num_frames = bfBytesReadUint32LE(chuck_data);
        chuck_data += sizeof(num_frames);

        anim->name       = bfStringClone(self, anim_name);
        anim->frames     = (bfAnimationFrame*)bfAllocate(self, sizeof(bfAnimationFrame) * num_frames);
        anim->num_frames = num_frames;

        for (uint32_t f = 0; f < num_frames; ++f)
        {
          bfAnimationFrame* frame_instance = anim->frames + f;

          memcpy(frame_instance, chuck_data, sizeof(*frame_instance));
          chuck_data += sizeof(bfAnimationFrame);
        }
      }
    }
    else if (memcmp(chunk_type, "EDIT", 4) == 0)
    {
      memcpy(sheet->guid, chuck_data, sizeof(sheet->guid));
      chuck_data += sizeof(sheet->guid);
    }
    else if (memcmp(chunk_type, "FOOT", 4) == 0)
    {
      break;
    }
    else
    {
      // Unhandled chunk type.
    }

    chucks += chunk_data_length;

    if (chucks > bytes_end)
    {
      return false;
    }
  }

  return true;
}

// Networking

bfAnim2DPacketHeader bfAnim2DPacketHeader_read(const char* bytes)
{
  bfAnim2DPacketHeader self;

  self.packet_size = bfBytesReadUint32LE((const bfByte*)bytes + 0u);
  self.packet_type = bfBytesReadUint8LE((const bfByte*)bytes + sizeof(self.packet_size));
  memcpy(self.spritesheet_uuid, bytes + k_bfAnim2DHeaderSize, sizeof(self.spritesheet_uuid));

  return self;
}

bfAnim2DPacketSpritesheetChanged bfAnim2DPacketSpritesheetChanged_read(const char* bytes)
{
  bfAnim2DPacketSpritesheetChanged self;

  self.header          = bfAnim2DPacketHeader_read(bytes);
  self.atlas_data_size = bfBytesReadUint32LE((const bfByte*)bytes + k_bfAnim2DTotalHeaderSize);
  self.atlas_data      = (const uint8_t*)bytes + k_bfAnim2DTotalHeaderSize + sizeof(std::uint32_t);

  return self;
}

bfAnim2DPacketTextureChanged bfAnim2DPacketTextureChanged_read(const char* bytes)
{
  bfAnim2DPacketTextureChanged self;

  self.header            = bfAnim2DPacketHeader_read(bytes);
  self.texture_data_size = bfBytesReadUint32LE((const bfByte*)bytes + k_bfAnim2DTotalHeaderSize);
  self.texture_data      = bytes + k_bfAnim2DTotalHeaderSize + sizeof(std::uint32_t);

  return self;
}

bool NetworkingData::readPackets(bfAnim2DCtx* self, bfAnim2DChangeEvent* out_event)
{
  bool did_receive_event = false;

  if (!is_connected)
  {
    return did_receive_event;
  }

  const auto old_current_packet_size = current_packet.size();
  const auto received_data           = socket.receiveDataFrom(read_buffer, sizeof(read_buffer));

  // Connection Ended
  if (received_data.received_bytes_size == 0 || received_data.received_bytes_size == -2)
  {
    current_packet.clear();
    is_connected = false;
    socket.close();

    return did_receive_event;
  }

  // received_data.received_bytes_size == -1 when waiting on a message
  for (int i = 0; i < received_data.received_bytes_size; ++i)
  {
    current_packet.push(read_buffer[i]);
  }

  const auto new_current_packet_size = current_packet.size();

  if (new_current_packet_size >= k_bfAnim2DTotalHeaderSize)
  {
    // We have enough data now but before we did not.
    if (old_current_packet_size < k_bfAnim2DTotalHeaderSize)
    {
      current_packet_header = bfAnim2DPacketHeader_read(current_packet.data());
    }

    // Packet Fully Constructed
    if (uint32_t(new_current_packet_size) >= current_packet_header.packet_size)
    {
      const auto findSpritesheet = [self](const char* uuid) -> bfSpritesheet* {
        bfSpritesheet* sheet = self->spritesheet_list;

        while (sheet)
        {
          if (memcmp(uuid, sheet->guid, k_bfAnim2DGUIDSize) == 0)
          {
            return sheet;
          }

          sheet = sheet->next;
        }

        return nullptr;
      };

      switch (current_packet_header.packet_type)
      {
        case bfAnim2DPacketType_SpritesheetChanged:
        {
          bfAnim2DPacketSpritesheetChanged packet = bfAnim2DPacketSpritesheetChanged_read(current_packet.data());
          bfSpritesheet* const             sheet  = findSpritesheet(packet.header.spritesheet_uuid);

          if (sheet)
          {
            bfLoadUpSpritesheetFromData(self, sheet, packet.atlas_data, packet.atlas_data_size);

            bfAnim2DChangeEvent change_event;
            change_event.type        = bfAnim2DChange_Animation;
            change_event.spritesheet = sheet;

            *out_event        = change_event;
            did_receive_event = true;
          }

          break;
        }
        case bfAnim2DPacketType_TextureChanged:
        {
          bfAnim2DPacketTextureChanged packet = bfAnim2DPacketTextureChanged_read(current_packet.data());
          bfSpritesheet* const         sheet  = findSpritesheet(packet.header.spritesheet_uuid);

          if (sheet)
          {
            bfAnim2DChangeEvent change_event;
            change_event.type                           = bfAnim2DChange_Texture;
            change_event.spritesheet                    = sheet;
            change_event.texture.texture_bytes_png      = packet.texture_data;
            change_event.texture.texture_bytes_png_size = packet.texture_data_size;

            *out_event        = change_event;
            did_receive_event = true;
          }

          break;
        }
        default:
        {
          assert(!"Invalid packet type received.");
          break;
        }
      }

      const uint32_t num_bytes_left = uint32_t(new_current_packet_size) - current_packet_header.packet_size;

      if (num_bytes_left != 0)
      {
        // Put leftover bytes in the beginning of the array.
        std::rotate(
         current_packet.rbegin(),
         current_packet.rbegin() + num_bytes_left,
         current_packet.rend());

        current_packet.resize(num_bytes_left);

        if (num_bytes_left >= k_bfAnim2DTotalHeaderSize)
        {
          current_packet_header = bfAnim2DPacketHeader_read(current_packet.data());
        }
      }
      else
      {
        current_packet.clear();
      }
    }
  }

  return did_receive_event;
}
