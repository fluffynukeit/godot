/*************************************************************************/
/*  shape.h                                                              */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2018 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2018 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

/**
 * @author AndreaCatania
 */

#ifndef PARTICLE_SHAPE_H
#define PARTICLE_SHAPE_H

#include "resource.h"

/// The particle shape is a resource that doesn't have a specific RID that identify the shape in the ParticlePhysicsServer
/// Because it's not necessary
class ParticleShape : public Resource {
    GDCLASS(ParticleShape, Resource);
    OBJ_SAVE_TYPE(ParticleShape);
    RES_BASE_EXTENSION("particle_shape");

public:
    struct Particle {
        Vector3 relative_position;
        real_t mass;
        Vector<int> link;
        Vector<float> length;
        Vector<float> stiffness;

        Particle() :
                relative_position(0, 0, 0),
                mass(0) {}
    };

protected:
    static void _bind_methods();

private:
    Vector<Particle> particles;

protected:
    bool _set(const StringName &p_name, const Variant &p_property);
    bool _get(const StringName &p_name, Variant &r_property) const;
    void _get_property_list(List<PropertyInfo> *p_list) const;

public:
    ParticleShape();

    void set_particles(Vector<Particle> &p_particles);
    const Vector<Particle> &get_particles() const;
};

#endif // PARTICLE_SHAPE_H
