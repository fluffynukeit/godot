
#include "particle_physics_server.h"

#include "core/method_bind_ext.gen.inc"

void ParticleBodyCommands::_bind_methods() {

	ClassDB::bind_method(D_METHOD("load_model", "shape", "initial_transform"), &ParticleBodyCommands::load_model);

	ClassDB::bind_method(D_METHOD("set_particle_position_mass", "particle_index", "position", "mass"), &ParticleBodyCommands::set_particle_position_mass);

	ClassDB::bind_method(D_METHOD("get_particle_position", "particle_index"), &ParticleBodyCommands::get_particle_position);
	ClassDB::bind_method(D_METHOD("get_particle_mass", "particle_index"), &ParticleBodyCommands::get_particle_mass);

	ClassDB::bind_method(D_METHOD("get_particle_velocity", "particle_index"), &ParticleBodyCommands::get_particle_velocity);
	ClassDB::bind_method(D_METHOD("set_particle_velocity", "particle_index", "velocity"), &ParticleBodyCommands::set_particle_velocity);
}

ParticlePhysicsServer *ParticlePhysicsServer::singleton = NULL;

void ParticlePhysicsServer::_bind_methods() {

	ClassDB::bind_method(D_METHOD("space_create"), &ParticlePhysicsServer::space_create);
	ClassDB::bind_method(D_METHOD("space_set_active", "space", "active"), &ParticlePhysicsServer::space_set_active);
	ClassDB::bind_method(D_METHOD("space_is_active", "space"), &ParticlePhysicsServer::space_is_active);

	ClassDB::bind_method(D_METHOD("body_create"), &ParticlePhysicsServer::body_create);

	ClassDB::bind_method(D_METHOD("body_set_space", "body", "space"), &ParticlePhysicsServer::body_set_space);
	ClassDB::bind_method(D_METHOD("body_set_callback", "body", "callback_type", "receiver", "method"), &ParticlePhysicsServer::body_set_callback);
	ClassDB::bind_method(D_METHOD("body_set_object_instance", "body", "object_instance"), &ParticlePhysicsServer::body_set_object_instance);

	ClassDB::bind_method(D_METHOD("body_get_commands", "body"), &ParticlePhysicsServer::body_get_commands);

	ClassDB::bind_method(D_METHOD("body_set_collision_group", "body", "group"), &ParticlePhysicsServer::body_set_collision_group);
	ClassDB::bind_method(D_METHOD("body_get_collision_group", "body"), &ParticlePhysicsServer::body_get_collision_group);

	ClassDB::bind_method(D_METHOD("body_set_collision_flag", "body", "flag", "active"), &ParticlePhysicsServer::body_set_collision_flag);
	ClassDB::bind_method(D_METHOD("body_get_collision_flag", "body", "flag"), &ParticlePhysicsServer::body_get_collision_flag);

	ClassDB::bind_method(D_METHOD("body_set_primitive_collision_mask", "body", "mask"), &ParticlePhysicsServer::body_set_collision_primitive_mask);
	ClassDB::bind_method(D_METHOD("body_get_primitive_collision_mask", "body"), &ParticlePhysicsServer::body_get_collision_primitive_mask);

	ClassDB::bind_method(D_METHOD("body_remove_particle", "body", "particle_index"), &ParticlePhysicsServer::body_remove_particle);

	ClassDB::bind_method(D_METHOD("body_remove_rigid", "body", "rigid_index"), &ParticlePhysicsServer::body_remove_rigid);

	ClassDB::bind_method(D_METHOD("body_get_particle_count", "body"), &ParticlePhysicsServer::body_get_particle_count);
	ClassDB::bind_method(D_METHOD("body_get_spring_count", "body"), &ParticlePhysicsServer::body_get_spring_count);
	ClassDB::bind_method(D_METHOD("body_get_rigid_count", "body"), &ParticlePhysicsServer::body_get_rigid_count);

	ClassDB::bind_method(D_METHOD("body_set_pressure", "body", "pressure"), &ParticlePhysicsServer::body_set_pressure);
	ClassDB::bind_method(D_METHOD("body_get_pressure", "body"), &ParticlePhysicsServer::body_get_pressure);

	ClassDB::bind_method(D_METHOD("body_can_rendered_using_skeleton", "body"), &ParticlePhysicsServer::body_can_rendered_using_skeleton);
	ClassDB::bind_method(D_METHOD("body_can_rendered_using_pvparticles", "body"), &ParticlePhysicsServer::body_can_rendered_using_pvparticles);

	ClassDB::bind_method(D_METHOD("body_set_monitorable", "body", "monitorable"), &ParticlePhysicsServer::body_set_monitorable);
	ClassDB::bind_method(D_METHOD("body_is_monitorable", "body"), &ParticlePhysicsServer::body_is_monitorable);

	ClassDB::bind_method(D_METHOD("body_set_monitoring_primitives", "body", "monitoring"), &ParticlePhysicsServer::body_set_monitoring_primitives_contacts);
	ClassDB::bind_method(D_METHOD("body_is_monitoring_primitives", "body"), &ParticlePhysicsServer::body_is_monitoring_primitives_contacts);

	ClassDB::bind_method(D_METHOD("primitive_body_create"), &ParticlePhysicsServer::primitive_body_create);
	ClassDB::bind_method(D_METHOD("primitive_body_set_space", "body", "space"), &ParticlePhysicsServer::primitive_body_set_space);
	ClassDB::bind_method(D_METHOD("primitive_body_set_shape", "body", "shape"), &ParticlePhysicsServer::primitive_body_set_shape);

	ClassDB::bind_method(D_METHOD("primitive_body_set_callback", "body", "callback_type", "receiver", "method"), &ParticlePhysicsServer::primitive_body_set_callback);
	ClassDB::bind_method(D_METHOD("primitive_body_set_object_instance", "body", "object_instance"), &ParticlePhysicsServer::primitive_body_set_object_instance);

	ClassDB::bind_method(D_METHOD("primitive_body_set_transform", "body", "transform", "is_teleport"), &ParticlePhysicsServer::primitive_body_set_transform);

	ClassDB::bind_method(D_METHOD("primitive_body_set_as_area", "body", "area"), &ParticlePhysicsServer::primitive_body_set_as_area);
	ClassDB::bind_method(D_METHOD("primitive_body_is_area", "body"), &ParticlePhysicsServer::primitive_body_is_area);

	ClassDB::bind_method(D_METHOD("primitive_body_set_particles_monitoring", "body", "monitoring"), &ParticlePhysicsServer::primitive_body_set_monitoring_particles_contacts);
	ClassDB::bind_method(D_METHOD("primitive_body_is_particles_monitoring", "body"), &ParticlePhysicsServer::primitive_body_is_monitoring_particles_contacts);

	ClassDB::bind_method(D_METHOD("primitive_shape_create", "shape_type"), &ParticlePhysicsServer::primitive_shape_create);

	ClassDB::bind_method(D_METHOD("primitive_shape_set_data", "shape", "data"), &ParticlePhysicsServer::primitive_shape_set_data);
	ClassDB::bind_method(D_METHOD("primitive_shape_get_data", "shape"), &ParticlePhysicsServer::primitive_shape_get_data);

	ClassDB::bind_method(D_METHOD("free", "rid"), &ParticlePhysicsServer::free);

	ClassDB::bind_method(D_METHOD("create_soft_particle_body_model", "trimesh", "radius", "global_stiffness", "internal_sample", "particle_spacing", "sampling", "cluster_spacing", "cluster_radius", "cluster_stiffness", "link_radius", "link_stiffness", "plastic_thresold", "plastic_creep"), &ParticlePhysicsServer::create_soft_particle_body_model);
	ClassDB::bind_method(D_METHOD("create_cloth_particle_body_model", "trimesh", "stretch_stiffness", "bend_stiffness", "tether_stiffness", "tether_give", "pressure"), &ParticlePhysicsServer::create_cloth_particle_body_model);
	ClassDB::bind_method(D_METHOD("create_rigid_particle_body_model", "trimesh", "radius", "expand"), &ParticlePhysicsServer::create_rigid_particle_body_model);

	BIND_ENUM_CONSTANT(PARTICLE_BODY_CALLBACK_SYNC);
	BIND_ENUM_CONSTANT(PARTICLE_BODY_CALLBACK_PARTICLEINDEXCHANGED);
	BIND_ENUM_CONSTANT(PARTICLE_BODY_CALLBACK_SPRINGINDEXCHANGED);
	BIND_ENUM_CONSTANT(PARTICLE_BODY_CALLBACK_PRIMITIVECONTACT);

	BIND_ENUM_CONSTANT(PARTICLE_PRIMITIVE_BODY_CALLBACK_SYNC);
	BIND_ENUM_CONSTANT(PARTICLE_PRIMITIVE_BODY_CALLBACK_PARTICLECONTACT);

	BIND_ENUM_CONSTANT(PARTICLE_COLLISION_FLAG_SELF_COLLIDE);
	BIND_ENUM_CONSTANT(PARTICLE_COLLISION_FLAG_SELF_COLLIDE_FILTER);
	BIND_ENUM_CONSTANT(PARTICLE_COLLISION_FLAG_FLUID);

	BIND_ENUM_CONSTANT(PARTICLE_PRIMITIVE_SHAPE_TYPE_BOX);
	BIND_ENUM_CONSTANT(PARTICLE_PRIMITIVE_SHAPE_TYPE_CAPSULE);
	BIND_ENUM_CONSTANT(PARTICLE_PRIMITIVE_SHAPE_TYPE_SPHERE);
}

ParticlePhysicsServer *ParticlePhysicsServer::get_singleton() {
	return singleton;
}

ParticlePhysicsServer::ParticlePhysicsServer() :
		Object() {
	ERR_FAIL_COND(singleton);
	singleton = this;
}

ParticlePhysicsServer::~ParticlePhysicsServer() {
	singleton = NULL;
}
