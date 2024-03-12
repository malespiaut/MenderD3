TEMPLATE = app
TARGET = md3
CONFIG -= moc

QT += openglwidgets widgets

QMAKE_CFLAGS += -Wall -Wextra -Wpedantic -Wshadow -Wstrict-aliasing=2 -Wdouble-promotion
QMAKE_CXXFLAGS += -fpermissive -Wall -Wextra -Wpedantic -Wshadow -Wstrict-aliasing=2 -Wdouble-promotion

LIBS += -lGL -lGLU -lX11 -lm -L/usr/X11R6/lib

SOURCES += main.cpp accum.c gl_widget.cpp gui.cpp md3_parse.c quaternion.c render.c tga.c util.c world.c 

HEADERS += accum.h \
	   definitions.h \
	   gl_widget.h \
	   gui.h \
	   jitter.h \
	   md3_parse.h \
	   quaternion.h \
	   render.h \
	   tga.h \
	   util.h \
	   world.h
