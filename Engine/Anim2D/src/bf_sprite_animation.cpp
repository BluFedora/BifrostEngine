#include "bf/anim2D/bf_sprite_animation_api.h"

#include "bf/anim2D/bf_anim2D_network.h"  // bfAnim2DPacketHeader

#include <bf/DenseMap.hpp>        // DenseMap
#include <bf/IMemoryManager.hpp>  // IMemoryManager
#include <bf/MemoryUtils.h>       // bfBytesReadUint*
#include <bf/bf_net.hpp>          // Networking API

#include <assert.h> /* assert              */
#include <stdlib.h> /* realloc, free, NULL */

// Constants

/* clang-format off */

#define k_bfAnimSpriteFlagIsPlaying  ((uint8_t)(1u << 0u)) /*!< Must be defined as 1 for an optimization in the 'bfAnimationScene_advance'. */
#define k_bfAnimSpriteFlagIsLooping  ((uint8_t)(1u << 1u)) /*!< */
#define k_bfAnimSpriteFlagIsPingPong ((uint8_t)(1u << 2u)) /*!< Only matters if the 'k_bfAnimSpriteFlagIsLooping' flag is also set. */
#define k_bfAnimSpriteFlagIsDone     ((uint8_t)(1u << 3u)) /*!< */
#define k_bfAnimSpriteInvalidAnimIdx ((uint16_t)(0xFFFF))  /*!< */

/* clang-format on */

// Helper Struct Definitions

// NOTE(SR):
//   The two levels of indirection here is kinda gross
//   but is needed for code reuse, maybe that is a bad goal / reason ...
struct CallbackAllocator final : public bf::IMemoryManager
{
  void*                  user_data;
  bfAnimation2DAllocator alloc_fn;

  CallbackAllocator(void* ud, bfAnimation2DAllocator alloc) :
    bf::IMemoryManager(),
    user_data{ud},
    alloc_fn{alloc}
  {
  }

  virtual void* allocate(std::size_t size) override
  {
    return alloc_fn(nullptr, 0u, size, user_data);
  }

  virtual void deallocate(void* ptr, std::size_t old_size) override
  {
    alloc_fn(ptr, old_size, 0u, user_data);
  }
};

// Struct Definitions

struct NetworkingData;

struct bfSpritesheetPrivate : public bfSpritesheet_t
{
  char                  guid[37]; /*!< */
  bfSpritesheetPrivate* prev;     /*!< */
  bfSpritesheetPrivate* next;     /*!< */
};

struct bfAnimation2DCtx_t /*!< @copydoc bfAnimation2DCtx */
{
  bfAnim2DCreateParams  params; /*!< */
  CallbackAllocator     allocator;
  bfAnim2DScene*        scene_list;       /*!< */
  bfSpritesheetPrivate* spritesheet_list; /*!< */
  NetworkingData*       network_module;

  bfAnimation2DCtx_t(const bfAnim2DCreateParams& params) :
    params{params},
    allocator{params.user_data, params.allocator},
    scene_list{nullptr},
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
    current_packet_header{0u, 0u},
    read_buffer{}
  {
    bfNet::Startup();
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
        is_connected = socket.connectTo(address).isSuccess();
      }
    }
  }

  void readPackets(bfAnimation2DCtx* self);
};

struct bfAnim2DScene_t
{
  bf::DenseMap<bfAnim2DSprite> active_sprites;
  struct bfAnim2DScene_t*      prev; /*!< The previous scene in the bfAnimation2DCtx::scene_list linked list         */
  struct bfAnim2DScene_t*      next; /*!< The next scene in the bfAnimation2DCtx::scene_list linked list             */

  bfAnim2DScene_t(CallbackAllocator& allocator) :
    active_sprites{allocator},
    prev{nullptr},
    next{nullptr}
  {
  }
};

