/**************************************************************************/
/*  gi.cpp                                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "gi.h"

#include "core/config/project_settings.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "servers/rendering/rendering_server_default.h"

// Debug recreating everything every frame.
//#define DIRTY_ALL_FRAMES

using namespace RendererRD;

const Vector3i GI::HDDAGI::Cascade::DIRTY_ALL = Vector3i(0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF);

GI *GI::singleton = nullptr;

////////////////////////////////////////////////////////////////////////////////
// VOXEL GI STORAGE

RID GI::voxel_gi_allocate() {
	return voxel_gi_owner.allocate_rid();
}

void GI::voxel_gi_free(RID p_voxel_gi) {
	voxel_gi_allocate_data(p_voxel_gi, Transform3D(), AABB(), Vector3i(), Vector<uint8_t>(), Vector<uint8_t>(), Vector<uint8_t>(), Vector<int>()); //deallocate
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	voxel_gi->dependency.deleted_notify(p_voxel_gi);
	voxel_gi_owner.free(p_voxel_gi);
}

void GI::voxel_gi_initialize(RID p_voxel_gi) {
	voxel_gi_owner.initialize_rid(p_voxel_gi, VoxelGI());
}

void GI::voxel_gi_allocate_data(RID p_voxel_gi, const Transform3D &p_to_cell_xform, const AABB &p_aabb, const Vector3i &p_octree_size, const Vector<uint8_t> &p_octree_cells, const Vector<uint8_t> &p_data_cells, const Vector<uint8_t> &p_distance_field, const Vector<int> &p_level_counts) {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	if (voxel_gi->octree_buffer.is_valid()) {
		RD::get_singleton()->free(voxel_gi->octree_buffer);
		RD::get_singleton()->free(voxel_gi->data_buffer);
		if (voxel_gi->sdf_texture.is_valid()) {
			RD::get_singleton()->free(voxel_gi->sdf_texture);
		}

		voxel_gi->sdf_texture = RID();
		voxel_gi->octree_buffer = RID();
		voxel_gi->data_buffer = RID();
		voxel_gi->octree_buffer_size = 0;
		voxel_gi->data_buffer_size = 0;
		voxel_gi->cell_count = 0;
	}

	voxel_gi->to_cell_xform = p_to_cell_xform;
	voxel_gi->bounds = p_aabb;
	voxel_gi->octree_size = p_octree_size;
	voxel_gi->level_counts = p_level_counts;

	if (p_octree_cells.size()) {
		ERR_FAIL_COND(p_octree_cells.size() % 32 != 0); //cells size must be a multiple of 32

		uint32_t cell_count = p_octree_cells.size() / 32;

		ERR_FAIL_COND(p_data_cells.size() != (int)cell_count * 16); //see that data size matches

		voxel_gi->cell_count = cell_count;
		voxel_gi->octree_buffer = RD::get_singleton()->storage_buffer_create(p_octree_cells.size(), p_octree_cells);
		voxel_gi->octree_buffer_size = p_octree_cells.size();
		voxel_gi->data_buffer = RD::get_singleton()->storage_buffer_create(p_data_cells.size(), p_data_cells);
		voxel_gi->data_buffer_size = p_data_cells.size();

		if (p_distance_field.size()) {
			RD::TextureFormat tf;
			tf.format = RD::DATA_FORMAT_R8_UNORM;
			tf.width = voxel_gi->octree_size.x;
			tf.height = voxel_gi->octree_size.y;
			tf.depth = voxel_gi->octree_size.z;
			tf.texture_type = RD::TEXTURE_TYPE_3D;
			tf.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_CAN_UPDATE_BIT | RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
			Vector<Vector<uint8_t>> s;
			s.push_back(p_distance_field);
			voxel_gi->sdf_texture = RD::get_singleton()->texture_create(tf, RD::TextureView(), s);
			RD::get_singleton()->set_resource_name(voxel_gi->sdf_texture, "VoxelGI SDF Texture");
		}
	}

	voxel_gi->version++;
	voxel_gi->data_version++;

	voxel_gi->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

AABB GI::voxel_gi_get_bounds(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, AABB());

	return voxel_gi->bounds;
}

Vector3i GI::voxel_gi_get_octree_size(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector3i());
	return voxel_gi->octree_size;
}

Vector<uint8_t> GI::voxel_gi_get_octree_cells(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<uint8_t>());

	if (voxel_gi->octree_buffer.is_valid()) {
		return RD::get_singleton()->buffer_get_data(voxel_gi->octree_buffer);
	}
	return Vector<uint8_t>();
}

Vector<uint8_t> GI::voxel_gi_get_data_cells(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<uint8_t>());

	if (voxel_gi->data_buffer.is_valid()) {
		return RD::get_singleton()->buffer_get_data(voxel_gi->data_buffer);
	}
	return Vector<uint8_t>();
}

Vector<uint8_t> GI::voxel_gi_get_distance_field(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<uint8_t>());

	if (voxel_gi->data_buffer.is_valid()) {
		return RD::get_singleton()->texture_get_data(voxel_gi->sdf_texture, 0);
	}
	return Vector<uint8_t>();
}

Vector<int> GI::voxel_gi_get_level_counts(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<int>());

	return voxel_gi->level_counts;
}

Transform3D GI::voxel_gi_get_to_cell_xform(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Transform3D());

	return voxel_gi->to_cell_xform;
}

void GI::voxel_gi_set_dynamic_range(RID p_voxel_gi, float p_range) {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->dynamic_range = p_range;
	voxel_gi->version++;
}

float GI::voxel_gi_get_dynamic_range(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);

	return voxel_gi->dynamic_range;
}

void GI::voxel_gi_set_propagation(RID p_voxel_gi, float p_range) {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->propagation = p_range;
	voxel_gi->version++;
}

float GI::voxel_gi_get_propagation(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->propagation;
}

void GI::voxel_gi_set_energy(RID p_voxel_gi, float p_energy) {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->energy = p_energy;
}

float GI::voxel_gi_get_energy(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->energy;
}

void GI::voxel_gi_set_baked_exposure_normalization(RID p_voxel_gi, float p_baked_exposure) {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->baked_exposure = p_baked_exposure;
}

float GI::voxel_gi_get_baked_exposure_normalization(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->baked_exposure;
}

void GI::voxel_gi_set_bias(RID p_voxel_gi, float p_bias) {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->bias = p_bias;
}

float GI::voxel_gi_get_bias(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->bias;
}

void GI::voxel_gi_set_normal_bias(RID p_voxel_gi, float p_normal_bias) {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->normal_bias = p_normal_bias;
}

float GI::voxel_gi_get_normal_bias(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->normal_bias;
}

void GI::voxel_gi_set_interior(RID p_voxel_gi, bool p_enable) {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->interior = p_enable;
}

void GI::voxel_gi_set_use_two_bounces(RID p_voxel_gi, bool p_enable) {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->use_two_bounces = p_enable;
	voxel_gi->version++;
}

bool GI::voxel_gi_is_using_two_bounces(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, false);
	return voxel_gi->use_two_bounces;
}

bool GI::voxel_gi_is_interior(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, false);
	return voxel_gi->interior;
}

uint32_t GI::voxel_gi_get_version(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->version;
}

uint32_t GI::voxel_gi_get_data_version(RID p_voxel_gi) {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->data_version;
}

RID GI::voxel_gi_get_octree_buffer(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, RID());
	return voxel_gi->octree_buffer;
}

RID GI::voxel_gi_get_data_buffer(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, RID());
	return voxel_gi->data_buffer;
}

RID GI::voxel_gi_get_sdf_texture(RID p_voxel_gi) {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, RID());

	return voxel_gi->sdf_texture;
}

Dependency *GI::voxel_gi_get_dependency(RID p_voxel_gi) const {
	VoxelGI *voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, nullptr);

	return &voxel_gi->dependency;
}

void GI::hddagi_reset() {
	hddagi_current_version++;
}

////////////////////////////////////////////////////////////////////////////////
// HDDAGI

static RID create_clear_texture(const RD::TextureFormat &p_format, const String &p_name) {
	RID texture = RD::get_singleton()->texture_create(p_format, RD::TextureView());
	ERR_FAIL_COND_V_MSG(texture.is_null(), RID(), String("Cannot create texture: ") + p_name);

	RD::get_singleton()->set_resource_name(texture, p_name);
	RD::get_singleton()->texture_clear(texture, Color(0, 0, 0, 0), 0, p_format.mipmaps, 0, p_format.array_layers);

	return texture;
}

void GI::HDDAGI::create(RID p_env, const Vector3 &p_world_position, uint32_t p_requested_history_size, GI *p_gi) {
	//RendererRD::TextureStorage *texture_storage = RendererRD::TextureStorage::get_singleton();
	//RendererRD::MaterialStorage *material_storage = RendererRD::MaterialStorage::get_singleton();

	cascade_size.x = cascade_size.z = CASCADE_SIZE;

	cascade_format = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_cascade_format(p_env);
	switch (cascade_format) {
		case RS::ENV_HDDAGI_CASCADE_FORMAT_16x16x16: {
			cascade_size.y = CASCADE_SIZE;
			y_mult = 1.0;
		} break;
		case RS::ENV_HDDAGI_CASCADE_FORMAT_16x16x16_50_PERCENT_HEIGHT: {
			cascade_size.y = CASCADE_SIZE;
			y_mult = 2.0;
		} break;
		case RS::ENV_HDDAGI_CASCADE_FORMAT_16x16x16_75_PERCENT_HEIGHT: {
			cascade_size.y = CASCADE_SIZE;
			y_mult = 1.5;
		} break;
		case RS::ENV_HDDAGI_CASCADE_FORMAT_16x8x16:
		default: {
			cascade_size.y = CASCADE_SIZE / 2;
			y_mult = 1.0;
		} break;
	}

	gi = p_gi;
	num_cascades = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_cascades(p_env);
	min_cell_size = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_min_cell_size(p_env);
	using_probe_filter = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_filter_probes(p_env);
	using_reflection_filter = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_filter_reflection(p_env);
	using_ambient_filter = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_filter_ambient(p_env);
	reflection_bias = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_reflection_bias(p_env);
	probe_bias = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_probe_bias(p_env);
	occlusion_bias = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_occlusion_bias(p_env);
	normal_bias = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_normal_bias(p_env);
	frames_to_converge = p_requested_history_size;
	version = gi->hddagi_current_version;
	cascades.resize(num_cascades);

	solid_cell_ratio = gi->hddagi_solid_cell_ratio;
	solid_cell_count = uint32_t(float(cascade_size.x * cascade_size.y * cascade_size.z) * solid_cell_ratio);

	float base_cell_size = min_cell_size;

	RD::TextureFormat tf_base;
	tf_base.format = RD::DATA_FORMAT_R8_UNORM;
	tf_base.width = cascade_size.x;
	tf_base.height = cascade_size.y;
	tf_base.depth = cascade_size.z;
	tf_base.texture_type = RD::TEXTURE_TYPE_3D;
	tf_base.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_TO_BIT | RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT;

	{
		RD::TextureFormat tf_voxel = tf_base;
		tf_voxel.format = RD::DATA_FORMAT_R32G32_UINT; // 4x4 region in a cache friendly format
		tf_voxel.width /= 4;
		tf_voxel.height /= 4;
		tf_voxel.depth /= 4;
		tf_voxel.texture_type = RD::TEXTURE_TYPE_3D;
		tf_voxel.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_TO_BIT | RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT;

		tf_voxel.height *= cascades.size();
		voxel_bits_tex = create_clear_texture(tf_voxel, "HDDAGI Voxel Field");
	}

	{
		RD::TextureFormat tf_voxel = tf_base;
		tf_voxel.format = RD::DATA_FORMAT_R16_UINT; // 4x4 region in a cache friendly format
		tf_voxel.width /= REGION_CELLS;
		tf_voxel.height /= REGION_CELLS;
		tf_voxel.depth /= REGION_CELLS;
		tf_voxel.height *= cascades.size();
		tf_voxel.texture_type = RD::TEXTURE_TYPE_3D;
		tf_voxel.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_TO_BIT | RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
		region_version_data = create_clear_texture(tf_voxel, "HDDAGI Region Version");
	}

	{
		RD::TextureFormat tf_disocc = tf_base;
		tf_disocc.format = RD::DATA_FORMAT_R8_UINT; // 4x4 region in a cache friendly format
		tf_disocc.texture_type = RD::TEXTURE_TYPE_3D;
		tf_disocc.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_TO_BIT | RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
		tf_disocc.height *= cascades.size();
		voxel_disocclusion_tex = create_clear_texture(tf_disocc, "HDDAGI Voxel Disocclusion");
	}

	{
		RD::TextureFormat tf_voxel_region = tf_base;
		tf_voxel_region.format = RD::DATA_FORMAT_R8_UINT;
		tf_voxel_region.width /= REGION_CELLS;
		tf_voxel_region.height /= REGION_CELLS;
		tf_voxel_region.depth /= REGION_CELLS;
		tf_voxel_region.texture_type = RD::TEXTURE_TYPE_3D;
		tf_voxel_region.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_TO_BIT | RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
		RD::TextureFormat tf_sdf = tf_base;

		tf_voxel_region.height *= cascades.size();
		voxel_region_tex = create_clear_texture(tf_voxel_region, "HDDAGI Voxel Regions");
	}

	{
		{
			RD::TextureFormat tf_light = tf_base;
			tf_light.format = RD::DATA_FORMAT_R32_UINT;
			tf_light.height *= cascades.size();
			tf_light.shareable_formats.push_back(RD::DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32);
			tf_light.shareable_formats.push_back(RD::DATA_FORMAT_R32_UINT);

			voxel_light_tex_data = create_clear_texture(tf_light, "HDDAGI Cascade Light Data");

			RD::TextureView tv;
			tv.format_override = RD::DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32;
			voxel_light_tex = RD::get_singleton()->texture_create_shared(tv, voxel_light_tex_data);
		}

		{
			RD::TextureFormat tf_neighbour = tf_base;
			tf_neighbour.format = RD::DATA_FORMAT_R32_UINT;
			tf_neighbour.height *= cascades.size();

			voxel_light_neighbour_data = create_clear_texture(tf_neighbour, "HDDAGI Cascade Light Neighbours");
		}

		{ // Albedo texture, this is anisotropic (x6).
			RD::TextureFormat tf_render_albedo = tf_base;
			tf_render_albedo.width /= 2; // Albedo is half size..
			tf_render_albedo.height /= 2;
			tf_render_albedo.depth /= 2;
			tf_render_albedo.depth *= 6; // ..but anisotropic.
			tf_render_albedo.format = RD::DATA_FORMAT_R16_UINT;
			render_albedo = create_clear_texture(tf_render_albedo, "HDDAGI Render Albedo");
		}

		{ // Emission texture, this is anisotropic but in a different way (main light, then x6 aniso weight) to save space.
			RD::TextureFormat tf_render_emission = tf_base;
			tf_render_emission.width /= 2; // Emission is half size..
			tf_render_emission.height /= 2;
			tf_render_emission.depth /= 2;
			tf_render_emission.format = RD::DATA_FORMAT_R32_UINT;
			render_emission = create_clear_texture(tf_render_emission, "HDDAGI Render Emission");
			render_emission_aniso = create_clear_texture(tf_render_emission, "HDDAGI Render Emission Aniso");
		}

		{ // Aniso normals
			RD::TextureFormat tf_render_aniso_normals = tf_base;
			tf_render_aniso_normals.format = RD::DATA_FORMAT_R32_UINT;
			render_aniso_normals = create_clear_texture(tf_render_aniso_normals, String("HDDAGI Render Solid Bits "));
		}
	}

	light_process_buffer_render = RD::get_singleton()->storage_buffer_create(sizeof(HDDAGI::Cascade::LightProcessCell) * solid_cell_count);
	light_process_dispatch_buffer_render = RD::get_singleton()->storage_buffer_create(sizeof(uint32_t) * 4, Vector<uint8_t>(), RD::STORAGE_BUFFER_USAGE_DISPATCH_INDIRECT);

	cascades_ubo = RD::get_singleton()->uniform_buffer_create(sizeof(HDDAGI::Cascade::UBO) * HDDAGI::MAX_CASCADES);

	// lightprobes
	Vector3i PROBE_DIVISOR = cascade_size / REGION_CELLS;

	{
		RD::TextureFormat tf_lightprobes;
		tf_lightprobes.texture_type = RD::TEXTURE_TYPE_2D_ARRAY;
		tf_lightprobes.format = RD::DATA_FORMAT_R32_UINT;
		tf_lightprobes.width = (PROBE_DIVISOR.x + 1) * (LIGHTPROBE_OCT_SIZE + 2); // Divisor +1 because need an extra one on the outside of the box for proper interpolation. This contains also the Z probes towards x+
		tf_lightprobes.height = (PROBE_DIVISOR.y + 1) * (PROBE_DIVISOR.z + 1) * (LIGHTPROBE_OCT_SIZE + 2); // OctSize +2 because it needs a border to interpolate properly
		tf_lightprobes.depth = 1;
		tf_lightprobes.array_layers = cascades.size();
		tf_lightprobes.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_TO_BIT | RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
		tf_lightprobes.shareable_formats.push_back(RD::DATA_FORMAT_R32_UINT);
		tf_lightprobes.shareable_formats.push_back(RD::DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32);

		lightprobe_specular_data = create_clear_texture(tf_lightprobes, String("HDDAGI Lighprobe Specular"));
		lightprobe_diffuse_data = create_clear_texture(tf_lightprobes, String("HDDAGI Lighprobe Diffuse"));
		lightprobe_diffuse_filter_data = create_clear_texture(tf_lightprobes, String("HDDAGI Lighprobe Diffuse Filtered"));

		RD::TextureView tv;
		tv.format_override = RD::DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		lightprobe_diffuse_tex = RD::get_singleton()->texture_create_shared(tv, lightprobe_diffuse_data);
		lightprobe_diffuse_filter_tex = RD::get_singleton()->texture_create_shared(tv, lightprobe_diffuse_filter_data);
		lightprobe_specular_tex = RD::get_singleton()->texture_create_shared(tv, lightprobe_specular_data);

		RD::TextureFormat tf_cache_data = tf_lightprobes;
		tf_cache_data.format = RD::DATA_FORMAT_R32_UINT;
		tf_cache_data.width = (PROBE_DIVISOR.x + 1) * LIGHTPROBE_OCT_SIZE;
		tf_cache_data.height = (PROBE_DIVISOR.y + 1) * (PROBE_DIVISOR.z + 1) * LIGHTPROBE_OCT_SIZE;
		tf_cache_data.array_layers *= frames_to_converge;
		tf_cache_data.shareable_formats.clear();
		lightprobe_hit_cache_data = create_clear_texture(tf_cache_data, String("HDDAGI Lighprobe Hit Cache"));

		tf_cache_data.format = RD::DATA_FORMAT_R16_UINT;
		lightprobe_hit_cache_version_data = create_clear_texture(tf_cache_data, String("HDDAGI Lighprobe Hit Cache Version"));

		{
			tf_cache_data.format = RD::DATA_FORMAT_R32_UINT;

			lightprobe_moving_average_history = create_clear_texture(tf_cache_data, String("HDDAGI Lighprobe Moving Average History"));

			RD::TextureFormat tf_moving_average = tf_cache_data;
			tf_moving_average.width *= 3; // no RGB32 UI so..
			tf_moving_average.array_layers = cascades.size(); // Return to just cascades, no history.
			lightprobe_moving_average = create_clear_texture(tf_moving_average, String("HDDAGI Lighprobe Moving Average"));
		}

		RD::TextureFormat tf_ambient = tf_lightprobes;
		tf_ambient.width = (PROBE_DIVISOR.x + 1);
		tf_ambient.height = (PROBE_DIVISOR.y + 1) * (PROBE_DIVISOR.z + 1);
		tf_ambient.format = RD::DATA_FORMAT_R16G16B16A16_SFLOAT;
		tf_ambient.shareable_formats.clear();
		lightprobe_ambient_tex = create_clear_texture(tf_ambient, String("HDDAGI Ambient Light Texture"));

		RD::TextureFormat tf_neighbours;
		tf_neighbours.texture_type = RD::TEXTURE_TYPE_2D_ARRAY;
		tf_neighbours.format = RD::DATA_FORMAT_R32_UINT;
		tf_neighbours.width = (PROBE_DIVISOR.x + 1);
		tf_neighbours.height = (PROBE_DIVISOR.y + 1) * (PROBE_DIVISOR.z + 1);
		tf_neighbours.depth = 1;
		tf_neighbours.array_layers = cascades.size();
		tf_neighbours.usage_bits = RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_TO_BIT | RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
		lightprobe_neighbour_visibility_map = create_clear_texture(tf_neighbours, String("HDDAGI Neighbour Visibility Map"));

		RD::TextureFormat tf_geometry_proximity = tf_neighbours;

		tf_geometry_proximity.format = RD::DATA_FORMAT_R8_UNORM;
		lightprobe_geometry_proximity_map = create_clear_texture(tf_geometry_proximity, String("HDDAGI Geometry Proximity Map"));
		lightprobe_camera_visibility_map = create_clear_texture(tf_geometry_proximity, String("HDDAGI Camera Visibility Map"));

		for (uint32_t i = 0; i < cascades.size(); i++) {
			for (int j = 0; j < 4; j++) {
				lightprobe_camera_buffers.push_back(RD::get_singleton()->uniform_buffer_create(sizeof(HDDAGIShader::IntegrateCameraUBO)));
			}
		}

		RD::TextureFormat tf_process_frame = tf_neighbours;
		tf_process_frame.format = RD::DATA_FORMAT_R32_UINT;
		lightprobe_process_frame = create_clear_texture(tf_process_frame, String("HDDAGI Lightprobe Frame"));
	}

	// Occlusion

	{
		RD::TextureFormat tf_occlusion = tf_base;
		tf_occlusion.format = RD::DATA_FORMAT_R16_UINT;
		tf_occlusion.width += 2; // Extra border for proper blending.
		tf_occlusion.height += 2; // Extra border for proper blending.
		tf_occlusion.depth += 2; // Extra border for proper blending.
		tf_occlusion.height *= cascades.size();
		tf_occlusion.shareable_formats.push_back(RD::DATA_FORMAT_R4G4B4A4_UNORM_PACK16);
		tf_occlusion.shareable_formats.push_back(RD::DATA_FORMAT_R16_UINT);
		tf_occlusion.usage_bits = RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_CAN_COPY_TO_BIT | RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT;

		for (int i = 0; i < 2; i++) {
			occlusion_data[i] = create_clear_texture(tf_occlusion, String("HDDAGI Occlusion ") + itos(i));
			RD::TextureView tv;
			tv.format_override = RD::DATA_FORMAT_R4G4B4A4_UNORM_PACK16;
			occlusion_tex[i] = RD::get_singleton()->texture_create_shared(tv, occlusion_data[i]);
		}
	}

	for (HDDAGI::Cascade &cascade : cascades) {
		/* 3D Textures */

		cascade.light_process_buffer = RD::get_singleton()->storage_buffer_create(sizeof(HDDAGI::Cascade::LightProcessCell) * solid_cell_count);
		cascade.light_process_dispatch_buffer = RD::get_singleton()->storage_buffer_create(sizeof(uint32_t) * 4, Vector<uint8_t>(), RD::STORAGE_BUFFER_USAGE_DISPATCH_INDIRECT);
		cascade.light_process_dispatch_buffer_copy = RD::get_singleton()->storage_buffer_create(sizeof(uint32_t) * 4, Vector<uint8_t>(), RD::STORAGE_BUFFER_USAGE_DISPATCH_INDIRECT);

		cascade.light_position_bufer = RD::get_singleton()->storage_buffer_create(sizeof(HDDAGIShader::Light) * MAX(HDDAGI::MAX_STATIC_LIGHTS, HDDAGI::MAX_DYNAMIC_LIGHTS));

		cascade.cell_size = base_cell_size;
		Vector3 world_position = p_world_position;
		world_position.y *= y_mult;
		Vector3i probe_cells = cascade_size / REGION_CELLS;
		Vector3 probe_size = Vector3(1, 1, 1) * cascade.cell_size * Vector3(probe_cells);
		Vector3i probe_pos = Vector3i((world_position / probe_size + Vector3(0.5, 0.5, 0.5)).floor());
		cascade.position = probe_pos * probe_cells;

		cascade.dirty_regions = HDDAGI::Cascade::DIRTY_ALL;

		// lightprobes

		base_cell_size *= 2.0;
	}

	bounce_feedback = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_bounce_feedback(p_env);
	energy = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_energy(p_env);
	normal_bias = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_normal_bias(p_env);
	reflection_bias = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_reflection_bias(p_env);
	probe_bias = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_probe_bias(p_env);
	occlusion_bias = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_occlusion_bias(p_env);
	reads_sky = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_read_sky_light(p_env);
}

