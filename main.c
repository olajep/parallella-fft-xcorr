#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <complex.h>
#include <stdbool.h>
#include <stdint.h>

/* Internal */
#include "demo.h"

/* API */
#include "libfft-demo.h"

bool initialized = false;

#define IMG_W 64
#define IMG_H 64

/* Returns true on success */
__attribute__ ((visibility ("default")))
bool calculateXCorr(uint8_t *jpeg1, size_t jpeg1_size,
		    uint8_t *jpeg2, size_t jpeg2_size,
		    float *corr)
{
	bool ret = true;
	uint8_t *A, *B;
	int width, height;

	if (!initialized) {
		if (!fftimpl_init()) {
			fprintf(stderr,
				"ERROR: failed to initialize fft library\n");
			return false;
		}
		initialized = true;
	}

	A = jpeg_to_grayscale_int(jpeg1, jpeg1_size, &width, &height);
	if (!A) {
		ret = false;
		goto out;
	}

	B = jpeg_to_grayscale_int(jpeg2, jpeg2_size, &width, &height);
	if (!B) {
		ret = false;
		goto free_A;
	}

	if (!fftimpl_xcorr(A, B, 1, width, height, corr)) {
		fprintf(stderr, "ERROR: xcorr failed\n");
		ret = false;
	}

	free(B);
free_A:
	free(A);
out:
	return ret;
}

/* Returns true on success */
__attribute__ ((visibility ("default")))
bool calculateXCorr2(struct jpeg_image *ref_img, struct jpeg_image *compare,
		     int ncompare, float *corr)
{
	bool ret = true;
	uint8_t *ref, *tmp;
	int width, height;
	int i;
	uint8_t *img_buf;

	if (!initialized) {
		if (!fftimpl_init()) {
			fprintf(stderr,
				"ERROR: failed to initialize fft library\n");
			return false;
		}
		initialized = true;
	}

	ref = jpeg_to_grayscale_int(ref_img->data, ref_img->size, &width, &height);
	if (!ref)
		return false;

	img_buf = (uint8_t *) malloc(MAX_BITMAPS * IMG_W * IMG_H * sizeof(*img_buf));

	if (width != IMG_W || height != IMG_H) {
		fprintf(stderr,
			"ERROR: Wrong image dimension. Need %dx%d\n",
			IMG_W, IMG_H);
		ret = false;
		goto out;
	}

	while (ncompare) {
		for (i = 0; i < MAX_BITMAPS && ncompare; i++, ncompare--, compare++) {
			tmp = jpeg_to_grayscale_int(compare->data, compare->size, &width, &height);
			if (!tmp) {
				ret = false;
				goto out;
			}

			if (width != IMG_W || height != IMG_H) {
				fprintf(stderr,
					"ERROR: Wrong image dimension. Need %dx%d\n",
					IMG_W, IMG_H);
				ret = false;
				free(tmp);
				goto out;
			}
			memcpy(&img_buf[IMG_W * IMG_H * i], tmp,
					IMG_W * IMG_H * sizeof(*img_buf));

			free(tmp);
		}

		if (!fftimpl_xcorr(ref, img_buf, i, IMG_W, IMG_H, corr)) {
			fprintf(stderr, "ERROR: xcorr failed\n");
			ret = false;
			goto out;
		}
		corr += i;
	}

out:
	free(img_buf);
	free(ref);
	return ret;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int i;
	char *file1 = "A.jpg";
	char *file2 = "B.jpg";
	float *A = NULL, *B = NULL;
	uint8_t *A_jpg_data = NULL;
	uint8_t *B_jpg_data = NULL;
	unsigned long A_jpg_sz = 0, B_jpg_sz = 0;
	int width, height;

	if (!initialized) {
		if (!fftimpl_init()) {
			fprintf(stderr,
				"ERROR: failed to initialize fft library\n");
			exit(1);
		}
		initialized = true;
	}

	if (argc > 1)
		file1 = argv[1];

	if (argc > 2)
		file2 = argv[2];

	struct jpeg_image jpgimg[10];

	float corr[10];

	A = jpeg_file_to_grayscale(file1, &width, &height);
	if (!A) {
		ret = 1;
		goto out;
	}
	A_jpg_data = (uint8_t *) grayscale_to_jpeg(A, width, height, &A_jpg_sz);

	B = jpeg_file_to_grayscale(file2, &width, &height);
	if (!B) {
		ret = 2;
		goto free_A;
	}
	B_jpg_data = (uint8_t *) grayscale_to_jpeg(B, width, height, &B_jpg_sz);

	jpgimg[0].data = A_jpg_data;
	jpgimg[0].size = A_jpg_sz;
	for (i = 1; i < 10; i++) {
		jpgimg[i].data = i % 2 ? A_jpg_data : B_jpg_data;
		jpgimg[i].size = i % 2 ? A_jpg_sz : B_jpg_sz;
	}

	if (!calculateXCorr2(&jpgimg[0], &jpgimg[1], 9, corr)) {
		fprintf(stderr, "ERROR: xcorr failed\n");
		ret = 3;
	}

	for (i = 0; i < 9; i++)
		printf("%f,%s,%s\n", corr[i], file1, file2);

	free(B);
free_A:
	free(A);
out:
	return ret;
}
