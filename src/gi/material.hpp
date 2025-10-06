#ifndef GI_BAH8454_MATERIAL
#define GI_BAH8454_MATERIAL

#include "util.hpp"

class MaterialInfo {
public:
	Vector3 position;
	Vector3 normal;
	Vector3 light_position;
	Vector3 light_color;
};

template <typename ObjectType>
class Material {
public:
	// static Vector3 default_shadow_shader(const ObjectType& object, const MaterialInfo& info) { return Vector3{ 0, 0, 0 }; } // TODO: Default shadow shader.

	//using ShaderType = std::function<Vector3(const ObjectType&, const MaterialInfo&)>;
	//using ShaderType = Vector3(*)(const ObjectType&, const MaterialInfo&);

	Material() {}

	Material(
		Real reflection_constant,
		Real transmission_constant,
		Real medium_index
		//const ShaderType shader,
		//const ShaderType shadow_shader
	) :
		reflection_constant{ reflection_constant },
		transmission_constant{ transmission_constant },
		medium_index{ medium_index }
		//shader{ shader },
		//shadow_shader{ shadow_shader }
	{}

	Real reflection_constant = 0;
	Real transmission_constant = 0;
	Real medium_index = 1;

	//ShaderType shader;
	//ShaderType shadow_shader;
};

namespace shader {

inline Vector3 phong(
	const MaterialInfo& info,
	const Vector3& color,
	const Vector3& specular_color,
	Real ambient_constant,
	Real diffuse_constant,
	Real specular_constant,
	Real shininess
) {
	// Calculate the ambient color.
	Vector3 ambient_color = color.cwiseProduct(info.light_color);
	// Set up the diffuse calculations.
	Vector3 l = (info.light_position - info.position).normalized();
	Real n_dot_l = std::max(info.normal.dot(l), 0_r);
	// Calculate the diffuse color.
	Vector3 diffuse_color = color.cwiseProduct(info.light_color) * n_dot_l;
	// Set up the specular calculations.
	Vector3 r = reflect(-l, info.normal).normalized();
	Real r_dot_v = std::max(r.dot(-info.position.normalized()), 0_r);
	// Calculate the specular color.
	Vector3 resultant_specular_color = specular_color.cwiseProduct(info.light_color) * pow(r_dot_v, shininess);
	// Return the final color.
	return ((specular_constant * resultant_specular_color)
		+ (diffuse_constant * diffuse_color)
		+ (ambient_constant * ambient_color)).eval();
}

inline Vector3 phong_shadow(const MaterialInfo& info, const Vector3& color, Real ambient_constant) {
	return (ambient_constant * color.cwiseProduct(info.light_color)).eval();
}

};

#endif