void GI::HDDAGI::render_region(Ref<RenderSceneBuffersRD> p_render_buffers, int p_region, const PagedArray<RenderGeometryInstance *> &p_instances, float p_exposure_normalization) {
	//print_line("rendering region " + itos(p_region));
	ERR_FAIL_COND(p_render_buffers.is_null()); // we wouldn't be here if this failed but...
	AABB bounds;
	Vector3i from;
	Vector3i size;
	Vector3i scroll;
	Vector3i region_ofs;

	int cascade = get_pending_region_data(p_region, from, size, bounds, scroll, region_ofs);
	ERR_FAIL_COND(cascade < 0);

	//initialize render
	//@TODO: Add a clear region to RenderingDevice to optimize this part
	RD::get_singleton()->texture_clear(render_albedo, Color(0, 0, 0, 0), 0, 1, 0, 1);
	RD::get_singleton()->texture_clear(render_emission, Color(0, 0, 0, 0), 0, 1, 0, 1);
	RD::get_singleton()->texture_clear(render_emission_aniso, Color(0, 0, 0, 0), 0, 1, 0, 1);
	RD::get_singleton()->texture_clear(render_aniso_normals, Color(0, 0, 0, 0), 0, 1, 0, 1);
	RD::get_singleton()->buffer_clear(light_process_dispatch_buffer_render, 0, sizeof(uint32_t) * 4);

	if (scroll == HDDAGI::Cascade::DIRTY_ALL) {
		RD::get_singleton()->buffer_clear(light_process_buffer_render, 0, sizeof(HDDAGI::Cascade::LightProcessCell) * solid_cell_count);

		RD::get_singleton()->texture_clear(lightprobe_hit_cache_data, Color(0, 0, 0, 0), 0, 1, cascade * frames_to_converge, frames_to_converge);
		RD::get_singleton()->texture_clear(lightprobe_hit_cache_version_data, Color(0, 0, 0, 0), 0, 1, cascade * frames_to_converge, frames_to_converge);
		RD::get_singleton()->texture_clear(lightprobe_moving_average_history, Color(0, 0, 0, 0), 0, 1, cascade * frames_to_converge, frames_to_converge);
		RD::get_singleton()->texture_clear(lightprobe_moving_average, Color(0, 0, 0, 0), 0, 1, cascade, 1);
		RD::get_singleton()->texture_clear(lightprobe_specular_data, Color(0, 0, 0, 0), 0, 1, cascade, 1);
		RD::get_singleton()->texture_clear(lightprobe_diffuse_data, Color(0, 0, 0, 0), 0, 1, cascade, 1);
		RD::get_singleton()->texture_clear(lightprobe_process_frame, Color(0, 0, 0, 0), 0, 1, cascade, 1);
	}

	//print_line("rendering cascade " + itos(p_region) + " objects: " + itos(p_cull_count) + " bounds: " + bounds + " from: " + from + " size: " + size + " cell size: " + rtos(cascades[cascade].cell_size));

	RendererSceneRenderRD::get_singleton()->_render_hddagi(p_render_buffers, from, size, bounds, p_instances, render_albedo, render_emission, render_emission_aniso, render_aniso_normals, p_exposure_normalization);

	RD::get_singleton()->draw_command_begin_label("HDDAGI Create Cascade SDF");

	RENDER_TIMESTAMP("> HDDAGI Update SDF");
	//done rendering! must update SDF
	//clear dispatch indirect data

	HDDAGIShader::PreprocessPushConstant push_constant;
	memset(&push_constant, 0, sizeof(HDDAGIShader::PreprocessPushConstant));

	Vector3i dispatch_size;
	//scroll
	if (scroll != HDDAGI::Cascade::DIRTY_ALL) {
		//for scroll
		push_constant.scroll[0] = scroll.x;
		push_constant.scroll[1] = scroll.y;
		push_constant.scroll[2] = scroll.z;

		for (int i = 0; i < 3; i++) {
			if (scroll[i] > 0) {
				push_constant.offset[i] = 0;
				push_constant.limit[i] = scroll[i];
				dispatch_size[i] = scroll[i];
			} else if (scroll[i] < 0) {
				push_constant.offset[i] = cascade_size[i] + scroll[i];
				push_constant.limit[i] = cascade_size[i];
				dispatch_size[i] = -scroll[i];
			} else {
				push_constant.offset[i] = 0;
				push_constant.limit[i] = cascade_size[i];
				dispatch_size[i] = cascade_size[i];
			}
		}

	} else {
		//for no scroll
		push_constant.scroll[0] = 0;
		push_constant.scroll[1] = 0;
		push_constant.scroll[2] = 0;

		push_constant.offset[0] = 0;
		push_constant.offset[1] = 0;
		push_constant.offset[2] = 0;

		push_constant.limit[0] = cascade_size[0];
		push_constant.limit[1] = cascade_size[1];
		push_constant.limit[2] = cascade_size[2];

		dispatch_size.x = cascade_size[0];
		dispatch_size.y = cascade_size[1];
		dispatch_size.z = cascade_size[2];
	}
	push_constant.grid_size[0] = cascade_size[0];
	push_constant.grid_size[1] = cascade_size[1];
	push_constant.grid_size[2] = cascade_size[2];

	Vector3i probe_axis_count = cascade_size / REGION_CELLS + Vector3i(1, 1, 1);

	push_constant.probe_axis_size[0] = probe_axis_count[0];
	push_constant.probe_axis_size[1] = probe_axis_count[1];
	push_constant.probe_axis_size[2] = probe_axis_count[2];

	push_constant.region_world_pos[0] = region_ofs[0];
	push_constant.region_world_pos[1] = region_ofs[1];
	push_constant.region_world_pos[2] = region_ofs[2];

	if (cascade < int(cascades.size() - 1) && scroll != HDDAGI::Cascade::DIRTY_ALL) {
		Vector3 upper_position = cascades[cascade + 1].position;

		// Get proper upper cascade position (before scroll)
		for (int k = 0; k < 3; k++) {
			if (cascades[cascade + 1].dirty_regions[k] != 0) {
				upper_position[k] -= cascades[cascade + 1].dirty_regions[k];
			}
		}

		push_constant.upper_region_world_pos[0] = upper_position[0] / REGION_CELLS;
		push_constant.upper_region_world_pos[1] = upper_position[1] / REGION_CELLS;
		push_constant.upper_region_world_pos[2] = upper_position[2] / REGION_CELLS;

	} else {
		push_constant.upper_region_world_pos[0] = 0;
		push_constant.upper_region_world_pos[1] = 0;
		push_constant.upper_region_world_pos[2] = 0;
	}

	push_constant.maximum_light_cells = solid_cell_count;
	push_constant.cascade_count = cascades.size();

	push_constant.ray_hit_cache_frames = frames_to_converge;

	static const uint32_t frames_to_update_table[RS::ENV_HDDAGI_INACTIVE_PROBE_MAX] = {
		1, 2, 4, 8
	};

	push_constant.probe_update_frames = frames_to_update_table[gi->inactive_probe_frames];

	push_constant.cascade = cascade;
	push_constant.occlusion_offset = 0;

	cascades[cascade].latest_version++;

	push_constant.region_version = cascades[cascade].latest_version;

	//full size jumpflood
	RENDER_TIMESTAMP("HDDAGI Scroll");

	RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();

	if (scroll != HDDAGI::Cascade::DIRTY_ALL) {
		// Scroll happened

		{ // Light Scroll
			RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->hddagi_shader.preprocess_pipeline[HDDAGIShader::PRE_PROCESS_LIGHT_SCROLL]);
			RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
					gi->hddagi_shader.preprocess_shader_version[HDDAGIShader::PRE_PROCESS_LIGHT_SCROLL],
					0,
					RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 1, voxel_light_tex_data),
					RD::Uniform(RD::UNIFORM_TYPE_STORAGE_BUFFER, 5, light_process_dispatch_buffer_render),
					RD::Uniform(RD::UNIFORM_TYPE_STORAGE_BUFFER, 6, light_process_buffer_render),
					RD::Uniform(RD::UNIFORM_TYPE_STORAGE_BUFFER, 7, cascades[cascade].light_process_dispatch_buffer_copy),
					RD::Uniform(RD::UNIFORM_TYPE_STORAGE_BUFFER, 8, cascades[cascade].light_process_buffer)

			);

			HDDAGIShader::PreprocessPushConstant push_constant_scroll = push_constant;

			for (int i = 0; i < 3; i++) {
				if (scroll[i] > 0) {
					push_constant_scroll.limit[i] = cascade_size[i] - scroll[i];
					push_constant_scroll.offset[i] = 1; //+1 because one extra is rendered below for consistency with neighbouring voxels.
				} else if (scroll[i] < 0) {
					push_constant_scroll.limit[i] = cascade_size[i] - 1; // -1 because one extra is rendered below for consistency with neighbouring voxels.
					push_constant_scroll.offset[i] = -scroll[i];
				} else {
					push_constant_scroll.limit[i] = cascade_size[i];
					push_constant_scroll.offset[i] = 0;
				}
			}

			RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);
			RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant_scroll, sizeof(HDDAGIShader::PreprocessPushConstant));
			RD::get_singleton()->compute_list_dispatch_indirect(compute_list, cascades[cascade].light_process_dispatch_buffer, 0);
		}

		{ // Probe Scroll
			RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->hddagi_shader.preprocess_pipeline[HDDAGIShader::PRE_PROCESS_LIGHTPROBE_SCROLL]);

			RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
					gi->hddagi_shader.preprocess_shader_version[HDDAGIShader::PRE_PROCESS_LIGHTPROBE_SCROLL],
					0,
					RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 1, lightprobe_specular_data),
					RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 2, lightprobe_diffuse_data),
					RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 3, lightprobe_ambient_tex),
					RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 4, lightprobe_hit_cache_data),
					RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 5, lightprobe_moving_average_history),
					RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 6, lightprobe_moving_average),
					RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 7, get_lightprobe_occlusion_textures()),
					RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 8, RendererRD::MaterialStorage::get_singleton()->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)));

			RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);

			Vector3i dispatch_cells = dispatch_size / REGION_CELLS + Vector3i(1, 1, 1);

			HDDAGIShader::PreprocessPushConstant push_constant_scroll = push_constant;

			for (int i = 0; i < 3; i++) {
				if (scroll[i] < 0) {
					push_constant_scroll.offset[i] += REGION_CELLS;
					dispatch_cells[i]--;
				} else if (scroll[i] > 0) {
					dispatch_cells[i]--;
				}
			}

			RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant_scroll, sizeof(HDDAGIShader::PreprocessPushConstant));
			RD::get_singleton()->compute_list_dispatch(compute_list, dispatch_cells.x, dispatch_cells.y, dispatch_cells.z);
		}

		RD::get_singleton()->compute_list_add_barrier(compute_list);
	}

	{ // Occlusion
		RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->hddagi_shader.preprocess_pipeline[HDDAGIShader::PRE_PROCESS_OCCLUSION]);

		for (int i = 0; i < 2; i++) {
			RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
					gi->hddagi_shader.preprocess_shader_version[HDDAGIShader::PRE_PROCESS_OCCLUSION],
					0,
					RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 3, render_aniso_normals),
					RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 4, occlusion_data[i]));

			push_constant.occlusion_offset = (i - 1) * 4; // Turns out Z needs to be swapped. I have no idea why. If you figure it out, let me know.
			RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);
			RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(HDDAGIShader::PreprocessPushConstant));
			RD::get_singleton()->compute_list_dispatch(compute_list, dispatch_size.x / REGION_CELLS, dispatch_size.y / REGION_CELLS, dispatch_size.z / REGION_CELLS);
		}
	}

	{ // Region Store
		RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->hddagi_shader.preprocess_pipeline[HDDAGIShader::PRE_PROCESS_REGION_STORE]);

		RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
				gi->hddagi_shader.preprocess_shader_version[HDDAGIShader::PRE_PROCESS_REGION_STORE],
				0,
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 1, render_aniso_normals),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 2, voxel_bits_tex),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 3, voxel_region_tex),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 4, region_version_data));

		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);
		RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(HDDAGIShader::PreprocessPushConstant));
		RD::get_singleton()->compute_list_dispatch_threads(compute_list, dispatch_size.x, dispatch_size.y, dispatch_size.z);
	}

	RD::get_singleton()->compute_list_add_barrier(compute_list); // store needs another barrier

	RENDER_TIMESTAMP("HDDAGI Store SDF");

	{
		// Storing light happens last (after barrier) because it needs occlusion information.
		RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->hddagi_shader.preprocess_pipeline[HDDAGIShader::PRE_PROCESS_LIGHT_STORE]);
		RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
				gi->hddagi_shader.preprocess_shader_version[HDDAGIShader::PRE_PROCESS_LIGHT_STORE],
				0,
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 1, render_albedo),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 2, render_emission),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 3, render_emission_aniso),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 4, render_aniso_normals),
				RD::Uniform(RD::UNIFORM_TYPE_STORAGE_BUFFER, 5, light_process_dispatch_buffer_render),
				RD::Uniform(RD::UNIFORM_TYPE_STORAGE_BUFFER, 6, light_process_buffer_render),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 7, lightprobe_neighbour_visibility_map),
				RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 8, get_lightprobe_occlusion_textures()),
				RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 9, RendererRD::MaterialStorage::get_singleton()->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 10, voxel_disocclusion_tex),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 11, voxel_light_neighbour_data),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 12, voxel_light_tex_data));

		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);

		Vector3i store_size = dispatch_size;

		if (scroll != HDDAGI::Cascade::DIRTY_ALL) {
			for (int i = 0; i < 3; i++) {
				if (scroll[i] > 0) {
					push_constant.offset[i] = 0;
					push_constant.limit[i] = scroll[i] + 1; //extra voxel to properly store light
					store_size[i] += 1;
				} else if (scroll[i] < 0) {
					push_constant.offset[i] = cascade_size[i] + scroll[i] - 1;
					push_constant.limit[i] = cascade_size[i];
					store_size[i] += 1;
				} else {
					push_constant.offset[i] = 0;
					push_constant.limit[i] = cascade_size[i];
				}
			}
		}

		RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(HDDAGIShader::PreprocessPushConstant));
		RD::get_singleton()->compute_list_dispatch_threads(compute_list, store_size.x, store_size.y, store_size.z);
		// Store processed ones into cascade
		SWAP(light_process_dispatch_buffer_render, cascades[cascade].light_process_dispatch_buffer);
		SWAP(light_process_buffer_render, cascades[cascade].light_process_buffer);

		cascades[cascade].static_lights_dirty = true;
		cascades[cascade].dynamic_lights_dirty = true;
	}

	{ // Probe Neighbours (no barrier needed)
		RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->hddagi_shader.preprocess_pipeline[HDDAGIShader::PRE_PROCESS_LIGHTPROBE_NEIGHBOURS]);

		RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
				gi->hddagi_shader.preprocess_shader_version[HDDAGIShader::PRE_PROCESS_LIGHTPROBE_NEIGHBOURS],
				0,
				RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 1, get_lightprobe_occlusion_textures()),
				RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 2, RendererRD::MaterialStorage::get_singleton()->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 3, lightprobe_neighbour_visibility_map)

		);

		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);

		Vector3i dispatch_cells = dispatch_size / REGION_CELLS + Vector3i(1, 1, 1);

		RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(HDDAGIShader::PreprocessPushConstant));
		RD::get_singleton()->compute_list_dispatch_threads(compute_list, dispatch_cells.x, dispatch_cells.y, dispatch_cells.z);
	}

	{ // Probe geometry proximity (no barrier needed)
		RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->hddagi_shader.preprocess_pipeline[HDDAGIShader::PRE_PROCESS_LIGHTPROBE_GEOMETRY_PROXIMITY]);

		RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
				gi->hddagi_shader.preprocess_shader_version[HDDAGIShader::PRE_PROCESS_LIGHTPROBE_GEOMETRY_PROXIMITY],
				0,
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 1, voxel_region_tex),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 2, lightprobe_geometry_proximity_map));

		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);

		Vector3i dispatch_cells = cascade_size / REGION_CELLS + Vector3i(1, 1, 1);

		RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(HDDAGIShader::PreprocessPushConstant));
		RD::get_singleton()->compute_list_dispatch_threads(compute_list, dispatch_cells.x, dispatch_cells.y, dispatch_cells.z);
	}

	{ // Probe frames
		RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->hddagi_shader.preprocess_pipeline[HDDAGIShader::PRE_PROCESS_LIGHTPROBE_UPDATE_FRAMES]);

		RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
				gi->hddagi_shader.preprocess_shader_version[HDDAGIShader::PRE_PROCESS_LIGHTPROBE_UPDATE_FRAMES],
				0,
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 1, lightprobe_process_frame));

		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);

		Vector3i dispatch_cells = cascade_size / REGION_CELLS + Vector3i(1, 1, 1);

		HDDAGIShader::PreprocessPushConstant push_constant_scroll = push_constant;

		if (scroll != HDDAGI::Cascade::DIRTY_ALL) {
			// Only edge if not all dirty
			for (int i = 0; i < 3; i++) {
				if (scroll[i] < 0) {
					push_constant_scroll.offset[i] += REGION_CELLS;
					dispatch_cells[i]--;
				} else if (scroll[i] > 0) {
					dispatch_cells[i]--;
				}
			}
		}

		RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant_scroll, sizeof(HDDAGIShader::PreprocessPushConstant));
		RD::get_singleton()->compute_list_dispatch(compute_list, dispatch_cells.x, dispatch_cells.y, dispatch_cells.z);
	}

	RD::get_singleton()->compute_list_end();

	RD::get_singleton()->buffer_copy(cascades[cascade].light_process_dispatch_buffer, cascades[cascade].light_process_dispatch_buffer_copy, 0, 0, sizeof(uint32_t) * 4);

	cascades[cascade].baked_exposure_normalization = p_exposure_normalization;

	RD::get_singleton()->draw_command_end_label();
	RENDER_TIMESTAMP("< HDDAGI Update SDF");
}

