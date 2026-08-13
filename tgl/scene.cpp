#include "scene.hpp"
#include "tgl/component/on_delete.hpp"
using namespace tgl;
using namespace std;

tgl::Scene::Scene() : id_(0) {}

tgl::Scene::Scene(Scene &&other)
	: id_(other.id_), inputUpdate(std::move(other.inputUpdate)), mouseMove(std::move(other.mouseMove)), mouseDown(std::move(other.mouseDown)),
	  mouseUp(std::move(other.mouseUp)), update(std::move(other.update)), m_data(std::move(other.m_data)), m_systems(std::move(other.m_systems)),
	  m_active_camera(std::move(other.m_active_camera)), m_next_entity_id(std::move(other.m_next_entity_id)) {
	for (auto &system : m_systems) {
		system->SetScene(this); // update internal pointer
	}
}

ComponentData &tgl::Scene::GetComponentData() { return m_data; }

void tgl::Scene::Update(float dt) {}

EntityInstance tgl::Scene::AddEntity(Entity &&entity) {
	// cout << "Adding entity\n";
	EntityId id = {m_next_entity_id++, id_};
	auto &components = entity.GetComponents();
	// cout << "Got entity components\n";

	for (auto &[type, component] : components) {
		// cout << "TYPE: " << type.name() << "\n";

		auto AddComponent = component_registry[type];
		AddComponent(m_data, id, component.get());
	}
	// cout << "Entity instance done?: " << "\n";

	return EntityInstance(id);
}

void tgl::Scene::DeleteEntity(EntityInstance i) { m_delete_queue.push(i); }

void tgl::Scene::FlushDeleteQueue() {
	while (m_delete_queue.size()) {
		auto instance = m_delete_queue.front();
		if (auto del = instance.Get<DeleteListener>()) {
			del->OnDelete().Update();
		}
		m_delete_queue.pop();
		m_data.RemoveEntityData(instance.GetId());
	}
}
