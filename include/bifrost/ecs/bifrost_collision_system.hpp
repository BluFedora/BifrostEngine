/******************************************************************************/
/*!
 * @file   bifrost_collision_system.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   References:
 *     [https://box2d.org/documentation/md__d_1__git_hub_box2d_docs_collision.html#autotoc_md49]
 *     [https://www.randygaul.net/2013/08/06/dynamic-aabb-tree/]
 *     [https://www.codeproject.com/Articles/832957/Dynamic-Bounding-Volume-Hiearchy-in-Csharp]
 *
 * @version 0.0.1
 * @date    2020-04-01
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#ifndef BIFROST_COLLISION_SYSTEM
#define BIFROST_COLLISION_SYSTEM

#include "bifrost/bifrost_math.hpp"                     // isAlmostEqual
#include "bifrost/bifrost_std.h"                        // bfInvalidDefaultCase
#include "bifrost/data_structures/bifrost_array.hpp"    // Array<T>
#include "bifrost/memory/bifrost_linear_allocator.hpp"  // LinearAllocator
#include "bifrost_iecs_system.hpp"                      // IECSSystem

#include <cfloat>   // FLT_MAX
#include <cstdint>  // uint16_t

namespace bifrost
{
  struct AABB final
  {
    float min[3];
    float max[3];

    AABB() = default;

    AABB(const BifrostTransform& transform)
    {
      // TODO: This isn't correct for rotation.
      const Vector3f half_extent = Vector3f(transform.world_scale) * 0.5f;
      const Vector3f min_v       = Vector3f(transform.world_position) - half_extent;
      const Vector3f max_v       = Vector3f(transform.world_position) + half_extent;

      min[0] = min_v.x;
      min[1] = min_v.y;
      min[2] = min_v.z;
      max[0] = max_v.x;
      max[1] = max_v.y;
      max[2] = max_v.z;
    }

    bool operator==(const AABB& rhs) const
    {
      for (int i = 0; i < 3; ++i)
      {
        if (!math::isAlmostEqual(min[i], rhs.min[i]) || !math::isAlmostEqual(max[i], rhs.max[i]))
        {
          return false;
        }
      }

      return true;
    }

    bool operator!=(const AABB& rhs) const
    {
      return !(*this == rhs);
    }
  };

  namespace math
  {
    static inline float min(const float a, const float b)
    {
      return a < b ? a : b;
    }
  }  // namespace math

  namespace aabb
  {
    /**
     * @brief
     *   Creates a new bounding box that contains both \p a and \p b.
     *
     * @param out
     *   The result of this function.
     *
     * @param a
     *   The first AABB to merge.
     * 
     * @param b
     *   The second AABB to merge.
     */
    static void mergeBounds(AABB& out, const AABB& a, const AABB& b)
    {
      for (int i = 0; i < 3; ++i)
      {
        out.min[i] = a.min[i] < b.min[i] ? a.min[i] : b.min[i];
        out.max[i] = a.max[i] > b.max[i] ? a.max[i] : b.max[i];
      }
    }

    /**
     * @copydoc mergeBounds(AABB&, const AABB&, const AABB&)
     */
    static AABB mergeBounds(const AABB& a, const AABB& b)
    {
      AABB out;  // NOLINT
      mergeBounds(out, a, b);
      return out;
    }

    static void expandBy(AABB& self, float amount)
    {
      for (int i = 0; i < 3; ++i)
      {
        self.min[i] = self.min[i] - amount;
        self.max[i] = self.max[i] + amount;
      }
    }

    static AABB expandedBy(const AABB& self, float amount)
    {
      AABB clone = self;
      expandBy(clone, amount);
      return clone;
    }

    static float surfaceArea(const AABB& self)
    {
      const float d[3] =
       {
        self.max[0] - self.min[0],
        self.max[1] - self.min[1],
        self.max[2] - self.min[2],
       };

      return 2.0f * (d[0] * d[1] + d[1] * d[2] + d[2] * d[0]);
    }
  }  // namespace aabb

  using BVHNodeOffset                                   = std::uint16_t;
  static constexpr BVHNodeOffset k_BVHNodeInvalidOffset = 0xFFFFu;
  static constexpr float         k_BVHRotationBenefit   = 0.3f;
  static constexpr float         k_BVHMergeDownBenefit  = 0.35f;
  static constexpr float         k_BVHBoundsSkin        = 0.1f;

  struct BVHNode final
  {
    union
    {
      struct
      {
        void*         user_data;    // Mapping back to the object.
        AABB          bounds;       // Bounds including children.
        BVHNodeOffset children[2];  // isLeaf = children[0] == children[1] (aka both k_BVHNodeInvalidOffset)
        BVHNodeOffset parent;       // Only needed by rotation code, so really leaves don't need this.
        BVHNodeOffset depth;        // For avl rotation balancing.
      };
      BVHNodeOffset next;  // Freelist, can be unioned with other data since if it is on the reelist then all node members are unused.
    };
  };

  namespace bvh_node
  {
    static bool isNull(BVHNodeOffset index)
    {
      return index == k_BVHNodeInvalidOffset;
    }

    static bool isLeaf(const BVHNode& node)
    {
      // TODO: add assert.
      return node.children[0] == node.children[1];
    }
  }  // namespace bvh_node

  struct BVH final
  {
    Array<BVHNode>       nodes;
    Array<BVHNodeOffset> nodes_to_optimize;
    BVHNodeOffset        root_idx;
    BVHNodeOffset        freelist;
    std::uint16_t        max_depth;

    explicit BVH(IMemoryManager& memory) :
      nodes{memory},
      nodes_to_optimize{memory},
      root_idx{k_BVHNodeInvalidOffset},
      freelist{k_BVHNodeInvalidOffset},
      max_depth{0}
    {
    }

    template<typename F>
    void traverse(BVHNodeOffset node, F&& callback)
    {
      if (!bvh_node::isNull(node))
      {
        callback(nodes[node]);

        if (!bvh_node::isLeaf(nodes[node]))
        {
          traverse(nodes[node].children[0], callback);
          traverse(nodes[node].children[1], callback);
        }
      }
    }

    template<typename F>
    void traverse(F&& callback)
    {
      traverse(root_idx, callback);
    }

    BVHNodeOffset insert(void* user_data, const AABB& bounds)
    {
      const AABB object_bounds = aabb::expandedBy(bounds, k_BVHBoundsSkin);

      if (!bvh_node::isNull(root_idx))
      {
        // Stage 1: Look for best leaf to be a sibling of.
        const BVHNode* current_node = &nodeAt(root_idx);

        while (!bvh_node::isLeaf(*current_node))
        {
          const BVHNode& left            = nodeAt(current_node->children[0]);
          const BVHNode& right           = nodeAt(current_node->children[1]);
          const float    left_sa         = aabb::surfaceArea(left.bounds);
          const float    right_sa        = aabb::surfaceArea(right.bounds);
          AABB           add_to_left     = aabb::mergeBounds(left.bounds, object_bounds);
          AABB           add_to_right    = aabb::mergeBounds(right.bounds, object_bounds);
          const float    add_to_left_sa  = aabb::surfaceArea(add_to_left) + right_sa;
          const float    add_to_right_sa = aabb::surfaceArea(add_to_right) + left_sa;

          if (add_to_left_sa < add_to_right_sa)
          {
            current_node = &left;
          }
          else
          {
            current_node = &right;
          }
        }

        // Stage 2: Add the Object as normal
        const BVHNodeOffset sibling    = nodeToIndex(*current_node);
        const BVHNodeOffset old_parent = current_node->parent;
        const BVHNodeOffset new_parent = createNode(nullptr, aabb::mergeBounds(object_bounds, current_node->bounds));
        const BVHNodeOffset new_leaf   = createNode(user_data, object_bounds);

        nodes[new_parent].children[0] = sibling;
        nodes[new_parent].children[1] = new_leaf;
        nodes[new_parent].parent      = old_parent;
        nodes[new_leaf].parent        = new_parent;
        nodes[sibling].parent         = new_parent;

        // Sibling is Root.
        if (bvh_node::isNull(old_parent))
        {
          root_idx = new_parent;
        }
        else
        {
          if (nodes[old_parent].children[0] == sibling)
          {
            nodes[old_parent].children[0] = new_parent;
          }
          else
          {
            nodes[old_parent].children[1] = new_parent;
          }
        }

        updateDepth(new_parent, current_node->depth);
        refitChildren(root_idx, true);  // TODO: This is doing more work than need be.

        return new_leaf;
      }

      root_idx = createNode(user_data, object_bounds);
      return root_idx;
    }

    // Call when the object associated with this leaf has moved.
    void markLeafDirty(BVHNodeOffset leaf, const AABB& bounds)
    {
      const AABB object_bounds = aabb::expandedBy(bounds, k_BVHBoundsSkin);

      nodes[leaf].bounds = object_bounds;

      if (leaf == root_idx)
      {
        return;
      }

      if (refitChildren(nodes[leaf].parent, true))
      {
        addNodeToRefit(nodes[leaf].parent);
      }
    }

    void remove(BVHNodeOffset leaf)
    {
      addToFreelist(leaf);

      if (leaf == root_idx)
      {
        root_idx = k_BVHNodeInvalidOffset;
        return;
      }

      const BVHNodeOffset parent      = nodes[leaf].parent;
      const BVHNodeOffset grandparent = nodes[parent].parent;
      const auto          old_depth   = nodes[parent].depth;

      nodes[parent]        = nodes[nodes[parent].children[(leaf == nodes[parent].children[0] ? 1 : 0)]];
      nodes[parent].parent = grandparent;
      nodes[parent].depth  = old_depth;

      refitChildren(bvh_node::isNull(grandparent) ? root_idx : grandparent, true);
    }

    /*!
     * @brief
     *   Optimizes the tree using some rotations to rebalance.
     *
     * @param temp_memory
     *   Pass in a nice and fast allocator for a temp stack.
     *
     * @param refit_parents_with_no_rotation
     *   Should be false 98% of the time.
    */
    void endFrame(LinearAllocator& temp_memory, bool refit_parents_with_no_rotation = true)
    {
      struct OffsetIndexPair final
      {
        BVHNodeOffset node;   // Index into BVH::nodes
        BVHNodeOffset index;  // Index into BVH::nodes_to_optimize
      };

      while (!nodes_to_optimize.isEmpty())
      {
        LinearAllocatorScope   memory_scope        = {temp_memory};
        const int              num_nodes           = int(nodes_to_optimize.size());
        int                    max_depth           = nodeAt(nodes_to_optimize[0]).depth;
        OffsetIndexPair* const max_depth_nodes     = temp_memory.allocateArrayTrivial<OffsetIndexPair>(num_nodes);
        int                    num_max_depth_nodes = 0;

        // Stage 1: Grab all the max depth nodes.

        max_depth_nodes[num_max_depth_nodes++] = {nodes_to_optimize[0], 0};

        for (int i = 1; i < num_nodes; ++i)
        {
          const BVHNodeOffset node_offset = nodes_to_optimize[i];
          auto&               node_at_i   = nodeAt(node_offset);

          if (max_depth > node_at_i.depth)
          {
            continue;
          }

          if (max_depth < node_at_i.depth)
          {
            max_depth           = node_at_i.depth;
            num_max_depth_nodes = 0;
          }

          max_depth_nodes[num_max_depth_nodes++] = {node_offset, static_cast<BVHNodeOffset>(i)};
        }

        // Stage 2: Clear out the level we will be evaluating.

        for (int i = 0; i < num_max_depth_nodes; ++i)
        {
          nodes_to_optimize[max_depth_nodes[i].index] = k_BVHNodeInvalidOffset;
        }

        const auto split = std::partition(nodes_to_optimize.begin(), nodes_to_optimize.end(), [](const BVHNodeOffset element) -> bool {
          return element < k_BVHNodeInvalidOffset;
        });

        const std::size_t new_size = split - nodes_to_optimize.begin();

        nodes_to_optimize.resize(new_size);

        // Stage 3: Removed the visted nodes and Attempt to rebalance the tree.

        for (int i = 0; i < num_max_depth_nodes; ++i)
        {
          BVHNode& node    = nodeAt(max_depth_nodes[i].node);
          BVHNode& child_l = nodeAt(node.children[0]);
          BVHNode& child_r = nodeAt(node.children[1]);

          if (bvh_node::isLeaf(child_l) && bvh_node::isLeaf(child_r))
          {
            if (!bvh_node::isNull(node.parent))
            {
              nodes_to_optimize.push(node.parent);
              continue;
            }
          }

          enum class RotationOp
          {
            NONE,
            L_RL,
            L_RR,
            R_LL,
            R_LR,
            LL_RR,
            LL_RL,
            MAX,
          };

          struct RotationCandidate final
          {
            float      cost;
            RotationOp op;
          };

          const float left_sa           = aabb::surfaceArea(child_l.bounds);
          const float right_sa          = aabb::surfaceArea(child_r.bounds);
          const float base_surface_area = left_sa + right_sa;
          float       rotation_candidates[int(RotationOp::MAX)];
          int         num_rotation_candidates = 0;

          rotation_candidates[num_rotation_candidates++] = base_surface_area;

          if (!bvh_node::isLeaf(child_r))
          {
            BVHNode& right_left  = nodeAt(child_r.children[0]);
            BVHNode& right_right = nodeAt(child_r.children[1]);
            AABB     aabb_lrr;  // NOLINT
            AABB     aabb_lrl;  // NOLINT

            aabb::mergeBounds(aabb_lrr, child_l.bounds, right_right.bounds);
            aabb::mergeBounds(aabb_lrl, child_l.bounds, right_left.bounds);

            rotation_candidates[num_rotation_candidates++] = aabb::surfaceArea(right_left.bounds) +
                                                             aabb::surfaceArea(aabb_lrr);  // RotationOp::L_RL
            rotation_candidates[num_rotation_candidates++] = aabb::surfaceArea(right_right.bounds) +
                                                             aabb::surfaceArea(aabb_lrl);  // RotationOp::L_RR
          }
          else
          {
            rotation_candidates[num_rotation_candidates++] = FLT_MAX;  // RotationOp::L_RL
            rotation_candidates[num_rotation_candidates++] = FLT_MAX;  // RotationOp::L_RR
          }

          if (!bvh_node::isLeaf(child_l))
          {
            BVHNode& left_left  = nodeAt(child_l.children[0]);
            BVHNode& left_right = nodeAt(child_l.children[1]);
            AABB     aabb_rlr;  // NOLINT
            AABB     aabb_rll;  // NOLINT

            aabb::mergeBounds(aabb_rlr, child_r.bounds, left_right.bounds);
            aabb::mergeBounds(aabb_rll, child_r.bounds, left_left.bounds);

            rotation_candidates[num_rotation_candidates++] = aabb::surfaceArea(left_left.bounds) +
                                                             aabb::surfaceArea(aabb_rlr);  // RotationOp::R_LL
            rotation_candidates[num_rotation_candidates++] = aabb::surfaceArea(left_right.bounds) +
                                                             aabb::surfaceArea(aabb_rll);  // RotationOp::R_LR
          }
          else
          {
            rotation_candidates[num_rotation_candidates++] = FLT_MAX;  // RotationOp::R_LL
            rotation_candidates[num_rotation_candidates++] = FLT_MAX;  // RotationOp::R_LR
          }

          if (!bvh_node::isLeaf(child_l) && !bvh_node::isLeaf(child_r))
          {
            BVHNode& left_left   = nodeAt(child_l.children[0]);
            BVHNode& left_right  = nodeAt(child_l.children[1]);
            BVHNode& right_left  = nodeAt(child_r.children[0]);
            BVHNode& right_right = nodeAt(child_r.children[1]);
            AABB     aabb_rrlr;  // NOLINT
            AABB     aabb_rlll;  // NOLINT
            AABB     aabb_rllr;  // NOLINT
            AABB     aabb_llrr;  // NOLINT

            aabb::mergeBounds(aabb_rrlr, right_right.bounds, left_right.bounds);
            aabb::mergeBounds(aabb_rlll, right_left.bounds, left_left.bounds);
            aabb::mergeBounds(aabb_rllr, right_left.bounds, left_right.bounds);
            aabb::mergeBounds(aabb_llrr, left_left.bounds, right_right.bounds);

            rotation_candidates[num_rotation_candidates++] = aabb::surfaceArea(aabb_rrlr) +
                                                             aabb::surfaceArea(aabb_rlll);  // RotationOp::LL_RR
            rotation_candidates[num_rotation_candidates++] = aabb::surfaceArea(aabb_rllr) +
                                                             aabb::surfaceArea(aabb_llrr);  // RotationOp::LL_RL
          }
          else
          {
            rotation_candidates[num_rotation_candidates++] = FLT_MAX;  // RotationOp::LL_RR
            rotation_candidates[num_rotation_candidates++] = FLT_MAX;  // RotationOp::LL_RL
          }

          RotationOp best_candidate = RotationOp::NONE;

          for (int j = 1; j < num_rotation_candidates; ++j)
          {
            if (rotation_candidates[i] < rotation_candidates[int(best_candidate)])
            {
              best_candidate = RotationOp(j);
            }
          }

          if (best_candidate == RotationOp::NONE)
          {
            if (!bvh_node::isNull(node.parent) && refit_parents_with_no_rotation)
            {
              addNodeToRefit(node.parent);
            }
          }
          else
          {
            if (!bvh_node::isNull(node.parent))
            {
              addNodeToRefit(node.parent);
            }

            const float factor = (base_surface_area - rotation_candidates[int(best_candidate)]) / base_surface_area;

            if (factor < k_BVHRotationBenefit)
            {
              continue;
            }

            const BVHNodeOffset self_idx = nodeToIndex(node);

            switch (best_candidate)
            {
              case RotationOp::L_RL:
              {
                const BVHNodeOffset swap       = nodeToIndex(child_l);
                BVHNode&            right_left = nodeAt(child_r.children[0]);

                adoptNode(self_idx, nodeToIndex(right_left), 0);
                adoptNode(nodeToIndex(child_r), swap, 0);
                refitChildren(nodeToIndex(child_r), false);
                updateDepth(self_idx, node.depth);
                break;
              }
              case RotationOp::L_RR:
              {
                BVHNode&            right_right = nodeAt(child_r.children[1]);
                const BVHNodeOffset swap        = nodeToIndex(child_l);

                adoptNode(self_idx, nodeToIndex(right_right), 0);
                adoptNode(nodeToIndex(child_r), swap, 1);
                refitChildren(nodeToIndex(child_r), false);
                updateDepth(self_idx, node.depth);
                break;
              }
              case RotationOp::R_LL:
              {
                const BVHNodeOffset swap      = nodeToIndex(child_r);
                BVHNode&            left_left = nodeAt(child_l.children[0]);

                adoptNode(self_idx, nodeToIndex(left_left), 1);
                adoptNode(nodeToIndex(child_l), swap, 0);
                refitChildren(nodeToIndex(child_l), false);
                updateDepth(self_idx, node.depth);
                break;
              }
              case RotationOp::R_LR:
              {
                const BVHNodeOffset swap      = nodeToIndex(child_r);
                BVHNode&            left_left = nodeAt(child_l.children[0]);

                adoptNode(self_idx, nodeToIndex(left_left), 1);
                adoptNode(nodeToIndex(child_l), swap, 1);
                refitChildren(nodeToIndex(child_l), false);
                updateDepth(self_idx, node.depth);
                break;
              }
              case RotationOp::LL_RR:
              {
                BVHNode&            right_right = nodeAt(child_r.children[1]);
                BVHNode&            left_left   = nodeAt(child_l.children[0]);
                const BVHNodeOffset swap        = nodeToIndex(left_left);

                adoptNode(nodeToIndex(child_l), nodeToIndex(right_right), 0);
                adoptNode(nodeToIndex(child_r), swap, 1);
                refitChildren(nodeToIndex(child_l), false);
                refitChildren(nodeToIndex(child_r), false);
                break;
              }
              case RotationOp::LL_RL:
              {
                BVHNode&            left_left  = nodeAt(child_l.children[0]);
                BVHNode&            right_left = nodeAt(child_r.children[0]);
                const BVHNodeOffset swap       = nodeToIndex(left_left);

                adoptNode(nodeToIndex(child_l), nodeToIndex(right_left), 0);
                adoptNode(nodeToIndex(child_r), swap, 0);
                refitChildren(nodeToIndex(child_l), false);
                refitChildren(nodeToIndex(child_r), false);

                break;
              }
              case RotationOp::NONE:
              case RotationOp::MAX:
                bfInvalidDefaultCase();
            }
          }
        }
      }
    }

   private:
    void addNodeToRefit(BVHNodeOffset node)
    {
      nodes_to_optimize.push(node);
    }

    BVHNode& nodeAt(BVHNodeOffset index)
    {
      return nodes[index];
    }

    BVHNodeOffset nodeToIndex(const BVHNode& node) const
    {
      return BVHNodeOffset(&node - nodes.data());
    }

    void adoptNode(BVHNodeOffset self, BVHNodeOffset child, int index)
    {
      nodes[self].children[index] = child;
      nodes[child].parent         = self;
    }

    bool refitChildren(BVHNodeOffset self, bool propogate)
    {
      const BVHNode& node       = nodeAt(self);
      const AABB     new_bounds = aabb::mergeBounds(nodes[node.children[0]].bounds, nodes[node.children[1]].bounds);

      if (nodes[self].bounds != new_bounds)
      {
        nodes[self].bounds = new_bounds;

        if (propogate && !bvh_node::isNull(node.parent))
        {
          refitChildren(node.parent, propogate);
        }

        return true;
      }

      return false;
    }

    void updateDepth(BVHNodeOffset self, int depth)
    {
      BVHNode& node = nodeAt(self);

      if (depth > max_depth)
      {
        max_depth = depth;
      }

      if (!bvh_node::isLeaf(node))
      {
        updateDepth(node.children[0], depth + 1);
        updateDepth(node.children[1], depth + 1);
      }
    }

    static void resetNode(BVHNode& node, void* user_data, const AABB& bounds)
    {
      node.bounds      = bounds;
      node.user_data   = user_data;
      node.parent      = k_BVHNodeInvalidOffset;
      node.children[0] = k_BVHNodeInvalidOffset;
      node.children[1] = k_BVHNodeInvalidOffset;
    }

    BVHNodeOffset createNode(void* user_data, const AABB& bounds)
    {
      if (!bvh_node::isNull(freelist))
      {
        const BVHNodeOffset idx  = freelist;
        BVHNode&            node = nodes[idx];
        resetNode(node, user_data, bounds);
        freelist = nodes[idx].next;

        return idx;
      }

      const BVHNodeOffset idx = BVHNodeOffset(nodes.size());

      resetNode(nodes.emplace(), user_data, bounds);

      return idx;
    }

    void addToFreelist(BVHNodeOffset index)
    {
      nodes[index].next = freelist;
      freelist          = index;
    }
  };

  class CollisionSystem final : public IECSSystem
  {
   private:
   protected:
    void onFrameBegin(Engine& engine, float dt) override;
    void onFrameUpdate(Engine& engine, float dt) override;
    void onFrameEnd(Engine& engine, float dt) override;
    void onFrameDraw(Engine& engine, float alpha) override;
  };
}  // namespace bifrost

#endif /* BIFROST_COLLISION_SYSTEM */