void GI::HDDAGI::free_data() {
	// we don't free things here, we handle HDDAGI differently at the moment destructing the object when it needs to change.
}

GI::HDDAGI::~HDDAGI() {
	for (const HDDAGI::Cascade &c : cascades) {
		RD::get_singleton()->free(c.light_process_buffer);
		RD::get_singleton()->free(c.light_process_dispatch_buffer);
		RD::get_singleton()->free(c.light_process_dispatch_buffer_copy);
	}

	RD::get_singleton()->free(render_albedo);
	RD::get_singleton()->free(render_aniso_normals);
	RD::get_singleton()->free(render_emission);
	RD::get_singleton()->free(render_emission_aniso);

	RD::get_singleton()->free(voxel_bits_tex);
	RD::get_singleton()->free(voxel_region_tex);
	RD::get_singleton()->free(voxel_light_tex_data);
	RD::get_singleton()->free(voxel_light_neighbour_data);
	RD::get_singleton()->free(region_version_data);

	RD::get_singleton()->free(light_process_buffer_render);
	RD::get_singleton()->free(light_process_dispatch_buffer_render);

	RD::get_singleton()->free(cascades_ubo);

	RD::get_singleton()->free(lightprobe_specular_data);
	RD::get_singleton()->free(lightprobe_diffuse_data);
	RD::get_singleton()->free(lightprobe_ambient_tex);
	RD::get_singleton()->free(lightprobe_diffuse_filter_data);
	RD::get_singleton()->free(lightprobe_hit_cache_data);
	RD::get_singleton()->free(lightprobe_hit_cache_version_data);

	RD::get_singleton()->free(lightprobe_moving_average);
	RD::get_singleton()->free(lightprobe_moving_average_history);

	RD::get_singleton()->free(lightprobe_neighbour_visibility_map);
	RD::get_singleton()->free(lightprobe_geometry_proximity_map);
	RD::get_singleton()->free(lightprobe_camera_visibility_map);
	for (int i = 0; i < lightprobe_camera_buffers.size(); i++) {
		RD::get_singleton()->free(lightprobe_camera_buffers[i]);
	}
	lightprobe_camera_buffers.clear();
	RD::get_singleton()->free(lightprobe_process_frame);

	RD::get_singleton()->free(occlusion_data[0]);
	RD::get_singleton()->free(occlusion_data[1]);

	if (debug_probes_scene_data_ubo.is_valid()) {
		RD::get_singleton()->free(debug_probes_scene_data_ubo);
		debug_probes_scene_data_ubo = RID();
	}
}

void GI::HDDAGI::update(RID p_env, const Vector3 &p_world_position) {
	bounce_feedback = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_bounce_feedback(p_env);
	energy = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_energy(p_env);
	reads_sky = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_read_sky_light(p_env);
	using_probe_filter = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_filter_probes(p_env);
	using_reflection_filter = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_filter_reflection(p_env);
	using_ambient_filter = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_filter_ambient(p_env);
	reflection_bias = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_reflection_bias(p_env);
	normal_bias = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_normal_bias(p_env);
	probe_bias = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_probe_bias(p_env);
	occlusion_bias = RendererSceneRenderRD::get_singleton()->environment_get_hddagi_occlusion_bias(p_env);

	int32_t drag_margin = REGION_CELLS / 2;

	int idx = 0;
	for (HDDAGI::Cascade &cascade : cascades) {
		cascade.dirty_regions = Vector3i();

		Vector3 probe_half_size = Vector3(1, 1, 1) * cascade.cell_size * float(REGION_CELLS) * 0.5;
		probe_half_size = Vector3(0, 0, 0);

		Vector3 world_position = p_world_position;
		world_position.y *= y_mult;
		Vector3i pos_in_cascade = Vector3i((world_position + probe_half_size) / cascade.cell_size);

		for (int j = 0; j < 3; j++) {
			if (pos_in_cascade[j] < cascade.position[j]) {
				while (pos_in_cascade[j] < (cascade.position[j] - drag_margin)) {
					cascade.position[j] -= drag_margin * 2;
					cascade.dirty_regions[j] += drag_margin * 2;
				}
			} else if (pos_in_cascade[j] > cascade.position[j]) {
				while (pos_in_cascade[j] > (cascade.position[j] + drag_margin)) {
					cascade.position[j] += drag_margin * 2;
					cascade.dirty_regions[j] -= drag_margin * 2;
				}
			}

			if (cascade.dirty_regions[j] == 0) {
				continue; // not dirty
			} else if (uint32_t(Math::abs(cascade.dirty_regions[j])) >= uint32_t(cascade_size[j])) {
				//moved too much, just redraw everything (make all dirty)
				cascade.dirty_regions = HDDAGI::Cascade::DIRTY_ALL;
				break;
			}
		}
#ifdef DIRTY_ALL_FRAMES
		// DEBUG
		cascade.dirty_regions = HDDAGI::Cascade::DIRTY_ALL;
		break;
#endif
		if (cascade.dirty_regions != Vector3i() && cascade.dirty_regions != HDDAGI::Cascade::DIRTY_ALL) {
			//see how much the total dirty volume represents from the total volume
			uint32_t total_volume = cascade_size.x * cascade_size.y * cascade_size.z;
			uint32_t safe_volume = 1;
			for (int j = 0; j < 3; j++) {
				safe_volume *= cascade_size[j] - Math::abs(cascade.dirty_regions[j]);
			}
			uint32_t dirty_volume = total_volume - safe_volume;
			if (dirty_volume > (safe_volume / 2)) {
				//more than half the volume is dirty, make all dirty so its only rendered once
				cascade.dirty_regions = HDDAGI::Cascade::DIRTY_ALL;
			}
		}

		if (cascade.dirty_regions != Vector3i()) {
			uint32_t dirty_mask = 0;
			for (int j = 0; j < 3; j++) {
				if (cascade.dirty_regions[j] != 0) {
					dirty_mask |= (1 << j);
				}
			}
			// Notify all cascades equal or smaller than this that some motion happened.
			// In an axis, which is used for light ray cache invalidation.

			for (int j = idx; j >= 0; j--) {
				cascades[j].motion_accum |= dirty_mask;
			}
		}

		idx++;
	}

	update_frame++;
}

void GI::HDDAGI::update_light() {
	RD::get_singleton()->draw_command_begin_label("HDDAGI Update dynamic Light");

	/* Update dynamic light */

	RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();
	RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->hddagi_shader.direct_light_pipeline[HDDAGIShader::DIRECT_LIGHT_MODE_DYNAMIC]);

	HDDAGIShader::DirectLightPushConstant push_constant;

	push_constant.grid_size[0] = cascade_size[0];
	push_constant.grid_size[1] = cascade_size[1];
	push_constant.grid_size[2] = cascade_size[2];
	push_constant.max_cascades = cascades.size();
	push_constant.probe_axis_size[0] = cascade_size[0] / REGION_CELLS + 1;
	push_constant.probe_axis_size[1] = cascade_size[1] / REGION_CELLS + 1;
	push_constant.probe_axis_size[2] = cascade_size[2] / REGION_CELLS + 1;

	push_constant.bounce_feedback = bounce_feedback;
	push_constant.y_mult = y_mult;
	push_constant.use_occlusion = uses_occlusion;
	push_constant.probe_cell_size = REGION_CELLS;

	for (uint32_t i = 0; i < cascades.size(); i++) {
		HDDAGI::Cascade &cascade = cascades[i];
		push_constant.light_count = cascade_dynamic_light_count[i];
		push_constant.cascade = i;
		push_constant.dirty_dynamic_update = cascades[i].dynamic_lights_dirty;

		cascades[i].dynamic_lights_dirty = false;

		if (gi->hddagi_frames_to_update_light == RS::ENV_HDDAGI_UPDATE_LIGHT_IN_1_FRAME) {
			push_constant.process_offset = 0;
			push_constant.process_increment = 1;
		} else {
			static const uint32_t frames_to_update_table[RS::ENV_HDDAGI_UPDATE_LIGHT_MAX] = {
				1, 2, 4, 8, 16
			};

			uint32_t frames_to_update = frames_to_update_table[gi->hddagi_frames_to_update_light];

			push_constant.process_offset = RSG::rasterizer->get_frame_number() % frames_to_update;
			push_constant.process_increment = frames_to_update;
		}

		RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
				gi->hddagi_shader.direct_light_shader_version[HDDAGIShader::DIRECT_LIGHT_MODE_DYNAMIC],
				0,
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 1, voxel_bits_tex),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 2, voxel_region_tex),
				RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 3, RendererRD::MaterialStorage::get_singleton()->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)),
				RD::Uniform(RD::UNIFORM_TYPE_STORAGE_BUFFER, 4, cascade.light_process_dispatch_buffer_copy),
				RD::Uniform(RD::UNIFORM_TYPE_STORAGE_BUFFER, 5, cascade.light_process_buffer),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 6, voxel_light_tex_data),
				RD::Uniform(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 7, cascades_ubo),
				RD::Uniform(RD::UNIFORM_TYPE_STORAGE_BUFFER, 8, cascade.light_position_bufer),
				RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 9, lightprobe_diffuse_tex));

		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);

		RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(HDDAGIShader::DirectLightPushConstant));
		RD::get_singleton()->compute_list_dispatch_indirect(compute_list, cascade.light_process_dispatch_buffer, 0);
	}

	RD::get_singleton()->compute_list_end();
	RD::get_singleton()->draw_command_end_label();
}

void GI::HDDAGI::update_probes(RID p_env, SkyRD::Sky *p_sky, uint32_t p_view_count, const Projection *p_projections, const Vector3 *p_eye_offsets, const Transform3D &p_cam_transform) {
	RD::get_singleton()->draw_command_begin_label("HDDAGI Update Probes");

	HDDAGIShader::IntegratePushConstant push_constant;
	push_constant.grid_size[0] = cascade_size[0];
	push_constant.grid_size[1] = cascade_size[1];
	push_constant.grid_size[2] = cascade_size[2];
	push_constant.max_cascades = cascades.size();
	push_constant.probe_axis_size[0] = cascade_size[0] / REGION_CELLS + 1;
	push_constant.probe_axis_size[1] = cascade_size[1] / REGION_CELLS + 1;
	push_constant.probe_axis_size[2] = cascade_size[2] / REGION_CELLS + 1;

	static const uint32_t frames_to_update_table[RS::ENV_HDDAGI_INACTIVE_PROBE_MAX] = {
		1, 2, 4, 8
	};
	push_constant.inactive_update_frames = frames_to_update_table[gi->inactive_probe_frames];
	push_constant.global_frame = RSG::rasterizer->get_frame_number();
	push_constant.history_size = frames_to_converge;
	push_constant.ray_bias = probe_bias;
	push_constant.store_ambient_texture = RendererSceneRenderRD::get_singleton()->environment_get_volumetric_fog_enabled(p_env);
	push_constant.sky_mode = 0;
	push_constant.y_mult = y_mult;

	RID integrate_sky_uniform_set;
	int32_t probe_divisor = REGION_CELLS;

	if (reads_sky && p_env.is_valid()) {
		push_constant.sky_energy = RendererSceneRenderRD::get_singleton()->environment_get_bg_energy_multiplier(p_env);

		if (RendererSceneRenderRD::get_singleton()->environment_get_background(p_env) == RS::ENV_BG_CLEAR_COLOR) {
			push_constant.sky_mode |= HDDAGIShader::IntegratePushConstant::SKY_FLAGS_MODE_COLOR;
			Color c = RSG::texture_storage->get_default_clear_color().srgb_to_linear();
			push_constant.sky_color[0] = c.r;
			push_constant.sky_color[1] = c.g;
			push_constant.sky_color[2] = c.b;
		} else if (RendererSceneRenderRD::get_singleton()->environment_get_background(p_env) == RS::ENV_BG_COLOR) {
			push_constant.sky_mode |= HDDAGIShader::IntegratePushConstant::SKY_FLAGS_MODE_COLOR;
			Color c = RendererSceneRenderRD::get_singleton()->environment_get_bg_color(p_env);
			push_constant.sky_color[0] = c.r;
			push_constant.sky_color[1] = c.g;
			push_constant.sky_color[2] = c.b;

		} else if (RendererSceneRenderRD::get_singleton()->environment_get_background(p_env) == RS::ENV_BG_SKY) {
			if (p_sky && p_sky->radiance.is_valid()) {
				integrate_sky_uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
						gi->hddagi_shader.integrate.version_get_shader(gi->hddagi_shader.integrate_shader, 0),
						1,
						RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 0, p_sky->radiance),
						RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 1, RendererRD::MaterialStorage::get_singleton()->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)));
				push_constant.sky_mode |= HDDAGIShader::IntegratePushConstant::SKY_FLAGS_MODE_SKY;
				// Encode sky orientation as quaternion in existing push constants.
				const Basis sky_basis = RendererSceneRenderRD::get_singleton()->environment_get_sky_orientation(p_env);
				const Quaternion sky_quaternion = sky_basis.get_quaternion().inverse();
				push_constant.sky_color[0] = sky_quaternion.x;
				push_constant.sky_color[1] = sky_quaternion.y;
				push_constant.sky_color[2] = sky_quaternion.z;
				// Ideally we would reconstruct the largest component for least error, but sky contribution to GI is low frequency so just needs to get the idea across.
				push_constant.sky_mode |= HDDAGIShader::IntegratePushConstant::SKY_FLAGS_ORIENTATION_SIGN * (sky_quaternion.w < 0.0 ? 0 : 1);
			}
		}
	}

	{
		RD::get_singleton()->texture_clear(lightprobe_camera_visibility_map, Color(0, 0, 0, 0), 0, 1, 0, cascades.size());

		// Setup buffers first (must be done outside compute list).
		for (uint32_t i = 0; i < cascades.size(); i++) {
			Vector3 cascade_pos = Vector3((Vector3i(1, 1, 1) * -(cascade_size >> 1) + cascades[i].position)) * cascades[i].cell_size;
			float cascade_to_cell = 1.0 / cascades[i].cell_size;
			Transform3D local_xform = p_cam_transform;
			local_xform.origin -= cascade_pos;
			local_xform.scale(Vector3(1, y_mult, 1) * cascade_to_cell);

			for (uint32_t j = 0; j < p_view_count; j++) {
				Vector<Plane> planes = p_projections[j].get_projection_planes(local_xform);
				HDDAGIShader::IntegrateCameraUBO camera_ubo;
				for (int k = 0; k < planes.size(); k++) {
					Plane plane = planes[k];
					camera_ubo.planes[k * 4 + 0] = plane.normal.x;
					camera_ubo.planes[k * 4 + 1] = plane.normal.y;
					camera_ubo.planes[k * 4 + 2] = plane.normal.z;
					camera_ubo.planes[k * 4 + 3] = plane.d;
				}
				Vector3 endpoints[8];
				p_projections[j].get_endpoints(local_xform, endpoints);
				for (int k = 0; k < 8; k++) {
					Vector3 p = endpoints[k];
					camera_ubo.points[k * 4 + 0] = p.x;
					camera_ubo.points[k * 4 + 1] = p.y;
					camera_ubo.points[k * 4 + 2] = p.z;
					camera_ubo.points[k * 4 + 3] = 1;
				}
				int buffer_index = j * cascades.size() + i;
				RD::get_singleton()->buffer_update(lightprobe_camera_buffers[buffer_index], 0, sizeof(HDDAGIShader::IntegrateCameraUBO), &camera_ubo);
			}
		}

		// Do visibility testing (all cascades and views in parallel).

		RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();
		RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->hddagi_shader.integrate_pipeline[HDDAGIShader::INTEGRATE_MODE_CAMERA_VISIBILITY]);

		for (uint32_t i = 0; i < cascades.size(); i++) {
			push_constant.cascade = i;
			push_constant.world_offset[0] = cascades[i].position.x / probe_divisor;
			push_constant.world_offset[1] = cascades[i].position.y / probe_divisor;
			push_constant.world_offset[2] = cascades[i].position.z / probe_divisor;

			for (uint32_t j = 0; j < p_view_count; j++) {
				int buffer_index = j * cascades.size() + i;

				RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
						gi->hddagi_shader.integrate_shader_version[HDDAGIShader::INTEGRATE_MODE_CAMERA_VISIBILITY],
						0,
						RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 1, lightprobe_camera_visibility_map),
						RD::Uniform(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 2, lightprobe_camera_buffers[buffer_index]));

				RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);

				RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(HDDAGIShader::IntegratePushConstant));
				Vector3i dispatch_threads = cascade_size / REGION_CELLS;
				RD::get_singleton()->compute_list_dispatch_threads(compute_list, dispatch_threads.x, dispatch_threads.y, dispatch_threads.z);
			}
		}
		RD::get_singleton()->compute_list_end();
	}

	if (integrate_sky_uniform_set.is_null()) {
		integrate_sky_uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
				gi->hddagi_shader.integrate.version_get_shader(gi->hddagi_shader.integrate_shader, 0),
				1,
				RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 0, RendererRD::TextureStorage::get_singleton()->texture_rd_get_default(RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_CUBEMAP_WHITE)),
				RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 1, RendererRD::MaterialStorage::get_singleton()->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)));
	}

	render_pass++;

	RID integrate_unifom_set = UniformSetCacheRD::get_singleton()->get_cache(
			gi->hddagi_shader.integrate.version_get_shader(gi->hddagi_shader.integrate_shader, 0),
			0,
			RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 1, voxel_bits_tex),
			RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 2, voxel_region_tex),
			RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 3, voxel_light_tex),
			RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 4, RendererRD::MaterialStorage::get_singleton()->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)),
			RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 5, lightprobe_specular_data),
			RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 6, lightprobe_diffuse_data),
			RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 7, lightprobe_ambient_tex),
			RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 8, lightprobe_hit_cache_data),
			RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 9, lightprobe_hit_cache_version_data),
			RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 10, region_version_data),
			RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 11, lightprobe_moving_average_history),
			RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 12, lightprobe_moving_average),
			RD::Uniform(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 13, cascades_ubo),
			RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 14, lightprobe_process_frame),
			RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 15, lightprobe_geometry_proximity_map),
			RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 16, lightprobe_camera_visibility_map));

	RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();
	RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->hddagi_shader.integrate_pipeline[HDDAGIShader::INTEGRATE_MODE_PROCESS]);

	Vector3i probe_axis_count = cascade_size / REGION_CELLS + Vector3i(1, 1, 1);

	for (uint32_t i = 0; i < cascades.size(); i++) {
		push_constant.cascade = i;
		push_constant.motion_accum = cascades[i].motion_accum;
		cascades[i].motion_accum = 0; //clear after use.

		push_constant.world_offset[0] = cascades[i].position.x / probe_divisor;
		push_constant.world_offset[1] = cascades[i].position.y / probe_divisor;
		push_constant.world_offset[2] = cascades[i].position.z / probe_divisor;

		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, integrate_unifom_set, 0);
		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, integrate_sky_uniform_set, 1);

		RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(HDDAGIShader::IntegratePushConstant));
		RD::get_singleton()->compute_list_dispatch_threads(compute_list, probe_axis_count.x * LIGHTPROBE_OCT_SIZE, probe_axis_count.y * probe_axis_count.z * LIGHTPROBE_OCT_SIZE, 1);
	}

	if (using_probe_filter) {
		RD::get_singleton()->compute_list_add_barrier(compute_list);

		RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->hddagi_shader.integrate_pipeline[HDDAGIShader::INTEGRATE_MODE_FILTER]);

		integrate_unifom_set = UniformSetCacheRD::get_singleton()->get_cache(
				gi->hddagi_shader.integrate_shader_version[HDDAGIShader::INTEGRATE_MODE_FILTER],
				0,
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 1, lightprobe_diffuse_data),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 2, lightprobe_diffuse_filter_data),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 3, lightprobe_neighbour_visibility_map),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 4, lightprobe_geometry_proximity_map),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 5, lightprobe_camera_visibility_map));

		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, integrate_unifom_set, 0);

		for (uint32_t i = 0; i < cascades.size(); i++) {
			push_constant.cascade = i;
			RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(HDDAGIShader::IntegratePushConstant));

			push_constant.world_offset[0] = cascades[i].position.x / probe_divisor;
			push_constant.world_offset[1] = cascades[i].position.y / probe_divisor;
			push_constant.world_offset[2] = cascades[i].position.z / probe_divisor;

			RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(HDDAGIShader::IntegratePushConstant));
			RD::get_singleton()->compute_list_dispatch_threads(compute_list, probe_axis_count.x * LIGHTPROBE_OCT_SIZE, probe_axis_count.y * probe_axis_count.z * LIGHTPROBE_OCT_SIZE, 1);
		}
	}

	RD::get_singleton()->compute_list_end();

	RD::get_singleton()->draw_command_end_label();
}

