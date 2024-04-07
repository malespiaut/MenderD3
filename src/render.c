/*
 *	This file is part of MenderD3
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

// #include <GL/glu.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "md3_parse.h"
#include "definitions.h"
#include "quaternion.h"
#include "world.h"
#include "util.h"
#include "jitter.h"
#include "accum.h"
#include "render.h"

/* bright white material */
struct material_t white_material = {
  {1.0, 1.0, 1.0, 1.0},
  {1.0, 1.0, 1.0, 1.0},
  {1.0, 1.0, 1.0, 1.0},
  {0.0, 0.0, 0.0, 1.0},
  32.0};

/* glass material */
struct material_t glass_material = {
  {0.5, 0.5, 0.5, 0.5},
  {0.1, 0.1, 0.1, 0.5},
  {1.0, 1.0, 1.0, 1.0},
  {0.0, 0.0, 0.0, 1.0},
  32.0};

/*
 *	The pseudo tag is used for rendering the root model
 *	for applied custom rotation.
 */
md3_tag_t pseudo_tag = {
  {0},
  {0, 0, 0},
  {{1, 0, 0}, /* the normal axis is the identity matrix */
   {0, 1, 0},
   {0, 0, 1}}};

static void render_scene();
static void apply_custom_rotation(md3_model_t* model, md3_tag_t* tag, quat_t* quat);

/*
 *	Render the scene for the current engine setup.
 */
void
render_c()
{
  render_scene();

  /* Flush the GL pipeline */
  glFlush();
}

/*
 *	Render the scene.
 */
static void
render_scene()
{
  render_primitives(1);

  /* render the mirror images if enabled */
  if (WORLD_IS_SET(RENDER_MIRRORS))
    draw_mirrors(g_world->mirrors);
}

/*
 *	Render primitive objects.
 *
 *	A "primitive" object refers to one that does not have
 *	special charastics within the engine.
 *	ie:
 *		A model is primitive, but a mirror is not.
 */
void
render_primitives(int apply_names)
{
  glPushMatrix();
  glRotatef(-90, 1, 0, 0);
  md3_render(g_world->root_model, apply_names, NULL);
  glPopMatrix();

  /* draw the flashlight */
  if (WORLD_IS_SET(RENDER_FLASHLIGHT))
    render_flashlight();
}

/*
 *	Draw the flashlight.
 */
void
render_flashlight()
{
  int lighting_enabled = WORLD_IS_SET(ENGINE_LIGHTING);
  md3_model_t* light_model = world_get_model_by_type(MD3_LIGHT);

  /* disable lighting */
  world_set_options(g_world, 0, ENGINE_LIGHTING);
  glDisable(GL_LIGHTING);

  glPushMatrix();
  /* translate to the flashlight origin */
  glTranslatef(g_world->light[0].position[0], g_world->light[0].position[1], g_world->light[0].position[2]);

  /* rotate up/down */
  glRotatef(g_world->light[0].dir_prot, 0, 0, 1);

  /*	rotate right/left */
  glRotatef(g_world->light[0].dir_trot, 0, 1, 0);

  glRotatef(-90, 0, 1, 0);
  glScalef(0.8, 0.8, 0.8);

  md3_render(light_model, 1, NULL);
  glPopMatrix();

  /* reenable lighting if it was previously set */
  if (lighting_enabled)
  {
    world_set_options(g_world, ENGINE_LIGHTING, 0);
    glEnable(GL_LIGHTING);
  }
}

/*
 *	Render a model and all the links starting at the given model pointer.
 *
 *	Link tag is the tag in the parent model where this model links for this frame.
 *	This is needed for custom rotation of the current model part.
 *	If the base model is passed, give link_tag as NULL.
 */
