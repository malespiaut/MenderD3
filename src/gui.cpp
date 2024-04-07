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

#include <QSurfaceFormat>
#include <QApplication>
#include <QLayout>
#include "gl_widget.h"
#include "gui.h"
#include "world.h"

/* global to GUI widget - singleton */
class gui_widget* g_gui = NULL;

/*
 *	Entry point for the Qt portion.
 */
int
gui_start(int argc, char** argv)
{
  QApplication app(argc, argv);

  gui_widget* gptr = new gui_widget(argc, argv);

  gptr->show();

  int ret = app.exec();

  delete gptr;
  return ret;
}

/***********************************************************************************
 *
 *	gui_widget
 *
 ***********************************************************************************/

/*
 *	gui_widget::gui_widget()
 */
gui_widget::gui_widget(int argc, char** argv)
  : QFrame()
{
  this->setGeometry(10, 10, 900, 675);
  this->setWindowTitle("MenderD3");

  /* Set the global GUI widget to this */
  g_gui = this;

  /*
   *	Create the base 2x2 grid layout.
   */
  this->base_grid = new QGridLayout(this);
  this->base_grid->addLayout(this->base_grid, 2, 2);
  this->base_grid->setContentsMargins(1, 1, 1, 1);
  this->base_grid->setSpacing(5);
  this->setLayout(this->base_grid);

  /*
   *	Create the OpenGL widget and add to grid position 0,0
   */
  QSurfaceFormat surface_format;
  surface_format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  this->gl = new gl_widget(argc, argv, surface_format, this);
  this->base_grid->addWidget(this->gl, 0, 0);

  /*
   *	Set the streching factors for the columns.
   */
  this->base_grid->setColumnStretch(0, 2000);
  this->base_grid->setColumnStretch(1, 1);

  /*
   *	Set the streching factors for the rows.
   */
  this->base_grid->setRowStretch(0, 2000);
  this->base_grid->setRowStretch(1, 1);

  /*
   *	Make the side layout.
   */
  this->side_layout = new QVBoxLayout();
  this->base_grid->addLayout(this->side_layout, 0, 1);
  this->base_grid->setContentsMargins(1, 1, 1, 1);
  this->base_grid->setSpacing(5);
  this->setLayout(this->base_grid);

  /*
   *	Add model groupbox widget to side layout.
   */
  this->model = new model_widget(MODEL_TYPE, 2, Qt::Vertical, "Model", this);
  this->side_layout->addWidget(this->model);

  /*
   *	Add weapon groupbox widget to side layout.
   */
  this->weapon = new model_widget(WEAPON_TYPE, 2, Qt::Vertical, "Weapon", this);
  this->side_layout->addWidget(this->weapon);

  /*
   *	Add animate groupbox widget to side layout
   */
  this->animate = new animate_widget(4, Qt::Vertical, "Animate", this);
  this->side_layout->addWidget(this->animate);

  /*
   *	Add options groupbox widget to side layout
   */
  this->opt = new opt_widget(4, Qt::Vertical, "Options", this);
  this->side_layout->addWidget(this->opt);

  /*
   *	Add scaling and rotation groupbox widget to side layout
   */
  this->srot = new srot_widget(4, Qt::Vertical, "Selected Body Area", this);
  this->side_layout->addWidget(this->srot);

  /*
   *	Add the credits to the side layout
   */
  this->credits = new QLabel(CREDITS_STR, this);
  this->credits->setAlignment(Qt::AlignRight | Qt::AlignBottom);
  this->side_layout->addWidget(this->credits);

  // this->side_layout->addStretch();

  /*
   *	Start of bottom layout
   */
  this->bottom_layout = new QHBoxLayout();
  this->base_grid->addLayout(this->bottom_layout, 1, 0, 1, 2);

  /*
   *	add camera position label to the bottom of the screen
   */
  this->cam_pos = new QLabel("Camera Position:   x = 0.00   y = 0.00   z = 0.00", this);
  this->cam_pos->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  this->cam_pos->setAlignment(Qt::AlignCenter);
  this->bottom_layout->addWidget(this->cam_pos);

  /*
   *	add model information label to bottom of the screen
   */
  this->model_inf = new QLabel("info", this);
  this->model_inf->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  this->model_inf->setAlignment(Qt::AlignCenter);
  this->bottom_layout->addWidget(this->model_inf);

  /* set model info */
  char buf[64];
  sprintf(buf, "Trianges: %i     Frames: %i", g_world->model_triangles, g_world->root_model ? g_world->root_model->num_frames : 0);
  this->model_inf->setText(buf);

  /*
   *	add a frames per second label to the bottom of the screen
   */
  this->fps = new QLabel("fps", this);
  this->fps->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  this->fps->setAlignment(Qt::AlignCenter);
  this->bottom_layout->addWidget(this->fps);
}