void GI::HDDAGI::store_probes() {
}

int GI::HDDAGI::get_pending_region_count() const {
	int dirty_count = 0;
	for (const RendererRD::GI::HDDAGI::Cascade &c : cascades) {
		if (c.dirty_regions == RendererRD::GI::HDDAGI::Cascade::DIRTY_ALL) {
			dirty_count++;
		} else {
			for (int j = 0; j < 3; j++) {
				if (c.dirty_regions[j] != 0) {
					dirty_count++;
				}
			}
		}
	}

	return dirty_count;
}

int GI::HDDAGI::get_pending_region_data(int p_region, Vector3i &r_local_offset, Vector3i &r_local_size, AABB &r_bounds, Vector3i &r_scroll, Vector3i &r_region_world) const {
	// higher cascades need to be processed first
	int dirty_count = 0;
	for (uint32_t i = 0; i < cascades.size(); i++) {
		const HDDAGI::Cascade &c = cascades[i];

		if (c.dirty_regions == HDDAGI::Cascade::DIRTY_ALL) {
			if (dirty_count == p_region) {
				r_local_offset = Vector3i();
				r_local_size = cascade_size;
				r_scroll = HDDAGI::Cascade::DIRTY_ALL;
				r_region_world = c.position / REGION_CELLS;

				r_bounds.position = Vector3((Vector3i(1, 1, 1) * -(cascade_size >> 1) + c.position)) * c.cell_size * Vector3(1, 1.0 / y_mult, 1);
				r_bounds.size = Vector3(r_local_size) * c.cell_size * Vector3(1, 1.0 / y_mult, 1);
				return i;
			}
			dirty_count++;
		} else {
			for (int j = 0; j < 3; j++) {
				if (c.dirty_regions[j] != 0) {
					if (dirty_count == p_region) {
						Vector3i from = Vector3i(0, 0, 0);
						Vector3i to = cascade_size;

						r_scroll = Vector3i();
						r_scroll[j] = c.dirty_regions[j];

						if (c.dirty_regions[j] > 0) {
							//fill from the beginning
							to[j] = c.dirty_regions[j] + 2; // 2 extra voxels needed to rebuild light properly
						} else {
							//fill from the end
							from[j] = to[j] + c.dirty_regions[j] - 2; // 2 extra voxels needed to rebuild light properly
						}

						r_local_offset = from;
						r_local_size = to - from;

						Vector3i cascade_position = c.position;

						// Remove next axes positions so we don't voxelize the wrong region
						for (int k = j + 1; k < 3; k++) {
							if (c.dirty_regions[k] != 0) {
								cascade_position[k] += c.dirty_regions[k];
							}
						}

						r_region_world = cascade_position / REGION_CELLS;

						r_bounds.position = Vector3(from + Vector3i(1, 1, 1) * -Vector3(cascade_size >> 1) + cascade_position) * c.cell_size * Vector3(1, 1.0 / y_mult, 1);
						r_bounds.size = Vector3(r_local_size) * c.cell_size * Vector3(1, 1.0 / y_mult, 1);

						return i;
					}

					dirty_count++;
				}
			}
		}
	}
	return -1;
}

void GI::HDDAGI::update_cascades() {
	//update cascades
	HDDAGI::Cascade::UBO cascade_data[HDDAGI::MAX_CASCADES];

	for (uint32_t i = 0; i < cascades.size(); i++) {
		Vector3 pos = Vector3((Vector3i(1, 1, 1) * -(cascade_size >> 1) + cascades[i].position)) * cascades[i].cell_size;

		cascade_data[i].offset[0] = pos.x;
		cascade_data[i].offset[1] = pos.y;
		cascade_data[i].offset[2] = pos.z;
		cascade_data[i].to_cell = 1.0 / cascades[i].cell_size;
		cascade_data[i].region_world_offset[0] = cascades[i].position.x / REGION_CELLS;
		cascade_data[i].region_world_offset[1] = cascades[i].position.y / REGION_CELLS;
		cascade_data[i].region_world_offset[2] = cascades[i].position.z / REGION_CELLS;
		cascade_data[i].pad = 0;
	}

	RD::get_singleton()->buffer_update(cascades_ubo, 0, sizeof(HDDAGI::Cascade::UBO) * HDDAGI::MAX_CASCADES, cascade_data);
}

void GI::HDDAGI::debug_draw(uint32_t p_view_count, const Projection *p_projections, const Transform3D &p_transform, int p_width, int p_height, RID p_render_target, RID p_texture, const Vector<RID> &p_texture_views) {
	RendererRD::TextureStorage *texture_storage = RendererRD::TextureStorage::get_singleton();
	RendererRD::MaterialStorage *material_storage = RendererRD::MaterialStorage::get_singleton();
	RendererRD::CopyEffects *copy_effects = RendererRD::CopyEffects::get_singleton();

	for (uint32_t v = 0; v < p_view_count; v++) {
		RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
				gi->hddagi_shader.debug_shader_version,
				0,
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 1, voxel_bits_tex),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 2, voxel_region_tex),
				RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 3, voxel_light_tex),
				RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 4, material_storage->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)),
				RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 5, material_storage->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_NEAREST, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)),
				RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 6, lightprobe_diffuse_tex),
				RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 7, get_lightprobe_occlusion_textures()),
				RD::Uniform(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 8, cascades_ubo),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 9, p_texture_views[v]),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 10, voxel_light_neighbour_data));

		RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();
		RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->hddagi_shader.debug_pipeline);
		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);

		HDDAGIShader::DebugPushConstant push_constant;
		push_constant.grid_size[0] = cascade_size[0];
		push_constant.grid_size[1] = cascade_size[1];
		push_constant.grid_size[2] = cascade_size[2];
		push_constant.max_cascades = cascades.size();
		push_constant.screen_size = p_width;
		push_constant.screen_size |= p_height << 16;
		push_constant.esm_strength = 1.0;
		push_constant.y_mult = y_mult;

		push_constant.z_near = -p_projections[v].get_z_near();

		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				push_constant.cam_basis[i][j] = p_transform.basis.rows[j][i];
			}
		}

		push_constant.cam_origin[0] = p_transform.origin[0];
		push_constant.cam_origin[1] = p_transform.origin[1];
		push_constant.cam_origin[2] = p_transform.origin[2];

		// need to properly unproject for asymmetric projection matrices in stereo..
		Projection inv_projection = p_projections[v].inverse();
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 3; j++) {
				push_constant.inv_projection[j][i] = inv_projection.columns[i][j];
			}
		}

		RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(HDDAGIShader::DebugPushConstant));

		RD::get_singleton()->compute_list_dispatch_threads(compute_list, p_width, p_height, 1);
		RD::get_singleton()->compute_list_end();
	}

	Size2i rtsize = texture_storage->render_target_get_size(p_render_target);
	copy_effects->copy_to_fb_rect(p_texture, texture_storage->render_target_get_rd_framebuffer(p_render_target), Rect2i(Point2i(), rtsize), true, false, false, false, RID(), p_view_count > 1);
}

void GI::HDDAGI::debug_probes(RID p_framebuffer, const uint32_t p_view_count, const Projection *p_camera_with_transforms) {
	// setup scene data
	{
		HDDAGIShader::DebugProbesSceneData scene_data;

		if (debug_probes_scene_data_ubo.is_null()) {
			debug_probes_scene_data_ubo = RD::get_singleton()->uniform_buffer_create(sizeof(HDDAGIShader::DebugProbesSceneData));
		}

		for (uint32_t v = 0; v < p_view_count; v++) {
			RendererRD::MaterialStorage::store_camera(p_camera_with_transforms[v], scene_data.projection[v]);
		}

		RD::get_singleton()->buffer_update(debug_probes_scene_data_ubo, 0, sizeof(HDDAGIShader::DebugProbesSceneData), &scene_data);
	}

	// setup push constant
	HDDAGIShader::DebugProbesPushConstant push_constant;

	//gen spheres from strips
	uint32_t band_points = 16;
	push_constant.band_power = 4;
	push_constant.sections_in_band = ((band_points / 2) - 1);
	push_constant.band_mask = band_points - 2;
	push_constant.section_arc = Math::TAU / float(push_constant.sections_in_band);
	push_constant.y_mult = y_mult;
	push_constant.oct_size = LIGHTPROBE_OCT_SIZE;

	Vector3i probe_axis_count = cascade_size / REGION_CELLS + Vector3i(1, 1, 1);
	uint32_t total_points = push_constant.sections_in_band * band_points;
	uint32_t total_probes = probe_axis_count.x * probe_axis_count.y * probe_axis_count.z;

	push_constant.grid_size[0] = cascade_size[0];
	push_constant.grid_size[1] = cascade_size[1];
	push_constant.grid_size[2] = cascade_size[2];
	push_constant.cascade = 0;

	push_constant.probe_axis_size[0] = probe_axis_count[0];
	push_constant.probe_axis_size[1] = probe_axis_count[1];
	push_constant.probe_axis_size[2] = probe_axis_count[2];

	HDDAGIShader::ProbeDebugMode mode = p_view_count > 1 ? HDDAGIShader::PROBE_DEBUG_PROBES_MULTIVIEW : HDDAGIShader::PROBE_DEBUG_PROBES;

	RID debug_probes_uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
			gi->hddagi_shader.debug_probes_shader_version[mode],
			0,
			RD::Uniform(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 1, cascades_ubo),
			RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 2, get_lightprobe_occlusion_textures()),
			RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 3, RendererRD::MaterialStorage::get_singleton()->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)),
			RD::Uniform(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 4, debug_probes_scene_data_ubo),
			RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 5, lightprobe_diffuse_tex)

	);

	RD::DrawListID draw_list = RD::get_singleton()->draw_list_begin(p_framebuffer);
	RD::get_singleton()->draw_command_begin_label("Debug HDDAGI");

	RD::get_singleton()->draw_list_bind_render_pipeline(draw_list, gi->hddagi_shader.debug_probes_pipeline[mode].get_render_pipeline(RD::INVALID_FORMAT_ID, RD::get_singleton()->framebuffer_get_format(p_framebuffer)));
	RD::get_singleton()->draw_list_bind_uniform_set(draw_list, debug_probes_uniform_set, 0);
	RD::get_singleton()->draw_list_set_push_constant(draw_list, &push_constant, sizeof(HDDAGIShader::DebugProbesPushConstant));
	RD::get_singleton()->draw_list_draw(draw_list, false, total_probes, total_points);

	if (gi->hddagi_debug_probe_dir != Vector3()) {
		uint32_t cascade = 0;
		Vector3 offset = Vector3((Vector3i(1, 1, 1) * -(cascade_size >> 1) + cascades[cascade].position)) * cascades[cascade].cell_size * Vector3(1.0, 1.0 / y_mult, 1.0);
		Vector3 probe_size = cascades[cascade].cell_size * REGION_CELLS * Vector3(1.0, 1.0 / y_mult, 1.0);
		Vector3 ray_from = gi->hddagi_debug_probe_pos;
		Vector3 ray_to = gi->hddagi_debug_probe_pos + gi->hddagi_debug_probe_dir * cascades[cascade].cell_size * Math::sqrt(3.0) * cascade_size[0];
		float sphere_radius = 0.2;
		float closest_dist = 1e20;
		gi->hddagi_debug_probe_enabled = false;

		Vector3i probe_from = cascades[cascade].position / REGION_CELLS;
		for (int i = 0; i < (cascade_size[0] / REGION_CELLS + 1); i++) {
			for (int j = 0; j < (cascade_size[1] / REGION_CELLS + 1); j++) {
				for (int k = 0; k < (cascade_size[2] / REGION_CELLS + 1); k++) {
					Vector3 pos = offset + probe_size * Vector3(i, j, k);
					Vector3 res;
					if (Geometry3D::segment_intersects_sphere(ray_from, ray_to, pos, sphere_radius, &res)) {
						float d = ray_from.distance_to(res);
						if (d < closest_dist) {
							closest_dist = d;
							gi->hddagi_debug_probe_enabled = true;
							gi->hddagi_debug_probe_index = probe_from + Vector3i(i, j, k);
						}
					}
				}
			}
		}

		print_line("Pos: ", gi->hddagi_debug_probe_pos);
		print_line("Dir: ", gi->hddagi_debug_probe_dir);
		print_line("Select: ", gi->hddagi_debug_probe_index);

#if 0

		Vector3i offset3 = (gi->hddagi_debug_probe_index & probe_axis_count);
		Vector2i offset2(offset3.x, offset3.y + probe_axis_count.y * offset3.z);
		offset2 *= (OCCLUSION_OCT_SIZE + 2);

		Vector<uint8_t> data = RD::get_singleton()->texture_get_data(occlusion_process, 0);
		Ref<Image> image = Image::create_from_data((OCCLUSION_OCT_SIZE + 2) * probe_axis_count.x, ((OCCLUSION_OCT_SIZE + 2) * probe_axis_count.y * probe_axis_count.z), false, Image::FORMAT_R8, data);
		image->crop_from_point(offset2.x, offset2.y, OCCLUSION_OCT_SIZE + 2, OCCLUSION_OCT_SIZE + 2);
		image->save_png("occlusion_probe.png");

#endif

		gi->hddagi_debug_probe_dir = Vector3();
	}

	if (gi->hddagi_debug_probe_enabled) {
		uint32_t cascade = 0;
		Vector3i probe_cells = (cascade_size / HDDAGI::REGION_CELLS);
		Vector3i probe_from = cascades[cascade].position / REGION_CELLS;
		Vector3i ofs = gi->hddagi_debug_probe_index - probe_from;

		bool probe_valid = true;
		if (ofs.x < 0 || ofs.y < 0 || ofs.z < 0) {
			probe_valid = false;
		}
		if (ofs.x > probe_cells.x || ofs.y > probe_cells.y || ofs.z > probe_cells.z) {
			probe_valid = false;
		}

		if (probe_valid) {
			Vector3i mult = probe_cells + Vector3i(1, 1, 1);
			uint32_t index = ofs.z * mult.x * mult.y + ofs.y * mult.x + ofs.x;

			push_constant.probe_debug_index = index;

			uint32_t cell_count = HDDAGI::REGION_CELLS * 2 * HDDAGI::REGION_CELLS * 2 * HDDAGI::REGION_CELLS * 2;

			debug_probes_uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
					gi->hddagi_shader.debug_probes_shader_version[p_view_count > 1 ? HDDAGIShader::PROBE_DEBUG_OCCLUSION_MULTIVIEW : HDDAGIShader::PROBE_DEBUG_OCCLUSION],
					0,
					RD::Uniform(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 1, cascades_ubo),
					RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 2, get_lightprobe_occlusion_textures()),
					RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 3, RendererRD::MaterialStorage::get_singleton()->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)),
					RD::Uniform(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 4, debug_probes_scene_data_ubo),
					RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 5, lightprobe_diffuse_tex)

			);
			RD::get_singleton()->draw_list_bind_render_pipeline(draw_list, gi->hddagi_shader.debug_probes_pipeline[p_view_count > 1 ? HDDAGIShader::PROBE_DEBUG_OCCLUSION_MULTIVIEW : HDDAGIShader::PROBE_DEBUG_OCCLUSION].get_render_pipeline(RD::INVALID_FORMAT_ID, RD::get_singleton()->framebuffer_get_format(p_framebuffer)));
			RD::get_singleton()->draw_list_bind_uniform_set(draw_list, debug_probes_uniform_set, 0);
			RD::get_singleton()->draw_list_set_push_constant(draw_list, &push_constant, sizeof(HDDAGIShader::DebugProbesPushConstant));
			RD::get_singleton()->draw_list_draw(draw_list, false, cell_count, total_points);
		}
	}

	RD::get_singleton()->draw_list_end();
	RD::get_singleton()->draw_command_end_label();
}

void GI::HDDAGI::pre_process_gi(const Transform3D &p_transform, RenderDataRD *p_render_data) {
	if (p_render_data->hddagi_update_data == nullptr) {
		return;
	}

	RendererRD::LightStorage *light_storage = RendererRD::LightStorage::get_singleton();
	/* Update general HDDAGI Buffer */

	HDDAGIData hddagi_data;

	hddagi_data.grid_size[0] = cascade_size[0];
	hddagi_data.grid_size[1] = cascade_size[1];
	hddagi_data.grid_size[2] = cascade_size[2];

	hddagi_data.max_cascades = cascades.size();

	hddagi_data.probe_axis_size[0] = cascade_size[0] / REGION_CELLS + 1;
	hddagi_data.probe_axis_size[1] = cascade_size[1] / REGION_CELLS + 1;
	hddagi_data.probe_axis_size[2] = cascade_size[2] / REGION_CELLS + 1;

	hddagi_data.y_mult = y_mult;
	hddagi_data.normal_bias = normal_bias;
	hddagi_data.reflection_bias = reflection_bias;
	hddagi_data.esm_strength = 1.0;

	hddagi_data.energy = energy;

	for (int32_t i = 0; i < hddagi_data.max_cascades; i++) {
		HDDAGIData::ProbeCascadeData &c = hddagi_data.cascades[i];
		Vector3 pos = Vector3((Vector3i(1, 1, 1) * -(cascade_size >> 1) + cascades[i].position)) * cascades[i].cell_size;
		Vector3 cam_origin = p_transform.origin;
		cam_origin.y *= y_mult;
		pos -= cam_origin; //make pos local to camera, to reduce numerical error
		c.position[0] = pos.x;
		c.position[1] = pos.y;
		c.position[2] = pos.z;
		c.to_probe = 1.0 / (float(cascade_size[0]) * cascades[i].cell_size / float(hddagi_data.probe_axis_size[0] - 1));

		c.region_world_offset[0] = cascades[i].position.x / REGION_CELLS;
		c.region_world_offset[1] = cascades[i].position.y / REGION_CELLS;
		c.region_world_offset[2] = cascades[i].position.z / REGION_CELLS;

		c.to_cell = 1.0 / cascades[i].cell_size;
		c.exposure_normalization = 1.0;
		if (p_render_data->camera_attributes.is_valid()) {
			float exposure_normalization = RSG::camera_attributes->camera_attributes_get_exposure_normalization_factor(p_render_data->camera_attributes);
			c.exposure_normalization = exposure_normalization / cascades[i].baked_exposure_normalization;
		}
	}

	RD::get_singleton()->buffer_update(gi->hddagi_ubo, 0, sizeof(HDDAGIData), &hddagi_data);

	/* Update dynamic lights in HDDAGI cascades */

	for (uint32_t i = 0; i < cascades.size(); i++) {
		HDDAGI::Cascade &cascade = cascades[i];

		HDDAGIShader::Light lights[HDDAGI::MAX_DYNAMIC_LIGHTS];
		uint32_t idx = 0;
		for (uint32_t j = 0; j < (uint32_t)p_render_data->hddagi_update_data->directional_lights->size(); j++) {
			if (idx == HDDAGI::MAX_DYNAMIC_LIGHTS) {
				break;
			}

			RID light_instance = p_render_data->hddagi_update_data->directional_lights->get(j);
			ERR_CONTINUE(!light_storage->owns_light_instance(light_instance));

			RID light = light_storage->light_instance_get_base_light(light_instance);
			Transform3D light_transform = light_storage->light_instance_get_base_transform(light_instance);

			if (RSG::light_storage->light_directional_get_sky_mode(light) == RS::LIGHT_DIRECTIONAL_SKY_MODE_SKY_ONLY) {
				continue;
			}

			Vector3 dir = -light_transform.basis.get_column(Vector3::AXIS_Z);
			dir.y *= y_mult;
			dir.normalize();
			lights[idx].direction[0] = dir.x;
			lights[idx].direction[1] = dir.y;
			lights[idx].direction[2] = dir.z;
			Color color = RSG::light_storage->light_get_color(light);
			color = color.srgb_to_linear();
			lights[idx].color[0] = color.r;
			lights[idx].color[1] = color.g;
			lights[idx].color[2] = color.b;
			lights[idx].type = RS::LIGHT_DIRECTIONAL;
			lights[idx].energy = RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_ENERGY) * RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_INDIRECT_ENERGY);
			if (RendererSceneRenderRD::get_singleton()->is_using_physical_light_units()) {
				lights[idx].energy *= RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_INTENSITY);
			}

			if (p_render_data->camera_attributes.is_valid()) {
				lights[idx].energy *= RSG::camera_attributes->camera_attributes_get_exposure_normalization_factor(p_render_data->camera_attributes);
			}

			lights[idx].has_shadow = RSG::light_storage->light_has_shadow(light);

			idx++;
		}

		AABB cascade_aabb;
		cascade_aabb.position = Vector3((Vector3i(1, 1, 1) * -(cascade_size >> 1) + cascade.position)) * cascade.cell_size;
		cascade_aabb.size = Vector3(1, 1, 1) * cascade_size * cascade.cell_size;

		for (uint32_t j = 0; j < p_render_data->hddagi_update_data->positional_light_count; j++) {
			if (idx == HDDAGI::MAX_DYNAMIC_LIGHTS) {
				break;
			}

			RID light_instance = p_render_data->hddagi_update_data->positional_light_instances[j];
			ERR_CONTINUE(!light_storage->owns_light_instance(light_instance));

			RID light = light_storage->light_instance_get_base_light(light_instance);
			AABB light_aabb = light_storage->light_instance_get_base_aabb(light_instance);
			Transform3D light_transform = light_storage->light_instance_get_base_transform(light_instance);

			uint32_t max_hddagi_cascade = RSG::light_storage->light_get_max_hddagi_cascade(light);
			if (i > max_hddagi_cascade) {
				continue;
			}

			if (!cascade_aabb.intersects(light_aabb)) {
				continue;
			}

			Vector3 dir = -light_transform.basis.get_column(Vector3::AXIS_Z);
			//faster to not do this here
			//dir.y *= y_mult;
			//dir.normalize();
			lights[idx].direction[0] = dir.x;
			lights[idx].direction[1] = dir.y;
			lights[idx].direction[2] = dir.z;
			Vector3 pos = light_transform.origin;
			pos.y *= y_mult;
			lights[idx].position[0] = pos.x;
			lights[idx].position[1] = pos.y;
			lights[idx].position[2] = pos.z;
			Color color = RSG::light_storage->light_get_color(light);
			color = color.srgb_to_linear();
			lights[idx].color[0] = color.r;
			lights[idx].color[1] = color.g;
			lights[idx].color[2] = color.b;
			lights[idx].type = RSG::light_storage->light_get_type(light);

			lights[idx].energy = RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_ENERGY) * RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_INDIRECT_ENERGY);
			if (RendererSceneRenderRD::get_singleton()->is_using_physical_light_units()) {
				lights[idx].energy *= RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_INTENSITY);

				// Convert from Luminous Power to Luminous Intensity
				if (lights[idx].type == RS::LIGHT_OMNI) {
					lights[idx].energy *= 1.0 / (Math::PI * 4.0);
				} else if (lights[idx].type == RS::LIGHT_SPOT) {
					// Spot Lights are not physically accurate, Luminous Intensity should change in relation to the cone angle.
					// We make this assumption to keep them easy to control.
					lights[idx].energy *= 1.0 / Math::PI;
				}
			}

			if (p_render_data->camera_attributes.is_valid()) {
				lights[idx].energy *= RSG::camera_attributes->camera_attributes_get_exposure_normalization_factor(p_render_data->camera_attributes);
			}

			lights[idx].has_shadow = RSG::light_storage->light_has_shadow(light);
			lights[idx].attenuation = RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_ATTENUATION);
			lights[idx].radius = RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_RANGE);
			lights[idx].cos_spot_angle = Math::cos(Math::deg_to_rad(RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_SPOT_ANGLE)));
			lights[idx].inv_spot_attenuation = 1.0f / RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_SPOT_ATTENUATION);

			idx++;
		}

		if (idx > 0) {
			RD::get_singleton()->buffer_update(cascade.light_position_bufer, 0, idx * sizeof(HDDAGIShader::Light), lights);
		}

		cascade_dynamic_light_count[i] = idx;
	}
}