void
md3_render(md3_model_t* model, int apply_names, md3_tag_t* link_tag)
{
  int i = 0;
  int frame;
  int next_frame;

  md3_tag_t* tag = NULL;
  md3_tag_t* next_tag = NULL;
  int itag = 0;
  float* rot1 = NULL;
  float* rot2 = NULL;
  float rot[16];
  quat_t q1;
  quat_t q2;
  quat_t q3;
  struct vec3_t* origin1 = NULL;
  struct vec3_t* origin2 = NULL;
  struct vec3_t origin;

  if (!model)
    return;

  frame = model->anim_state.frame;
  next_frame = model->anim_state.frame;

  /*
   *	Instantly apply custom rotation.
   *	No interpolation since there is no time duration.
   *
   *	The rotation will apply to all children as well.
   */
  if (!link_tag)
    /*
     *	If this is the base object and link_tag is NULL,
     *	use a pseudo tag with normal orientation.
     */
    link_tag = &pseudo_tag;

  quat_init(&q1);
  apply_custom_rotation(model, link_tag, &q1);
  quat_to_matrix_4x4(&q1, NULL, rot);
  glMultMatrixf(rot);

  /* Apply custom scale for this model only (no children) */
  if (model->scale_factor)
  {
    glPushMatrix();
    glScalef(model->scale_factor, model->scale_factor, model->scale_factor);
  }

  /*	Render this model	*/
  md3_render_single(model, apply_names);

  if (model->scale_factor)
    glPopMatrix();

  /*	Render each link	*/
  for (i = 0; i < model->num_tags; ++i)
  {
    /* if no model link here (possible load error) then skip */
    if (!model->links[i])
      continue;

    /*
     *	Get the tag index for this frame.
     *
     *	For saftey modulate the frame by the total number of frames.
     *	The multiply by the number of tags since each frame has
     *	X continuous entries (where X is the number of tags)
     *	in the tag array.
     *	Then offset to the current tag by adding the current tag number.
     */
    glPushMatrix();

    /* SLERP the rotation */
    itag = (((model->anim_state.frame % model->num_frames) * model->num_tags) + i);
    tag = &(model->tags[itag]);

    itag = (((model->anim_state.next_frame % model->num_frames) * model->num_tags) + i);
    next_tag = &(model->tags[itag]);

    /* LERP the origin translation - needed? */
    origin1 = &tag->origin;
    origin2 = &next_tag->origin;
    LERP_VERTEX(origin1, origin2, model->anim_state.t, (&origin));

    /*
     *	If there was a custom scale set, it must also be
     *	applied to the origin so that the body parts align.
     */
    if (model->scale_factor)
      SCALE_VERTEX((&origin), model->scale_factor);

    rot1 = (float*)tag->axis;
    rot2 = (float*)next_tag->axis;

    /* convert the 3x3 matricies to quaternions */
    quat_from_matrix_3x3(&q1, rot1);
    quat_from_matrix_3x3(&q2, rot2);

    /* slerp the quaternions */
    quat_slerp(&q1, &q2, model->anim_state.t, &q3);

    /* convert the quaternion to 4x4 matrix and apply */
    quat_to_matrix_4x4(&q3, &origin, rot);
    glMultMatrixf(rot);

    /* Render child */
    md3_render(model->links[i], apply_names, tag);

    glPopMatrix();
  }
}

/*
 *	Render only a single model link.
 *	There is no SLERP here.
 */