/*
 *	gui_widget::~gui_widget()
 */
gui_widget::~gui_widget()
{
}

/***********************************************************************************
 *
 *	model_widget
 *
 ***********************************************************************************/

/*
 *	model_widget::model_widget()
 */
model_widget::model_widget(enum loadable_types mtype, int strips, Qt::Orientation orientation, const QString& title, QWidget* parent, const char* name)
  : QGroupBox(title, parent)
{

  this->model = NULL;
  this->type = mtype;

  this->open = new QPushButton("Open", this);
  connect(open, SIGNAL(clicked()), this, SLOT(load()));

  /*
   *	makes the groupbox auto-fit to the the widgets inside
   */
  this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  this->setMaximumWidth(MAX_MENU_WIDTH);

/*
 *	Load default models if defined.
 */
#ifdef DEFAULT_LOAD_MODEL
  if (mtype == MODEL_TYPE)
    this->model = load_model(DEFAULT_LOAD_MODEL);

#ifdef DEFAULT_LOAD_WEAPON
  if (mtype == WEAPON_TYPE)
    this->model = load_weapon(DEFAULT_LOAD_WEAPON, "../");
#endif

  /*
   *	Set the default animations.
   */
  SET_DEFAULT_ANIMATIONS();

#endif
#ifdef DEFAULT_LOAD_LIGHT0
  load_light_model(DEFAULT_LOAD_LIGHT0, 0);
#endif
}

/*
 *	model_widget::load()
 *
 *	Load a new model.
 */
void
model_widget::load()
{
  if (this->type == MODEL_TYPE)
  {
    /* model */

    /* get the file to be loaded */
    QString s = QFileDialog::getOpenFileName(this, "Open Model File", MODELS_PATH, "Character Models (*.mod)");

    if (s.isEmpty())
      return;

    /*
     *	If there was previously a model loaded unload it.
     *
     *	Note that since the weapon is linked to the model tree were are
     *	removing, the weapon too will be deleted.
     *	For this reason after the model has been loaded we must relink
     *	the weapon back into the new tree.
     */
    md3_model_t* weapon = world_get_model_by_type(MD3_WEAPON);
    if (this->model)
      unload_model(this->model, 0);

    /* load the full model */
    this->model = load_model((char*)s.toLatin1().data());

    /* relink the weapon */
    if (weapon)
      world_link_model(g_world, weapon);

    /* now that the model has been loaded the GUI animation stuff must be reset */
    g_gui->animate->reset_animation();

    char buf[64];
    sprintf(buf, "Trianges: %i     Frames: %i", g_world->model_triangles, g_world->root_model ? g_world->root_model->num_frames : 0);
    g_gui->model_inf->setText(buf);
  }
  else
  {
    /* weapon */

    /* get the file to be loaded */
    QString s = QFileDialog::getOpenFileName(this, "Open Model File", WEAPONS_PATH, "Weapon Models (*.md3)");

    if (s.isEmpty())
      return;

    /* if there was previously a model loaded unload it */
    if (this->model)
      unload_weapon(this->model);

    /* load the weapon model */
    this->model = load_weapon((char*)s.toLatin1().data(), "../");
  }
}