void GI::HDDAGI::render_static_lights(RenderDataRD *p_render_data, Ref<RenderSceneBuffersRD> p_render_buffers, uint32_t p_cascade_count, const uint32_t *p_cascade_indices, const PagedArray<RID> *p_positional_light_cull_result) {
	ERR_FAIL_COND(p_render_buffers.is_null()); // we wouldn't be here if this failed but...

	RendererRD::LightStorage *light_storage = RendererRD::LightStorage::get_singleton();

	RD::get_singleton()->draw_command_begin_label("HDDAGI Render Static Lights");

	update_cascades();

	HDDAGIShader::Light lights[HDDAGI::MAX_STATIC_LIGHTS];
	uint32_t light_count[HDDAGI::MAX_STATIC_LIGHTS];

	for (uint32_t i = 0; i < p_cascade_count; i++) {
		ERR_CONTINUE(p_cascade_indices[i] >= cascades.size());

		HDDAGI::Cascade &cc = cascades[p_cascade_indices[i]];

		{ //fill light buffer

			AABB cascade_aabb;
			cascade_aabb.position = Vector3((Vector3i(1, 1, 1) * -(cascade_size >> 1) + cc.position)) * cc.cell_size;
			cascade_aabb.size = Vector3(1, 1, 1) * cascade_size * cc.cell_size;

			int idx = 0;

			for (uint32_t j = 0; j < (uint32_t)p_positional_light_cull_result[i].size(); j++) {
				if (idx == HDDAGI::MAX_STATIC_LIGHTS) {
					break;
				}

				RID light_instance = p_positional_light_cull_result[i][j];
				ERR_CONTINUE(!light_storage->owns_light_instance(light_instance));

				RID light = light_storage->light_instance_get_base_light(light_instance);
				AABB light_aabb = light_storage->light_instance_get_base_aabb(light_instance);
				Transform3D light_transform = light_storage->light_instance_get_base_transform(light_instance);

				uint32_t max_hddagi_cascade = RSG::light_storage->light_get_max_hddagi_cascade(light);
				if (p_cascade_indices[i] > max_hddagi_cascade) {
					continue;
				}

				if (!cascade_aabb.intersects(light_aabb)) {
					continue;
				}

				lights[idx].type = RSG::light_storage->light_get_type(light);

				Vector3 dir = -light_transform.basis.get_column(Vector3::AXIS_Z);
				if (lights[idx].type == RS::LIGHT_DIRECTIONAL) {
					dir.y *= y_mult; //only makes sense for directional
					dir.normalize();
				}
				lights[idx].direction[0] = dir.x;
				lights[idx].direction[1] = dir.y;
				lights[idx].direction[2] = dir.z;
				Vector3 pos = light_transform.origin;
				pos.y *= y_mult;
				lights[idx].position[0] = pos.x;
				lights[idx].position[1] = pos.y;
				lights[idx].position[2] = pos.z;
				Color color = RSG::light_storage->light_get_color(light);
				color = color.srgb_to_linear();
				lights[idx].color[0] = color.r;
				lights[idx].color[1] = color.g;
				lights[idx].color[2] = color.b;

				lights[idx].energy = RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_ENERGY) * RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_INDIRECT_ENERGY);
				if (RendererSceneRenderRD::get_singleton()->is_using_physical_light_units()) {
					lights[idx].energy *= RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_INTENSITY);

					// Convert from Luminous Power to Luminous Intensity
					if (lights[idx].type == RS::LIGHT_OMNI) {
						lights[idx].energy *= 1.0 / (Math::PI * 4.0);
					} else if (lights[idx].type == RS::LIGHT_SPOT) {
						// Spot Lights are not physically accurate, Luminous Intensity should change in relation to the cone angle.
						// We make this assumption to keep them easy to control.
						lights[idx].energy *= 1.0 / Math::PI;
					}
				}

				if (p_render_data->camera_attributes.is_valid()) {
					lights[idx].energy *= RSG::camera_attributes->camera_attributes_get_exposure_normalization_factor(p_render_data->camera_attributes);
				}

				lights[idx].has_shadow = RSG::light_storage->light_has_shadow(light);
				lights[idx].attenuation = RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_ATTENUATION);
				lights[idx].radius = RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_RANGE);
				lights[idx].cos_spot_angle = Math::cos(Math::deg_to_rad(RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_SPOT_ANGLE)));
				lights[idx].inv_spot_attenuation = 1.0f / RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_SPOT_ATTENUATION);

				idx++;
			}

			if (idx > 0) {
				RD::get_singleton()->buffer_update(cc.light_position_bufer, 0, idx * sizeof(HDDAGIShader::Light), lights);
			}

			light_count[i] = idx;
		}
	}

	/* Static Lights */
	RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();

	RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->hddagi_shader.direct_light_pipeline[HDDAGIShader::DIRECT_LIGHT_MODE_STATIC]);

	HDDAGIShader::DirectLightPushConstant dl_push_constant;

	Vector3i probe_axis_count = cascade_size / REGION_CELLS + Vector3i(1, 1, 1);

	dl_push_constant.grid_size[0] = cascade_size[0];
	dl_push_constant.grid_size[1] = cascade_size[1];
	dl_push_constant.grid_size[2] = cascade_size[2];
	dl_push_constant.max_cascades = cascades.size();
	dl_push_constant.probe_axis_size[0] = probe_axis_count[0];
	dl_push_constant.probe_axis_size[1] = probe_axis_count[1];
	dl_push_constant.probe_axis_size[2] = probe_axis_count[2];
	dl_push_constant.bounce_feedback = 0.0; // this is static light, do not multibounce yet
	dl_push_constant.y_mult = y_mult;
	dl_push_constant.use_occlusion = uses_occlusion;
	dl_push_constant.probe_cell_size = REGION_CELLS;

	//all must be processed
	dl_push_constant.process_offset = 0;
	dl_push_constant.process_increment = 1;

	for (uint32_t i = 0; i < p_cascade_count; i++) {
		ERR_CONTINUE(p_cascade_indices[i] >= cascades.size());

		HDDAGI::Cascade &cc = cascades[p_cascade_indices[i]];

		dl_push_constant.light_count = light_count[i];
		dl_push_constant.cascade = p_cascade_indices[i];

		if (dl_push_constant.light_count > 0) {
			RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
					gi->hddagi_shader.direct_light_shader_version[HDDAGIShader::DIRECT_LIGHT_MODE_STATIC],
					0,
					RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 1, voxel_bits_tex),
					RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 2, voxel_region_tex),
					RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 3, RendererRD::MaterialStorage::get_singleton()->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)),
					RD::Uniform(RD::UNIFORM_TYPE_STORAGE_BUFFER, 4, cc.light_process_dispatch_buffer_copy),
					RD::Uniform(RD::UNIFORM_TYPE_STORAGE_BUFFER, 5, cc.light_process_buffer),
					RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 6, voxel_light_tex_data),
					RD::Uniform(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 7, cascades_ubo),
					RD::Uniform(RD::UNIFORM_TYPE_STORAGE_BUFFER, 8, cc.light_position_bufer));

			RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);
			RD::get_singleton()->compute_list_set_push_constant(compute_list, &dl_push_constant, sizeof(HDDAGIShader::DirectLightPushConstant));
			RD::get_singleton()->compute_list_dispatch_indirect(compute_list, cc.light_process_dispatch_buffer, 0);
		}
	}

	RD::get_singleton()->compute_list_end();

	RD::get_singleton()->draw_command_end_label();
}

////////////////////////////////////////////////////////////////////////////////
// VoxelGIInstance