void
md3_render_single(md3_model_t* model, int apply_names)
{
  md3_surface_t* sptr = model->surface_ptr;
  md3_vertex_t* vptr1 = NULL;
  md3_vertex_t* vptr2 = NULL;
  md3_vertex_t vptr;
  md3_texcoord_t* tptr = NULL;
  struct tga_t* texture = NULL;
  int frame_offset;
  int next_frame_offset;
  int vertex;
  int i = 0;

  /* white material used for textures */
  apply_material(&white_material);

  /* set the name for this body part */
  if (apply_names)
    glLoadName(model->body_part);

  /* tick the model to update animation information */
  world_tick_model(model);

  while (sptr)
  {
    /* Get texture */
    if (WORLD_IS_SET(RENDER_TEXTURES))
    {
      texture = sptr->shader[0].texture;
      apply_texture(&(sptr->shader[0]));
    }
    else
      glDisable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    /* get correct frame information */
    frame_offset = ((model->anim_state.frame % sptr->num_frames) * sptr->num_verts);
    next_frame_offset = ((model->anim_state.next_frame % sptr->num_frames) * sptr->num_verts);

    for (i = 0; i < sptr->num_triangles; ++i)
    {
      if (WORLD_IS_SET(RENDER_WIREFRAME))
        glBegin(GL_LINE_STRIP);
      else
        glBegin(GL_TRIANGLES);

      /* draw the three verticies for the triangle */
      for (vertex = 0; vertex < 3; ++vertex)
      {
        /* get texture data */
        tptr = &(sptr->st[sptr->triangle[i].index[vertex]]);

        /* get vertex data for this frame and next frame */
        vptr1 = &(sptr->vertex[sptr->triangle[i].index[vertex] + frame_offset]);
        vptr2 = &(sptr->vertex[sptr->triangle[i].index[vertex] + next_frame_offset]);

        /* LERP the verticies */
        LERP_VERTEX(vptr1, vptr2, model->anim_state.t, (&vptr));

        /* LERP the normal */
        LERP_NORMAL(vptr1, vptr2, model->anim_state.t, (&vptr));

        /* set the normal and texture data */
        glNormal3f(vptr.normalxyz[0], vptr.normalxyz[1], vptr.normalxyz[2]);

        if (WORLD_IS_SET(RENDER_TEXTURES) && sptr->shader[0].gl_text_bound && tptr)
          glTexCoord2f((texture->hflip ? (1 - tptr->st[0]) : tptr->st[0]), (texture->vflip ? (1 - tptr->st[1]) : tptr->st[1]));

        /* draw it */
        glVertex3f((float)(vptr.x * MD3_XYZ_SCALE), (float)(vptr.y * MD3_XYZ_SCALE), (float)(vptr.z * MD3_XYZ_SCALE));
      }

      glEnd();
    }

    /*
     *	Draw the bounding box if this model has the flag
     *	set and this is also the first surface for the model.
     */
    if (model->draw_bounding_box && (sptr == model->surface_ptr))
    {
      md3_frame_t* f = &model->frames[0];
      float r = (f->radius / 2.5f);

      glDisable(GL_LIGHTING);
      glDisable(GL_TEXTURE_2D);

      glPushMatrix();
      glTranslatef(f->local_origin.x, f->local_origin.y, f->local_origin.z);
      glScalef(r, r, r);
      glCallList(g_world->gl_box_id);
      glPopMatrix();

      if (WORLD_IS_SET(ENGINE_LIGHTING))
        glEnable(GL_LIGHTING);
      if (WORLD_IS_SET(RENDER_TEXTURES))
        glEnable(GL_TEXTURE_2D);
    }

    sptr = sptr->next;
  }
}

static void
apply_custom_rotation(md3_model_t* model, md3_tag_t* tag, quat_t* quat)
{
  quat_t c_local;
  quat_init(&c_local);

  /* rotation x-axis */
  quat_rotate(&c_local, model->rot[0], tag->axis[1].x, tag->axis[1].y, tag->axis[1].z);
  quat_mult(quat, &c_local, quat);

  /* rotation y-axis */
  quat_rotate(&c_local, model->rot[1], tag->axis[0].x, tag->axis[0].y, tag->axis[0].z);
  quat_mult(quat, &c_local, quat);

  /* rotation z-axis */
  quat_rotate(&c_local, model->rot[2], tag->axis[2].x, tag->axis[2].y, tag->axis[2].z);
  quat_mult(quat, &c_local, quat);
}

unsigned int
make_bounding_box()
{
  int corner = 0;
  float scalers[7][3] = {
    {1, 1, -1},
    {1, -1, 1},
    {1, 1, -1},
    {-1, 1, 1},
    {1, 1, -1},
    {1, -1, 1},
    {1, 1, -1}};
  int gl_id;

  gl_id = glGenLists(1);
  glNewList(gl_id, GL_COMPILE);

  glPushMatrix();
  glLineWidth(2.0f);

  for (; corner < 8; ++corner)
  {
    glBegin(GL_LINES);
    /* -- */
    glColor3f(0.0f, 0.5f, 0.5f);
    glVertex3f(1, -1, 1);
    glVertex3f(1, -1 + 0.2, 1);

    /* | */
    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex3f(1, -1, 1);
    glVertex3f(1, -1, 1 - 0.2);

    /* / */
    glColor3f(0.5f, 0.5f, 0.0f);
    glVertex3f(1, -1, 1);
    glVertex3f(1 - 0.2, -1, 1);
    glEnd();
    glScalef(scalers[corner][0], scalers[corner][1], scalers[corner][2]);
  }

  glLineWidth(1.0f);
  glPopMatrix();

  glEndList();

  return gl_id;
}