/***********************************************************************************
 *
 *	animate_widget
 *
 ***********************************************************************************/

/*
 *	animate_widget::animate_widget()
 */
animate_widget::animate_widget(int strips, Qt::Orientation orientation, const QString& title, QWidget* parent, const char* name)
  : QGroupBox(title, parent)
{

  this->base = new QWidget(this);
  this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  this->setMaximumWidth(MAX_MENU_WIDTH);

  /*
   *	creates a grid layout to organize the widgets
   */
  this->ani_grid = new QGridLayout(this->base);
  this->ani_grid->addLayout(this->ani_grid, 4, 3);
  this->ani_grid->setContentsMargins(1, 1, 1, 1);
  this->ani_grid->setSpacing(5);
  this->setLayout(this->ani_grid);

  /*
   *	first column of the grid
   *	3 Comboboxes for selecting animations
   *	1 pushbutton for stopping all animations
   */
  this->tl_CB = new QComboBox(this->base);
  this->ani_grid->addWidget(this->tl_CB, 0, 0);
  tl_CB->insertItem(0, "-Torso and Legs-");
  tl_CB->insertItem(1, "Death 1");
  tl_CB->insertItem(2, "Death 2");
  tl_CB->insertItem(3, "Death 3");
  tl_CB->insertItem(4, "Dead 1");
  tl_CB->insertItem(5, "Dead 2");
  tl_CB->insertItem(6, "Dead 3");

  this->t_CB = new QComboBox(this->base);
  this->ani_grid->addWidget(this->t_CB, 1, 0);
  t_CB->insertItem(0, "-Torso-");
  t_CB->insertItem(1, "Gesture");
  t_CB->insertItem(2, "Attack 1");
  t_CB->insertItem(3, "Attack 2");
  t_CB->insertItem(4, "Drop");
  t_CB->insertItem(5, "Raise");
  t_CB->insertItem(6, "Stand 1");
  t_CB->insertItem(7, "Stand 2");

  this->l_CB = new QComboBox(this->base);
  this->ani_grid->addWidget(this->l_CB, 2, 0);
  l_CB->insertItem(0, "-Legs-");
  l_CB->insertItem(1, "Crouch Walk");
  l_CB->insertItem(2, "Walk");
  l_CB->insertItem(3, "Run");
  l_CB->insertItem(4, "Back");
  l_CB->insertItem(5, "Swim");
  l_CB->insertItem(6, "Jump");
  l_CB->insertItem(7, "Land");
  l_CB->insertItem(8, "Jump Back");
  l_CB->insertItem(9, "Land Back");
  l_CB->insertItem(10, "Idle");
  l_CB->insertItem(11, "Crouch Idle");
  l_CB->insertItem(12, "Turn");

  this->stopall = new QPushButton("Stop All", this->base);
  this->ani_grid->addWidget(this->stopall, 3, 0);
  connect(stopall, SIGNAL(clicked()), this, SLOT(stopall_clicked()));

  /*
   *	second column of the grid
   *	3 pushbuttons to start a selected animation
   *	1 pushbutton to reset the model to its orginal position
   */
  this->tl_start = new QPushButton("Start", this->base);
  this->ani_grid->addWidget(this->tl_start, 0, 1);
  connect(tl_start, SIGNAL(clicked()), this, SLOT(tl_clicked()));

  this->t_start = new QPushButton("Start", this->base);
  this->ani_grid->addWidget(this->t_start, 1, 1);
  connect(t_start, SIGNAL(clicked()), this, SLOT(t_clicked()));

  this->l_start = new QPushButton("Start", this->base);
  this->ani_grid->addWidget(this->l_start, 2, 1);
  connect(l_start, SIGNAL(clicked()), this, SLOT(l_clicked()));

  this->reset = new QPushButton("Reset", this->base);
  this->ani_grid->addWidget(this->reset, 3, 1);
  connect(reset, SIGNAL(clicked()), this, SLOT(reset_clicked()));

  /*
   *	third column of the grid
   *	3 pushbuttons to stop the selected animation
   *	1 checkbox to put the model in a infinate loop of annimation
   */
  this->tl_stop = new QPushButton("Stop", this->base);
  this->ani_grid->addWidget(this->tl_stop, 0, 2);
  connect(tl_stop, SIGNAL(clicked()), this, SLOT(tls_clicked()));

  this->t_stop = new QPushButton("Stop", this->base);
  this->ani_grid->addWidget(this->t_stop, 1, 2);
  connect(t_stop, SIGNAL(clicked()), this, SLOT(ts_clicked()));

  this->l_stop = new QPushButton("Stop", this->base);
  this->ani_grid->addWidget(this->l_stop, 2, 2);
  connect(l_stop, SIGNAL(clicked()), this, SLOT(ls_clicked()));

  this->loop_check = new QCheckBox("Loop", this->base);
  this->ani_grid->addWidget(this->loop_check, 3, 2);
  connect(loop_check, SIGNAL(clicked()), this, SLOT(loop_checked()));
}

