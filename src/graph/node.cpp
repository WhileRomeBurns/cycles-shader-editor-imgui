#include "node.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <random>

#include "node_enums.h"

static std::mutex node_id_rng_mutex;
static std::mt19937 node_id_rng;

static const float M_PI{ acos(-1.0f) };

csg::Node::Node(const NodeType type, const csc::Int2 position) : position{ position }, _type{ type }
{
	switch (type) {
		//////
		// Output
		//////
	case NodeType::MATERIAL_OUTPUT:
		_slots.push_back(Slot{ "Surface",      "surface",      SlotDirection::INPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Volume",       "volume",       SlotDirection::INPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Displacement", "displacement", SlotDirection::INPUT, SlotType::VECTOR });
		break;
		//////
		// Color
		//////
	case NodeType::RGB_CURVES:
		_slots.push_back(Slot{ "Color",  "color",  SlotDirection::OUTPUT, SlotType::COLOR });
		_slots.push_back(Slot{ "Curves", "curves", RGBCurveSlotValue{} });
		_slots.push_back(Slot{ "Fac",    "fac",    FloatSlotValue{ 1.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Color",  "color",  ColorSlotValue{ csc::Float3{ 0.0f, 0.0f, 0.0f} } });
		break;
		//////
		// Shader
		//////
	case NodeType::ADD_SHADER:
		_slots.push_back(Slot{ "Closure",  "closure",  SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Closure1", "closure1", SlotDirection::INPUT,  SlotType::CLOSURE });
		_slots.push_back(Slot{ "Closure2", "closure2", SlotDirection::INPUT,  SlotType::CLOSURE });
		break;
	case NodeType::ANISOTROPIC_BSDF:
		_slots.push_back(Slot{ "BSDF",         "BSDF",         SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Distribution", "distribution", EnumSlotValue{ AnisotropicDistribution::GGX } });
		_slots.push_back(Slot{ "Color",        "color",        ColorSlotValue{ csc::Float3{ 0.9f, 0.9f, 0.9f} } });
		_slots.push_back(Slot{ "Roughness",    "roughness",    FloatSlotValue{ 0.0f,  0.0f, 1.0f } });
		_slots.push_back(Slot{ "Anisotropy",   "anisotropy",   FloatSlotValue{ 0.5f, -1.0f, 1.0f } });
		_slots.push_back(Slot{ "Rotation",     "rotation",     FloatSlotValue{ 0.0f,  0.0f, 1.0f } });
		_slots.push_back(Slot{ "Normal",       "normal",       SlotDirection::INPUT, SlotType::VECTOR });
		_slots.push_back(Slot{ "Tangent",      "tangent",      SlotDirection::INPUT, SlotType::VECTOR });
		break;
	case NodeType::DIFFUSE_BSDF:
		_slots.push_back(Slot{ "BSDF",      "BSDF",      SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Color",     "color",     ColorSlotValue{ csc::Float3{ 0.9f, 0.9f, 0.9f} } });
		_slots.push_back(Slot{ "Roughness", "roughness", FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Normal",    "normal",    SlotDirection::INPUT, SlotType::VECTOR });
		break;
	case NodeType::EMISSION:
		_slots.push_back(Slot{ "Emission", "emission", SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Color",    "color",    ColorSlotValue{ csc::Float3{ 0.9f, 0.9f, 0.9f} } });
		_slots.push_back(Slot{ "Strength", "strength", FloatSlotValue{ 0.0f, 0.0f, FLT_MAX } });
		break;
	case NodeType::GLASS_BSDF:
		_slots.push_back(Slot{ "BSDF",         "BSDF",         SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Distribution", "distribution", EnumSlotValue{ GlassDistribution::GGX } });
		_slots.push_back(Slot{ "Color",        "color",        ColorSlotValue{ csc::Float3{ 0.9f, 0.9f, 0.9f} } });
		_slots.push_back(Slot{ "Roughness",    "roughness",    FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "IOR",          "ior",          FloatSlotValue{ 1.45f, 0.0f, 100.0f } });
		_slots.push_back(Slot{ "Normal",       "normal",       SlotDirection::INPUT, SlotType::VECTOR });
		break;
	case NodeType::GLOSSY_BSDF:
		_slots.push_back(Slot{ "BSDF",         "BSDF",         SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Distribution", "distribution", EnumSlotValue{ GlossyDistribution::GGX } });
		_slots.push_back(Slot{ "Color",        "color",        ColorSlotValue{ csc::Float3{ 0.9f, 0.9f, 0.9f} } });
		_slots.push_back(Slot{ "Roughness",    "roughness",    FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Normal",       "normal",       SlotDirection::INPUT, SlotType::VECTOR });
		break;
	case NodeType::HAIR_BSDF:
		_slots.push_back(Slot{ "BSDF",       "BSDF",        SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Component",  "component",   EnumSlotValue{ HairComponent::REFLECTION } });
		_slots.push_back(Slot{ "Color",      "color",       ColorSlotValue{ csc::Float3{ 0.9f, 0.9f, 0.9f} } });
		_slots.push_back(Slot{ "Offset",     "offset",      FloatSlotValue{ 0.0f, -90.0f, 90.0f, 2 } });
		_slots.push_back(Slot{ "RoughnessU", "roughness_u", FloatSlotValue{ 0.1f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "RoughnessV", "roughness_v", FloatSlotValue{ 1.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Tangent",    "tangent",     SlotDirection::INPUT, SlotType::VECTOR });
		break;
	case NodeType::HOLDOUT:
		_slots.push_back(Slot{ "Holdout", "holdout", SlotDirection::OUTPUT, SlotType::CLOSURE });
		break;
	case NodeType::MIX_SHADER:
		_slots.push_back(Slot{ "Closure",  "closure",  SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Fac",      "fac",      FloatSlotValue{ 0.5f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Closure1", "closure1", SlotDirection::INPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Closure2", "closure2", SlotDirection::INPUT, SlotType::CLOSURE });
		break;
	case NodeType::PRINCIPLED_BSDF:
		_slots.push_back(Slot{ "BSDF",                "BSDF",                 SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Distribution",        "distribution",         EnumSlotValue{ PrincipledBSDFDistribution::GGX } });
		_slots.push_back(Slot{ "Base Color",          "base_color",           ColorSlotValue{ csc::Float3{ 0.8f, 0.8f, 0.8f} } });
		_slots.push_back(Slot{ "Subsurface Method",   "subsurface_method",    EnumSlotValue{ PrincipledBSDFSubsurfaceMethod::BURLEY } });
		_slots.push_back(Slot{ "Subsurface",          "subsurface",           FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Subsurface Radius",   "subsurface_radius",    VectorSlotValue{
			csc::Float3{ 1.0f, 0.2f, 0.1f }, csc::Float3{ 0.0f, 0.0f, 0.0f } , csc::Float3{ FLT_MAX, FLT_MAX, FLT_MAX }
		}});
		_slots.push_back(Slot{ "Subsurface Color",    "subsurface_color",     ColorSlotValue{ csc::Float3{ 0.7f, 1.0f, 1.0f} } });
		_slots.push_back(Slot{ "Metallic",            "metallic",             FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Specular",            "specular",             FloatSlotValue{ 0.5f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Specular Tint",       "specular_tint",        FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Roughness",           "roughness",            FloatSlotValue{ 0.5f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Anisotropic",         "anisotropic",          FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Rotation",            "anisotropic_rotation", FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Sheen",               "sheen",                FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Sheen Tint",          "sheen_tint",           FloatSlotValue{ 0.5f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Clearcoat",           "clearcoat",            FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Clearcoat Roughness", "clearcoat_roughness",  FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "IOR",                 "ior",                  FloatSlotValue{ 1.45f, 0.0f, 100.0f } });
		_slots.push_back(Slot{ "Transmission",        "transmission",         FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Emission",            "emission",             ColorSlotValue{ csc::Float3{ 0.0f, 0.0f, 0.0f } } });
		_slots.push_back(Slot{ "Alpha",               "alpha",                FloatSlotValue{ 1.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Normal",              "normal",               SlotDirection::INPUT, SlotType::VECTOR });
		_slots.push_back(Slot{ "Clearcoat Normal",    "clearcoat_normal",     SlotDirection::INPUT, SlotType::VECTOR });
		_slots.push_back(Slot{ "Tangent",             "tangent",              SlotDirection::INPUT, SlotType::VECTOR });
		break;
	case NodeType::PRINCIPLED_HAIR:
		_slots.push_back(Slot{ "BSDF",                   "BSDF",                   SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Coloring",               "coloring",               EnumSlotValue{ PrincipledHairColoring::DIRECT_COLORING } });
		_slots.push_back(Slot{ "Color",                  "color",                  ColorSlotValue{ csc::Float3{ 0.017513f, 0.005763f, 0.002059f } } });
		_slots.push_back(Slot{ "Melanin",                "melanin",                FloatSlotValue{ 0.8f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Melanin Redness",        "melanin_redness",        FloatSlotValue{ 1.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Tint",                   "tint",                   ColorSlotValue{ csc::Float3{ 1.0f, 1.0f, 1.0f } } });
		_slots.push_back(Slot{ "Absorption Coefficient", "absorption_coefficient", VectorSlotValue{
			csc::Float3{ 0.245531f, 0.52f, 1.365f }, csc::Float3{ 0.0f, 0.0f, 0.0f } , csc::Float3{ FLT_MAX, FLT_MAX, FLT_MAX }
		} });
		_slots.push_back(Slot{ "Roughness",              "roughness",              FloatSlotValue{ 0.3f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Radial Roughness",       "radial_roughness",       FloatSlotValue{ 0.3f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Coat",                   "coat",                   FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "IOR",                    "ior",                    FloatSlotValue{ 1.55f, 0.0f, 1000.0f } });
		_slots.push_back(Slot{ "Offset (rad)",           "offset",                 FloatSlotValue{ 2 * M_PI / 180.0f, M_PI / -2.0f , M_PI / 2.0f } });
		_slots.push_back(Slot{ "Random Roughness",       "random_roughness",       FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Random Color",           "random_color",           FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Random",                 "random",                 FloatSlotValue{ 0.0f, 0.0f, FLT_MAX } });
		break;
	case NodeType::PRINCIPLED_VOLUME:
		_slots.push_back(Slot{ "Volume",              "volume",              SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Color",               "color",               ColorSlotValue{ csc::Float3{ 0.5f, 0.5f, 0.5f } } });
		_slots.push_back(Slot{ "Density",             "density",             FloatSlotValue{ 1.0f, 0.0f, FLT_MAX } });
		_slots.push_back(Slot{ "Anisotropy",          "anisotropy",          FloatSlotValue{ 0.0f, -1.0f, 1.0f } });
		_slots.push_back(Slot{ "Absorption Color",    "absorption_color",    ColorSlotValue{ csc::Float3{ 0.0f, 0.0f, 0.0f } } });
		_slots.push_back(Slot{ "Emission Strength",   "emission_strength",   FloatSlotValue{ 0.0f, 0.0f, FLT_MAX } });
		_slots.push_back(Slot{ "Emission Color",      "emission_color",      ColorSlotValue{ csc::Float3{ 1.0f, 1.0f, 1.0f } } });
		_slots.push_back(Slot{ "Blackbody Intensity", "blackbody_intensity", FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Blackbody Tint",      "blackbody_tint",      ColorSlotValue{ csc::Float3{ 1.0f, 1.0f, 1.0f } } });
		_slots.push_back(Slot{ "Temperature",         "temperature",         FloatSlotValue{ 1000.0f, 0.0f, 8000.0f } });
		break;
	case NodeType::REFRACTION_BSDF:
		_slots.push_back(Slot{ "BSDF",         "BSDF",         SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Distribution", "distribution", EnumSlotValue{ RefractionDistribution::GGX } });
		_slots.push_back(Slot{ "Color",        "color",        ColorSlotValue{ csc::Float3{ 0.9f, 0.9f, 0.9f} } });
		_slots.push_back(Slot{ "Roughness",    "roughness",    FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "IOR",          "ior",          FloatSlotValue{ 1.45f, 0.0f, 100.0f } });
		_slots.push_back(Slot{ "Normal",       "normal",       SlotDirection::INPUT, SlotType::VECTOR });
		break;
	case NodeType::SUBSURFACE_SCATTER:
		_slots.push_back(Slot{ "BSSRDF",       "BSSRDF",       SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Falloff",      "falloff",      EnumSlotValue{ SubsurfaceScatterFalloff::BURLEY } });
		_slots.push_back(Slot{ "Color",        "color",        ColorSlotValue{ csc::Float3{ 0.9f, 0.9f, 0.9f} } });
		_slots.push_back(Slot{ "Scale",        "scale",        FloatSlotValue{ 1.0f, 0.0f, FLT_MAX } });
		_slots.push_back(Slot{ "Radius",       "radius",       VectorSlotValue{
			csc::Float3{ 1.0f, 1.0f, 1.0f }, csc::Float3{ 0.0f, 0.0f, 0.0f } , csc::Float3{ FLT_MAX, FLT_MAX, FLT_MAX }
		} });
		_slots.push_back(Slot{ "Sharpness",    "sharpness",    FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Texture Blur", "texture_blur", FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Normal",       "normal",       SlotDirection::INPUT, SlotType::VECTOR });
		break;
	case NodeType::TOON_BSDF:
		_slots.push_back(Slot{ "BSDF",      "BSDF",      SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Component", "component", EnumSlotValue{ ToonComponent::DIFFUSE } });
		_slots.push_back(Slot{ "Color",     "color",     ColorSlotValue{ csc::Float3{ 0.9f, 0.9f, 0.9f} } });
		_slots.push_back(Slot{ "Size",      "size",      FloatSlotValue{ 0.5f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Smooth",    "smooth",    FloatSlotValue{ 0.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Normal",    "normal",    SlotDirection::INPUT, SlotType::VECTOR });
		break;
	case NodeType::TRANSLUCENT_BSDF:
		_slots.push_back(Slot{ "BSDF",   "BSDF",   SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Color",  "color",  ColorSlotValue{ csc::Float3{ 0.9f, 0.9f, 0.9f} } });
		_slots.push_back(Slot{ "Normal", "normal", SlotDirection::INPUT, SlotType::VECTOR });
		break;
	case NodeType::TRANSPARENT_BSDF:
		_slots.push_back(Slot{ "BSDF",  "BSDF",  SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Color", "color", ColorSlotValue{ csc::Float3{ 0.9f, 0.9f, 0.9f} } });
		break;
	case NodeType::VELVET_BSDF:
		_slots.push_back(Slot{ "BSDF",   "BSDF",   SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Color",  "color",  ColorSlotValue{ csc::Float3{ 0.9f, 0.9f, 0.9f} } });
		_slots.push_back(Slot{ "Sigma",  "sigma",  FloatSlotValue{ 1.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Normal", "normal", SlotDirection::INPUT, SlotType::VECTOR });
		break;
	case NodeType::VOL_ABSORPTION:
		_slots.push_back(Slot{ "Volume",  "volume",  SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Color",   "color",   ColorSlotValue{ csc::Float3{ 0.9f, 0.9f, 0.9f} } });
		_slots.push_back(Slot{ "Density", "density", FloatSlotValue{ 1.0f, 0.0f, FLT_MAX } });
		break;
	case NodeType::VOL_SCATTER:
		_slots.push_back(Slot{ "Volume",     "volume",     SlotDirection::OUTPUT, SlotType::CLOSURE });
		_slots.push_back(Slot{ "Color",      "color",      ColorSlotValue{ csc::Float3{ 0.9f, 0.9f, 0.9f} } });
		_slots.push_back(Slot{ "Density",    "density",    FloatSlotValue{ 1.0f, 0.0f, FLT_MAX } });
		_slots.push_back(Slot{ "Anisotropy", "anisotropy", FloatSlotValue{ 0.0f, -1.0f, 1.0f } });
		break;
		//////
		// Vector
		//////
	case NodeType::BUMP:
		_slots.push_back(Slot{ "Normal",   "normal",   SlotDirection::OUTPUT, SlotType::VECTOR });
		_slots.push_back(Slot{ "Invert",   "invert",   BoolSlotValue{ false } });
		_slots.push_back(Slot{ "Strength", "strength", FloatSlotValue{ 1.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Distance", "distance", FloatSlotValue{ 1.0f, 0.0f, FLT_MAX } });
		_slots.push_back(Slot{ "Height",   "height",   SlotDirection::INPUT, SlotType::FLOAT });
		_slots.push_back(Slot{ "Normal",   "normal",   SlotDirection::INPUT, SlotType::VECTOR });
		break;
	case NodeType::DISPLACEMENT:
		_slots.push_back(Slot{ "Displacement", "displacement", SlotDirection::OUTPUT, SlotType::VECTOR });
		_slots.push_back(Slot{ "Space",        "space",        EnumSlotValue{ DisplacementSpace::OBJECT } });
		_slots.push_back(Slot{ "Height",       "height",       FloatSlotValue{ 0.0f, 0.0f, FLT_MAX } });
		_slots.push_back(Slot{ "Midlevel",     "midlevel",     FloatSlotValue{ 0.5f, 0.0f, FLT_MAX } });
		_slots.push_back(Slot{ "Scale",        "scale",        FloatSlotValue{ 1.0f, 0.0f, FLT_MAX } });
		_slots.push_back(Slot{ "Normal",       "normal",       SlotDirection::INPUT, SlotType::VECTOR });
		break;
	case NodeType::NORMAL_MAP:
		_slots.push_back(Slot{ "Normal",   "normal",   SlotDirection::OUTPUT, SlotType::VECTOR });
		_slots.push_back(Slot{ "Space",    "space",    EnumSlotValue{ NormalMapSpace::TANGENT } });
		_slots.push_back(Slot{ "Strength", "strength", FloatSlotValue{ 1.0f, 0.0f, 10.0f } });
		_slots.push_back(Slot{ "Color",    "color",    ColorSlotValue{ csc::Float3{ 0.5f, 0.5f, 1.0f} } });
		break;
	case NodeType::VECTOR_CURVES:
		_slots.push_back(Slot{ "Vector", "vector", SlotDirection::OUTPUT, SlotType::VECTOR });
		_slots.push_back(Slot{ "Curves", "curves", VectorCurveSlotValue{ csc::Float2{ -1.0f, -1.0f }, csc::Float2{ 1.0f, 1.0f} } });
		_slots.push_back(Slot{ "Fac",    "fac",    FloatSlotValue{ 1.0f, 0.0f, 1.0f } });
		_slots.push_back(Slot{ "Vector", "vector", VectorSlotValue{
			csc::Float3{ 0.0f, 0.0f, 0.0f }, csc::Float3{ -FLT_MAX, -FLT_MAX, -FLT_MAX } , csc::Float3{ FLT_MAX, FLT_MAX, FLT_MAX }
		} });
		break;
	case NodeType::VECTOR_DISPLACEMENT:
		_slots.push_back(Slot{ "Displacement", "displacement", SlotDirection::OUTPUT, SlotType::VECTOR });
		_slots.push_back(Slot{ "Space",        "space",        EnumSlotValue{ VectorDisplacementSpace::TANGENT } });
		_slots.push_back(Slot{ "Vector",       "vector",       SlotDirection::INPUT, SlotType::COLOR });
		_slots.push_back(Slot{ "Midlevel",     "midlevel",     FloatSlotValue{ 0.0f, 0.0f, FLT_MAX } });
		_slots.push_back(Slot{ "Scale",        "scale",        FloatSlotValue{ 1.0f, 0.0f, FLT_MAX } });
		break;
	case NodeType::VECTOR_TRANSFORM:
		_slots.push_back(Slot{ "Vector",       "vector", SlotDirection::OUTPUT, SlotType::VECTOR });
		_slots.push_back(Slot{ "Type",         "type", EnumSlotValue{ VectorTransformType::VECTOR } });
		_slots.push_back(Slot{ "Convert From", "convert_from", EnumSlotValue{ VectorTransformSpace::WORLD } });
		_slots.push_back(Slot{ "Convert To",   "convert_to", EnumSlotValue{ VectorTransformSpace::OBJECT } });
		_slots.push_back(Slot{ "Vector",       "vector", VectorSlotValue{
			csc::Float3{ 1.0f, 1.0f, 1.0f }, csc::Float3{ -FLT_MAX, -FLT_MAX, -FLT_MAX } , csc::Float3{ FLT_MAX, FLT_MAX, FLT_MAX }
		} });
		break;
	default:
		// Uncomment the below assert once all node types have been implemented
		//assert(false);
		;
	}
	roll_id();
}

csg::Node::Node(const NodeType type, const csc::Int2 position, const NodeId id) :
	Node(type, position)
{
	_id = id;
}

boost::optional<size_t> csg::Node::slot_index(const SlotDirection dir, const std::string& slot_name) const
{
	for (size_t i = 0; i < _slots.size(); i++)
	{
		if (_slots[i].dir() == dir && _slots[i].name() == slot_name) {
			return i;
		}
	}
	return boost::none;
}

boost::optional<csg::Slot> csg::Node::slot(const size_t index) const
{
	if (index >= _slots.size()) {
		return boost::none;
	}
	return _slots.at(index);
}

void csg::Node::copy_from(const Node& other)
{
	// Copy everything except id
	_type = other._type;
	_slots = other._slots;
}

bool csg::Node::operator==(const Node& other) const
{
	if (id() != other.id()) {
		return false;
	}

	if (type() != other.type()) {
		return false;
	}

	if (position != other.position) {
		return false;
	}

	if (_slots.size() != other._slots.size()) {
		return false;
	}

	for (size_t i = 0; i < _slots.size(); i++) {
		if (_slots[i] != other._slots[i]) {
			return false;
		}
	}

	return true;
}

csg::NodeId csg::Node::roll_id()
{
	static_assert(sizeof(uint32_t) * 2 == sizeof(NodeId), "NodeId should be twice the size of uint32_t");
	uint32_t* const id_ptr{ reinterpret_cast<uint32_t*>(&_id) };
	std::lock_guard<std::mutex> lock{ node_id_rng_mutex };
	id_ptr[0] = node_id_rng();
	id_ptr[1] = node_id_rng();
	return _id;
}