#pragma once
#include "../utilities/GeometryUtilities.hpp"
#include "../utilities/RandomGenerator.hpp"
#include "core/object/ref_counted.h"
#include <vector>

namespace Mtree {

struct Property {
	virtual float execute(float x) = 0;
};

struct ConstantProperty : Property {
	float value;

	ConstantProperty(float value = 1) :
			value(value) {}

	float execute(float x) override {
		return value;
	}
};

struct RandomProperty : Property {
	RandomGenerator rand_gen;
	float min_value;
	float max_value;

	RandomProperty(float min = 0, float max = 1) :
			min_value(min), max_value(max) {}

	float execute(float x) override {
		return Geometry::lerp(min_value, max_value, rand_gen.get_0_1());
	}
};

struct SimpleCurveProperty : Property {
	float x_min = 0;
	float x_max = 1;
	float y_min = 0;
	float y_max = 1;
	float power = 1;

	SimpleCurveProperty(float x_min = 0, float x_max = 1, float y_min = 0, float y_max = 1, float power = 1) :
			x_min(x_min), x_max(x_max), y_min(y_min), y_max(y_max), power(power) {}

	float execute(float x) override {
		float factor = std::clamp((x - x_min) / std::max(0.001f, (x_max - x_min)), 0.f, 1.f);
		if (power > 0 && power != 1) {
			factor = std::pow(factor, power);
		}
		return Geometry::lerp(y_min, y_max, factor);
	}
};

class PropertyWrapper : RefCounted {
	GDCLASS(PropertyWrapper, RefCounted);

public:
	enum class Type {
		CONSTANT,
		RANDOM,
		CURVE
	};
	Type type = Type::CONSTANT;
	std::shared_ptr<Property> property;
	std::shared_ptr<Property> random_property;
	std::shared_ptr<Property> curve_property;

	PropertyWrapper() { property = std::make_shared<ConstantProperty>(1); }

	template <class T>
	PropertyWrapper(T &property) {
		this->property = std::make_shared<T>(property);
	}

	template <class T>
	PropertyWrapper(T &&property) {
		this->property = std::make_shared<T>(property);
	}

	template <class T>
	void set_property(T &property) {
		this->property = std::make_shared<T>(property);
	}

	float execute(float x) {
		return property->execute(x);
	}
};
} //namespace Mtree