/*
 *	animate_widget::reset_animation()
 *
 *	Reset the animation stuff to default.
 */
void
animate_widget::reset_animation()
{
  /*
   *	Set animation to default.
   */
  SET_DEFAULT_ANIMATIONS();

  /*
   *	Set drop-down boxes to first elements.
   */
  this->tl_CB->setCurrentIndex(0);
  this->t_CB->setCurrentIndex(0);
  this->l_CB->setCurrentIndex(0);
}

/*
 *	animate_widget::tl_clicked()
 *
 *	Animate both torso and legs.
 */
void
animate_widget::tl_clicked()
{
  md3_animations_e anims[] = {
    BOTH_DEATH1,
    BOTH_DEATH2,
    BOTH_DEATH3,
    BOTH_DEAD1,
    BOTH_DEAD2,
    BOTH_DEAD3};

  int selected = tl_CB->currentIndex();

  /* if the title is selected, then nothing can be animated */
  if (!selected)
    return;

  set_model_animation((md3_animations_e)anims[selected - 1]);
}

/*
 *	animate_widget::t_clicked()
 *
 *	Animation torso.
 */
void
animate_widget::t_clicked()
{
  md3_animations_e anims[] = {
    TORSO_GESTURE,
    TORSO_ATTACK,
    TORSO_ATTACK2,
    TORSO_DROP,
    TORSO_RAISE,
    TORSO_STAND,
    TORSO_STAND2};

  int selected = t_CB->currentIndex();

  /* if the title is selected, then nothing can be animated */
  if (!selected)
    return;

  set_model_animation((md3_animations_e)anims[selected - 1]);
}

/*
 *	animate_widget::l_clicked()
 *
 *	Animate legs.
 */
void
animate_widget::l_clicked()
{
  md3_animations_e anims[] = {
    LEGS_WALKCR,
    LEGS_WALK,
    LEGS_RUN,
    LEGS_BACK,
    LEGS_SWIM,
    LEGS_JUMP,
    LEGS_LAND,
    LEGS_JUMPB,
    LEGS_LANDB,
    LEGS_IDLE,
    LEGS_IDLECR,
    LEGS_TURN};

  int selected = l_CB->currentIndex();

  /* if the title is selected, then nothing can be animated */
  if (!selected)
    return;

  set_model_animation((md3_animations_e)anims[selected - 1]);
}

/*
 *	animate_widget::tls_clicked()
 *
 *	Stop animation of both torso and legs.
 */
void
animate_widget::tls_clicked()
{
  world_stop_model_animation(MD3_LEGS | MD3_TORSO);
}