struct bfAnim2DSprite_t
{
  float                 playback_speed;
  float                 time_left_for_frame;
  int32_t               max_frames_minus_one;
  int32_t               current_frame;
  uint8_t               flags;
  uint8_t               padd0;
  uint16_t              animation_index;
  bfSpritesheetPrivate* attached_sheet;
};

// const int num_sprites_per_cacheline = 64 / sizeof(bfAnim2DSprite_t);

// Helpers Fwd Declarations

template<typename T>
static void bfPrependDoublyLL(T*& head, T* item);
template<typename T>
static void bfRemoveDoublyLL(T*& head, T* item);

static void*         bfDefaultAllocator(void* ptr, size_t old_size, size_t new_size, void* user_data);
static void*         bfAllocate(const bfAnimation2DCtx* parent, size_t size);
static void          bfFree(const bfAnimation2DCtx* parent, void* ptr, size_t size);
static bfOwnedString bfStringClone(const bfAnimation2DCtx* parent, const bfStringSpan& string);
static void          bfStringFree(const bfAnimation2DCtx* parent, bfOwnedString string);
static void          bfAnim2DSceneAdvance(bfAnim2DScene* self, uint32_t offset, uint32_t num_sprites, float delta_time);
static bool          bfLoadUpSpritesheetFromData(bfAnimation2DCtx* self, bfSpritesheetPrivate* sheet, const uint8_t* srsm_bytes, size_t num_srsm_bytes);

// API Definitions

bfAnimation2DCtx* bfAnimation2D_new(const bfAnim2DCreateParams* params)
{
  bfAnim2DCreateParams safe_params = *params;

  if (!safe_params.allocator)
  {
    safe_params.allocator = bfDefaultAllocator;
  }

  bfAnimation2DCtx* self = new (safe_params.allocator(NULL, 0u, sizeof(*self), safe_params.user_data)) bfAnimation2DCtx(safe_params);

  if (self)
  {
    if (safe_params.on_spritesheet_changed)
    {
      self->network_module = new (bfAllocate(self, sizeof(*self->network_module))) NetworkingData(self->allocator);
    }
  }

  return self;
}

void* bfAnimation2D_userData(const bfAnimation2DCtx* self)
{
  return self->params.user_data;
}

bfAnim2DScene* bfAnimation2D_createScene(bfAnimation2DCtx* self)
{
  bfAnim2DScene* const scene = new (bfAllocate(self, sizeof(bfAnim2DScene))) bfAnim2DScene(self->allocator);

  if (scene)
  {
    bfPrependDoublyLL(self->scene_list, scene);
  }

  return scene;
}

bfSpritesheet* bfAnimation2D_createSpritesheet(bfAnimation2DCtx* self, const bfSpritesheetCreateParams* params)
{
  bfSpritesheetPrivate* sheet = new (bfAllocate(self, sizeof(bfSpritesheetPrivate))) bfSpritesheetPrivate;

  if (sheet)
  {
    // TODO(SR): All allocations can probably be merged into one since a 'bfSpritesheet' is pretty immutable.

    sheet->name           = bfStringClone(self, params->name);
    sheet->animations     = (bfAnimation*)bfAllocate(self, sizeof(bfAnimation) * params->num_animations);
    sheet->uvs            = (bfUVRect*)bfAllocate(self, sizeof(bfUVRect) * params->num_uvs);
    sheet->num_animations = params->num_animations;
    sheet->num_uvs        = params->num_uvs;
    sheet->user_data      = params->user_atlas;
    memset(sheet->guid, 0x0, sizeof(sheet->guid));

    bfPrependDoublyLL(self->spritesheet_list, sheet);
  }

  return sheet;
}

bfSpritesheet* bfAnimation2D_loadSpritesheet(bfAnimation2DCtx* self, bfStringSpan name, const uint8_t* srsm_bytes, size_t num_srsm_bytes)
{
  bfSpritesheetPrivate* sheet = new (bfAllocate(self, sizeof(bfSpritesheetPrivate))) bfSpritesheetPrivate;

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
    bfAnimation2D_destroySpritesheet(self, sheet);
    sheet = nullptr;
  }

  return sheet;
}