/*
 *	Draw a tranlucent plane.
 */
unsigned int
make_tes_plane()
{
  float x, z;
  unsigned int gl_id;
  int fx = 20;
  int fz = 20;

  /*
   *	Optimization - at the cost of realisim.
   *
   *	Glass surface moved from 2500 polygons to 400.
   *	This increased framerate by 10.
   *	Light still looks okay shining on it, but not as
   *	smooth as it did.
   */

  gl_id = glGenLists(1);
  glNewList(gl_id, GL_COMPILE);

  /* draw the floor */
  apply_material(&glass_material);
  glDisable(GL_TEXTURE_2D);

  glNormal3f(0.0, 1.0, 0.0);

  for (x = 0; x < fx; ++x)
  {
    for (z = 0; z < fz; ++z)
    {
      glBegin(GL_QUADS);
      glVertex3f(x / fx, 0, z / fz);
      glVertex3f((x + 1) / fx, 0, z / fz);
      glVertex3f((x + 1) / fx, 0, (z + 1) / fz);
      glVertex3f(x / fx, 0, (z + 1) / fz);
      glEnd();
    }
  }

  glEnable(GL_TEXTURE_2D);

  glEndList();
  return gl_id;
}

/*
 *	Draw the mirrors and reflections.
 *
 *	This function is VASTLY inefficient.
 *	For each mirror:
 *		- render the mirror to the stencil buffer
 *		- render the entire scene (horrid)
 *		- render the mirror
 */
void
draw_mirrors(struct mirror_t* m)
{
  while (m)
  {
    glClear(GL_STENCIL_BUFFER_BIT);

    /* setup the stencil buffer */
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(0, 0, 0, 0);

    glDisable(GL_DEPTH_TEST);

    /* draw each plane to the stencil buffer */
    glPushMatrix();
    /* rotate so plane is on z=0 */
    glRotatef(m->angle, m->axis[0], m->axis[1], m->axis[2]);

    /* translate to upper right hand corner of plane */
    glTranslatef(m->origin[0], m->origin[1], m->origin[2]);

    glScalef(100.0, 1.0, 100.0);
    glCallList(g_world->gl_plane_id);

    /* the clipping plane rests on this plane */
    glEnable(GL_CLIP_PLANE0);
    glClipPlane(GL_CLIP_PLANE0, m->clip);
    glPopMatrix();

    glEnable(GL_DEPTH_TEST);

    glColorMask(1, 1, 1, 1);
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    /* now draw the actual reflections */
    glCullFace(GL_BACK); /* the inversion seems to flip the faces */

    glPushMatrix();
    glTranslatef(
      -2 * m->origin[0] * m->normal[0],
      -2 * m->origin[1] * m->normal[1],
      -2 * m->origin[2] * m->normal[2]);

    glScalef(
      m->normal[0] ? m->normal[0] : 1,
      m->normal[1] ? m->normal[1] : 1,
      m->normal[2] ? m->normal[2] : 1);

    /* draw the scene */
    render_primitives(0);
    glPopMatrix();

    /* diable the clipping plane */
    glDisable(GL_CLIP_PLANE0);

    glCullFace(GL_FRONT);
    glCullFace(GL_FRONT);

    glDisable(GL_STENCIL_TEST);

    /* draw the mirror */
    glPushMatrix();
    /* rotate so plane is on z=0 */
    glRotatef(m->angle, m->axis[0], m->axis[1], m->axis[2]);

    /* translate to upper right hand corner of plane */
    glTranslatef(m->origin[0], m->origin[1], m->origin[2]);

    glScalef(100.0, 1.0, 100.0);
    glCallList(g_world->gl_plane_id);
    glPopMatrix();

    m = m->next;
  }
}