/*
 *	animate_widget::ts_clicked()
 *
 *	Stop animation of torso.
 */
void
animate_widget::ts_clicked()
{
  world_stop_model_animation(MD3_TORSO);
}

/*
 *	animate_widget::ls_clicked()
 *
 *	Stop animation of legs.
 */
void
animate_widget::ls_clicked()
{
  world_stop_model_animation(MD3_LEGS);
}

/*
 *	animate_widget::stopall_clicked()
 *
 *	Stop animation of both torso and legs.
 */
void
animate_widget::stopall_clicked()
{
  world_stop_model_animation(MD3_LEGS | MD3_TORSO);
}

/*
 *	animate_widget::reset_clicked()
 *
 *	Reset the animation stuff to default.
 */
void
animate_widget::reset_clicked()
{
  this->reset_animation();
}

/*
 *	animate_widget::loop_checked()
 *
 *	Enable animation looping.
 */
void
animate_widget::loop_checked()
{
  if (this->loop_check->isChecked() == true)
    world_set_options(g_world, RENDER_ANIM_LOOP, 0);
  else
    world_set_options(g_world, 0, RENDER_ANIM_LOOP);
}

/***********************************************************************************
 *
 *	opt_widget
 *
 ***********************************************************************************/

/*
 *	opt_widget::opt_widget()
 */
opt_widget::opt_widget(int strips, Qt::Orientation orientation, const QString& title, QWidget* parent, const char* name)
  : QGroupBox(title, parent)
{

  this->base = new QWidget(this);

  /*
   *	makes the groupbox auto-fit to the the widgets inside
   */
  this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  this->setMaximumWidth(MAX_MENU_WIDTH);

  /*
   *	creates a grid layout to organize the widgets
   */
  this->opt_grid = new QGridLayout(this->base);
  this->opt_grid->addLayout(this->opt_grid, 5, 2);
  this->opt_grid->setContentsMargins(1, 1, 1, 1);
  this->opt_grid->setSpacing(5);
  this->setLayout(this->opt_grid);

  /*
   *	first column
   */
  this->wireCB = new QCheckBox("Wire Frame", this->base);
  this->opt_grid->addWidget(this->wireCB, 0, 0);
  connect(wireCB, SIGNAL(clicked()), this, SLOT(wire_checked()));

  this->noTextCB = new QCheckBox("No Textures", this->base);
  this->opt_grid->addWidget(this->noTextCB, 1, 0);
  connect(noTextCB, SIGNAL(clicked()), this, SLOT(noText_checked()));

  this->lightCB = new QCheckBox("No Lighting", this->base);
  this->opt_grid->addWidget(this->lightCB, 2, 0);
  connect(lightCB, SIGNAL(clicked()), this, SLOT(light_checked()));

  this->zLabel = new QLabel("Zoom In/Out", this->base);
  this->opt_grid->addWidget(this->zLabel, 3, 0);

  this->reset_lights = new QPushButton("Reset Light", this->base);
  this->opt_grid->addWidget(this->reset_lights, 4, 0, 1, 2);
  connect(reset_lights, SIGNAL(clicked()), this, SLOT(resetLights_pushed()));

  /*
   *	second column
   */
  this->mirrorCB = new QCheckBox("Mirrors", this->base);
  this->opt_grid->addWidget(this->mirrorCB, 0, 1);
  connect(mirrorCB, SIGNAL(clicked()), this, SLOT(mirror_checked()));

  this->no_interpCB = new QCheckBox("No Interpolation", this->base);
  this->opt_grid->addWidget(this->no_interpCB, 1, 1);
  connect(no_interpCB, SIGNAL(clicked()), this, SLOT(nointerp_checked()));

  this->view_lightsCB = new QCheckBox("View Light(s)", this->base);
  this->opt_grid->addWidget(this->view_lightsCB, 2, 1);
  connect(view_lightsCB, SIGNAL(clicked()), this, SLOT(vlights_checked()));

  this->zoom = new QSlider(Qt::Horizontal, this->base);
  this->zoom->setRange(-200, 0);
  this->zoom->setPageStep(1);
  this->zoom->setValue(-100);
  this->opt_grid->addWidget(this->zoom, 3, 1);
  connect(zoom, SIGNAL(valueChanged(int)), this, SLOT(zoom_changed(int)));
}