void bfAnimation2D_beginFrame(bfAnimation2DCtx* self)
{
  if (self->network_module)
  {
    self->network_module->establishConnection();
    self->network_module->readPackets(self);
  }
}

void bfAnimation2D_stepFrameEx(const bfAnimation2DCtx* self, const bfAnim2DStepFrameExOptions* settings)
{
  bfAnim2DSceneAdvance(settings->scene, settings->offset, settings->num_sprites, settings->delta_time);
}

void bfAnimation2D_stepFrame(const bfAnimation2DCtx* self, float delta_time)
{
  bfAnim2DScene* scene = self->scene_list;

  while (scene)
  {
    bfAnim2DStepFrameExOptions settings;

    settings.scene       = scene;
    settings.offset      = 0u;
    settings.num_sprites = uint32_t(scene->active_sprites.size());
    settings.delta_time  = delta_time;

    bfAnimation2D_stepFrameEx(self, &settings);

    scene = scene->next;
  }
}

static void bfAnimation2D_clearSpritesheet(bfAnimation2DCtx* self, bfSpritesheet* spritesheet)
{
  bfSpritesheetPrivate* const sheet_priv = (bfSpritesheetPrivate*)spritesheet;

  for (uint32_t a = 0; a < sheet_priv->num_animations; ++a)
  {
    bfAnimation* const anim = sheet_priv->animations + a;

    bfStringFree(self, anim->name);
    bfFree(self, anim->frames, sizeof(*anim->frames) * anim->num_frames);
  }

  bfFree(self, sheet_priv->uvs, sizeof(*sheet_priv->uvs) * sheet_priv->num_uvs);
}

void bfAnimation2D_destroySpritesheet(bfAnimation2DCtx* self, bfSpritesheet* spritesheet)
{
  bfSpritesheetPrivate* const sheet_priv = (bfSpritesheetPrivate*)spritesheet;

  bfStringFree(self, sheet_priv->name);
  bfAnimation2D_clearSpritesheet(self, sheet_priv);

  bfRemoveDoublyLL(self->spritesheet_list, sheet_priv);
  sheet_priv->~bfSpritesheetPrivate();
  bfFree(self, sheet_priv, sizeof(*sheet_priv));
}

void bfAnimation2D_destroyScene(bfAnimation2DCtx* self, bfAnim2DScene* scene)
{
  bfRemoveDoublyLL(self->scene_list, scene);
  scene->~bfAnim2DScene();
  bfFree(self, scene, sizeof(*scene));
}

void bfAnimation2D_delete(bfAnimation2DCtx* self)
{
  while (self->scene_list)
  {
    bfAnimation2D_destroyScene(self, self->scene_list);
  }

  while (self->spritesheet_list)
  {
    bfAnimation2D_destroySpritesheet(self, self->spritesheet_list);
  }

  if (self->network_module)
  {
    self->network_module->~NetworkingData();
    bfFree(self, self->network_module, sizeof(*self->network_module));
  }

  bfFree(self, self, sizeof(*self));
}

bfAnim2DSpriteHandle bfAnim2DScene_addSprite(bfAnim2DScene* self)
{
  bfAnim2DSpriteHandle sprite_handle;

  sprite_handle.scene    = self;
  sprite_handle.id       = self->active_sprites.add().id_index;
  sprite_handle.reserved = 0xFF;

  std::memset(&self->active_sprites.find(sprite_handle.id), 0x00, sizeof(bfAnim2DSprite_t));

  return sprite_handle;
}

void bfAnim2DScene_destroySprite(bfAnim2DScene* self, bfAnim2DSpriteHandle* sprite)
{
  self->active_sprites.remove(sprite->id);
  *sprite = bfAnim2DSprite_invalidHandle();
}

