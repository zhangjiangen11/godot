// Copyright © 2024 Cory Petkovsek, Roope Palmroos, and Contributors.

#ifndef GENERATEDTEXTURE_CLASS_H
#define GENERATEDTEXTURE_CLASS_H
#include "core/io/image.h"

#include "constants.h"

using namespace godot;

class GeneratedTexture {
	CLASS_NAME_STATIC("Terrain3DGenTex");

private:
	RID _rid = RID();
	Ref<Image> _image;
	bool _dirty = false;

public:
	void clear();
	bool is_dirty() const { return _dirty; }
	RID create(const TypedArray<Image> &p_layers);
	RID create(const Ref<Image> &p_image);
	Ref<Image> get_image() const { return _image; }
	RID get_rid() const { return _rid; }
};

#endif // GENERATEDTEXTURE_CLASS_H