void GI::VoxelGIInstance::update(bool p_update_light_instances, const Vector<RID> &p_light_instances, const PagedArray<RenderGeometryInstance *> &p_dynamic_objects) {
	RendererRD::LightStorage *light_storage = RendererRD::LightStorage::get_singleton();
	RendererRD::MaterialStorage *material_storage = RendererRD::MaterialStorage::get_singleton();

	uint32_t data_version = gi->voxel_gi_get_data_version(probe);

	// (RE)CREATE IF NEEDED

	if (last_probe_data_version != data_version) {
		//need to re-create everything
		free_resources();

		Vector3i octree_size = gi->voxel_gi_get_octree_size(probe);

		if (octree_size != Vector3i()) {
			//can create a 3D texture
			Vector<int> levels = gi->voxel_gi_get_level_counts(probe);

			RD::TextureFormat tf;
			tf.format = RD::DATA_FORMAT_R8G8B8A8_UNORM;
			tf.width = octree_size.x;
			tf.height = octree_size.y;
			tf.depth = octree_size.z;
			tf.texture_type = RD::TEXTURE_TYPE_3D;
			tf.mipmaps = levels.size();

			tf.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_TO_BIT;

			texture = RD::get_singleton()->texture_create(tf, RD::TextureView());
			RD::get_singleton()->set_resource_name(texture, "VoxelGI Instance Texture");

			RD::get_singleton()->texture_clear(texture, Color(0, 0, 0, 0), 0, levels.size(), 0, 1);

			{
				int total_elements = 0;
				for (int i = 0; i < levels.size(); i++) {
					total_elements += levels[i];
				}

				write_buffer = RD::get_singleton()->storage_buffer_create(total_elements * 16);
			}

			for (int i = 0; i < levels.size(); i++) {
				VoxelGIInstance::Mipmap mipmap;
				mipmap.texture = RD::get_singleton()->texture_create_shared_from_slice(RD::TextureView(), texture, 0, i, 1, RD::TEXTURE_SLICE_3D);
				mipmap.level = levels.size() - i - 1;
				mipmap.cell_offset = 0;
				for (uint32_t j = 0; j < mipmap.level; j++) {
					mipmap.cell_offset += levels[j];
				}
				mipmap.cell_count = levels[mipmap.level];

				Vector<RD::Uniform> uniforms;
				{
					RD::Uniform u;
					u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
					u.binding = 1;
					u.append_id(gi->voxel_gi_get_octree_buffer(probe));
					uniforms.push_back(u);
				}
				{
					RD::Uniform u;
					u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
					u.binding = 2;
					u.append_id(gi->voxel_gi_get_data_buffer(probe));
					uniforms.push_back(u);
				}

				{
					RD::Uniform u;
					u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
					u.binding = 4;
					u.append_id(write_buffer);
					uniforms.push_back(u);
				}
				{
					RD::Uniform u;
					u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
					u.binding = 9;
					u.append_id(gi->voxel_gi_get_sdf_texture(probe));
					uniforms.push_back(u);
				}
				{
					RD::Uniform u;
					u.uniform_type = RD::UNIFORM_TYPE_SAMPLER;
					u.binding = 10;
					u.append_id(material_storage->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED));
					uniforms.push_back(u);
				}

				{
					Vector<RD::Uniform> copy_uniforms = uniforms;
					if (i == 0) {
						{
							RD::Uniform u;
							u.uniform_type = RD::UNIFORM_TYPE_UNIFORM_BUFFER;
							u.binding = 3;
							u.append_id(gi->voxel_gi_lights_uniform);
							copy_uniforms.push_back(u);
						}

						mipmap.uniform_set = RD::get_singleton()->uniform_set_create(copy_uniforms, gi->voxel_gi_lighting_shader_version_shaders[VOXEL_GI_SHADER_VERSION_COMPUTE_LIGHT], 0);

						copy_uniforms = uniforms; //restore

						{
							RD::Uniform u;
							u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
							u.binding = 5;
							u.append_id(texture);
							copy_uniforms.push_back(u);
						}
						mipmap.second_bounce_uniform_set = RD::get_singleton()->uniform_set_create(copy_uniforms, gi->voxel_gi_lighting_shader_version_shaders[VOXEL_GI_SHADER_VERSION_COMPUTE_SECOND_BOUNCE], 0);
					} else {
						mipmap.uniform_set = RD::get_singleton()->uniform_set_create(copy_uniforms, gi->voxel_gi_lighting_shader_version_shaders[VOXEL_GI_SHADER_VERSION_COMPUTE_MIPMAP], 0);
					}
				}

				{
					RD::Uniform u;
					u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
					u.binding = 5;
					u.append_id(mipmap.texture);
					uniforms.push_back(u);
				}

				mipmap.write_uniform_set = RD::get_singleton()->uniform_set_create(uniforms, gi->voxel_gi_lighting_shader_version_shaders[VOXEL_GI_SHADER_VERSION_WRITE_TEXTURE], 0);

				mipmaps.push_back(mipmap);
			}

			{
				uint32_t dynamic_map_size = MAX(MAX(octree_size.x, octree_size.y), octree_size.z);
				uint32_t oversample = nearest_power_of_2_templated(4);
				int mipmap_index = 0;

				while (mipmap_index < mipmaps.size()) {
					VoxelGIInstance::DynamicMap dmap;

					if (oversample > 0) {
						dmap.size = dynamic_map_size * (1 << oversample);
						dmap.mipmap = -1;
						oversample--;
					} else {
						dmap.size = dynamic_map_size >> mipmap_index;
						dmap.mipmap = mipmap_index;
						mipmap_index++;
					}

					RD::TextureFormat dtf;
					dtf.width = dmap.size;
					dtf.height = dmap.size;
					dtf.format = RD::DATA_FORMAT_R16G16B16A16_SFLOAT;
					dtf.usage_bits = RD::TEXTURE_USAGE_STORAGE_BIT;

					if (dynamic_maps.is_empty()) {
						dtf.usage_bits |= RD::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
					}
					dmap.texture = RD::get_singleton()->texture_create(dtf, RD::TextureView());
					RD::get_singleton()->set_resource_name(dmap.texture, "VoxelGI Instance DMap Texture");

					if (dynamic_maps.is_empty()) {
						// Render depth for first one.
						// Use 16-bit depth when supported to improve performance.
						dtf.format = RD::get_singleton()->texture_is_format_supported_for_usage(RD::DATA_FORMAT_D16_UNORM, RD::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ? RD::DATA_FORMAT_D16_UNORM : RD::DATA_FORMAT_X8_D24_UNORM_PACK32;
						dtf.usage_bits = RD::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
						dmap.fb_depth = RD::get_singleton()->texture_create(dtf, RD::TextureView());
						RD::get_singleton()->set_resource_name(dmap.fb_depth, "VoxelGI Instance DMap FB Depth");
					}

					//just use depth as-is
					dtf.format = RD::DATA_FORMAT_R32_SFLOAT;
					dtf.usage_bits = RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;

					dmap.depth = RD::get_singleton()->texture_create(dtf, RD::TextureView());
					RD::get_singleton()->set_resource_name(dmap.depth, "VoxelGI Instance DMap Depth");

					if (dynamic_maps.is_empty()) {
						dtf.format = RD::DATA_FORMAT_R8G8B8A8_UNORM;
						dtf.usage_bits = RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
						dmap.albedo = RD::get_singleton()->texture_create(dtf, RD::TextureView());
						RD::get_singleton()->set_resource_name(dmap.albedo, "VoxelGI Instance DMap Albedo");
						dmap.normal = RD::get_singleton()->texture_create(dtf, RD::TextureView());
						RD::get_singleton()->set_resource_name(dmap.normal, "VoxelGI Instance DMap Normal");
						dmap.orm = RD::get_singleton()->texture_create(dtf, RD::TextureView());
						RD::get_singleton()->set_resource_name(dmap.orm, "VoxelGI Instance DMap ORM");

						Vector<RID> fb;
						fb.push_back(dmap.albedo);
						fb.push_back(dmap.normal);
						fb.push_back(dmap.orm);
						fb.push_back(dmap.texture); //emission
						fb.push_back(dmap.depth);
						fb.push_back(dmap.fb_depth);

						dmap.fb = RD::get_singleton()->framebuffer_create(fb);

						{
							Vector<RD::Uniform> uniforms;
							{
								RD::Uniform u;
								u.uniform_type = RD::UNIFORM_TYPE_UNIFORM_BUFFER;
								u.binding = 3;
								u.append_id(gi->voxel_gi_lights_uniform);
								uniforms.push_back(u);
							}

							{
								RD::Uniform u;
								u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
								u.binding = 5;
								u.append_id(dmap.albedo);
								uniforms.push_back(u);
							}
							{
								RD::Uniform u;
								u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
								u.binding = 6;
								u.append_id(dmap.normal);
								uniforms.push_back(u);
							}
							{
								RD::Uniform u;
								u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
								u.binding = 7;
								u.append_id(dmap.orm);
								uniforms.push_back(u);
							}
							{
								RD::Uniform u;
								u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
								u.binding = 8;
								u.append_id(dmap.fb_depth);
								uniforms.push_back(u);
							}
							{
								RD::Uniform u;
								u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
								u.binding = 9;
								u.append_id(gi->voxel_gi_get_sdf_texture(probe));
								uniforms.push_back(u);
							}
							{
								RD::Uniform u;
								u.uniform_type = RD::UNIFORM_TYPE_SAMPLER;
								u.binding = 10;
								u.append_id(material_storage->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED));
								uniforms.push_back(u);
							}
							{
								RD::Uniform u;
								u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
								u.binding = 11;
								u.append_id(dmap.texture);
								uniforms.push_back(u);
							}
							{
								RD::Uniform u;
								u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
								u.binding = 12;
								u.append_id(dmap.depth);
								uniforms.push_back(u);
							}

							dmap.uniform_set = RD::get_singleton()->uniform_set_create(uniforms, gi->voxel_gi_lighting_shader_version_shaders[VOXEL_GI_SHADER_VERSION_DYNAMIC_OBJECT_LIGHTING], 0);
						}
					} else {
						bool plot = dmap.mipmap >= 0;
						bool write = dmap.mipmap < (mipmaps.size() - 1);

						Vector<RD::Uniform> uniforms;

						{
							RD::Uniform u;
							u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
							u.binding = 5;
							u.append_id(dynamic_maps[dynamic_maps.size() - 1].texture);
							uniforms.push_back(u);
						}
						{
							RD::Uniform u;
							u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
							u.binding = 6;
							u.append_id(dynamic_maps[dynamic_maps.size() - 1].depth);
							uniforms.push_back(u);
						}

						if (write) {
							{
								RD::Uniform u;
								u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
								u.binding = 7;
								u.append_id(dmap.texture);
								uniforms.push_back(u);
							}
							{
								RD::Uniform u;
								u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
								u.binding = 8;
								u.append_id(dmap.depth);
								uniforms.push_back(u);
							}
						}

						{
							RD::Uniform u;
							u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
							u.binding = 9;
							u.append_id(gi->voxel_gi_get_sdf_texture(probe));
							uniforms.push_back(u);
						}
						{
							RD::Uniform u;
							u.uniform_type = RD::UNIFORM_TYPE_SAMPLER;
							u.binding = 10;
							u.append_id(material_storage->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED));
							uniforms.push_back(u);
						}

						if (plot) {
							{
								RD::Uniform u;
								u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
								u.binding = 11;
								u.append_id(mipmaps[dmap.mipmap].texture);
								uniforms.push_back(u);
							}
						}

						dmap.uniform_set = RD::get_singleton()->uniform_set_create(
								uniforms,
								gi->voxel_gi_lighting_shader_version_shaders[(write && plot) ? VOXEL_GI_SHADER_VERSION_DYNAMIC_SHRINK_WRITE_PLOT : (write ? VOXEL_GI_SHADER_VERSION_DYNAMIC_SHRINK_WRITE : VOXEL_GI_SHADER_VERSION_DYNAMIC_SHRINK_PLOT)],
								0);
					}

					dynamic_maps.push_back(dmap);
				}
			}
		}

		last_probe_data_version = data_version;
		p_update_light_instances = true; //just in case

		RendererSceneRenderRD::get_singleton()->base_uniforms_changed();
	}

	// UDPDATE TIME

	if (has_dynamic_object_data) {
		//if it has dynamic object data, it needs to be cleared
		RD::get_singleton()->texture_clear(texture, Color(0, 0, 0, 0), 0, mipmaps.size(), 0, 1);
	}

	uint32_t light_count = 0;

	if (p_update_light_instances || p_dynamic_objects.size() > 0) {
		light_count = MIN(gi->voxel_gi_max_lights, (uint32_t)p_light_instances.size());

		{
			Transform3D to_cell = gi->voxel_gi_get_to_cell_xform(probe);
			Transform3D to_probe_xform = to_cell * transform.affine_inverse();

			//update lights

			for (uint32_t i = 0; i < light_count; i++) {
				VoxelGILight &l = gi->voxel_gi_lights[i];
				RID light_instance = p_light_instances[i];
				RID light = light_storage->light_instance_get_base_light(light_instance);

				l.type = RSG::light_storage->light_get_type(light);
				if (l.type == RS::LIGHT_DIRECTIONAL && RSG::light_storage->light_directional_get_sky_mode(light) == RS::LIGHT_DIRECTIONAL_SKY_MODE_SKY_ONLY) {
					light_count--;
					continue;
				}

				l.attenuation = RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_ATTENUATION);
				l.energy = RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_ENERGY) * RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_INDIRECT_ENERGY);

				if (RendererSceneRenderRD::get_singleton()->is_using_physical_light_units()) {
					l.energy *= RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_INTENSITY);

					l.energy *= gi->voxel_gi_get_baked_exposure_normalization(probe);

					// Convert from Luminous Power to Luminous Intensity
					if (l.type == RS::LIGHT_OMNI) {
						l.energy *= 1.0 / (Math::PI * 4.0);
					} else if (l.type == RS::LIGHT_SPOT) {
						// Spot Lights are not physically accurate, Luminous Intensity should change in relation to the cone angle.
						// We make this assumption to keep them easy to control.
						l.energy *= 1.0 / Math::PI;
					}
				}

				l.radius = to_cell.basis.xform(Vector3(RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_RANGE), 0, 0)).length();
				Color color = RSG::light_storage->light_get_color(light).srgb_to_linear();
				l.color[0] = color.r;
				l.color[1] = color.g;
				l.color[2] = color.b;

				l.cos_spot_angle = Math::cos(Math::deg_to_rad(RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_SPOT_ANGLE)));
				l.inv_spot_attenuation = 1.0f / RSG::light_storage->light_get_param(light, RS::LIGHT_PARAM_SPOT_ATTENUATION);

				Transform3D xform = light_storage->light_instance_get_base_transform(light_instance);

				Vector3 pos = to_probe_xform.xform(xform.origin);
				Vector3 dir = to_probe_xform.basis.xform(-xform.basis.get_column(2)).normalized();

				l.position[0] = pos.x;
				l.position[1] = pos.y;
				l.position[2] = pos.z;

				l.direction[0] = dir.x;
				l.direction[1] = dir.y;
				l.direction[2] = dir.z;

				l.has_shadow = RSG::light_storage->light_has_shadow(light);
			}

			RD::get_singleton()->buffer_update(gi->voxel_gi_lights_uniform, 0, sizeof(VoxelGILight) * light_count, gi->voxel_gi_lights);
		}
	}

	if (has_dynamic_object_data || p_update_light_instances || p_dynamic_objects.size()) {
		// PROCESS MIPMAPS
		if (mipmaps.size()) {
			//can update mipmaps

			Vector3i probe_size = gi->voxel_gi_get_octree_size(probe);

			VoxelGIPushConstant push_constant;

			push_constant.limits[0] = probe_size.x;
			push_constant.limits[1] = probe_size.y;
			push_constant.limits[2] = probe_size.z;
			push_constant.stack_size = mipmaps.size();
			push_constant.emission_scale = 1.0;
			push_constant.propagation = gi->voxel_gi_get_propagation(probe);
			push_constant.dynamic_range = gi->voxel_gi_get_dynamic_range(probe);
			push_constant.light_count = light_count;
			push_constant.aniso_strength = 0;

			/*		print_line("probe update to version " + itos(last_probe_version));
			print_line("propagation " + rtos(push_constant.propagation));
			print_line("dynrange " + rtos(push_constant.dynamic_range));
	*/
			RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();

			int passes;
			if (p_update_light_instances) {
				passes = gi->voxel_gi_is_using_two_bounces(probe) ? 2 : 1;
			} else {
				passes = 1; //only re-blitting is necessary
			}
			int wg_size = 64;
			int64_t wg_limit_x = (int64_t)RD::get_singleton()->limit_get(RD::LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_X);

			for (int pass = 0; pass < passes; pass++) {
				if (p_update_light_instances) {
					for (int i = 0; i < mipmaps.size(); i++) {
						if (i == 0) {
							RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->voxel_gi_lighting_shader_version_pipelines[pass == 0 ? VOXEL_GI_SHADER_VERSION_COMPUTE_LIGHT : VOXEL_GI_SHADER_VERSION_COMPUTE_SECOND_BOUNCE]);
						} else if (i == 1) {
							RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->voxel_gi_lighting_shader_version_pipelines[VOXEL_GI_SHADER_VERSION_COMPUTE_MIPMAP]);
						}

						if (pass == 1 || i > 0) {
							RD::get_singleton()->compute_list_add_barrier(compute_list); //wait til previous step is done
						}
						if (pass == 0 || i > 0) {
							RD::get_singleton()->compute_list_bind_uniform_set(compute_list, mipmaps[i].uniform_set, 0);
						} else {
							RD::get_singleton()->compute_list_bind_uniform_set(compute_list, mipmaps[i].second_bounce_uniform_set, 0);
						}

						push_constant.cell_offset = mipmaps[i].cell_offset;
						push_constant.cell_count = mipmaps[i].cell_count;

						int64_t wg_todo = (mipmaps[i].cell_count + wg_size - 1) / wg_size;
						while (wg_todo) {
							int64_t wg_count = MIN(wg_todo, wg_limit_x);
							RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(VoxelGIPushConstant));
							RD::get_singleton()->compute_list_dispatch(compute_list, wg_count, 1, 1);
							wg_todo -= wg_count;
							push_constant.cell_offset += wg_count * wg_size;
						}
					}

					RD::get_singleton()->compute_list_add_barrier(compute_list); //wait til previous step is done
				}

				RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->voxel_gi_lighting_shader_version_pipelines[VOXEL_GI_SHADER_VERSION_WRITE_TEXTURE]);

				for (int i = 0; i < mipmaps.size(); i++) {
					RD::get_singleton()->compute_list_bind_uniform_set(compute_list, mipmaps[i].write_uniform_set, 0);

					push_constant.cell_offset = mipmaps[i].cell_offset;
					push_constant.cell_count = mipmaps[i].cell_count;

					int64_t wg_todo = (mipmaps[i].cell_count + wg_size - 1) / wg_size;
					while (wg_todo) {
						int64_t wg_count = MIN(wg_todo, wg_limit_x);
						RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(VoxelGIPushConstant));
						RD::get_singleton()->compute_list_dispatch(compute_list, wg_count, 1, 1);
						wg_todo -= wg_count;
						push_constant.cell_offset += wg_count * wg_size;
					}
				}
			}

			RD::get_singleton()->compute_list_end();
		}
	}

	has_dynamic_object_data = false; //clear until dynamic object data is used again

	if (p_dynamic_objects.size() && dynamic_maps.size()) {
		Vector3i octree_size = gi->voxel_gi_get_octree_size(probe);
		int multiplier = dynamic_maps[0].size / MAX(MAX(octree_size.x, octree_size.y), octree_size.z);

		Transform3D oversample_scale;
		oversample_scale.basis.scale(Vector3(multiplier, multiplier, multiplier));

		Transform3D to_cell = oversample_scale * gi->voxel_gi_get_to_cell_xform(probe);
		Transform3D to_world_xform = transform * to_cell.affine_inverse();
		Transform3D to_probe_xform = to_world_xform.affine_inverse();

		AABB probe_aabb(Vector3(), octree_size);

		//this could probably be better parallelized in compute..
		for (int i = 0; i < (int)p_dynamic_objects.size(); i++) {
			RenderGeometryInstance *instance = p_dynamic_objects[i];

			//transform aabb to voxel_gi
			AABB aabb = (to_probe_xform * instance->get_transform()).xform(instance->get_aabb());

			//this needs to wrap to grid resolution to avoid jitter
			//also extend margin a bit just in case
			Vector3i begin = aabb.position - Vector3i(1, 1, 1);
			Vector3i end = aabb.position + aabb.size + Vector3i(1, 1, 1);

			for (int j = 0; j < 3; j++) {
				if ((end[j] - begin[j]) & 1) {
					end[j]++; //for half extents split, it needs to be even
				}
				begin[j] = MAX(begin[j], 0);
				end[j] = MIN(end[j], octree_size[j] * multiplier);
			}

			//aabb = aabb.intersection(probe_aabb); //intersect
			aabb.position = begin;
			aabb.size = end - begin;

			//print_line("aabb: " + aabb);

			for (int j = 0; j < 6; j++) {
				//if (j != 0 && j != 3) {
				//	continue;
				//}
				static const Vector3 render_z[6] = {
					Vector3(1, 0, 0),
					Vector3(0, 1, 0),
					Vector3(0, 0, 1),
					Vector3(-1, 0, 0),
					Vector3(0, -1, 0),
					Vector3(0, 0, -1),
				};
				static const Vector3 render_up[6] = {
					Vector3(0, 1, 0),
					Vector3(0, 0, 1),
					Vector3(0, 1, 0),
					Vector3(0, 1, 0),
					Vector3(0, 0, 1),
					Vector3(0, 1, 0),
				};

				Vector3 render_dir = render_z[j];
				Vector3 up_dir = render_up[j];

				Vector3 center = aabb.get_center();
				Transform3D xform;
				xform.set_look_at(center - aabb.size * 0.5 * render_dir, center, up_dir);

				Vector3 x_dir = xform.basis.get_column(0).abs();
				int x_axis = int(Vector3(0, 1, 2).dot(x_dir));
				Vector3 y_dir = xform.basis.get_column(1).abs();
				int y_axis = int(Vector3(0, 1, 2).dot(y_dir));
				Vector3 z_dir = -xform.basis.get_column(2);
				int z_axis = int(Vector3(0, 1, 2).dot(z_dir.abs()));

				Rect2i rect(aabb.position[x_axis], aabb.position[y_axis], aabb.size[x_axis], aabb.size[y_axis]);
				bool x_flip = bool(Vector3(1, 1, 1).dot(xform.basis.get_column(0)) < 0);
				bool y_flip = bool(Vector3(1, 1, 1).dot(xform.basis.get_column(1)) < 0);
				bool z_flip = bool(Vector3(1, 1, 1).dot(xform.basis.get_column(2)) > 0);

				Projection cm;
				cm.set_orthogonal(-rect.size.width / 2, rect.size.width / 2, -rect.size.height / 2, rect.size.height / 2, 0.0001, aabb.size[z_axis]);

				if (RendererSceneRenderRD::get_singleton()->cull_argument.size() == 0) {
					RendererSceneRenderRD::get_singleton()->cull_argument.push_back(nullptr);
				}
				RendererSceneRenderRD::get_singleton()->cull_argument[0] = instance;

				float exposure_normalization = 1.0;
				if (RendererSceneRenderRD::get_singleton()->is_using_physical_light_units()) {
					exposure_normalization = gi->voxel_gi_get_baked_exposure_normalization(probe);
				}

				RendererSceneRenderRD::get_singleton()->_render_material(to_world_xform * xform, cm, true, RendererSceneRenderRD::get_singleton()->cull_argument, dynamic_maps[0].fb, Rect2i(Vector2i(), rect.size), exposure_normalization);

				VoxelGIDynamicPushConstant push_constant;
				memset(&push_constant, 0, sizeof(VoxelGIDynamicPushConstant));
				push_constant.limits[0] = octree_size.x;
				push_constant.limits[1] = octree_size.y;
				push_constant.limits[2] = octree_size.z;
				push_constant.light_count = p_light_instances.size();
				push_constant.x_dir[0] = x_dir[0];
				push_constant.x_dir[1] = x_dir[1];
				push_constant.x_dir[2] = x_dir[2];
				push_constant.y_dir[0] = y_dir[0];
				push_constant.y_dir[1] = y_dir[1];
				push_constant.y_dir[2] = y_dir[2];
				push_constant.z_dir[0] = z_dir[0];
				push_constant.z_dir[1] = z_dir[1];
				push_constant.z_dir[2] = z_dir[2];
				push_constant.z_base = xform.origin[z_axis];
				push_constant.z_sign = (z_flip ? -1.0 : 1.0);
				push_constant.pos_multiplier = float(1.0) / multiplier;
				push_constant.dynamic_range = gi->voxel_gi_get_dynamic_range(probe);
				push_constant.flip_x = x_flip;
				push_constant.flip_y = y_flip;
				push_constant.rect_pos[0] = rect.position[0];
				push_constant.rect_pos[1] = rect.position[1];
				push_constant.rect_size[0] = rect.size[0];
				push_constant.rect_size[1] = rect.size[1];
				push_constant.prev_rect_ofs[0] = 0;
				push_constant.prev_rect_ofs[1] = 0;
				push_constant.prev_rect_size[0] = 0;
				push_constant.prev_rect_size[1] = 0;
				push_constant.on_mipmap = false;
				push_constant.propagation = gi->voxel_gi_get_propagation(probe);
				push_constant.pad[0] = 0;
				push_constant.pad[1] = 0;
				push_constant.pad[2] = 0;

				//process lighting
				RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();
				RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->voxel_gi_lighting_shader_version_pipelines[VOXEL_GI_SHADER_VERSION_DYNAMIC_OBJECT_LIGHTING]);
				RD::get_singleton()->compute_list_bind_uniform_set(compute_list, dynamic_maps[0].uniform_set, 0);
				RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(VoxelGIDynamicPushConstant));
				RD::get_singleton()->compute_list_dispatch(compute_list, Math::division_round_up(rect.size.x, 8), Math::division_round_up(rect.size.y, 8), 1);
				//print_line("rect: " + itos(i) + ": " + rect);

				for (int k = 1; k < dynamic_maps.size(); k++) {
					// enlarge the rect if needed so all pixels fit when downscaled,
					// this ensures downsampling is smooth and optimal because no pixels are left behind

					//x
					if (rect.position.x & 1) {
						rect.size.x++;
						push_constant.prev_rect_ofs[0] = 1; //this is used to ensure reading is also optimal
					} else {
						push_constant.prev_rect_ofs[0] = 0;
					}
					if (rect.size.x & 1) {
						rect.size.x++;
					}

					rect.position.x >>= 1;
					rect.size.x = MAX(1, rect.size.x >> 1);

					//y
					if (rect.position.y & 1) {
						rect.size.y++;
						push_constant.prev_rect_ofs[1] = 1;
					} else {
						push_constant.prev_rect_ofs[1] = 0;
					}
					if (rect.size.y & 1) {
						rect.size.y++;
					}

					rect.position.y >>= 1;
					rect.size.y = MAX(1, rect.size.y >> 1);

					//shrink limits to ensure plot does not go outside map
					if (dynamic_maps[k].mipmap > 0) {
						for (int l = 0; l < 3; l++) {
							push_constant.limits[l] = MAX(1, push_constant.limits[l] >> 1);
						}
					}

					//print_line("rect: " + itos(i) + ": " + rect);
					push_constant.rect_pos[0] = rect.position[0];
					push_constant.rect_pos[1] = rect.position[1];
					push_constant.prev_rect_size[0] = push_constant.rect_size[0];
					push_constant.prev_rect_size[1] = push_constant.rect_size[1];
					push_constant.rect_size[0] = rect.size[0];
					push_constant.rect_size[1] = rect.size[1];
					push_constant.on_mipmap = dynamic_maps[k].mipmap > 0;

					RD::get_singleton()->compute_list_add_barrier(compute_list);

					if (dynamic_maps[k].mipmap < 0) {
						RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->voxel_gi_lighting_shader_version_pipelines[VOXEL_GI_SHADER_VERSION_DYNAMIC_SHRINK_WRITE]);
					} else if (k < dynamic_maps.size() - 1) {
						RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->voxel_gi_lighting_shader_version_pipelines[VOXEL_GI_SHADER_VERSION_DYNAMIC_SHRINK_WRITE_PLOT]);
					} else {
						RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, gi->voxel_gi_lighting_shader_version_pipelines[VOXEL_GI_SHADER_VERSION_DYNAMIC_SHRINK_PLOT]);
					}
					RD::get_singleton()->compute_list_bind_uniform_set(compute_list, dynamic_maps[k].uniform_set, 0);
					RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(VoxelGIDynamicPushConstant));
					RD::get_singleton()->compute_list_dispatch(compute_list, Math::division_round_up(rect.size.x, 8), Math::division_round_up(rect.size.y, 8), 1);
				}

				RD::get_singleton()->compute_list_end();
			}
		}

		has_dynamic_object_data = true; //clear until dynamic object data is used again
	}

	last_probe_version = gi->voxel_gi_get_version(probe);
}

void GI::VoxelGIInstance::free_resources() {
	if (texture.is_valid()) {
		RD::get_singleton()->free(texture);
		RD::get_singleton()->free(write_buffer);

		texture = RID();
		write_buffer = RID();
		mipmaps.clear();
	}

	for (int i = 0; i < dynamic_maps.size(); i++) {
		RD::get_singleton()->free(dynamic_maps[i].texture);
		RD::get_singleton()->free(dynamic_maps[i].depth);

		// these only exist on the first level...
		if (dynamic_maps[i].fb_depth.is_valid()) {
			RD::get_singleton()->free(dynamic_maps[i].fb_depth);
		}
		if (dynamic_maps[i].albedo.is_valid()) {
			RD::get_singleton()->free(dynamic_maps[i].albedo);
		}
		if (dynamic_maps[i].normal.is_valid()) {
			RD::get_singleton()->free(dynamic_maps[i].normal);
		}
		if (dynamic_maps[i].orm.is_valid()) {
			RD::get_singleton()->free(dynamic_maps[i].orm);
		}
	}
	dynamic_maps.clear();
}