bfAnim2DSpriteHandle bfAnim2DSprite_invalidHandle(void)
{
  bfAnim2DSpriteHandle handle;

  handle.scene    = NULL;
  handle.id       = k_bfAnim2DInvalidSpriteHandleID;
  handle.reserved = 0xFF;

  return handle;
}

bfBool8 bfAnim2DSprite_isInvalidHandle(bfAnim2DSpriteHandle handle)
{
  return handle.scene == NULL || handle.id == k_bfAnim2DInvalidSpriteHandleID;
}

void bfAnim2DSprite_setSpritesheet(bfAnim2DSpriteHandle self, bfSpritesheet* sheet)
{
  bfAnim2DSprite& sprite = self.scene->active_sprites.find(self.id);

  sprite.flags           = sprite.flags & ~k_bfAnimSpriteFlagIsPlaying;
  sprite.attached_sheet  = (bfSpritesheetPrivate*)sheet;
  sprite.animation_index = k_bfAnimSpriteInvalidAnimIdx;
}

void bfAnim2DSprite_playAnimationEx(bfAnim2DSpriteHandle self, const bfAnim2DPlayExOptions* options)
{
  assert(options->animation);
  assert(options->start_frame < options->animation->num_frames);

  bfAnim2DSprite& sprite              = self.scene->active_sprites.find(self.id);
  const uint16_t  old_animation_index = sprite.animation_index;

  sprite.animation_index      = uint16_t(options->animation - sprite.attached_sheet->animations);
  sprite.playback_speed       = options->playback_speed;
  sprite.max_frames_minus_one = options->animation->num_frames - 1;

  if (options->animation->num_frames > 0)
  {
    sprite.flags |= k_bfAnimSpriteFlagIsPlaying;
    sprite.time_left_for_frame = options->animation->frames[options->start_frame].frame_time;
  }
  else
  {
    sprite.flags &= ~k_bfAnimSpriteFlagIsPlaying;
  }

  if (options->force_restart || old_animation_index != sprite.animation_index)
  {
    sprite.current_frame = options->start_frame;
  }

  if (options->is_looping)
  {
    sprite.flags |= k_bfAnimSpriteFlagIsLooping;
  }
  else
  {
    sprite.flags &= ~k_bfAnimSpriteFlagIsLooping;
  }

  if (options->does_ping_ponging)
  {
    sprite.flags |= k_bfAnimSpriteFlagIsPingPong;
  }
  else
  {
    sprite.flags &= ~k_bfAnimSpriteFlagIsPingPong;
  }
}

bfBool8 bfAnim2DSprite_grabState(bfAnim2DSpriteHandle self, bfAnim2DSpriteState* out_state)
{
  bfAnim2DSprite& sprite = self.scene->active_sprites.find(self.id);
  bfSpritesheet*  sheet  = sprite.attached_sheet;

  if (sheet && sprite.animation_index < sheet->num_animations)
  {
    bfAnimation* const anim = sprite.attached_sheet->animations + sprite.animation_index;

    if (sprite.current_frame < anim->num_frames)
    {
      out_state->animation           = sheet->animations + sprite.animation_index;
      out_state->time_left_for_frame = sprite.time_left_for_frame;
      out_state->current_frame       = anim->frames[sprite.current_frame];

      // TODO(SR): This should ALWAYS be true, otherwise that means the spritesheet data was incorrectly generated...
      if (out_state->current_frame.frame_index < sheet->num_uvs)
      {
        out_state->uv_rect = sheet->uvs[out_state->current_frame.frame_index];
      }
      else
      {
        out_state->uv_rect = bfUVRect{0.0f, 0.0f, 1.0f, 1.0f};
      }

      return true;
    }
  }

  return false;
}

// Helper Definitions

