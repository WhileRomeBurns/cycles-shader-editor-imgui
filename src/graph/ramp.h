#pragma once

#include <cassert>
#include <cstddef>
#include <vector>

#include "shader_core/vector.h"

namespace csg {
	struct ColorRampPoint {
	public:
		ColorRampPoint(float pos, csc::Float3 color, float alpha) : pos{ pos }, color{ color }, alpha{ alpha } {}

		float pos;
		csc::Float3 color;
		float alpha;
	};

	class ColorRamp {
	public:
		ColorRamp();

		size_t size() const { return points.size(); }

		ColorRampPoint get(size_t index) const {
			assert(index < points.size());
			return points[index];
		}

		void set(size_t index, ColorRampPoint new_point);

		bool similar(const ColorRamp& other, float margin) const;

	private:
		void sort_points();

		std::vector<ColorRampPoint> points;
	};
}