void GI::VoxelGIInstance::debug(RD::DrawListID p_draw_list, RID p_framebuffer, const Projection &p_camera_with_transform, bool p_lighting, bool p_emission, float p_alpha) {
	RendererRD::MaterialStorage *material_storage = RendererRD::MaterialStorage::get_singleton();

	if (mipmaps.is_empty()) {
		return;
	}

	Projection cam_transform = (p_camera_with_transform * Projection(transform)) * Projection(gi->voxel_gi_get_to_cell_xform(probe).affine_inverse());

	int level = 0;
	Vector3i octree_size = gi->voxel_gi_get_octree_size(probe);

	VoxelGIDebugPushConstant push_constant;
	push_constant.alpha = p_alpha;
	push_constant.dynamic_range = gi->voxel_gi_get_dynamic_range(probe);
	push_constant.cell_offset = mipmaps[level].cell_offset;
	push_constant.level = level;

	push_constant.bounds[0] = octree_size.x >> level;
	push_constant.bounds[1] = octree_size.y >> level;
	push_constant.bounds[2] = octree_size.z >> level;
	push_constant.pad = 0;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			push_constant.projection[i * 4 + j] = cam_transform.columns[i][j];
		}
	}

	if (gi->voxel_gi_debug_uniform_set.is_valid()) {
		RD::get_singleton()->free(gi->voxel_gi_debug_uniform_set);
	}
	Vector<RD::Uniform> uniforms;
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
		u.binding = 1;
		u.append_id(gi->voxel_gi_get_data_buffer(probe));
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		u.binding = 2;
		u.append_id(texture);
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_SAMPLER;
		u.binding = 3;
		u.append_id(material_storage->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_NEAREST, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED));
		uniforms.push_back(u);
	}

	int cell_count;
	if (!p_emission && p_lighting && has_dynamic_object_data) {
		cell_count = push_constant.bounds[0] * push_constant.bounds[1] * push_constant.bounds[2];
	} else {
		cell_count = mipmaps[level].cell_count;
	}

	gi->voxel_gi_debug_uniform_set = RD::get_singleton()->uniform_set_create(uniforms, gi->voxel_gi_debug_shader_version_shaders[0], 0);

	int voxel_gi_debug_pipeline = VOXEL_GI_DEBUG_COLOR;
	if (p_emission) {
		voxel_gi_debug_pipeline = VOXEL_GI_DEBUG_EMISSION;
	} else if (p_lighting) {
		voxel_gi_debug_pipeline = has_dynamic_object_data ? VOXEL_GI_DEBUG_LIGHT_FULL : VOXEL_GI_DEBUG_LIGHT;
	}
	RD::get_singleton()->draw_list_bind_render_pipeline(
			p_draw_list,
			gi->voxel_gi_debug_shader_version_pipelines[voxel_gi_debug_pipeline].get_render_pipeline(RD::INVALID_ID, RD::get_singleton()->framebuffer_get_format(p_framebuffer)));
	RD::get_singleton()->draw_list_bind_uniform_set(p_draw_list, gi->voxel_gi_debug_uniform_set, 0);
	RD::get_singleton()->draw_list_set_push_constant(p_draw_list, &push_constant, sizeof(VoxelGIDebugPushConstant));
	RD::get_singleton()->draw_list_draw(p_draw_list, false, cell_count, 36);
}

////////////////////////////////////////////////////////////////////////////////
// GI

GI::GI() {
	singleton = this;

	hddagi_frames_to_converge = RS::EnvironmentHDDAGIFramesToConverge(CLAMP(int32_t(GLOBAL_GET("rendering/global_illumination/hddagi/frames_to_converge")), 0, int32_t(RS::ENV_HDDAGI_CONVERGE_MAX - 1)));
	hddagi_frames_to_update_light = RS::EnvironmentHDDAGIFramesToUpdateLight(CLAMP(int32_t(GLOBAL_GET("rendering/global_illumination/hddagi/frames_to_update_lights")), 0, int32_t(RS::ENV_HDDAGI_UPDATE_LIGHT_MAX - 1)));
}

GI::~GI() {
	singleton = nullptr;
}

void GI::init(SkyRD *p_sky) {
	/* GI */

	{
		//kinda complicated to compute the amount of slots, we try to use as many as we can

		voxel_gi_lights = memnew_arr(VoxelGILight, voxel_gi_max_lights);
		voxel_gi_lights_uniform = RD::get_singleton()->uniform_buffer_create(voxel_gi_max_lights * sizeof(VoxelGILight));
		voxel_gi_quality = RS::VoxelGIQuality(CLAMP(int(GLOBAL_GET("rendering/global_illumination/voxel_gi/quality")), 0, 1));

		String defines = "\n#define MAX_LIGHTS " + itos(voxel_gi_max_lights) + "\n";

		Vector<String> versions;
		versions.push_back("\n#define MODE_COMPUTE_LIGHT\n");
		versions.push_back("\n#define MODE_SECOND_BOUNCE\n");
		versions.push_back("\n#define MODE_UPDATE_MIPMAPS\n");
		versions.push_back("\n#define MODE_WRITE_TEXTURE\n");
		versions.push_back("\n#define MODE_DYNAMIC\n#define MODE_DYNAMIC_LIGHTING\n");
		versions.push_back("\n#define MODE_DYNAMIC\n#define MODE_DYNAMIC_SHRINK\n#define MODE_DYNAMIC_SHRINK_WRITE\n");
		versions.push_back("\n#define MODE_DYNAMIC\n#define MODE_DYNAMIC_SHRINK\n#define MODE_DYNAMIC_SHRINK_PLOT\n");
		versions.push_back("\n#define MODE_DYNAMIC\n#define MODE_DYNAMIC_SHRINK\n#define MODE_DYNAMIC_SHRINK_PLOT\n#define MODE_DYNAMIC_SHRINK_WRITE\n");

		voxel_gi_shader.initialize(versions, defines);
		voxel_gi_lighting_shader_version = voxel_gi_shader.version_create();
		for (int i = 0; i < VOXEL_GI_SHADER_VERSION_MAX; i++) {
			voxel_gi_lighting_shader_version_shaders[i] = voxel_gi_shader.version_get_shader(voxel_gi_lighting_shader_version, i);
			voxel_gi_lighting_shader_version_pipelines[i] = RD::get_singleton()->compute_pipeline_create(voxel_gi_lighting_shader_version_shaders[i]);
		}
	}

	{
		String defines;
		Vector<String> versions;
		versions.push_back("\n#define MODE_DEBUG_COLOR\n");
		versions.push_back("\n#define MODE_DEBUG_LIGHT\n");
		versions.push_back("\n#define MODE_DEBUG_EMISSION\n");
		versions.push_back("\n#define MODE_DEBUG_LIGHT\n#define MODE_DEBUG_LIGHT_FULL\n");

		voxel_gi_debug_shader.initialize(versions, defines);
		voxel_gi_debug_shader_version = voxel_gi_debug_shader.version_create();
		for (int i = 0; i < VOXEL_GI_DEBUG_MAX; i++) {
			voxel_gi_debug_shader_version_shaders[i] = voxel_gi_debug_shader.version_get_shader(voxel_gi_debug_shader_version, i);

			RD::PipelineRasterizationState rs;
			rs.cull_mode = RD::POLYGON_CULL_FRONT;
			RD::PipelineDepthStencilState ds;
			ds.enable_depth_test = true;
			ds.enable_depth_write = true;
			ds.depth_compare_operator = RD::COMPARE_OP_GREATER_OR_EQUAL;

			voxel_gi_debug_shader_version_pipelines[i].setup(voxel_gi_debug_shader_version_shaders[i], RD::RENDER_PRIMITIVE_TRIANGLES, rs, RD::PipelineMultisampleState(), ds, RD::PipelineColorBlendState::create_disabled(), 0);
		}
	}

	/* SDGFI */

	{
		Vector<String> preprocess_modes;
		preprocess_modes.push_back("\n#define MODE_REGION_STORE\n");
		preprocess_modes.push_back("\n#define MODE_LIGHT_STORE\n");
		preprocess_modes.push_back("\n#define MODE_LIGHT_SCROLL\n");
		preprocess_modes.push_back("\n#define MODE_OCCLUSION\n");
		preprocess_modes.push_back("\n#define MODE_OCCLUSION_STORE\n");
		preprocess_modes.push_back("\n#define MODE_LIGHTPROBE_SCROLL\n");
		preprocess_modes.push_back("\n#define MODE_LIGHTPROBE_NEIGHBOURS\n");
		preprocess_modes.push_back("\n#define MODE_LIGHTPROBE_GEOMETRY_PROXIMITY\n");
		preprocess_modes.push_back("\n#define MODE_LIGHTPROBE_UPDATE_FRAMES\n");

		String defines = "\n#define LIGHTPROBE_OCT_SIZE " + itos(HDDAGI::LIGHTPROBE_OCT_SIZE) + "\n#define OCCLUSION_OCT_SIZE " + itos(HDDAGI::OCCLUSION_OCT_SIZE) + "\n#define OCCLUSION_OCT_SIZE_HALF " + itos(HDDAGI::OCCLUSION_OCT_SIZE / 2) + "\n#define OCCLUSION_SUBPIXELS " + itos(HDDAGI::OCCLUSION_SUBPIXELS) + "\n";

		hddagi_shader.preprocess.initialize(preprocess_modes, defines);
		hddagi_shader.preprocess_shader = hddagi_shader.preprocess.version_create();
		for (int i = 0; i < HDDAGIShader::PRE_PROCESS_MAX; i++) {
			hddagi_shader.preprocess_shader_version[i] = hddagi_shader.preprocess.version_get_shader(hddagi_shader.preprocess_shader, i);
			hddagi_shader.preprocess_pipeline[i] = RD::get_singleton()->compute_pipeline_create(hddagi_shader.preprocess_shader_version[i]);
		}
	}

	{
		//calculate tables
		String defines = "\n#define LIGHTPROBE_OCT_SIZE " + itos(HDDAGI::LIGHTPROBE_OCT_SIZE) + "\n#define OCCLUSION_OCT_SIZE " + itos(HDDAGI::OCCLUSION_OCT_SIZE) + "\n";

		Vector<String> direct_light_modes;
		direct_light_modes.push_back("\n#define MODE_PROCESS_STATIC\n");
		direct_light_modes.push_back("\n#define MODE_PROCESS_DYNAMIC\n");
		hddagi_shader.direct_light.initialize(direct_light_modes, defines);
		hddagi_shader.direct_light_shader = hddagi_shader.direct_light.version_create();
		for (int i = 0; i < HDDAGIShader::DIRECT_LIGHT_MODE_MAX; i++) {
			hddagi_shader.direct_light_shader_version[i] = hddagi_shader.direct_light.version_get_shader(hddagi_shader.direct_light_shader, i);
			hddagi_shader.direct_light_pipeline[i] = RD::get_singleton()->compute_pipeline_create(hddagi_shader.direct_light_shader_version[i]);
		}
	}

	{
		//calculate tables
		String defines = "\n#define LIGHTPROBE_OCT_SIZE " + itos(HDDAGI::LIGHTPROBE_OCT_SIZE) + "\n#define OCCLUSION_OCT_SIZE " + itos(HDDAGI::OCCLUSION_OCT_SIZE) + "\n";

		defines += "\n#define SH_SIZE " + itos(HDDAGI::SH_SIZE) + "\n";
		if (p_sky->sky_use_cubemap_array) {
			defines += "\n#define USE_CUBEMAP_ARRAY\n";
		}

		Vector<String> integrate_modes;
		integrate_modes.push_back("\n#define MODE_PROCESS\n");
		integrate_modes.push_back("\n#define MODE_FILTER\n");
		integrate_modes.push_back("\n#define MODE_CAMERA_VISIBILITY\n");
		hddagi_shader.integrate.initialize(integrate_modes, defines);
		hddagi_shader.integrate_shader = hddagi_shader.integrate.version_create();

		for (int i = 0; i < HDDAGIShader::INTEGRATE_MODE_MAX; i++) {
			hddagi_shader.integrate_shader_version[i] = hddagi_shader.integrate.version_get_shader(hddagi_shader.integrate_shader, i);
			hddagi_shader.integrate_pipeline[i] = RD::get_singleton()->compute_pipeline_create(hddagi_shader.integrate_shader_version[i]);
		}
	}

	//GK
	{
		//calculate tables
		String defines = "\n#define LIGHTPROBE_OCT_SIZE " + itos(HDDAGI::LIGHTPROBE_OCT_SIZE) + "\n#define OCCLUSION_OCT_SIZE " + itos(HDDAGI::OCCLUSION_OCT_SIZE) + "\n";
		if (RendererSceneRenderRD::get_singleton()->is_vrs_supported()) {
			defines += "\n#define USE_VRS\n";
		}
		if (!RD::get_singleton()->sampler_is_format_supported_for_filter(RD::DATA_FORMAT_R8G8_UINT, RD::SAMPLER_FILTER_LINEAR)) {
			defines += "\n#define SAMPLE_VOXEL_GI_NEAREST\n";
		}

		Vector<String> gi_modes;

		gi_modes.push_back("\n#define USE_VOXEL_GI_INSTANCES\n"); // MODE_VOXEL_GI
		gi_modes.push_back("\n#define USE_HDDAGI\n"); // MODE_HDDAGI
		gi_modes.push_back("\n#define USE_HDDAGI\n\n#define USE_VOXEL_GI_INSTANCES\n"); // MODE_COMBINED
		gi_modes.push_back("\n#define USE_HDDAGI\n\n#define USE_AMBIENT_BLEND\n"); // MODE_HDDAGI
		gi_modes.push_back("\n#define USE_HDDAGI\n\n#define USE_VOXEL_GI_INSTANCES\n\n#define USE_AMBIENT_BLEND\n"); // MODE_COMBINED

		shader.initialize(gi_modes, defines);
		shader_version = shader.version_create();

		Vector<RD::PipelineSpecializationConstant> specialization_constants;

		{
			RD::PipelineSpecializationConstant sc;
			sc.type = RD::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_BOOL;
			sc.constant_id = 0; // SHADER_SPECIALIZATION_HALF_RES
			sc.bool_value = false;
			specialization_constants.push_back(sc);

			sc.type = RD::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_BOOL;
			sc.constant_id = 1; // SHADER_SPECIALIZATION_USE_FULL_PROJECTION_MATRIX
			sc.bool_value = false;
			specialization_constants.push_back(sc);

			sc.type = RD::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_BOOL;
			sc.constant_id = 2; // SHADER_SPECIALIZATION_USE_VRS
			sc.bool_value = false;
			specialization_constants.push_back(sc);
		}

		for (int v = 0; v < SHADER_SPECIALIZATION_VARIATIONS; v++) {
			specialization_constants.ptrw()[0].bool_value = (v & SHADER_SPECIALIZATION_HALF_RES) ? true : false;
			specialization_constants.ptrw()[1].bool_value = (v & SHADER_SPECIALIZATION_USE_FULL_PROJECTION_MATRIX) ? true : false;
			specialization_constants.ptrw()[2].bool_value = (v & SHADER_SPECIALIZATION_USE_VRS) ? true : false;
			for (int i = 0; i < MODE_MAX; i++) {
				pipelines[v][i] = RD::get_singleton()->compute_pipeline_create(shader.version_get_shader(shader_version, i), specialization_constants);
			}
		}

		hddagi_ubo = RD::get_singleton()->uniform_buffer_create(sizeof(HDDAGIData));
	}
	{
		String defines;
		//calculate tables
		if (RendererSceneRenderRD::get_singleton()->is_vrs_supported()) {
			defines += "\n#define USE_VRS\n";
		}
		Vector<String> filter_modes;
		filter_modes.push_back("\n#define MODE_BILATERAL_FILTER\n");
		filter_modes.push_back("\n#define MODE_BILATERAL_FILTER\n#define HALF_SIZE\n");
		filter_shader.initialize(filter_modes, defines);
		filter_shader_version = filter_shader.version_create();

		Vector<RD::PipelineSpecializationConstant> specialization_constants;

		{
			RD::PipelineSpecializationConstant sc;
			sc.type = RD::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_BOOL;
			sc.constant_id = 0; // FILTER_SHADER_SPECIALIZATION_HALF_RES
			sc.bool_value = false;
			specialization_constants.push_back(sc);

			sc.type = RD::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_BOOL;
			sc.constant_id = 1; // FILTER_SHADER_SPECIALIZATION_USE_FULL_PROJECTION_MATRIX
			sc.bool_value = false;
			specialization_constants.push_back(sc);
		}

		for (int v = 0; v < FILTER_SHADER_SPECIALIZATION_VARIATIONS; v++) {
			specialization_constants.ptrw()[0].bool_value = (v & FILTER_SHADER_SPECIALIZATION_HALF_RES) ? true : false;
			specialization_constants.ptrw()[1].bool_value = (v & FILTER_SHADER_SPECIALIZATION_USE_FULL_PROJECTION_MATRIX) ? true : false;
			for (int i = 0; i < FILTER_MODE_MAX; i++) {
				filter_pipelines[v][i] = RD::get_singleton()->compute_pipeline_create(filter_shader.version_get_shader(filter_shader_version, i), specialization_constants);
			}
		}
	}

	{
		String defines = "\n#define LIGHTPROBE_OCT_SIZE " + itos(HDDAGI::LIGHTPROBE_OCT_SIZE) + "\n#define OCCLUSION_OCT_SIZE " + itos(HDDAGI::OCCLUSION_OCT_SIZE) + "\n";
		Vector<String> debug_modes;
		debug_modes.push_back("");
		hddagi_shader.debug.initialize(debug_modes, defines);
		hddagi_shader.debug_shader = hddagi_shader.debug.version_create();
		hddagi_shader.debug_shader_version = hddagi_shader.debug.version_get_shader(hddagi_shader.debug_shader, 0);
		hddagi_shader.debug_pipeline = RD::get_singleton()->compute_pipeline_create(hddagi_shader.debug_shader_version);
	}
	{
		String defines = "\n#define LIGHTPROBE_OCT_SIZE " + itos(HDDAGI::LIGHTPROBE_OCT_SIZE) + "\n#define OCCLUSION_OCT_SIZE " + itos(HDDAGI::OCCLUSION_OCT_SIZE) + "\n#define OCCLUSION_SUBPIXELS " + itos(HDDAGI::OCCLUSION_SUBPIXELS) + "\n";

		Vector<String> versions;
		versions.push_back("\n#define MODE_PROBES\n");
		versions.push_back("\n#define MODE_PROBES\n#define USE_MULTIVIEW\n");
		versions.push_back("\n#define MODE_OCCLUSION\n");
		versions.push_back("\n#define MODE_OCCLUSION\n#define USE_MULTIVIEW\n");

		hddagi_shader.debug_probes.initialize(versions, defines);

		// TODO disable multiview versions if turned off

		hddagi_shader.debug_probes_shader = hddagi_shader.debug_probes.version_create();

		{
			RD::PipelineRasterizationState rs;
			rs.cull_mode = RD::POLYGON_CULL_DISABLED;
			RD::PipelineDepthStencilState ds;
			ds.enable_depth_test = true;
			ds.enable_depth_write = true;
			ds.depth_compare_operator = RD::COMPARE_OP_GREATER_OR_EQUAL;
			RD::PipelineColorBlendState cb = RD::PipelineColorBlendState::create_disabled();
			RD::RenderPrimitive rp = RD::RENDER_PRIMITIVE_TRIANGLE_STRIPS;

			for (int i = 0; i < HDDAGIShader::PROBE_DEBUG_MAX; i++) {
				// TODO check if version is enabled

				hddagi_shader.debug_probes_shader_version[i] = hddagi_shader.debug_probes.version_get_shader(hddagi_shader.debug_probes_shader, i);
				hddagi_shader.debug_probes_pipeline[i].setup(hddagi_shader.debug_probes_shader_version[i], rp, rs, RD::PipelineMultisampleState(), ds, cb, 0);
			}
		}
	}
	default_voxel_gi_buffer = RD::get_singleton()->uniform_buffer_create(sizeof(VoxelGIData) * MAX_VOXEL_GI_INSTANCES);
	half_resolution = GLOBAL_GET("rendering/global_illumination/gi/use_half_resolution");
}

void GI::free() {
	if (default_voxel_gi_buffer.is_valid()) {
		RD::get_singleton()->free(default_voxel_gi_buffer);
	}
	if (voxel_gi_lights_uniform.is_valid()) {
		RD::get_singleton()->free(voxel_gi_lights_uniform);
	}
	if (hddagi_ubo.is_valid()) {
		RD::get_singleton()->free(hddagi_ubo);
	}

	if (voxel_gi_debug_shader_version.is_valid()) {
		voxel_gi_debug_shader.version_free(voxel_gi_debug_shader_version);
	}
	if (voxel_gi_lighting_shader_version.is_valid()) {
		voxel_gi_shader.version_free(voxel_gi_lighting_shader_version);
	}
	if (shader_version.is_valid()) {
		shader.version_free(shader_version);
	}
	if (hddagi_shader.debug_probes_shader.is_valid()) {
		hddagi_shader.debug_probes.version_free(hddagi_shader.debug_probes_shader);
	}
	if (hddagi_shader.debug_shader.is_valid()) {
		hddagi_shader.debug.version_free(hddagi_shader.debug_shader);
	}
	if (hddagi_shader.direct_light_shader.is_valid()) {
		hddagi_shader.direct_light.version_free(hddagi_shader.direct_light_shader);
	}
	if (hddagi_shader.integrate_shader.is_valid()) {
		hddagi_shader.integrate.version_free(hddagi_shader.integrate_shader);
	}
	if (hddagi_shader.preprocess_shader.is_valid()) {
		hddagi_shader.preprocess.version_free(hddagi_shader.preprocess_shader);
	}

	if (voxel_gi_lights) {
		memdelete_arr(voxel_gi_lights);
	}
}

