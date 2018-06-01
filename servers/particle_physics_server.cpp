
#include "particle_physics_server.h"

void ParticleBodyCommands::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_particle_position", "particle_id"), &ParticleBodyCommands::get_particle_position);
}

ParticlePhysicsServer *ParticlePhysicsServer::singleton = NULL;

void ParticlePhysicsServer::_bind_methods() {

    ClassDB::bind_method(D_METHOD("space_create"), &ParticlePhysicsServer::space_create);
    ClassDB::bind_method(D_METHOD("space_set_active", "space", "active"), &ParticlePhysicsServer::space_set_active);
    ClassDB::bind_method(D_METHOD("space_is_active", "space"), &ParticlePhysicsServer::space_is_active);

    ClassDB::bind_method(D_METHOD("body_create"), &ParticlePhysicsServer::body_create);
    ClassDB::bind_method(D_METHOD("body_set_space", "body", "space"), &ParticlePhysicsServer::body_set_space);
    ClassDB::bind_method(D_METHOD("body_add_particle", "body", "local_position", "mass"), &ParticlePhysicsServer::body_add_particle);
    ClassDB::bind_method(D_METHOD("body_remove_particle", "body", "particle_id"), &ParticlePhysicsServer::body_remove_particle);
    ClassDB::bind_method(D_METHOD("body_set_sync_callback", "receiver", "method"), &ParticlePhysicsServer::body_set_sync_callback);

    ClassDB::bind_method(D_METHOD("free", "rid"), &ParticlePhysicsServer::free);
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
