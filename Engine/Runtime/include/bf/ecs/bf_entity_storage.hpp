// Feb 20th
#ifndef BF_ENTITY_STORAGE_HPP
#define BF_ENTITY_STORAGE_HPP

#include "bf/IMemoryManager.hpp"     // IMemoryManager
#include "bf/ListView.hpp"           // List<T>
#include "bf/PoolAllocator.hpp"      // PoolAllocator<T, num_elements>
#include "bf_component_storage.hpp"  // ComponentStorage
#include "bf_entity.hpp"             // Entity

#include <array>

namespace bf
{
  static constexpr std::uint16_t k_MaxEntitiesPerChunk       = (1u << 16) - 1u;
  static constexpr std::uint16_t k_MaxEntitiesPerChunkPiece  = (1u << 12);
  static constexpr std::uint16_t k_NumEntityChunksPerStorage = (k_MaxEntitiesPerChunk + 1u) / k_MaxEntitiesPerChunkPiece;

  class Scene;

  struct EntityChunkPiece
  {
    using Allocator = PoolAllocator<Entity, k_MaxEntitiesPerChunk>;

    Allocator     allocator;
    std::uint16_t num_allocated_left;

    EntityChunkPiece() :
      allocator{},
      num_allocated_left{k_MaxEntitiesPerChunk}
    {
    }

    Entity* alloc(Scene& scene)
    {
      return allocator.allocateT<Entity>(scene);
    }
  };

  using EntityChunkPieceArray = std::array<EntityChunkPiece*, k_NumEntityChunksPerStorage>;
  using EntityIDToEntity      = HashTable<std::uint16_t, Entity*>;

  class EntityChunk
  {
   private:
    IMemoryManager&       m_Memory;
    BVH                   m_BVH;
    ComponentStorage      m_ActiveComponents;
    ComponentStorage      m_InactiveComponents;
    EntityChunkPieceArray m_Pieces;
    std::uint16_t         m_NumEntitiesLeft;

   public:
    EntityChunk(IMemoryManager& memory) :
      m_Memory{memory},
      m_BVH{memory},
      m_ActiveComponents{memory},
      m_InactiveComponents{memory},
      m_Pieces{},
      m_NumEntitiesLeft{k_MaxEntitiesPerChunk}
    {
      std::fill(m_Pieces.begin(), m_Pieces.end(), nullptr);
    }

    std::uint16_t allocateEntities(std::size_t num_entities, Scene& scene, Entity** results)
    {
      const std::uint16_t num_actual_alloc = std::min(m_NumEntitiesLeft, std::uint16_t(num_entities));
      std::uint16_t       num_to_alloc     = num_actual_alloc;

      while (num_to_alloc)
      {
        for (EntityChunkPiece*& piece : m_Pieces)
        {
          if (!piece)
          {
            piece = m_Memory.allocateT<EntityChunkPiece>();
          }

          const std::uint16_t num_to_alloc_in_piece = std::min(piece->num_allocated_left, num_to_alloc);

          num_to_alloc -= num_to_alloc_in_piece;

          for (std::uint16_t i = 0u; i < num_to_alloc_in_piece; ++i)
          {
            *results++ = piece->alloc(scene);
          }

          if (!num_to_alloc)
          {
            break;
          }
        }
      }

      m_NumEntitiesLeft -= num_actual_alloc;

      return num_actual_alloc;
    }

    void destroy()
    {
      for (EntityChunkPiece* piece : m_Pieces)
      {
        m_Memory.deallocateT(piece);
      }
    }
  };

  struct EntityHandle
  {
    EntityChunk*  chunk;
    std::uint16_t instance_id;
  };

  class EntityStorage
  {
   private:
    List<EntityChunk> m_Chunks;
    EntityIDToEntity  m_IDMapping;

   public:
    EntityStorage(IMemoryManager& memory) :
      m_Chunks{memory},
      m_IDMapping{}
    {
    }
  };
}  // namespace bf

#endif /* BF_ENTITY_STORAGE_HPP */