template<typename T>
static void bfPrependDoublyLL(T*& head, T* item)
{
  item->prev = NULL;
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
    ptr = NULL;
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

static void* bfAllocate(const bfAnimation2DCtx* parent, size_t size)
{
  return parent->params.allocator(NULL, 0u, size, parent->params.user_data);
}

static void bfFree(const bfAnimation2DCtx* parent, void* ptr, size_t size)
{
  parent->params.allocator(ptr, size, 0u, parent->params.user_data);
}

static bfOwnedString bfStringClone(const bfAnimation2DCtx* parent, const bfStringSpan& string)
{
  bfOwnedString ret;

  ret.str     = (char*)bfAllocate(parent, string.str_len + 1);
  ret.str_len = string.str_len;

  std::memcpy(ret.str, string.str, string.str_len);

  ret.str[ret.str_len] = '\0';

  return ret;
}

static void bfStringFree(const bfAnimation2DCtx* parent, bfOwnedString string)
{
  bfFree(parent, string.str, string.str_len + 1);
}

static void bfAnim2DSceneAdvance(bfAnim2DScene* self, uint32_t offset, uint32_t num_sprites, float delta_time)
{
#define BIFROST_SPRITE_OPTIMIZATION_BRANCHLESS_IS_PLAYING 1
#define BIFROST_SPRITE_OPTIMIZATION_BRANCHLESS_LOOPING 1

  const auto sprites_bgn = self->active_sprites.begin() + offset;
  const auto sprites_end = sprites_bgn + num_sprites;

  assert(offset + num_sprites <= uint32_t(self->active_sprites.size()));

  auto sprite = sprites_bgn;

  while (sprite != sprites_end)
  {
#if !BIFROST_SPRITE_OPTIMIZATION_BRANCHLESS_IS_PLAYING
    if (!(sprite->flags & k_bfAnimSpriteFlagIsPlaying))
    {
      ++sprite;
      continue;
    }
#endif

    const float playback_speed       = sprite->playback_speed;
    const int   playback_is_positive = playback_speed >= 0.0f;
    const float abs_playback_speed   = (playback_is_positive ? playback_speed : -playback_speed);
    const float playback_delta       = abs_playback_speed * delta_time;

#if BIFROST_SPRITE_OPTIMIZATION_BRANCHLESS_IS_PLAYING

    /*
       NOTE(Shareef):
         This works because 'k_bfAnimSpriteFlagIsPlaying' is defined as 1,
         so the only options are 1 or 0.
    */

    sprite->time_left_for_frame -= (float)(sprite->flags & k_bfAnimSpriteFlagIsPlaying) * playback_delta;
#else
    sprite->time_left_for_frame -= playback_delta;
#endif

    if (sprite->time_left_for_frame <= 0.0f)
    {
      const int32_t num_frames_minus_one = sprite->max_frames_minus_one;
      const int32_t last_frame           = playback_is_positive ? num_frames_minus_one : 0;

      if (sprite->current_frame != last_frame)
      {
        const int32_t delta_frame = playback_is_positive ? 1 : -1;

        sprite->current_frame += delta_frame;

        sprite->flags &= ~k_bfAnimSpriteFlagIsDone;
      }
      else
      {
#if !BIFROST_SPRITE_OPTIMIZATION_BRANCHLESS_LOOPING
        if (sprite->flags & k_bfAnimSpriteFlagIsLooping)
#endif
        {
          if (sprite->flags & k_bfAnimSpriteFlagIsPingPong)
          {
            sprite->playback_speed *= -1.0f;
          }
          else
          {
            const int32_t first_frame = playback_is_positive ? 0 : num_frames_minus_one;

#if BIFROST_SPRITE_OPTIMIZATION_BRANCHLESS_LOOPING
            sprite->current_frame = (sprite->flags & k_bfAnimSpriteFlagIsLooping) ? first_frame : sprite->current_frame;
#else
            sprite->current_frame = first_frame;
#endif
          }
        }

        sprite->flags |= k_bfAnimSpriteFlagIsDone;
      }

      if (sprite->attached_sheet && sprite->animation_index < sprite->attached_sheet->num_animations)
      {
        bfAnimation* const animation = sprite->attached_sheet->animations + sprite->animation_index;

        if (sprite->current_frame < animation->num_frames)
        {
          sprite->time_left_for_frame = animation->frames[sprite->current_frame].frame_time;
        }
      }
    }

    ++sprite;
  }
}

bool bfLoadUpSpritesheetFromData(bfAnimation2DCtx* self, bfSpritesheetPrivate* sheet, const uint8_t* srsm_bytes, size_t num_srsm_bytes)
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
  //   like not freeing if there are minimal changes but this is simpler.

  // sheet->name           = bfStringClone(self, name);
  // sheet->user_data      = nullptr;
  bfAnimation2D_clearSpritesheet(self, sheet);
  sheet->animations     = nullptr;
  sheet->uvs            = nullptr;
  sheet->num_animations = 0;
  sheet->num_uvs        = 0;
  memset(sheet->guid, '\0', sizeof(sheet->guid));

  const bfByte* const chucks_start = bytes_start + header_data_offset;
  const bfByte*       chucks       = chucks_start;

  for (uint8_t i = 0; i < header_num_chunks; ++i)
  {
    uint8_t       chunk_type[4];
    uint32_t      chunk_data_length;
    const bfByte* chuck_data;

    memcpy(chunk_type, chucks, sizeof(chunk_type));
    chucks += sizeof(chunk_type);

    chunk_data_length = bfBytesReadUint32LE(chucks);
    chucks += sizeof(chunk_data_length);

    chuck_data = chucks;

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

void NetworkingData::readPackets(bfAnimation2DCtx* self)
{
  if (!is_connected)
  {
    return;
  }

  const auto old_current_packet_size = current_packet.size();
  const bool is_beginning_of_packet  = old_current_packet_size < k_bfAnim2DTotalHeaderSize;
  const auto received_data           = socket.receiveDataFrom(read_buffer, sizeof(read_buffer));

  // Connection Ended
  if (received_data.received_bytes_size == 0 || received_data.received_bytes_size == -2)
  {
    current_packet.clear();
    is_connected = false;
    socket       = {};
    return;
  }

  // received_data.received_bytes_size == -1 when Waiting On a Message
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
      const auto findSpritesheet = [self](const char* uuid) -> bfSpritesheetPrivate* {
        bfSpritesheetPrivate* sheet = self->spritesheet_list;

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
          auto                             sheet  = findSpritesheet(packet.header.spritesheet_uuid);

          if (sheet)
          {
            bfLoadUpSpritesheetFromData(self, sheet, packet.atlas_data, packet.atlas_data_size);

            for (bfAnim2DScene* scene = self->scene_list; scene; scene = scene->next)
            {
              for (auto& sprite : scene->active_sprites)
              {
                if (sprite.attached_sheet == sheet && sprite.animation_index < sheet->num_animations)
                {
                  sprite.max_frames_minus_one = sheet->animations[sprite.animation_index].num_frames - 1;
                }
              }
            }

            bfAnim2DChangeEvent change_event;
            change_event.type        = bfAnim2DChange_Animation;
            change_event.spritesheet = sheet;

            self->params.on_spritesheet_changed(self, sheet, change_event);
          }

          break;
        }
        case bfAnim2DPacketType_TextureChanged:
        {
          bfAnim2DPacketTextureChanged packet = bfAnim2DPacketTextureChanged_read(current_packet.data());
          auto                         sheet  = findSpritesheet(packet.header.spritesheet_uuid);

          if (sheet)
          {
            bfAnim2DChangeEvent change_event;
            change_event.type                           = bfAnim2DChange_Texture;
            change_event.spritesheet                    = sheet;
            change_event.texture.texture_bytes_png      = packet.texture_data;
            change_event.texture.texture_bytes_png_size = packet.texture_data_size;

            self->params.on_spritesheet_changed(self, sheet, change_event);
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
}
