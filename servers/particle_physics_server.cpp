
#include "particle_physics_server.h"

#include "core/method_bind_ext.gen.inc"

void ParticleBodyCommands::_bind_methods() {

	ClassDB::bind_method(D_METHOD("load_model", "shape", "initial_transform"), &ParticleBodyCommands::load_model);

	ClassDB::bind_method(D_METHOD("reset_particle", "particle_index", "position", "mass"), &ParticleBodyCommands::reset_particle);

	ClassDB::bind_method(D_METHOD("get_particle_position", "particle_index"), &ParticleBodyCommands::get_particle_position);

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

	ClassDB::bind_method(D_METHOD("body_set_collision_layer", "body", "layer"), &ParticlePhysicsServer::body_set_collision_layer);
	ClassDB::bind_method(D_METHOD("body_get_collision_layer", "body"), &ParticlePhysicsServer::body_get_collision_layer);

	ClassDB::bind_method(D_METHOD("body_add_particle", "body", "local_position", "mass"), &ParticlePhysicsServer::body_add_particle);
	ClassDB::bind_method(D_METHOD("body_remove_particle", "body", "particle_id"), &ParticlePhysicsServer::body_remove_particle);

	ClassDB::bind_method(D_METHOD("primitive_body_create"), &ParticlePhysicsServer::primitive_body_create);
	ClassDB::bind_method(D_METHOD("primitive_body_set_space", "body", "space"), &ParticlePhysicsServer::primitive_body_set_space);

	ClassDB::bind_method(D_METHOD("primitive_shape_create", "shape_type"), &ParticlePhysicsServer::primitive_shape_create);

	ClassDB::bind_method(D_METHOD("primitive_shape_set_data", "shape", "data"), &ParticlePhysicsServer::primitive_shape_set_data);
	ClassDB::bind_method(D_METHOD("primitive_shape_get_data", "shape"), &ParticlePhysicsServer::primitive_shape_get_data);

	ClassDB::bind_method(D_METHOD("free", "rid"), &ParticlePhysicsServer::free);

	ClassDB::bind_method(D_METHOD("create_soft_particle_body_model", "trimesh", "cloth", "sampling", "cluster_spacing", "cluster_radius", "cluster_stiffness", "link_radius", "link_stiffness"), &ParticlePhysicsServer::create_soft_particle_body_model);

	BIND_ENUM_CONSTANT(PARTICLE_BODY_CALLBACK_SYNC);
	BIND_ENUM_CONSTANT(PARTICLE_BODY_CALLBACK_PARTICLEINDEXCHANGED);
	BIND_ENUM_CONSTANT(PARTICLE_BODY_CALLBACK_SPRINGINDEXCHANGED);

	BIND_ENUM_CONSTANT(PARTICLE_PRIMITIVE_SHAPE_TYPE_BOX)
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