/*
 *	opt_widget::noText_checked()
 *
 *	Toggle textures.
 */
void
opt_widget::noText_checked()
{
  if (this->noTextCB->isChecked() == true)
    world_set_options(g_world, 0, RENDER_TEXTURES);
  else
    world_set_options(g_world, RENDER_TEXTURES, 0);
}

/*
 *	opt_widget::wire_checked()
 *
 *	Toggle wireframe.
 */
void
opt_widget::wire_checked()
{
  if (this->wireCB->isChecked() == true)
    world_set_options(g_world, RENDER_WIREFRAME, 0);
  else
    world_set_options(g_world, 0, RENDER_WIREFRAME);
}

/*
 *	opt_widget::mirror_checked()
 *
 *	Toggle rendering mirrors.
 */
void
opt_widget::mirror_checked()
{
  if (this->mirrorCB->isChecked() == true)
    world_set_options(g_world, RENDER_MIRRORS, 0);
  else
    world_set_options(g_world, 0, RENDER_MIRRORS);
}

/*
 *	opt_widget::light_checked()
 *
 *	Toggle lighting.
 */
void
opt_widget::light_checked()
{
  if (this->lightCB->isChecked() == true)
    world_set_options(g_world, 0, ENGINE_LIGHTING);
  else
    world_set_options(g_world, ENGINE_LIGHTING, 0);
}

/*
 *	opt_widget::nointerp_checked()
 *
 *	Toggle interpolation.
 */
void
opt_widget::nointerp_checked()
{
  if (this->no_interpCB->isChecked() == true)
    world_set_options(g_world, 0, ENGINE_INTERPOLATE);
  else
    world_set_options(g_world, ENGINE_INTERPOLATE, 0);
}

/*
 *	opt_widget::zoom_checked()
 *
 *	Set the zoom factor.
 */
void
opt_widget::zoom_changed(int zfactor)
{
  world_set_camera_distance(g_world, (zfactor * -1.0f));
}

/*
 *	opt_widget::vlights_checked()
 *
 *	zoom out to view light(s), mainly used to move the lights.
 */
void
opt_widget::vlights_checked()
{
  if (this->view_lightsCB->isChecked() == true)
  {
    zoom->setValue(-200);
    world_set_camera_distance(g_world, 400.0f);

    /* enable rendering of the flashlight model */
    world_set_options(g_world, RENDER_FLASHLIGHT, 0);
  }
  else
  {
    world_set_camera_distance(g_world, 100.0f);
    zoom->setValue(-100);

    /* disable rendering of the flashlight model */
    world_set_options(g_world, 0, RENDER_FLASHLIGHT);

    /* force deslection of the flashlight */
    if (g_gui->gl->selected_object && (g_gui->gl->selected_object->body_part == MD3_LIGHT))
    {
      g_gui->gl->selected_object = NULL;
      g_gui->srot->object_selected(NULL);
    }
  }
}

/*
 *	opt_widget::resetLight_pushed()
 *
 *	resets the cameria to its original position
 */
void
opt_widget::resetLights_pushed()
{
  g_world->light[0].position[0] = 100.0f; /* positon */
  g_world->light[0].position[1] = 10.0f;
  g_world->light[0].position[2] = 10.0f;
  g_world->light[0].position[1] = 1.0f;
  g_world->light[0].trot = DEFAULT_LIGHT_TROT;
  g_world->light[0].prot = DEFAULT_LIGHT_PROT;
  g_world->light[0].r = DEFAULT_LIGHT_DISTANCE;
  g_world->light[0].dir_trot = DEFAULT_LIGHT_DIR_TROT;
  g_world->light[0].dir_prot = DEFAULT_LIGHT_DIR_PROT;
}