Ref<GI::HDDAGI> GI::create_hddagi(RID p_env, const Vector3 &p_world_position, uint32_t p_requested_history_size) {
	Ref<HDDAGI> hddagi;
	hddagi.instantiate();

	hddagi->create(p_env, p_world_position, p_requested_history_size, this);

	return hddagi;
}

void GI::setup_voxel_gi_instances(RenderDataRD *p_render_data, Ref<RenderSceneBuffersRD> p_render_buffers, const Transform3D &p_transform, const PagedArray<RID> &p_voxel_gi_instances, uint32_t &r_voxel_gi_instances_used) {
	ERR_FAIL_COND(p_render_buffers.is_null());

	RendererRD::TextureStorage *texture_storage = RendererRD::TextureStorage::get_singleton();
	ERR_FAIL_NULL(texture_storage);

	r_voxel_gi_instances_used = 0;

	Ref<RenderBuffersGI> rbgi = p_render_buffers->get_custom_data(RB_SCOPE_GI);
	ERR_FAIL_COND(rbgi.is_null());

	RID voxel_gi_buffer = rbgi->get_voxel_gi_buffer();
	VoxelGIData voxel_gi_data[MAX_VOXEL_GI_INSTANCES];

	Transform3D to_camera;
	to_camera.origin = p_transform.origin; //only translation, make local

	for (int i = 0; i < MAX_VOXEL_GI_INSTANCES; i++) {
		RID texture;
		if (i < (int)p_voxel_gi_instances.size()) {
			VoxelGIInstance *gipi = voxel_gi_instance_owner.get_or_null(p_voxel_gi_instances[i]);

			if (gipi) {
				texture = gipi->texture;
				VoxelGIData &gipd = voxel_gi_data[i];

				RID base_probe = gipi->probe;

				Transform3D to_cell = voxel_gi_get_to_cell_xform(gipi->probe) * gipi->transform.affine_inverse() * to_camera;

				gipd.xform[0] = to_cell.basis.rows[0][0];
				gipd.xform[1] = to_cell.basis.rows[1][0];
				gipd.xform[2] = to_cell.basis.rows[2][0];
				gipd.xform[3] = 0;
				gipd.xform[4] = to_cell.basis.rows[0][1];
				gipd.xform[5] = to_cell.basis.rows[1][1];
				gipd.xform[6] = to_cell.basis.rows[2][1];
				gipd.xform[7] = 0;
				gipd.xform[8] = to_cell.basis.rows[0][2];
				gipd.xform[9] = to_cell.basis.rows[1][2];
				gipd.xform[10] = to_cell.basis.rows[2][2];
				gipd.xform[11] = 0;
				gipd.xform[12] = to_cell.origin.x;
				gipd.xform[13] = to_cell.origin.y;
				gipd.xform[14] = to_cell.origin.z;
				gipd.xform[15] = 1;

				Vector3 bounds = voxel_gi_get_octree_size(base_probe);

				gipd.bounds[0] = bounds.x;
				gipd.bounds[1] = bounds.y;
				gipd.bounds[2] = bounds.z;

				gipd.dynamic_range = voxel_gi_get_dynamic_range(base_probe) * voxel_gi_get_energy(base_probe);
				gipd.bias = voxel_gi_get_bias(base_probe);
				gipd.normal_bias = voxel_gi_get_normal_bias(base_probe);
				gipd.blend_ambient = !voxel_gi_is_interior(base_probe);
				gipd.mipmaps = gipi->mipmaps.size();
				gipd.exposure_normalization = 1.0;
				if (p_render_data->camera_attributes.is_valid()) {
					float exposure_normalization = RSG::camera_attributes->camera_attributes_get_exposure_normalization_factor(p_render_data->camera_attributes);
					gipd.exposure_normalization = exposure_normalization / voxel_gi_get_baked_exposure_normalization(base_probe);
				}
			}

			r_voxel_gi_instances_used++;
		}

		if (texture == RID()) {
			texture = texture_storage->texture_rd_get_default(RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_3D_WHITE);
		}

		if (texture != rbgi->voxel_gi_textures[i]) {
			rbgi->voxel_gi_textures[i] = texture;
		}
	}

	if (p_voxel_gi_instances.size() > 0) {
		RD::get_singleton()->draw_command_begin_label("VoxelGIs Setup");

		RD::get_singleton()->buffer_update(voxel_gi_buffer, 0, sizeof(VoxelGIData) * MIN((uint64_t)MAX_VOXEL_GI_INSTANCES, p_voxel_gi_instances.size()), voxel_gi_data);

		RD::get_singleton()->draw_command_end_label();
	}
}

RID GI::RenderBuffersGI::get_voxel_gi_buffer() {
	if (voxel_gi_buffer.is_null()) {
		voxel_gi_buffer = RD::get_singleton()->uniform_buffer_create(sizeof(GI::VoxelGIData) * GI::MAX_VOXEL_GI_INSTANCES);
	}
	return voxel_gi_buffer;
}

void GI::RenderBuffersGI::free_data() {
	if (scene_data_ubo.is_valid()) {
		RD::get_singleton()->free(scene_data_ubo);
		scene_data_ubo = RID();
	}

	if (voxel_gi_buffer.is_valid()) {
		RD::get_singleton()->free(voxel_gi_buffer);
		voxel_gi_buffer = RID();
	}
}

void GI::process_gi(Ref<RenderSceneBuffersRD> p_render_buffers, const RID *p_normal_roughness_slices, RID p_voxel_gi_buffer, RID p_environment, uint32_t p_view_count, const Projection *p_projections, const Vector3 *p_eye_offsets, const Transform3D &p_cam_transform, const PagedArray<RID> &p_voxel_gi_instances) {
	RendererRD::TextureStorage *texture_storage = RendererRD::TextureStorage::get_singleton();

	ERR_FAIL_COND_MSG(p_view_count > 2, "Maximum of 2 views supported for Processing GI.");

	bool use_hddagi = p_render_buffers->has_custom_data(RB_SCOPE_HDDAGI);
	if (!use_hddagi)
	{
		return;
	}
	RD::get_singleton()->draw_command_begin_label("GI Render");

	ERR_FAIL_COND(p_render_buffers.is_null());

	Ref<RenderBuffersGI> rbgi = p_render_buffers->get_custom_data(RB_SCOPE_GI);
	ERR_FAIL_COND(rbgi.is_null());

	Size2i internal_size = p_render_buffers->get_internal_size();

	if (rbgi->using_half_size_gi != half_resolution) {
		p_render_buffers->clear_context(RB_SCOPE_GI);
	}

	if (!p_render_buffers->has_texture(RB_SCOPE_GI, RB_TEX_AMBIENT)) {
		Size2i size = internal_size;

		if (half_resolution) {
			size.x >>= 1;
			size.y >>= 1;
		}

		RD::TextureFormat tf;
		tf.format = RD::DATA_FORMAT_R32_UINT;
		tf.width = size.x;
		tf.height = size.y;
		tf.depth = 1;
		tf.array_layers = 1;
		tf.shareable_formats.push_back(RD::DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32);
		tf.shareable_formats.push_back(RD::DATA_FORMAT_R32_UINT);
		tf.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT;

		RD::TextureFormat tf_blend;
		tf_blend.format = RD::DATA_FORMAT_R8G8_UNORM;

		tf_blend.width = size.x;
		tf_blend.height = size.y;
		tf_blend.depth = 1;
		tf_blend.array_layers = 1;
		tf_blend.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT;

		p_render_buffers->create_texture_from_format(RB_SCOPE_GI, RB_TEX_AMBIENT_U32, tf);
		p_render_buffers->create_texture_from_format(RB_SCOPE_GI, RB_TEX_REFLECTION_U32, tf);
		p_render_buffers->create_texture_from_format(RB_SCOPE_GI, RB_TEX_AMBIENT_REFLECTION_BLEND, tf_blend);

		p_render_buffers->create_texture_from_format(RB_SCOPE_GI, RB_TEX_REFLECTION_U32_FILTERED, tf);
		p_render_buffers->create_texture_from_format(RB_SCOPE_GI, RB_TEX_AMBIENT_REFLECTION_BLEND_FILTERED, tf_blend);

		RD::TextureView tv;
		tv.format_override = RD::DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		p_render_buffers->create_texture_view(RB_SCOPE_GI, RB_TEX_AMBIENT_U32, RB_TEX_AMBIENT, tv);
		p_render_buffers->create_texture_view(RB_SCOPE_GI, RB_TEX_REFLECTION_U32, RB_TEX_REFLECTION, tv);
		p_render_buffers->create_texture_view(RB_SCOPE_GI, RB_TEX_REFLECTION_U32_FILTERED, RB_TEX_REFLECTION_FILTERED, tv);

		rbgi->using_half_size_gi = half_resolution;
	}

	// Setup our scene data
	{
		SceneData scene_data;

		if (rbgi->scene_data_ubo.is_null()) {
			rbgi->scene_data_ubo = RD::get_singleton()->uniform_buffer_create(sizeof(SceneData));
		}

		Projection correction;
		correction.set_depth_correction(false);

		for (uint32_t v = 0; v < p_view_count; v++) {
			Projection temp = correction * p_projections[v];

			RendererRD::MaterialStorage::store_camera(temp.inverse(), scene_data.inv_projection[v]);
			scene_data.eye_offset[v][0] = p_eye_offsets[v].x;
			scene_data.eye_offset[v][1] = p_eye_offsets[v].y;
			scene_data.eye_offset[v][2] = p_eye_offsets[v].z;
			scene_data.eye_offset[v][3] = 0.0;
		}

		// Note that we will be ignoring the origin of this transform.
		RendererRD::MaterialStorage::store_transform(p_cam_transform, scene_data.cam_transform);

		scene_data.screen_size[0] = internal_size.x;
		scene_data.screen_size[1] = internal_size.y;

		RD::get_singleton()->buffer_update(rbgi->scene_data_ubo, 0, sizeof(SceneData), &scene_data);
	}

	// Now compute the contents of our buffers.
	RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();

	// Render each eye separately.
	// We need to look into whether we can make our compute shader use Multiview but not sure that works or makes a difference..

	// setup our push constant

	PushConstant push_constant;

	push_constant.max_voxel_gi_instances = MIN((uint64_t)MAX_VOXEL_GI_INSTANCES, p_voxel_gi_instances.size());
	push_constant.high_quality_vct = voxel_gi_quality == RS::VOXEL_GI_QUALITY_HIGH;

	// these should be the same for all views
	push_constant.orthogonal = p_projections[0].is_orthogonal();
	push_constant.z_near = p_projections[0].get_z_near();
	push_constant.z_far = p_projections[0].get_z_far();

	// these are only used if we have 1 view, else we use the projections in our scene data
	push_constant.proj_info[0] = -2.0f / (internal_size.x * p_projections[0].columns[0][0]);
	push_constant.proj_info[1] = -2.0f / (internal_size.y * p_projections[0].columns[1][1]);
	push_constant.proj_info[2] = (1.0f - p_projections[0].columns[0][2]) / p_projections[0].columns[0][0];
	push_constant.proj_info[3] = (1.0f + p_projections[0].columns[1][2]) / p_projections[0].columns[1][1];

	bool use_voxel_gi_instances = push_constant.max_voxel_gi_instances > 0;

	Ref<HDDAGI> hddagi;
	if (use_hddagi) {
		hddagi = p_render_buffers->get_custom_data(RB_SCOPE_HDDAGI);
	}

	uint32_t pipeline_specialization = 0;
	if (rbgi->using_half_size_gi) {
		pipeline_specialization |= SHADER_SPECIALIZATION_HALF_RES;
	}
	if (p_view_count > 1) {
		pipeline_specialization |= SHADER_SPECIALIZATION_USE_FULL_PROJECTION_MATRIX;
	}
	bool has_vrs_texture = p_render_buffers->has_texture(RB_SCOPE_VRS, RB_TEXTURE);
	if (has_vrs_texture) {
		pipeline_specialization |= SHADER_SPECIALIZATION_USE_VRS;
	}

	Mode mode;

	if (use_hddagi) {
		if (use_voxel_gi_instances) {
			mode = hddagi->using_ambient_filter ? MODE_COMBINED_BLEND_AMBIENT : MODE_COMBINED;
		} else {
			mode = hddagi->using_ambient_filter ? MODE_HDDAGI_BLEND_AMBIENT : MODE_HDDAGI;
		}
		push_constant.occlusion_bias = hddagi->occlusion_bias;
	} else {
		mode = MODE_VOXEL_GI;
		push_constant.occlusion_bias = 0;
	}

	for (uint32_t v = 0; v < p_view_count; v++) {
		push_constant.view_index = v;

		// setup our uniform set
		RD::Uniform vgiu;
		vgiu.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		vgiu.binding = 17;
		for (int i = 0; i < MAX_VOXEL_GI_INSTANCES; i++) {
			vgiu.append_id(rbgi->voxel_gi_textures[i]);
		}

		RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
				shader.version_get_shader(shader_version, 0),
				0,
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 1, hddagi->voxel_bits_tex),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 2, hddagi->voxel_region_tex),
				RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 3, hddagi->voxel_light_tex),
				RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 4, hddagi->lightprobe_specular_tex),
				RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 5, hddagi->using_probe_filter ? hddagi->lightprobe_diffuse_filter_tex : hddagi->lightprobe_diffuse_tex),
				RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 6, hddagi->get_lightprobe_occlusion_textures()),
				RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 7, RendererRD::MaterialStorage::get_singleton()->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)),
				RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 8, RendererRD::MaterialStorage::get_singleton()->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 9, hddagi->voxel_disocclusion_tex),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 10, hddagi->voxel_light_neighbour_data),

				RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 12, p_render_buffers->get_depth_texture(v)),
				RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 13, p_normal_roughness_slices[v]),
				RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 14, p_voxel_gi_buffer.is_valid() ? p_voxel_gi_buffer : texture_storage->texture_rd_get_default(RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_BLACK)),

				RD::Uniform(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 15, hddagi_ubo),
				RD::Uniform(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 16, rbgi->get_voxel_gi_buffer()),
				vgiu, // 17
				RD::Uniform(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 18, rbgi->scene_data_ubo),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 19, has_vrs_texture ? p_render_buffers->get_texture_slice(RB_SCOPE_VRS, RB_TEXTURE, v, 0) : texture_storage->texture_rd_get_default(RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_VRS)),

				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 20, p_render_buffers->get_texture_slice(RB_SCOPE_GI, RB_TEX_AMBIENT_U32, v, 0)),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 21, p_render_buffers->get_texture_slice(RB_SCOPE_GI, RB_TEX_REFLECTION_U32, v, 0)),
				RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 22, p_render_buffers->get_texture_slice(RB_SCOPE_GI, RB_TEX_AMBIENT_REFLECTION_BLEND, v, 0))

		);

		RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, pipelines[pipeline_specialization][mode]);
		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);
		RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(PushConstant));

		if (rbgi->using_half_size_gi) {
			RD::get_singleton()->compute_list_dispatch_threads(compute_list, internal_size.x >> 1, internal_size.y >> 1, 1);
		} else {
			RD::get_singleton()->compute_list_dispatch_threads(compute_list, internal_size.x, internal_size.y, 1);
		}
	}

	if (hddagi->using_reflection_filter) {
		uint32_t filter_pipeline_specialization = 0;
		if (rbgi->using_half_size_gi) {
			filter_pipeline_specialization |= FILTER_SHADER_SPECIALIZATION_HALF_RES;
		}
		if (p_view_count > 1) {
			filter_pipeline_specialization |= FILTER_SHADER_SPECIALIZATION_USE_FULL_PROJECTION_MATRIX;
		}

		FilterPushConstant filter_push_constant;
		filter_push_constant.orthogonal = push_constant.orthogonal;
		filter_push_constant.z_near = push_constant.z_near;
		filter_push_constant.z_far = push_constant.z_far;
		filter_push_constant.proj_info[0] = push_constant.proj_info[0];
		filter_push_constant.proj_info[1] = push_constant.proj_info[1];
		filter_push_constant.proj_info[2] = push_constant.proj_info[2];
		filter_push_constant.proj_info[3] = push_constant.proj_info[3];

		RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, filter_pipelines[filter_pipeline_specialization][rbgi->using_half_size_gi ? FILTER_MODE_BILATERAL_HALF_SIZE : FILTER_MODE_BILATERAL]);

		for (int i = 0; i < 2; i++) {
			RD::get_singleton()->compute_list_add_barrier(compute_list);

			filter_push_constant.filter_dir[1] = i & 1;
			filter_push_constant.filter_dir[0] = (i + 1) & 1;

			for (uint32_t v = 0; v < p_view_count; v++) {
				RID uniform_set = UniformSetCacheRD::get_singleton()->get_cache(
						filter_shader.version_get_shader(filter_shader_version, 0),
						0,
						RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 0, p_render_buffers->get_depth_texture(v)),
						RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 1, p_normal_roughness_slices[v]),
						RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 2, p_render_buffers->get_texture_slice(RB_SCOPE_GI, i == 0 ? RB_TEX_REFLECTION : RB_TEX_REFLECTION_FILTERED, v, 0)),
						RD::Uniform(RD::UNIFORM_TYPE_TEXTURE, 3, p_render_buffers->get_texture_slice(RB_SCOPE_GI, i == 0 ? RB_TEX_AMBIENT_REFLECTION_BLEND : RB_TEX_AMBIENT_REFLECTION_BLEND_FILTERED, v, 0)),
						RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 4, p_render_buffers->get_texture_slice(RB_SCOPE_GI, i == 0 ? RB_TEX_REFLECTION_U32_FILTERED : RB_TEX_REFLECTION_U32, v, 0)),
						RD::Uniform(RD::UNIFORM_TYPE_IMAGE, 5, p_render_buffers->get_texture_slice(RB_SCOPE_GI, i == 0 ? RB_TEX_AMBIENT_REFLECTION_BLEND_FILTERED : RB_TEX_AMBIENT_REFLECTION_BLEND, v, 0)),
						RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, 6, RendererRD::MaterialStorage::get_singleton()->sampler_rd_get_default(RS::CANVAS_ITEM_TEXTURE_FILTER_LINEAR, RS::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED)),
						RD::Uniform(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 7, rbgi->scene_data_ubo));

				filter_push_constant.view_index = v;
				RD::get_singleton()->compute_list_set_push_constant(compute_list, &filter_push_constant, sizeof(FilterPushConstant));
				RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);

				if (rbgi->using_half_size_gi) {
					RD::get_singleton()->compute_list_dispatch_threads(compute_list, internal_size.x >> 1, internal_size.y >> 1, 1);
				} else {
					RD::get_singleton()->compute_list_dispatch_threads(compute_list, internal_size.x, internal_size.y, 1);
				}
			}
		}
	}
	RD::get_singleton()->compute_list_end();
	RD::get_singleton()->draw_command_end_label();
}

RID GI::voxel_gi_instance_create(RID p_base) {
	VoxelGIInstance voxel_gi;
	voxel_gi.gi = this;
	voxel_gi.probe = p_base;
	RID rid = voxel_gi_instance_owner.make_rid(voxel_gi);
	return rid;
}

void GI::voxel_gi_instance_free(RID p_rid) {
	GI::VoxelGIInstance *voxel_gi = voxel_gi_instance_owner.get_or_null(p_rid);
	voxel_gi->free_resources();
	voxel_gi_instance_owner.free(p_rid);
}

void GI::voxel_gi_instance_set_transform_to_data(RID p_probe, const Transform3D &p_xform) {
	VoxelGIInstance *voxel_gi = voxel_gi_instance_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->transform = p_xform;
}

bool GI::voxel_gi_needs_update(RID p_probe) const {
	VoxelGIInstance *voxel_gi = voxel_gi_instance_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(voxel_gi, false);

	return voxel_gi->last_probe_version != voxel_gi_get_version(voxel_gi->probe);
}

void GI::voxel_gi_update(RID p_probe, bool p_update_light_instances, const Vector<RID> &p_light_instances, const PagedArray<RenderGeometryInstance *> &p_dynamic_objects) {
	VoxelGIInstance *voxel_gi = voxel_gi_instance_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->update(p_update_light_instances, p_light_instances, p_dynamic_objects);
}

void GI::debug_voxel_gi(RID p_voxel_gi, RD::DrawListID p_draw_list, RID p_framebuffer, const Projection &p_camera_with_transform, bool p_lighting, bool p_emission, float p_alpha) {
	VoxelGIInstance *voxel_gi = voxel_gi_instance_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->debug(p_draw_list, p_framebuffer, p_camera_with_transform, p_lighting, p_emission, p_alpha);
}