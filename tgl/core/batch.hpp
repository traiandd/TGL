#include "core/gpu/mesh.h"
#include "core/gpu/shader.h"
#include "tgl/archetype.hpp"
#include "tgl/component/3d_transform.hpp"
#include <vector>

// TODO: add texture support

struct BatchIdentifier {
	Mesh *mesh;
	Shader *shader;

	bool operator==(const BatchIdentifier &other) const { return mesh == other.mesh && shader == other.shader; }
};

namespace std {
template<> struct hash<BatchIdentifier> {
	size_t operator()(const BatchIdentifier &k) const { return ((std::hash<Mesh *>()(k.mesh) ^ (std::hash<Shader *>()(k.shader) << 1)) >> 1); }
};
} // namespace std

struct StaticBatch {
	Mesh *mesh;
	Shader *shader;
	std::vector<glm::mat4> modelMatrices;

	StaticBatch(Mesh *mesh, Shader *shader, std::vector<tgl::Archetype<Transform3dComponent>> transforms) : mesh(mesh), shader(shader) {
		modelMatrices.resize(transforms.size());
		for (int i = 0; i < transforms.size(); i++) {
			modelMatrices[i] = transforms[i].GlobalTransform();
		}
		unsigned int buffer;
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glBufferData(GL_ARRAY_BUFFER, transforms.size() * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

		unsigned int VAO = mesh->GetVAO();
		glBindVertexArray(VAO);
		// vertex attributes
		std::size_t vec4Size = sizeof(glm::vec4);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void *)0);
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void *)(1 * vec4Size));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void *)(2 * vec4Size));
		glEnableVertexAttribArray(7);
		glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void *)(3 * vec4Size));

		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);
		glVertexAttribDivisor(7, 1);

		glBindVertexArray(0);
	}

	void BindTexture() {
		Texture2D *texture = mesh->GetTexture();
		if (texture) {
			texture->Bind();
		}
	}

	void Render() {
		BindTexture();
		glBindVertexArray(mesh->GetVAO());
		glDrawElementsInstanced(GL_TRIANGLES, mesh->IndicesCount(), GL_UNSIGNED_INT, 0, modelMatrices.size());
	}
};