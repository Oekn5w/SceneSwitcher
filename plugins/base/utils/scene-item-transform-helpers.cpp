#include "scene-item-transform-helpers.hpp"

namespace advss {

static struct vec2 getSceneItemSize(obs_scene_item *item,
				    obs_sceneitem_crop *crop = nullptr,
				    obs_transform_info *info = nullptr)
{
	obs_sceneitem_crop instCrop;
	obs_transform_info instInfo;
	if (!crop) {
		obs_sceneitem_get_crop(item, &instCrop);
		crop = &instCrop;
	}
	if (!info) {
#if (LIBOBS_API_VER >= MAKE_SEMANTIC_VERSION(30, 1, 0))
		obs_sceneitem_get_info2(item, &instInfo);
#else
		obs_sceneitem_get_info(item, &instInfo);
#endif
		info = &instInfo;
	}

	struct vec2 size, scaledSize;

	obs_source_t *source = obs_sceneitem_get_source(item);
	size.x = float(obs_source_get_width(source));
	size.y = float(obs_source_get_height(source));
	scaledSize.x = fmaxf(0.0f, roundf((size.x - (float)(crop->left) -
					   (float)(crop->right)) *
					  info->scale.x));
	scaledSize.y = fmaxf(0.0f, roundf((size.y - (float)(crop->top) -
					   (float)(crop->bottom)) *
					  info->scale.y));
	return scaledSize;
}

static struct vec2 getAlignedCoordsExtrema(const float &position,
					   const float &dimension,
					   float alignmentNumber)
{
	static_assert(
		(OBS_ALIGN_CENTER == 0x00) && (OBS_ALIGN_LEFT == 0x01) &&
			(OBS_ALIGN_RIGHT == 0x02) &&
			(OBS_ALIGN_TOP == (0x01 << 2)) &&
			(OBS_ALIGN_BOTTOM == (0x02 << 2)),
		"Quadratic coefficents set for calculating the resulting boundaries "
		"of transformations do not match OBS alignment values! Update the "
		"coefficents");

	struct vec2 extrema;
	float factorDim;
	constexpr float opMinSquare = -0.75f;
	constexpr float opMinLinear = 1.25f;
	constexpr float opMinConst = -0.5f;
	factorDim = opMinSquare * alignmentNumber * alignmentNumber +
		    opMinLinear * alignmentNumber + opMinConst;
	extrema.ptr[0] = roundf(position + factorDim * dimension);
	constexpr float opMaxSquare = -0.75f;
	constexpr float opMaxLinear = 1.25f;
	constexpr float opMaxConst = 0.5f;
	factorDim = opMaxSquare * alignmentNumber * alignmentNumber +
		    opMaxLinear * alignmentNumber + opMaxConst;
	extrema.ptr[1] = roundf(position + factorDim * dimension);
	return extrema;
}

std::string GetSceneItemTransform(obs_scene_item *item)
{
	struct obs_transform_info info;
	struct obs_sceneitem_crop crop;
#if (LIBOBS_API_VER >= MAKE_SEMANTIC_VERSION(30, 1, 0))
	obs_sceneitem_get_info2(item, &info);
#else
	obs_sceneitem_get_info(item, &info);
#endif
	obs_sceneitem_get_crop(item, &crop);
	auto scaledSize = getSceneItemSize(item, &crop, &info);

	auto data = obs_data_create();
	SaveTransformState(data, info, crop);
	obs_data_t *obj = obs_data_create();
	obs_data_set_double(obj, "width", scaledSize.x);
	obs_data_set_double(obj, "height", scaledSize.y);
	obs_data_set_obj(data, "size", obj);
	obs_data_release(obj);
	auto json = std::string(obs_data_get_json(data));
	obs_data_release(data);
	return json;
}

std::string GetSceneItemNoRotBounds(obs_scene_item *item)
{
	struct obs_transform_info info;
	struct obs_sceneitem_crop crop;
#if (LIBOBS_API_VER >= MAKE_SEMANTIC_VERSION(30, 1, 0))
	obs_sceneitem_get_info2(item, &info);
#else
	obs_sceneitem_get_info(item, &info);
#endif
	obs_sceneitem_get_crop(item, &crop);
	auto scaledSize = getSceneItemSize(item, &crop, &info);

	struct vec2 extrema;
	obs_data_t *data = obs_data_create();
	// Y direction
	extrema =
		getAlignedCoordsExtrema(info.pos.y, scaledSize.y,
					(float)((info.alignment >> 2) & 0x03));
	obs_data_set_double(data, "top", extrema.ptr[0]);
	obs_data_set_double(data, "bottom", extrema.ptr[1]);
	// X direction
	extrema = getAlignedCoordsExtrema(info.pos.x, scaledSize.x,
					  (float)(info.alignment & 0x03));
	obs_data_set_double(data, "left", extrema.ptr[0]);
	obs_data_set_double(data, "right", extrema.ptr[1]);
	auto json = std::string(obs_data_get_json(data));
	obs_data_release(data);
	return json;
}

void LoadTransformState(obs_data_t *obj, struct obs_transform_info &info,
			struct obs_sceneitem_crop &crop)
{
	obs_data_get_vec2(obj, "pos", &info.pos);
	obs_data_get_vec2(obj, "scale", &info.scale);
	info.rot = (float)obs_data_get_double(obj, "rot");
	info.alignment = (uint32_t)obs_data_get_int(obj, "alignment");
	info.bounds_type =
		(enum obs_bounds_type)obs_data_get_int(obj, "bounds_type");
	info.bounds_alignment =
		(uint32_t)obs_data_get_int(obj, "bounds_alignment");
	obs_data_get_vec2(obj, "bounds", &info.bounds);
	crop.top = (int)obs_data_get_int(obj, "top");
	crop.bottom = (int)obs_data_get_int(obj, "bottom");
	crop.left = (int)obs_data_get_int(obj, "left");
	crop.right = (int)obs_data_get_int(obj, "right");
}

bool SaveTransformState(obs_data_t *obj, const struct obs_transform_info &info,
			const struct obs_sceneitem_crop &crop)
{
	struct vec2 pos = info.pos;
	struct vec2 scale = info.scale;
	float rot = info.rot;
	uint32_t alignment = info.alignment;
	uint32_t bounds_type = info.bounds_type;
	uint32_t bounds_alignment = info.bounds_alignment;
	struct vec2 bounds = info.bounds;

	obs_data_set_vec2(obj, "pos", &pos);
	obs_data_set_vec2(obj, "scale", &scale);
	obs_data_set_double(obj, "rot", rot);
	obs_data_set_int(obj, "alignment", alignment);
	obs_data_set_int(obj, "bounds_type", bounds_type);
	obs_data_set_vec2(obj, "bounds", &bounds);
	obs_data_set_int(obj, "bounds_alignment", bounds_alignment);
	obs_data_set_int(obj, "top", crop.top);
	obs_data_set_int(obj, "bottom", crop.bottom);
	obs_data_set_int(obj, "left", crop.left);
	obs_data_set_int(obj, "right", crop.right);

	return true;
}

} // namespace advss
