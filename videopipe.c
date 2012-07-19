#include <CoreServices/CoreServices.h>
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#include <unistd.h>

int windowWidth;
int windowHeight;

#undef FEATURE_POSITION_DEBUG

unsigned char *bitmap;
int iter = 0;

void display_func(void) {
	int i;
	unsigned char buffer[windowWidth * windowHeight];

	int count = fread(buffer, 1, windowWidth * windowHeight, stdin);
	for (i = 0; i < count; i++) {
		bitmap[iter] = buffer[i];
		iter++;
		iter %= windowWidth * windowHeight;
	}

	for (i = 0; i < windowWidth * windowHeight; i++) {
#ifdef FEATURE_POSITION_DEBUG
		if (i == iter) {
			glColor3ub(255, bitmap[i], bitmap[i]);
		} else 
#endif
		{
			glColor3ub(bitmap[i], bitmap[i], bitmap[i]);
		}
		glBegin(GL_POLYGON);
			glVertex2f((i % windowWidth)		* 2.0 / windowWidth - 1.0, (i / windowHeight)	* 2.0 / windowHeight - 1.0);
			glVertex2f((1 + i % windowWidth)	* 2.0 / windowWidth - 1.0, (i / windowHeight)	* 2.0 / windowHeight - 1.0);
			glVertex2f((1 + i % windowWidth)	* 2.0 / windowWidth - 1.0, (1 + i / windowHeight)	* 2.0 / windowHeight - 1.0);
			glVertex2f((i % windowWidth)		* 2.0 / windowWidth - 1.0, (1 + i / windowHeight)	* 2.0 / windowHeight - 1.0);
		glEnd();
	}

	glutSwapBuffers();
}

void idle(void) {
	glutPostRedisplay();
}

int isFullScreen;
void key(unsigned char key, int x, int y) {
	if (key == 'f') {
		if (isFullScreen) {
		    glutReshapeWindow(windowWidth, windowHeight);
		    glutPositionWindow(75, 100);
			glutSetCursor(GLUT_CURSOR_INHERIT);

		    isFullScreen = 0;
		} else {
			glutFullScreen();
			glutSetCursor(GLUT_CURSOR_NONE);

			isFullScreen = 1;
		}
	}
}

void usage() {
	fprintf(stderr, "Usage: cat /dev/urandom | audiopipe -w 128\n");
	fprintf(stderr, "  -w 64\t\tspecify window width and height, default: 64\n");
	exit(1);
}

int main(int argc, char** argv) {
	windowWidth = 64;
	windowHeight = 64;

	int opt;
 	extern char	*optarg;
	while ((opt = getopt(argc, argv, "w:")) != -1) {
		switch (opt) {
			case 'w':
				windowWidth = atoi(optarg);
				windowHeight = atoi(optarg);
				break;
			default:
				usage();
		}
	}

	freopen(NULL, "rb", stdin);
    isFullScreen = 0;

    bitmap = (unsigned char *)malloc(windowWidth * windowHeight); // TODO free

	glutInit(&argc, (char **)argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("videopipe");

	// register callbacks
	glutDisplayFunc(display_func);
	glutIdleFunc(idle);
	glutKeyboardFunc(key);

	glutMainLoop();

	return 0;
}