/***********************************************************************************
 *
 *	srot_widget
 *
 ***********************************************************************************/

/*
 *	srot_widget::srot_widget()
 */
srot_widget::srot_widget(int strips, Qt::Orientation orientation, const QString& title, QWidget* parent, const char* name)
  : QGroupBox(title, parent)
{

  this->base = new QWidget(this);
  this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  this->setMaximumWidth(MAX_MENU_WIDTH);

  this->selected_model = NULL;

  /*
   *	creates a grid layout to organize the widgets
   */
  this->move_grid = new QGridLayout(this->base);
  this->move_grid->addLayout(this->move_grid, 5, 2);
  this->move_grid->setContentsMargins(1, 1, 1, 1);
  this->move_grid->setSpacing(5);
  this->setLayout(this->move_grid);

  /*
   *	first column of the grid
   *	4 labels for scalling and rotating a selected body part
   */
  this->scale_Label = new QLabel("Scale", this->base);
  this->move_grid->addWidget(this->scale_Label, 0, 0);

  this->xrot_Label = new QLabel("X-Rotation", this->base);
  this->move_grid->addWidget(this->xrot_Label, 1, 0);

  this->yrot_Label = new QLabel("Y-Rotation", this->base);
  this->move_grid->addWidget(this->yrot_Label, 2, 0);

  this->zrot_Label = new QLabel("Z-Rotation", this->base);
  this->move_grid->addWidget(this->zrot_Label, 3, 0);

  /*
   *	second column of the grid
   *	4 sliders for adjusting values of scaling and rotatating
   */
  this->scale_S = new QSlider(Qt::Horizontal, this->base);
  this->scale_S->setRange(0, 200);
  this->scale_S->setPageStep(1);
  this->scale_S->setValue(100);
  this->move_grid->addWidget(this->scale_S, 0, 1);
  connect(this->scale_S, SIGNAL(valueChanged(int)), this, SLOT(scale_changed(int)));

  this->xrot_S = new QSlider(Qt::Horizontal, this->base);
  this->xrot_S->setRange(-180, 180);
  this->xrot_S->setPageStep(1);
  this->xrot_S->setValue(0);
  this->move_grid->addWidget(this->xrot_S, 1, 1);
  connect(this->xrot_S, SIGNAL(valueChanged(int)), this, SLOT(xrot_changed(int)));

  this->yrot_S = new QSlider(Qt::Horizontal, this->base);
  this->yrot_S->setRange(-180, 180);
  this->yrot_S->setPageStep(1);
  this->yrot_S->setValue(0);
  this->move_grid->addWidget(this->yrot_S, 2, 1);
  connect(this->yrot_S, SIGNAL(valueChanged(int)), this, SLOT(yrot_changed(int)));

  this->zrot_S = new QSlider(Qt::Horizontal, this->base);
  this->zrot_S->setRange(-180, 180);
  this->zrot_S->setPageStep(1);
  this->zrot_S->setValue(0);
  this->move_grid->addWidget(this->zrot_S, 3, 1);
  connect(this->zrot_S, SIGNAL(valueChanged(int)), this, SLOT(zrot_changed(int)));

  this->reset_sliders = new QPushButton("Reset", this->base);
  this->move_grid->addWidget(this->reset_sliders, 5, 0, 1, 2);
  connect(this->reset_sliders, SIGNAL(clicked()), this, SLOT(resets_clicked()));
}

/*
 *	srot_widget::object_selected()
 *
 *	A new object was selected.
 */
