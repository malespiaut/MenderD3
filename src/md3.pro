TEMPLATE = app
TARGET = md3
CONFIG -= moc

QT += opengl

LIBS += -lGL -lGLU -lX11 -lm -L/usr/X11R6/lib

INCLUDEPATH += ../include

SOURCES += main.cpp md3_parse.c render.c util.c gui.cpp gl_widget.cpp tga.c quaternion.c world.c accum.c

HEADERS +=	../include/definitions.h \
			../include/gui.h \
			../include/gl_widget.h \
			../include/md3_parse.h \
			../include/render.h \
			../include/util.h \
			../include/tga.h \
			../include/quaternion.h \
			../include/world.h \
			../include/jitter.h \
			../include/accum.h
