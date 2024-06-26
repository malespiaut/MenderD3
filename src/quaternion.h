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

#ifndef _QUATERNION_H
#define _QUATERNION_H

/*
 *	A quaternion object.
 *
 *	x, y, and z represent the axis.
 *	w represents the rotation angle around that axis.
 */
typedef struct quat_t quat_t;
struct quat_t
{
  float x;
  float y;
  float z;
  float w;
};

#ifdef __cplusplus
extern "C"
{
#endif

  void quat_init(quat_t* q);
  void quat_normalize(quat_t* q);
  void quat_mult(quat_t* a, quat_t* b, quat_t* c);
  void quat_rotate(quat_t* q, float ang, float x, float y, float z);
  void quat_to_matrix_4x4(quat_t* q, struct vec3_t* origin, float* m);
  void quat_from_matrix_4x4(quat_t* q, float* m);
  void quat_from_matrix_3x3(quat_t* q, float* m);

  void matrix_3x3_to_4x4(float* m3, float* m4, struct vec3_t* origin);

  void quat_slerp(quat_t* q1, quat_t* q2, float t, quat_t* q3);

#ifdef __cplusplus
}
#endif

#endif /* _QUATERNION_H */
