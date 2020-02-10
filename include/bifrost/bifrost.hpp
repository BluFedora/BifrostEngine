/*!
 * @file   bifrost.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Includes all of the Engine's sub modules.
 *
 * @version 0.0.1
 * @date    2020-01-12
 *
 * @copyright Copyright (c) 2020
 */
#ifndef BIFROST_HPP
#define BIFROST_HPP

// AssetIO
#include "asset_io/bifrost_asset_handle.hpp"
#include "asset_io/bifrost_assets.hpp"
#include "asset_io/bifrost_file.hpp"
#include "asset_io/bifrost_json_parser.hpp"
#include "asset_io/bifrost_json_value.hpp"
#include "asset_io/bifrost_scene.hpp"

// Core
#include "bifrost/core/bifrost_base_object.hpp"
#include "bifrost/core/bifrost_game_state_machine.hpp"
#include "bifrost/core/bifrost_igame_state_layer.hpp"

// Data Structures
#include "data_structures/bifrost_array_t.h"

// Debug
#include "debug/bifrost_dbg_logger.h"

// ECS
#include "ecs/bifrost_component.hpp"
#include "ecs/bifrost_entity.hpp"

// Graphics
#include "graphics/bifrost_gfx_api.h"

// Math
#include "bifrost_math.h"

// Meta
#include "meta/bifrost_meta_runtime.hpp"

// Script
#include "bifrost_vm.hpp"

// Utility
#include "utility/bifrost_non_copy_move.hpp"

#endif /* BIFROST_HPP */