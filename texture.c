#include <GL/gl.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "etcpak.h"

void glGenerateMipmap(GLenum target);

int main(int argc, char **argv) {
	if(argc < 2) {
		fprintf(stderr, "No input file provided.\n");
		return 1;
	}

	printf("Processing \"%s\"...", argv[1]);
	fflush(stdout);

	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
						SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);

	SDL_Window *window =
		SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
						 100, 100, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(window);

	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
	SDL_Surface *surface = IMG_Load(argv[1]);
	if(!surface) {
		fprintf(stderr, "Failed to open file \"%s\" for reading.\n", argv[1]);

		IMG_Quit();
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		SDL_Quit();

		return 1;
	}

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
					GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLint anisotropy;
	glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &anisotropy);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, anisotropy);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, surface->w, surface->h, 0, GL_RGB,
				 GL_UNSIGNED_BYTE, surface->pixels);
	SDL_FreeSurface(surface);

	glGenerateMipmap(GL_TEXTURE_2D);

	char *outName = strdup(argv[1]);
	const int outNameLength = strlen(outName);
	outName[outNameLength - 3] = 'b';
	outName[outNameLength - 2] = 'i';
	outName[outNameLength - 1] = 'n';
	SDL_RWops *outFile = SDL_RWFromFile(outName, "wb");
	free(outName);

	GLint level = 0, width, height;
	char *data = NULL;

	do {
		glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH,
								 &width);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT,
								 &height);

		// printf("Level %i size: %ix%i\n", level, width, height);

		int size;

		if(width >= 4 && height >= 4) {
			size = width * height * 3;
			data = malloc(size);
			glGetTexImage(GL_TEXTURE_2D, level, GL_RGB, GL_UNSIGNED_BYTE, data);

			surface = SDL_CreateRGBSurfaceWithFormatFrom(
				data, width, height, 24, width * 3, SDL_PIXELFORMAT_RGB24);

			const int nameLength = strlen(argv[1]) + 7;
			char *name = malloc(nameLength);
			sprintf(name, "mip%s%i-%s%c", level < 10 ? "0" : "", level, argv[1],
					'\0');

			IMG_SavePNG(surface, name);
			SDL_FreeSurface(surface);
			free(data);

			char *pvrName = strdup(name);
			pvrName[nameLength - 4] = 'p';
			pvrName[nameLength - 3] = 'v';
			pvrName[nameLength - 2] = 'r';
			// printf("etcpak: %s -> %s\n", name, pvrName);
			etcpak(3, (char *[]) { argv[0], name, pvrName });
			free(name);

			SDL_RWops *file = SDL_RWFromFile(pvrName, "rb");
			free(pvrName);
			SDL_RWseek(file, 52, RW_SEEK_SET);
			size = SDL_RWsize(file) - 52;
			data = malloc(size);
			SDL_RWread(file, data, size, 1);
			SDL_RWclose(file);
		}

		const unsigned short outWidth = (const unsigned short)width;
		SDL_RWwrite(outFile, &outWidth, sizeof(unsigned short), 1);
		const unsigned short outHeight = (const unsigned short)height;
		SDL_RWwrite(outFile, &outHeight, sizeof(unsigned short), 1);

		SDL_RWwrite(outFile, &size, sizeof(int), 1);
		SDL_RWwrite(outFile, data, size, 1);

		if(width > 4 && height > 4) free(data);

		level++;
	} while(width > 1 && height > 1);

	free(data);

	const unsigned char outLevels = (unsigned char)level;
	SDL_RWwrite(outFile, &outLevels, sizeof(unsigned char), 1);
	SDL_RWclose(outFile);

	glDeleteTextures(1, &texture);

	IMG_Quit();
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	printf("done.\n");

	return 0;
}
