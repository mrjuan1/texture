#include <GL/gl.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>

#include "etcpak.h"

#define PVR_HEADER_SIZE 52

#define fail()                                            \
	{                                                     \
		fprintf(stderr, "failed.\n%s\n", SDL_GetError()); \
		SDL_Quit();                                       \
		return 1;                                         \
	}

#define checkGLAttrib() \
	if(result) fail();

void glGenerateMipmap(GLenum target);

void showUsage(const char *arg0);
bool closeFile(SDL_RWops *file);

int main(int argc, char **argv) {
	if(argc < 2) {
		fprintf(stderr, "No input file(s) provided.\n\n");
		showUsage(argv[0]);

		return 1;
	}

	int i;
	for(i = 1; i < argc; i++)
		if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			showUsage(argv[0]);
			return 0;
		}

	printf("Initialising...");
	fflush(stdout);

	int result = SDL_Init(SDL_INIT_VIDEO);
	if(result) {
		printf("failed.\n%s\n", SDL_GetError());
		return 1;
	}

	result = SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
								 SDL_GL_CONTEXT_PROFILE_CORE);
	checkGLAttrib();
	result = SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 24);
	checkGLAttrib();
	result = SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	checkGLAttrib();
	result = SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	checkGLAttrib();
	result = SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	checkGLAttrib();
	result = SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	checkGLAttrib();
	result = SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
	checkGLAttrib();

	SDL_Window *window =
		SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
						 100, 100, SDL_WINDOW_OPENGL);
	if(!window) fail();

	SDL_GLContext context = SDL_GL_CreateContext(window);
	if(!context) {
		SDL_DestroyWindow(window);
		fail();
	}

	result = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
	if(!result) {
		fprintf(stderr, "failed.\n%s\n", IMG_GetError());

		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		SDL_Quit();

		return 1;
	}

	GLint anisotropy;
	glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &anisotropy);

	printf("done.\n");

	for(i = 1; i < argc; i++) {
		printf("\nProcessing \"%s\"...", argv[i]);
		fflush(stdout);

		SDL_Surface *surface = IMG_Load(argv[i]);
		if(!surface) {
			fprintf(stderr, "failed to open file for reading.\n%s\n",
					IMG_GetError());
			continue;
		}

		SDL_Surface *convertedSurface =
			SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB24, 0);
		SDL_FreeSurface(surface);
		if(!convertedSurface) {
			fprintf(stderr, "failed to convert to RGB24.\n%s\n",
					IMG_GetError());
			continue;
		}

		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
						GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if(anisotropy)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY,
							anisotropy);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, convertedSurface->w,
					 convertedSurface->h, 0, GL_RGB, GL_UNSIGNED_BYTE,
					 convertedSurface->pixels);
		SDL_FreeSurface(convertedSurface);

		glGenerateMipmap(GL_TEXTURE_2D);

		char *outName = strdup(argv[i]);
		if(!outName) {
			fprintf(stderr, "failed to allocate memory for output filename.\n");
			glDeleteTextures(1, &texture);

			continue;
		}

		const int outNameLength = strlen(outName);
		outName[outNameLength - 3] = 'b';
		outName[outNameLength - 2] = 'i';
		outName[outNameLength - 1] = 'n';

		SDL_RWops *outFile = SDL_RWFromFile(outName, "wb");
		if(!outFile) {
			fprintf(stderr, "failed to open output file \"%s\".\n%s.\n",
					outName, SDL_GetError());

			free(outName);
			glDeleteTextures(1, &texture);

			continue;
		}

		free(outName);

		GLint level = 0, width, height;
		char *data = NULL;
		bool errorOccurred = false;

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
				if(!data) {
					fprintf(
						stderr,
						"failed to allocate memory for texture level %i data.\n",
						level);
					errorOccurred = true;

					break;
				}

				glGetTexImage(GL_TEXTURE_2D, level, GL_RGB, GL_UNSIGNED_BYTE,
							  data);

				surface = SDL_CreateRGBSurfaceWithFormatFrom(
					data, width, height, 24, width * 3, SDL_PIXELFORMAT_RGB24);
				if(!surface) {
					fprintf(
						stderr,
						"failed to create output surface for texture level %i.\n%s\n",
						level, SDL_GetError());

					free(data);
					errorOccurred = true;

					break;
				}

				const int nameLength = strlen(argv[i]) + 7;
				char *name = malloc(nameLength);
				if(!name) {
					fprintf(
						stderr,
						"failed to allocate memory for texture level %i output filename.\n",
						level);

					SDL_FreeSurface(surface);
					free(data);
					errorOccurred = true;

					break;
				}

				sprintf(name, "mip%s%i-%s%c", level < 10 ? "0" : "", level,
						argv[i], '\0');

				result = IMG_SavePNG(surface, name);
				SDL_FreeSurface(surface);
				free(data);
				if(result) {
					fprintf(
						stderr,
						"failed to save texture level %i output image file \"%s\".\n%s\n",
						level, name, IMG_GetError());

					free(name);
					errorOccurred = true;

					break;
				}

				char *pvrName = strdup(name);
				if(!pvrName) {
					fprintf(
						stderr,
						"failed allocate memory for compressed texture level %i output filename.\n",
						level);

					free(name);
					errorOccurred = true;

					break;
				}

				pvrName[nameLength - 4] = 'p';
				pvrName[nameLength - 3] = 'v';
				pvrName[nameLength - 2] = 'r';
				// printf("etcpak: %s -> %s\n", name, pvrName);
				result = etcpak(3, (char *[]) { argv[0], name, pvrName });
				free(name);
				if(result) {
					fprintf(
						stderr,
						"failed to compress texture level %i to output file \"%s\".\n",
						level, pvrName);

					free(pvrName);
					errorOccurred = true;

					break;
				}

				SDL_RWops *file = SDL_RWFromFile(pvrName, "rb");
				if(!file) {
					fprintf(
						stderr,
						"failed to open texture level %i compressed file \"%s\" for reading.\n%s\n",
						level, pvrName, SDL_GetError());

					free(pvrName);
					errorOccurred = true;

					break;
				}

				free(pvrName);

				SDL_RWseek(file, PVR_HEADER_SIZE, RW_SEEK_SET);
				size = SDL_RWsize(file) - PVR_HEADER_SIZE;
				data = malloc(size);
				if(!data) {
					fprintf(
						stderr,
						"failed to allocate memory for texure level %i compressed texture data.\n",
						level);

					closeFile(file);
					errorOccurred = true;

					break;
				}

				result = SDL_RWread(file, data, size, 1);
				if(result != 1) {
					fprintf(
						stderr,
						"failed read texture level %i compressed texture data.\n%s\n",
						level, SDL_GetError());

					closeFile(file);
					errorOccurred = true;

					break;
				}

				if(!closeFile(file)) {
					errorOccurred = true;
					break;
				}
			}

			const unsigned short outWidth = (const unsigned short)width;
			result = SDL_RWwrite(outFile, &outWidth, sizeof(unsigned short), 1);
			if(result != 1) {
				fprintf(
					stderr,
					"failed to write texture level %i width to output file.\n%s\n",
					level, SDL_GetError());
				errorOccurred = true;

				break;
			}

			const unsigned short outHeight = (const unsigned short)height;
			result =
				SDL_RWwrite(outFile, &outHeight, sizeof(unsigned short), 1);
			if(result != 1) {
				fprintf(
					stderr,
					"failed to write texture level %i height to output file.\n%s\n",
					level, SDL_GetError());
				errorOccurred = true;

				break;
			}

			result = SDL_RWwrite(outFile, &size, sizeof(int), 1);
			if(result != 1) {
				fprintf(
					stderr,
					"failed to write texture level %i size to output file.\n%s\n",
					level, SDL_GetError());
				errorOccurred = true;

				break;
			}

			result = SDL_RWwrite(outFile, data, size, 1);
			if(result != 1) {
				fprintf(
					stderr,
					"failed to write texture level %i to output file.\n%s\n",
					level, SDL_GetError());
				errorOccurred = true;

				break;
			}

			if(width > 4 && height > 4) free(data);

			level++;
		} while(width > 1 && height > 1);

		if(data) free(data);

		if(errorOccurred) {
			closeFile(outFile);
			glDeleteTextures(1, &texture);

			continue;
		}

		const unsigned char outLevels = (unsigned char)level;
		result = SDL_RWwrite(outFile, &outLevels, sizeof(unsigned char), 1);
		glDeleteTextures(1, &texture);
		if(!result) {
			fprintf(stderr,
					"failed to write number of levels to output file.\n%s\n",
					SDL_GetError());
			closeFile(outFile);

			continue;
		}

		if(!closeFile(outFile)) continue;

		printf("done.\n");
	}

	IMG_Quit();
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	printf("\nDone.\n");

	return 0;
}

void showUsage(const char *arg0) {
	printf("Usage: %s [<option(s)...>] | <input file(s)...>\n", arg0);
	printf("Options:\n");
	printf("  -h  --help  Show this message\n");
	printf("\n");
}

bool closeFile(SDL_RWops *file) {
	int result = SDL_RWclose(file);
	if(result) {
		fprintf(stderr, "Failed to close file.\n%s\n", SDL_GetError());
		return false;
	}

	return true;
}