void
srot_widget::object_selected(md3_model_t* model)
{

  char* title = "";
  this->selected_model = model;

  /*
   *	A new object has been selected.
   */
  if (!model)
  {
    /*
     *	If the model pointer is NULL then nothing was selected.
     *
     *	Reset the sliders to default for a nice look.
     */
    this->setTitle("Selected Body Area");

    this->scale_S->setValue(100);
    this->xrot_S->setValue(0);
    this->yrot_S->setValue(0);
    this->zrot_S->setValue(0);
    return;
  }

  /*
   *	The engine defaults to "head", "lower", and "upper".
   *	For the sake of graphical interface, make them nicer.
   */
  switch (model->body_part)
  {
    case MD3_HEAD:
      title = "  Head  ";
      break;
    case MD3_TORSO:
      title = "  Torso  ";
      break;
    case MD3_LEGS:
      title = "  Legs  ";
      break;
    case MD3_WEAPON:
      title = "  Weapon  ";
      break;
    case MD3_LIGHT:
      title = model->model_name;
      break;
    default:
      /* this should never hit */
      title = "  Unknown Object  ";
  }
  this->setTitle(title);

  /*
   *	Set the scale slider to what this model scale factor is.
   */
  this->scale_S->setValue((int)(model->scale_factor * 100.0f));

  /*
          Sets all the rotation sliders to what the model's rotation factors are
  */
  this->xrot_S->setValue((int)(model->rot[0]));
  this->yrot_S->setValue((int)(model->rot[1]));
  this->zrot_S->setValue((int)(model->rot[2]));
}

/*
 *	srot_widget::scale_changed()
 *
 *	Change the scale factor.
 */
void
srot_widget::scale_changed(int s_factor)
{
  float factor = (s_factor / 100.0f);

  if (this->selected_model == NULL)
    return;

  /* the factor cannot be 0 */
  if (factor == 0)
    factor = 0.01;

  if (!this->selected_model)
    /* no model selected */
    return;

  this->selected_model->scale_factor = factor;
}

/*
 *	srot_widget::xrot_changed()
 *
 *	Change the x-axis rotation factor.
 */
void
srot_widget::xrot_changed(int x_factor)
{
  if (!this->selected_model)
    /* no model selected */
    return;

  rotate_model_absolute(this->selected_model->body_part, X_AXIS, x_factor);
}

/*
 *	srot_widget::yrot_changed()
 *
 *	Change the y-axis rotation factor.
 */
void
srot_widget::yrot_changed(int y_factor)
{
  if (!this->selected_model)
    /* no model selected */
    return;

  rotate_model_absolute(this->selected_model->body_part, Y_AXIS, y_factor);
}

/*
 *	srot_widget::zrot_changed()
 *
 *	Change the z-axis rotation factor.
 */
void
srot_widget::zrot_changed(int z_factor)
{
  if (!this->selected_model)
    /* no model selected */
    return;

  rotate_model_absolute(this->selected_model->body_part, Z_AXIS, z_factor);
}

/*
 *	srot_widget::resets_clicked()
 *
 *	Reset the scale and rotation for selected object or all.
 */
void
srot_widget::resets_clicked()
{
  if (!this->selected_model)
  {
    /*
     *	no model selected
     *
     *	In this case the reset is for all body parts.
     */
    rotate_all_models_absolute(X_AXIS, 0, MD3_LIGHT);
    rotate_all_models_absolute(Y_AXIS, 0, MD3_LIGHT);
    rotate_all_models_absolute(Z_AXIS, 0, MD3_LIGHT);
    scale_all_models(1.0f, MD3_LIGHT);
    return;
  }

  /* model is selected */

  scale_changed(100);
  this->scale_S->setValue((int)(this->selected_model->scale_factor * 100.0f));

  xrot_changed(0);
  this->xrot_S->setValue((int)(this->selected_model->rot[0]));

  yrot_changed(0);
  this->yrot_S->setValue((int)(this->selected_model->rot[1]));

  zrot_changed(0);
  this->zrot_S->setValue((int)(this->selected_model->rot[2]));
}
